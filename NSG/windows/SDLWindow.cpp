/*
-------------------------------------------------------------------------------
This file is part of nsg-library.
http://github.com/woodjazz/nsg-library

Copyright (c) 2014-2016 Néstor Silveira Gorski

-------------------------------------------------------------------------------
This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software. If you use this software
in a product, an acknowledgment in the product documentation would be
appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
-------------------------------------------------------------------------------
*/
#if SDL
#include "SDLWindow.h"
#include "SDL.h"
#ifdef _MSC_VER
#include "SDL_syswm.h"
#endif
#undef main
#include "Engine.h"
#include "RenderingContext.h"
#include "Tick.h"
#include "Keys.h"
#include "Log.h"
#include "Check.h"
#include "UTF8String.h"
#include "AppConfiguration.h"
#include "Object.h"
#include "Scene.h"
#include "imgui.h"
#include "Maths.h"
#include <memory>
#include <string>
#include <locale>
#include <thread>
#include <mutex>
#ifndef __GNUC__
#include <codecvt>
#endif

namespace NSG
{
    #if defined(IS_TARGET_MOBILE)
    static int EventWatch(void* userdata, SDL_Event* event)
    {
        SDLWindow* window = static_cast<SDLWindow*>(userdata);
        switch (event->type)
        {
            case SDL_APP_TERMINATING:
                {
                    LOGI("SDL_APP_TERMINATING");
                    SDL_Quit();
                    std::exit(0);
                    return 0;
                }
            case SDL_APP_LOWMEMORY:
                {
                    LOGW("SDL_APP_LOWMEMORY");
                    return 0;
                }
            case SDL_APP_WILLENTERBACKGROUND:
                {
                    LOGI("SDL_APP_WILLENTERBACKGROUND");
                    return 0;
                }
            case SDL_APP_DIDENTERBACKGROUND:
                {
                    LOGI("SDL_APP_DIDENTERBACKGROUND");
                    window->EnterBackground();
                    return 0;
                }
            case SDL_APP_WILLENTERFOREGROUND:
                {
                    LOGI("SDL_APP_WILLENTERFOREGROUND");
                    return 0;
                }
            case SDL_APP_DIDENTERFOREGROUND:
                {
                    LOGI("SDL_APP_DIDENTERFOREGROUND");
                    window->EnterForeground();
                    return 0;
                }
        }
        return 1;
    }
    #endif
    const char* InternalPointer = "InternalPointer";

    SDLWindow::SDLWindow(const std::string& name, WindowFlags flags)
        : Window(name),
          flags_(0)
    {
        const AppConfiguration& conf = Engine::GetPtr()->GetAppConfiguration();
        Initialize(conf.x_, conf.y_, conf.width_, conf.height_, flags);
        LOGI("Window %s created.", name_.c_str());
    }

    SDLWindow::SDLWindow(const std::string& name, int x, int y, int width, int height, WindowFlags flags)
        : Window(name),
          flags_(0)
    {
        Initialize(x, y, width, height, flags);
        LOGI("Window %s created.", name_.c_str());
    }

    SDLWindow::~SDLWindow()
    {
        Close();
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
    }

    void SDLWindow::OpenJoystick(int deviceIndex)
    {
        JoystickState state;
        state.deviceIndex = deviceIndex;
        state.joystick_ = SDL_JoystickOpen(deviceIndex);
        if (!state.joystick_)
        {
            LOGW("Cannot open joystick number: %d", deviceIndex);
        }
        #if !defined(EMSCRIPTEN)
        else if (SDL_IsGameController(deviceIndex))
        {
            state.pad_ = SDL_GameControllerOpen(deviceIndex);
            if (!state.pad_)
            {
                LOGW("Cannot open game controller number: %d", deviceIndex);
            }
        }
        state.instanceID_ = SDL_JoystickInstanceID(static_cast<SDL_Joystick*>(state.joystick_));
        #else
        state.instanceID_ = deviceIndex;
        #endif
        LOGI("Joystick number: %d has been added.", deviceIndex);
        joysticks_[state.instanceID_] = state;
    }

    void SDLWindow::CloseJoystick(int deviceIndex)
    {
        for (auto it : joysticks_)
        {
            auto& state = it.second;
            if (state.deviceIndex == deviceIndex)
            {
                if (SDL_IsGameController(state.deviceIndex))
                    SDL_GameControllerClose(static_cast<SDL_GameController*>(state.pad_));
                SDL_JoystickClose((SDL_Joystick*)state.joystick_);
                joysticks_.erase(state.instanceID_);
                LOGI("Joystick number: %d has been removed.", deviceIndex);
                break;
            }
        }
    }

