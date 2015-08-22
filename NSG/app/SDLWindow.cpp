/*
-------------------------------------------------------------------------------
This file is part of nsg-library.
http://github.com/woodjazz/nsg-library

Copyright (c) 2014-2015 Néstor Silveira Gorski

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
#include "Graphics.h"
#include "Tick.h"
#include "Keys.h"
#include "Log.h"
#include "UTF8String.h"
#include "AppConfiguration.h"
#include "Object.h"
#include "Graphics.h"
#include "Scene.h"
#include "imgui.h"
#include <memory>
#include <string>
#include <locale>
#include <thread>
#include <mutex>
#ifndef __GNUC__
#include <codecvt>
#endif

#if EMSCRIPTEN
#include <emscripten.h>
#include <html5.h>
#endif

namespace NSG
{
    #if EMSCRIPTEN
    static EM_BOOL EmscriptenResizeCallback(int eventType, const EmscriptenUiEvent* keyEvent, void* userData)
    {
        using namespace NSG;
        Window* window = Window::GetMainWindow();
        if (window)
            window->ViewChanged(keyEvent->windowInnerWidth, keyEvent->windowInnerHeight);
        return false;
    }
    #elif defined(IS_TARGET_MOBILE)
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
        if (this == graphics_->GetWindow())
            graphics_->SetWindow(nullptr);
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
                #if !defined(EMSCRIPTEN)
                if (SDL_IsGameController(state.deviceIndex))
                    SDL_GameControllerClose(static_cast<SDL_GameController*>(state.pad_));
                #endif
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
            #if EMSCRIPTEN
            int flags = 0;
            #else
            int flags = SDL_INIT_EVENTS;
            #endif

            CHECK_CONDITION(0 == SDL_Init(flags), __FILE__, __LINE__);

            OpenJoysticks();

            #if defined(IS_TARGET_MOBILE)
            SDL_SetEventFilter(EventWatch, this);
            #endif
        });

        #if EMSCRIPTEN
        flags_ = SDL_INIT_JOYSTICK | SDL_INIT_VIDEO;
        #else
        flags_ = SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_VIDEO;
        #endif

        CHECK_CONDITION(0 == SDL_InitSubSystem(flags_), __FILE__, __LINE__);

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

        #if EMSCRIPTEN
        {
            CHECK_CONDITION( nullptr != SDL_SetVideoMode(width, height, 32, SDL_OPENGL | SDL_RESIZABLE), __FILE__, __LINE__);
            #if 0
            SDL_Surface* surface = SDL_GetVideoSurface();
            LOGI("BitsPerPixel=%d", surface->format->BitsPerPixel);
            LOGI("BytesPerPixel=%d", surface->format->BytesPerPixel);
            LOGI("Rmask=%d", surface->format->Rmask);
            LOGI("Gmask=%d", surface->format->Gmask);
            LOGI("Bmask=%d", surface->format->Bmask);
            LOGI("Amask=%d", surface->format->Amask);
            #endif
            isMainWindow_ = true;
            Window::SetMainWindow(this);
            emscripten_set_resize_callback(nullptr, nullptr, false, EmscriptenResizeCallback);
        }
        #else
        {
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
            CHECK_CONDITION(win && "failed SDL_CreateWindow", __FILE__, __LINE__);
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
                    CHECK_ASSERT(!"Unknown pixel format!!!", __FILE__, __LINE__);
                    break;
            }
        }
        #endif

        SetSize(width, height);

        int value = 0;
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &value);
        LOGI("CONTEXT_MAJOR_VERSION=%d", value);
        CHECK_ASSERT(value >= CONTEXT_MAJOR_VERSION, __FILE__, __LINE__);
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &value);
        LOGI("GL_CONTEXT_MINOR_VERSION=%d", value);
        CHECK_ASSERT(value >= CONTEXT_MINOR_VERSION, __FILE__, __LINE__);
        SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &value);
        LOGI("GL_DOUBLEBUFFER=%d", value);
        CHECK_ASSERT(value == DOUBLE_BUFFER, __FILE__, __LINE__);
        SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &value);
        LOGI("GL_DEPTH_SIZE=%d", value);
        CHECK_ASSERT(value >= MIN_DEPTH_SIZE && value <= MAX_DEPTH_SIZE, __FILE__, __LINE__);
        SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &value);
        LOGI("GL_RED_SIZE=%d", value);
        CHECK_ASSERT(value == RED_SIZE, __FILE__, __LINE__);
        SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &value);
        LOGI("GL_GREEN_SIZE=%d", value);
        CHECK_ASSERT(value == GREEN_SIZE, __FILE__, __LINE__);
        SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &value);
        LOGI("GL_BLUE_SIZE=%d", value);
        CHECK_ASSERT(value == BLUE_SIZE, __FILE__, __LINE__);
        SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE, &value);
        LOGI("GL_ALPHA_SIZE=%d", value);
        CHECK_ASSERT(value == ALPHA_SIZE, __FILE__, __LINE__);
        SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &value);
        LOGI("GL_STENCIL_SIZE=%d", value);
        CHECK_ASSERT(value == STENCIL_SIZE, __FILE__, __LINE__);

        #if defined(IS_WINDOWS) || defined(IS_LINUX)
        {
            glewExperimental = true; // Needed for core profile. Solves issue with glGenVertexArrays
            CHECK_CONDITION(GLEW_OK == glewInit(), __FILE__, __LINE__);
            CHECK_CONDITION(GLEW_EXT_framebuffer_object && GLEW_EXT_packed_depth_stencil, __FILE__, __LINE__)
        }
        #endif

        OnReady();

        #if !defined(EMSCRIPTEN)
        SDL_SetWindowData(SDL_GetWindowFromID(windowID_), InternalPointer, this);
        #endif
    }

    void SDLWindow::Close()
    {
        Window::Close();
        SDL_QuitSubSystem(flags_);
    }

    void SDLWindow::Destroy()
    {
        #if !defined(EMSCRIPTEN)
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
        #endif
    }

    void SDLWindow::ViewChanged(int width, int height)
    {
        if (width_ != width || height_ != height)
        {
            #if EMSCRIPTEN
            CHECK_CONDITION( nullptr != SDL_SetVideoMode(width, height, 32, SDL_OPENGL | SDL_RESIZABLE), __FILE__, __LINE__);
            //emscripten_set_canvas_size(width, height);
            #endif
            Window::ViewChanged(width, height);
        }
    }

    void SDLWindow::SwapWindowBuffers()
    {
        #if !defined(EMSCRIPTEN)
        SDL_GL_SwapWindow(SDL_GetWindowFromID(windowID_));
        #endif
    }

    void SDLWindow::EnterBackground()
    {
        Window::EnterBackground();
        Object::InvalidateAll();
    }

    void SDLWindow::RestoreContext()
    {
        #if !defined(EMSCRIPTEN)
        if (!SDL_GL_GetCurrentContext())
        {
            // On Android the context may be lost behind the scenes as the application is minimized
            LOGI("OpenGL context has been lost. Restoring!!!");
            auto win = SDL_GetWindowFromID(windowID_);
            auto context = SDL_GL_CreateContext(win);
            SDL_GL_MakeCurrent(win, context);
            graphics_->ResetCachedState();
        }
        #endif
    }

    void SDLWindow::EnterForeground()
    {
        Window::EnterForeground();
    }

    SDLWindow* SDLWindow::GetWindowFromID(uint32_t windowID)
    {
        #if EMSCRIPTEN
        return static_cast<SDLWindow*>(mainWindow_);
        #else
        return static_cast<SDLWindow*>(SDL_GetWindowData(SDL_GetWindowFromID(windowID), InternalPointer));
        #endif
    }

    SDLWindow* SDLWindow::GetCurrentWindow()
    {
        #if EMSCRIPTEN
        return static_cast<SDLWindow*>(mainWindow_);
        #else
        return static_cast<SDLWindow*>(SDL_GetWindowData(SDL_GL_GetCurrentWindow(), InternalPointer));
        #endif
    }

    JoystickAxis SDLWindow::ConvertAxis(int axis)
    {
        #if !defined(EMSCRIPTEN)
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
        #else
        if (axis >= (int)JoystickAxis::FIRST && axis < (int)JoystickAxis::LAST)
            return (JoystickAxis)axis;
        else
        {
            LOGW("Unknown joystick axis: %d", axis);
            return JoystickAxis::UNKNOWN;
        }
        #endif
    }

    JoystickButton SDLWindow::ConvertButton(int button)
    {
        #if !defined(EMSCRIPTEN)
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
        #else
        if (button >= (int)JoystickButton::FIRST && button < (int)JoystickButton::LAST)
            return (JoystickButton)button;
        else
        {
            LOGW("Unknown joystick button: %d", button);
            return JoystickButton::UNKNOWN;
        }
        #endif
    }

    void SDLWindow::HandleEvents()
    {
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
            #if !defined(EMSCRIPTEN)
            else if (event.type == SDL_DROPFILE)
            {
                SDLWindow* window = GetCurrentWindow();
                if (!window) continue;
                window->DropFile(event.drop.file);
                SDL_free(event.drop.file);
            }
            #endif
            else if (event.type == SDL_KEYDOWN)
            {
                SDLWindow* window = GetWindowFromID(event.key.windowID);
                if (!window) continue;
                int key = event.key.keysym.sym & ~SDLK_SCANCODE_MASK;
                #if ANDROID
                {
                    if (key == SDL_SCANCODE_AC_BACK)
                    {
                        Graphics::GetPtr()->ResetCachedState();
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
                float x = (float)event.button.x;
                float y = (float)event.button.y;
                auto width = window->GetWidth();
                auto height = window->GetHeight();
                window->OnMouseDown(event.button.button, -1 + 2 * x / width, 1 + -2 * y / height);
            }
            else if (event.type == SDL_MOUSEBUTTONUP)
            {
                SDLWindow* window = GetWindowFromID(event.button.windowID);
                if (!window) continue;
                float x = (float)event.button.x;
                float y = (float)event.button.y;
                auto width = window->GetWidth();
                auto height = window->GetHeight();
                window->OnMouseUp(event.button.button, -1 + 2 * x / width, 1 + -2 * y / height);

            }
            else if (event.type == SDL_MOUSEMOTION)
            {
                SDLWindow* window = GetWindowFromID(event.button.windowID);
                if (!window) continue;
                window->OnMouseMove(event.motion.x, event.motion.y);
                float x = (float)event.motion.x;
                float y = (float)event.motion.y;
                auto width = window->GetWidth();
                auto height = window->GetHeight();
                window->OnMouseMove(-1 + 2 * x / width, 1 + -2 * y / height);
            }
            else if (event.type == SDL_MOUSEWHEEL)
            {
                SDLWindow* window = GetWindowFromID(event.wheel.windowID);
                if (!window) continue;
                window->OnMouseWheel((float)event.wheel.x, (float)event.wheel.y);
            }
            else if (event.type == SDL_FINGERDOWN)
            {
                SDLWindow* window = static_cast<SDLWindow*>(mainWindow_);
                if (!window) continue;
                float x = event.tfinger.x;
                float y = event.tfinger.y;
                window->OnMouseDown(0, -1 + 2 * x, 1 + -2 * y);
            }
            else if (event.type == SDL_FINGERUP)
            {
                SDLWindow* window = static_cast<SDLWindow*>(mainWindow_);
                if (!window) continue;
                float x = event.tfinger.x;
                float y = event.tfinger.y;
                window->OnMouseUp(0, -1 + 2 * x, 1 + -2 * y);
            }
            else if (event.type == SDL_FINGERMOTION)
            {
                SDLWindow* window = static_cast<SDLWindow*>(mainWindow_);
                if (!window) continue;
                float x = event.tfinger.x;
                float y = event.tfinger.y;
                window->OnMouseMove(-1 + 2 * x, 1 + -2 * y);
            }
            #if defined(IS_TARGET_MOBILE)
            else if (event.type == SDL_MULTIGESTURE)
            {
                SDLWindow* window = static_cast<SDLWindow*>(mainWindow_);
                if (!window) continue;
                float x = event.mgesture.x;
                float y = event.mgesture.y;
                window->OnMultiGesture(event.mgesture.timestamp, -1 + 2 * x, 1 + -2 * y, event.mgesture.dTheta, event.mgesture.dDist, (int)event.mgesture.numFingers);
            }
            #endif
            #if !defined(EMSCRIPTEN)
            else if (event.type == SDL_JOYDEVICEADDED)
            {
                SDLWindow* window = static_cast<SDLWindow*>(mainWindow_);
                if (!window) continue;
                auto which = event.jdevice.which;
                window->OpenJoystick(which);
            }
            else if (event.type == SDL_JOYDEVICEREMOVED)
            {
                SDLWindow* window = static_cast<SDLWindow*>(mainWindow_);
                if (!window) continue;
                auto which = event.jdevice.which;
                window->CloseJoystick(which);
            }
            else if (event.type == SDL_CONTROLLERBUTTONDOWN)
            {
                SDLWindow* window = static_cast<SDLWindow*>(mainWindow_);
                if (!window) continue;
                auto& state = window->joysticks_.find(event.cbutton.which)->second;
                CHECK_ASSERT(state.pad_, __FILE__, __LINE__);
                auto button = ConvertButton(event.cbutton.button);
                window->OnJoystickDown(state.instanceID_, button);
            }
            else if (event.type == SDL_CONTROLLERBUTTONUP)
            {
                SDLWindow* window = static_cast<SDLWindow*>(mainWindow_);
                if (!window) continue;
                auto& state = window->joysticks_.find(event.cbutton.which)->second;
                CHECK_ASSERT(state.pad_, __FILE__, __LINE__);
                auto button = ConvertButton(event.cbutton.button);
                window->OnJoystickUp(state.instanceID_, button);
            }
            else if (event.type == SDL_CONTROLLERAXISMOTION)
            {
                SDLWindow* window = static_cast<SDLWindow*>(mainWindow_);
                if (!window) continue;
                auto& state = window->joysticks_.find(event.caxis.which)->second;
                CHECK_ASSERT(state.pad_, __FILE__, __LINE__);
                auto axis = ConvertAxis(event.caxis.axis);
                float value = (float)event.caxis.value;
                if (std::abs(value) < 5000) value = 0;
                auto position = glm::clamp(value / 32767.0f, -1.0f, 1.0f);
                window->OnJoystickAxisMotion(state.instanceID_, axis, position);
            }
            #endif

            else if (event.type == SDL_JOYBUTTONDOWN)
            {
                SDLWindow* window = static_cast<SDLWindow*>(mainWindow_);
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
                SDLWindow* window = static_cast<SDLWindow*>(mainWindow_);
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
                SDLWindow* window = static_cast<SDLWindow*>(mainWindow_);
                if (!window) continue;
                auto& state = window->joysticks_.find(event.jaxis.which)->second;
                if (!state.pad_)
                {
                    auto axis = ConvertAxis(event.jaxis.axis);
                    float value = (float)event.jaxis.value;
                    if (std::abs(value) < 5000) value = 0;
                    auto position = glm::clamp(value / 32767.0f, -1.0f, 1.0f);
                    window->OnJoystickAxisMotion(state.instanceID_, axis, position);
                }
            }
        }
    }

    void SDLWindow::Show()
    {
        #if !defined(EMSCRIPTEN)
        SDL_ShowWindow(SDL_GetWindowFromID(windowID_));
        #endif
    }

    void SDLWindow::Hide()
    {
        #if !defined(EMSCRIPTEN)
        SDL_HideWindow(SDL_GetWindowFromID(windowID_));
        #endif

    }

    void SDLWindow::Raise()
    {
        #if !defined(EMSCRIPTEN)
        SDL_RaiseWindow(SDL_GetWindowFromID(windowID_));
        #endif
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
        ImGuiIO& io = ImGui::GetIO();
        io.KeyMap[ImGuiKey_Tab] = SDLK_TAB;                 // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
        io.KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
        io.KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
        io.KeyMap[ImGuiKey_UpArrow] = SDL_SCANCODE_UP;
        io.KeyMap[ImGuiKey_DownArrow] = SDL_SCANCODE_DOWN;
        io.KeyMap[ImGuiKey_PageUp] = SDL_SCANCODE_PAGEUP;
        io.KeyMap[ImGuiKey_PageDown] = SDL_SCANCODE_PAGEDOWN;
        io.KeyMap[ImGuiKey_Home] = SDL_SCANCODE_HOME;
        io.KeyMap[ImGuiKey_End] = SDL_SCANCODE_END;
        io.KeyMap[ImGuiKey_Delete] = SDLK_DELETE;
        io.KeyMap[ImGuiKey_Backspace] = SDLK_BACKSPACE;
        io.KeyMap[ImGuiKey_Enter] = SDLK_RETURN;
        io.KeyMap[ImGuiKey_Escape] = SDLK_ESCAPE;
        io.KeyMap[ImGuiKey_A] = SDLK_a;
        io.KeyMap[ImGuiKey_C] = SDLK_c;
        io.KeyMap[ImGuiKey_V] = SDLK_v;
        io.KeyMap[ImGuiKey_X] = SDLK_x;
        io.KeyMap[ImGuiKey_Y] = SDLK_y;
        io.KeyMap[ImGuiKey_Z] = SDLK_z;

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