    void SDLWindow::OpenJoysticks()
    {
        int size = SDL_NumJoysticks();
        for (int i = 0; i < size; ++i)
            OpenJoystick(i);
    }

    void SDLWindow::Initialize(int x, int y, int width, int height, WindowFlags flags)
    {
        static std::once_flag onceFlag_;
        std::call_once(onceFlag_, [&]()
        {
            int flags = SDL_INIT_EVENTS;
            CHECK_CONDITION(0 == SDL_Init(flags));
            OpenJoysticks();

            #if defined(IS_TARGET_MOBILE)
            SDL_SetEventFilter(EventWatch, this);
            #endif
        });

        flags_ = SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_VIDEO;

        CHECK_CONDITION(0 == SDL_InitSubSystem(flags_));

        SetSize(width, height);

        const int DOUBLE_BUFFER = 1;
        const int MIN_DEPTH_SIZE = 24;
        const int MAX_DEPTH_SIZE = 24;
        const int RED_SIZE = 8;
        const int GREEN_SIZE = 8;
        const int BLUE_SIZE = 8;
        const int ALPHA_SIZE = 8;
        const int STENCIL_SIZE = 8;
        const int CONTEXT_MAJOR_VERSION = 2;
        const int CONTEXT_MINOR_VERSION = 0;

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, CONTEXT_MAJOR_VERSION);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, CONTEXT_MINOR_VERSION);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, DOUBLE_BUFFER);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, MAX_DEPTH_SIZE);
        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, RED_SIZE);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, GREEN_SIZE);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, BLUE_SIZE);
        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, ALPHA_SIZE);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, STENCIL_SIZE);

        Uint32 sdlFlags = 0;

        if (flags & (int)WindowFlag::SHOWN)
            sdlFlags |= SDL_WINDOW_SHOWN;

        if (flags & (int)WindowFlag::HIDDEN)
            sdlFlags |= SDL_WINDOW_HIDDEN;

        #if IOS || ANDROID
        {
            sdlFlags |= SDL_WINDOW_FULLSCREEN | SDL_WINDOW_OPENGL;
        }
        #else
        {
            sdlFlags |= SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
        }
        #endif

        auto win = SDL_CreateWindow(name_.c_str(), x, y, width, height, sdlFlags);
        CHECK_CONDITION(win && "failed SDL_CreateWindow");
        windowID_ = SDL_GetWindowID(win);

        if (Window::mainWindow_)
        {
            isMainWindow_ = false;
            // Do not create a new context. Instead, share the main window's context.
            auto context = SDL_GL_GetCurrentContext();
            SDL_GL_MakeCurrent(win, context);
        }
        else
        {
            SDL_GL_CreateContext(win);
            Window::SetMainWindow(this);
        }
        SDL_GL_SetSwapInterval(1);
        SDL_GetWindowSize(win, &width, &height);
        SDL_DisplayMode mode;
        SDL_GetCurrentDisplayMode(0, &mode);
        LOGI("Display format = %s", SDL_GetPixelFormatName(mode.format));
        switch (mode.format)
        {
            case SDL_PIXELFORMAT_RGB888:
                SetPixelFormat(PixelFormat::RGB888);
                break;
            case SDL_PIXELFORMAT_RGB565:
                SetPixelFormat(PixelFormat::RGB565);
                break;
            case SDL_PIXELFORMAT_RGBA8888:
                SetPixelFormat(PixelFormat::RGBA8888);
                break;
            case SDL_PIXELFORMAT_ARGB8888:
                SetPixelFormat(PixelFormat::ARGB8888);
                break;
            default:
                CHECK_ASSERT(!"Unknown pixel format!!!");
                break;
        }

        SetSize(width, height);

        if (isMainWindow_)
        {
            int value = 0;
            SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &value);
            LOGI("CONTEXT_MAJOR_VERSION=%d", value);
            CHECK_ASSERT(value >= CONTEXT_MAJOR_VERSION);
            SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &value);
            LOGI("GL_CONTEXT_MINOR_VERSION=%d", value);
            CHECK_ASSERT(value >= CONTEXT_MINOR_VERSION);
            SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &value);
            LOGI("GL_DOUBLEBUFFER=%d", value);
            CHECK_ASSERT(value == DOUBLE_BUFFER);
            SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &value);
            LOGI("GL_DEPTH_SIZE=%d", value);
            CHECK_ASSERT(value >= MIN_DEPTH_SIZE && value <= MAX_DEPTH_SIZE);
            SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &value);
            LOGI("GL_RED_SIZE=%d", value);
            CHECK_ASSERT(value == RED_SIZE);
            SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &value);
            LOGI("GL_GREEN_SIZE=%d", value);
            CHECK_ASSERT(value == GREEN_SIZE);
            SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &value);
            LOGI("GL_BLUE_SIZE=%d", value);
            CHECK_ASSERT(value == BLUE_SIZE);
            SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE, &value);
            LOGI("GL_ALPHA_SIZE=%d", value);
            CHECK_ASSERT(value == ALPHA_SIZE);
            SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &value);
            LOGI("GL_STENCIL_SIZE=%d", value);
            CHECK_ASSERT(value == STENCIL_SIZE);
        }

        SDL_SetWindowData(SDL_GetWindowFromID(windowID_), InternalPointer, this);
    }

    void SDLWindow::Close()
    {
        Window::Close();
        SDL_QuitSubSystem(flags_);
    }

    void SDLWindow::SetContext()
    {
        auto context = SDL_GL_GetCurrentContext();
        SDL_GL_MakeCurrent(SDL_GetWindowFromID(windowID_), context);
    }

    void SDLWindow::Destroy()
    {
        if (!isClosed_)
        {
            isClosed_ = true;
            Window::NotifyOneWindow2Remove();
            if (isMainWindow_)
            {
                SDL_GL_DeleteContext(SDL_GL_GetCurrentContext());
                Window::SetMainWindow(nullptr);
            }
            SDL_DestroyWindow(SDL_GetWindowFromID(windowID_));
        }
    }

    void SDLWindow::SwapWindowBuffers()
    {
        SDL_GL_SwapWindow(SDL_GetWindowFromID(windowID_));
    }

    void SDLWindow::EnterBackground()
    {
        Window::EnterBackground();
        Object::InvalidateAll();
    }

    void SDLWindow::RestoreContext()
    {
        if (!SDL_GL_GetCurrentContext())
        {
            // On Android the context may be lost behind the scenes as the application is minimized
            LOGI("OpenGL context has been lost. Restoring!!!");
            auto win = SDL_GetWindowFromID(windowID_);
            auto context = SDL_GL_CreateContext(win);
            SDL_GL_MakeCurrent(win, context);
            context_->ResetCachedState();
        }
    }

    SDLWindow* SDLWindow::GetWindowFromID(uint32_t windowID)
    {
        return static_cast<SDLWindow*>(SDL_GetWindowData(SDL_GetWindowFromID(windowID), InternalPointer));
    }

    SDLWindow* SDLWindow::GetCurrentWindow()
    {
        return static_cast<SDLWindow*>(SDL_GetWindowData(SDL_GL_GetCurrentWindow(), InternalPointer));
    }

    JoystickAxis SDLWindow::ConvertAxis(int axis)
    {
        switch (axis)
        {
            case SDL_CONTROLLER_AXIS_LEFTX:
                return JoystickAxis::LEFTX;

            case SDL_CONTROLLER_AXIS_LEFTY:
                return JoystickAxis::LEFTY;

            case SDL_CONTROLLER_AXIS_RIGHTX:
                return JoystickAxis::RIGHTX;

            case SDL_CONTROLLER_AXIS_RIGHTY:
                return JoystickAxis::RIGHTY;

            case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
                return JoystickAxis::TRIGGERLEFT;

            case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
                return JoystickAxis::TRIGGERRIGHT;

            default:
                {
                    LOGW("Unknown joystick axis: %d", axis);
                    return JoystickAxis::UNKNOWN;
                }
        }
    }

    JoystickButton SDLWindow::ConvertButton(int button)
    {
        switch (button)
        {
            case SDL_CONTROLLER_BUTTON_A:
                return JoystickButton::BUTTON_A;
            case SDL_CONTROLLER_BUTTON_B:
                return JoystickButton::BUTTON_B;
            case SDL_CONTROLLER_BUTTON_X:
                return JoystickButton::BUTTON_X;
            case SDL_CONTROLLER_BUTTON_Y:
                return JoystickButton::BUTTON_Y;
            case SDL_CONTROLLER_BUTTON_BACK:
                return JoystickButton::BUTTON_BACK;
            case SDL_CONTROLLER_BUTTON_GUIDE:
                return JoystickButton::BUTTON_GUIDE;
            case SDL_CONTROLLER_BUTTON_START:
                return JoystickButton::BUTTON_START;
            case SDL_CONTROLLER_BUTTON_LEFTSTICK:
                return JoystickButton::BUTTON_LEFTSTICK;
            case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
                return JoystickButton::BUTTON_RIGHTSTICK;
            case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
                return JoystickButton::BUTTON_LEFTSHOULDER;
            case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
                return JoystickButton::BUTTON_RIGHTSHOULDER;
            case SDL_CONTROLLER_BUTTON_DPAD_UP:
                return JoystickButton::BUTTON_DPAD_UP;
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                return JoystickButton::BUTTON_DPAD_DOWN;
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                return JoystickButton::BUTTON_DPAD_LEFT;
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                return JoystickButton::BUTTON_DPAD_RIGHT;
            default:
                {
                    LOGW("Unknown joystick button: %d", button);
                    return JoystickButton::UNKNOWN;
                }
        }
    }

    static int64_t s_touchId = 0;
    void SDLWindow::HandleTouchUpEvent()
    {
        //SDL_FINGERUP event does not work. This is just a simulation
        SDLWindow* window = static_cast<SDLWindow*>(Window::mainWindow_);
        const int MaxFingers = 5;
        TouchFingerEvent touchEvent;
        touchEvent.type = TouchFingerEvent::Type::UP;
        touchEvent.touchId = s_touchId;
        touchEvent.x = 0;
        touchEvent.y = 0;
        touchEvent.dx = 0;
        touchEvent.dy = 0;
        touchEvent.pressure = 0;
        for (auto i = 0; i < MaxFingers; i++)
        {
            if (!SDL_GetTouchFinger(s_touchId, i))
            {
                touchEvent.fingerId = i;
                window->SigTouchFinger()->Run(touchEvent);
            }
        }
    }

    void SDLWindow::HandleEvents()
    {
        #if defined(IS_TARGET_MOBILE)
        SDLWindow::HandleTouchUpEvent();
        #endif
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_WINDOWEVENT)
            {
                SDLWindow* window = GetWindowFromID(event.window.windowID);
                if (!window) continue;
                switch (event.window.event)
                {
                    case SDL_WINDOWEVENT_CLOSE:
                        LOGI("SDL_WINDOWEVENT_CLOSE");
                        window->Close();
                        break;
                    case SDL_WINDOWEVENT_MINIMIZED:
                        LOGI("SDL_WINDOWEVENT_MINIMIZED");
                        window->EnterBackground();
                        break;
                    case SDL_WINDOWEVENT_MAXIMIZED:
                        LOGI("SDL_WINDOWEVENT_MAXIMIZED");
                        window->EnterForeground();
                        break;
                    case SDL_WINDOWEVENT_SIZE_CHANGED:
                        LOGI("SDL_WINDOWEVENT_SIZE_CHANGED");
                        window->ViewChanged(event.window.data1, event.window.data2);
                        break;
                    case SDL_WINDOWEVENT_RESTORED:
                        LOGI("SDL_WINDOWEVENT_RESTORED");
                        window->RestoreContext();
                        window->EnterForeground();
                        break;
                    default:
                        break;
                }
            }
            else if (event.type == SDL_DROPFILE)
            {
                SDLWindow* window = GetCurrentWindow();
                if (!window) continue;
                window->DropFile(event.drop.file);
                SDL_free(event.drop.file);
            }
            else if (event.type == SDL_KEYDOWN)
            {
                SDLWindow* window = GetWindowFromID(event.key.windowID);
                if (!window) continue;
                int key = event.key.keysym.sym & ~SDLK_SCANCODE_MASK;
                #if ANDROID
                {
                    if (key == SDL_SCANCODE_AC_BACK)
                    {
                        RenderingContext::GetPtr()->ResetCachedState();
                        window->Close();
                        exit(0);
                    }
                }
                #endif

                int action = NSG_KEY_PRESS;
                int modifier = event.key.keysym.mod;
                //int scancode = event.key.keysym.scancode;
                window->OnKey(key, action, modifier);
            }
            else if (event.type == SDL_KEYUP)
            {
                SDLWindow* window = GetWindowFromID(event.key.windowID);
                if (!window) continue;
                int key = event.key.keysym.sym & ~SDLK_SCANCODE_MASK;
                //int scancode = event.key.keysym.scancode;
                int action = NSG_KEY_RELEASE;
                int modifier = event.key.keysym.mod;
                window->OnKey(key, action, modifier);
            }
            else if (event.type == SDL_TEXTINPUT)
            {
                SDLWindow* window = GetWindowFromID(event.text.windowID);
                if (!window) continue;
                window->OnText(event.text.text);
                UTF8String utf8(event.text.text);
                unsigned unicode = utf8.AtUTF8(0);
                if (unicode)
                    window->OnChar(unicode);
            }
            else if (event.type == SDL_MOUSEBUTTONDOWN)
            {
                SDLWindow* window = GetWindowFromID(event.button.windowID);
                if (!window) continue;
                window->OnMouseDown(event.button.button, event.button.x, event.button.y);
            }
            else if (event.type == SDL_MOUSEBUTTONUP)
            {
                SDLWindow* window = GetWindowFromID(event.button.windowID);
                if (!window) continue;
                window->OnMouseUp(event.button.button, event.button.x, event.button.y);
            }
            else if (event.type == SDL_MOUSEMOTION)
            {
                SDLWindow* window = GetWindowFromID(event.button.windowID);
                if (!window) continue;
                window->OnMouseMove(event.motion.x, event.motion.y);
            }
            else if (event.type == SDL_MOUSEWHEEL)
            {
                SDLWindow* window = GetWindowFromID(event.wheel.windowID);
                if (!window) continue;
                window->OnMouseWheel((float)event.wheel.x, (float)event.wheel.y);
            }
            else if (event.type == SDL_FINGERDOWN || event.type == SDL_FINGERUP || event.type == SDL_FINGERMOTION)
            {
                SDLWindow* window = static_cast<SDLWindow*>(Window::mainWindow_);
                if (!window) continue;
                auto x = (int)(event.tfinger.x * window->GetWidth());
                auto y = (int)(event.tfinger.y * window->GetHeight());
                TouchFingerEvent touchEvent;
                touchEvent.type = TouchFingerEvent::Type::MOTION;
                if (event.type == SDL_FINGERDOWN)
                {
                    window->OnMouseDown(NSG_BUTTON_LEFT, x, y);
                    touchEvent.type = TouchFingerEvent::Type::DOWN;
                }
                else if (event.type == SDL_FINGERUP)
                {
                    window->OnMouseUp(NSG_BUTTON_LEFT, x, y);
                    touchEvent.type = TouchFingerEvent::Type::UP;
                }
                else
                    window->OnMouseMove(x, y);

                s_touchId = touchEvent.touchId = event.tfinger.touchId;
                touchEvent.fingerId = event.tfinger.fingerId;
                touchEvent.x = event.tfinger.x;
                touchEvent.y = event.tfinger.y;
                touchEvent.dx = event.tfinger.dx;
                touchEvent.dy = event.tfinger.dy;
                touchEvent.pressure = event.tfinger.pressure;
                window->SigTouchFinger()->Run(touchEvent);
            }
            #if defined(IS_TARGET_MOBILE)
            else if (event.type == SDL_MULTIGESTURE)
            {
                SDLWindow* window = static_cast<SDLWindow*>(Window::mainWindow_);
                if (!window) continue;
                float x = event.mgesture.x;
                float y = event.mgesture.y;
                window->OnMultiGesture(event.mgesture.timestamp, -1 + 2 * x, 1 + -2 * y, event.mgesture.dTheta, event.mgesture.dDist, (int)event.mgesture.numFingers);
            }
            #endif
            else if (event.type == SDL_JOYDEVICEADDED)
            {
                SDLWindow* window = static_cast<SDLWindow*>(Window::mainWindow_);
                if (!window) continue;
                auto which = event.jdevice.which;
                window->OpenJoystick(which);
            }
            else if (event.type == SDL_JOYDEVICEREMOVED)
            {
                SDLWindow* window = static_cast<SDLWindow*>(Window::mainWindow_);
                if (!window) continue;
                auto which = event.jdevice.which;
                window->CloseJoystick(which);
            }
            else if (event.type == SDL_CONTROLLERBUTTONDOWN)
            {
                SDLWindow* window = static_cast<SDLWindow*>(Window::mainWindow_);
                if (!window) continue;
                auto& state = window->joysticks_.find(event.cbutton.which)->second;
                CHECK_ASSERT(state.pad_);
                auto button = ConvertButton(event.cbutton.button);
                window->OnJoystickDown(state.instanceID_, button);
            }
            else if (event.type == SDL_CONTROLLERBUTTONUP)
            {
                SDLWindow* window = static_cast<SDLWindow*>(Window::mainWindow_);
                if (!window) continue;
                auto& state = window->joysticks_.find(event.cbutton.which)->second;
                CHECK_ASSERT(state.pad_);
                auto button = ConvertButton(event.cbutton.button);
                window->OnJoystickUp(state.instanceID_, button);
            }
            else if (event.type == SDL_CONTROLLERAXISMOTION)
            {
                SDLWindow* window = static_cast<SDLWindow*>(Window::mainWindow_);
                if (!window) continue;
                auto& state = window->joysticks_.find(event.caxis.which)->second;
                CHECK_ASSERT(state.pad_);
                auto axis = ConvertAxis(event.caxis.axis);
                float value = (float)event.caxis.value;
                if (std::abs(value) < 5000) value = 0;
                auto position = Clamp(value / 32767.0f, -1.0f, 1.0f);
                window->OnJoystickAxisMotion(state.instanceID_, axis, position);
            }

            else if (event.type == SDL_JOYBUTTONDOWN)
            {
                SDLWindow* window = static_cast<SDLWindow*>(Window::mainWindow_);
                if (!window) continue;
                auto& state = window->joysticks_.find(event.jbutton.which)->second;
                if (!state.pad_)
                {
                    auto button = ConvertButton(event.jbutton.button);
                    window->OnJoystickDown(state.instanceID_, button);
                }
            }
            else if (event.type == SDL_JOYBUTTONUP)
            {
                SDLWindow* window = static_cast<SDLWindow*>(Window::mainWindow_);
                if (!window) continue;
                auto& state = window->joysticks_.find(event.jbutton.which)->second;
                if (!state.pad_)
                {
                    auto button = ConvertButton(event.jbutton.button);
                    window->OnJoystickUp(state.instanceID_, button);
                }
            }
            else if (event.type == SDL_JOYAXISMOTION)
            {
                SDLWindow* window = static_cast<SDLWindow*>(Window::mainWindow_);
                if (!window) continue;
                auto& state = window->joysticks_.find(event.jaxis.which)->second;
                if (!state.pad_)
                {
                    auto axis = ConvertAxis(event.jaxis.axis);
                    float value = (float)event.jaxis.value;
                    if (std::abs(value) < 5000) value = 0;
                    auto position = Clamp(value / 32767.0f, -1.0f, 1.0f);
                    window->OnJoystickAxisMotion(state.instanceID_, axis, position);
                }
            }
        }
    }

    void SDLWindow::Show()
    {
        SDL_ShowWindow(SDL_GetWindowFromID(windowID_));
    }

    void SDLWindow::Hide()
    {
        SDL_HideWindow(SDL_GetWindowFromID(windowID_));
    }

    void SDLWindow::Raise()
    {
        SDL_RaiseWindow(SDL_GetWindowFromID(windowID_));
    }

    static const char* ImGuiGetClipboardText()
    {
        return SDL_GetClipboardText();
    }

    static void ImGuiSetClipboardText(const char* text)
    {
        SDL_SetClipboardText(text);
    }

    void SDLWindow::SetupImgui()
    {
        Window::SetupImgui();
        ImGuiIO& io = ImGui::GetIO();
        io.SetClipboardTextFn = ImGuiSetClipboardText;
        io.GetClipboardTextFn = ImGuiGetClipboardText;
    }

    void SDLWindow::BeginImguiRender()
    {
        ImGuiIO& io = ImGui::GetIO();
        #ifdef _MSC_VER
        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);
        SDL_GetWindowWMInfo(SDL_GetWindowFromID(windowID_), &wmInfo);
        io.ImeWindowHandle = wmInfo.info.win.window;
        #endif
        // Hide OS mouse cursor if ImGui is drawing it
        SDL_ShowCursor(io.MouseDrawCursor ? 0 : 1);
    }
}
#endif