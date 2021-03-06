/*
-------------------------------------------------------------------------------
This file is part of nsg-library.
http://github.com/woodjazz/nsg-library

Copyright (c) 2014-2017 Néstor Silveira Gorski

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
#include "Window.h"
#include "AndroidWindow.h"
#include "AppConfiguration.h"
#include "Check.h"
#include "EmscriptenWindow.h"
#include "Engine.h"
#include "ExternalWindow.h"
#include "FrameBuffer.h"
#include "GUI.h"
#include "IOSWindow.h"
#include "Keys.h"
#include "LinuxX11EGLWindow.h"
#include "LinuxX11GLXWindow.h"
#include "Log.h"
#include "Material.h"
#include "OSXWindow.h"
#include "Pass.h"
#include "Program.h"
#include "QuadMesh.h"
#include "Renderer.h"
#include "RenderingContext.h"
#include "Scene.h"
#include "SharedFromPointer.h"
#include "SignalSlots.h"
#include "TextMesh.h"
#include "Texture.h"
#include "UTF8String.h"
#include "WinWindow.h"
#include "imgui.h"
#include <algorithm>
#include <memory>
#include <thread>
#ifndef __GNUC__
#include <codecvt>
#endif

namespace NSG {
std::vector<PWeakWindow> Window::windows_;
Window* Window::mainWindow_ = nullptr;
int Window::nWindows2Remove_ = 0;

Window::Window(const std::string& name)
    : Object(name), isClosed_(false), minimized_(false), isMainWindow_(true),
      width_(0), height_(0), signalViewChanged_(new Signal<int, int>()),
      signalFloatFloat_(new Signal<float, float>()),
      signalMouseMoved_(new Signal<int, int>()),
      signalMouseDown_(new Signal<int, float, float>()),
      signalMouseDownInt_(new Signal<int, int, int>()),
      signalMouseUp_(new Signal<int, float, float>()),
      signalMouseUpInt_(new Signal<int, int, int>()),
      signalMouseWheel_(new Signal<float, float>()),
      signalKey_(new Signal<int, int, int>()),
      signalUnsigned_(new Signal<unsigned int>()),
      signalText_(new Signal<std::string>()),
      signalMultiGesture_(new Signal<int, float, float, float, float, int>()),
      signalDropFile_(new Signal<const std::string&>()),
      signalJoystickDown_(new SignalJoystickButton),
      signalJoystickUp_(new SignalJoystickButton),
      signalJoystickAxisMotion_(new Signal<int, JoystickAxis, float>),
      signalDrawIMGUI_(new Signal<>), signalTouchFinger_(new SignalTouchFinger),
      pixelFormat_(PixelFormat::UNKNOWN), render_(nullptr) {
    CHECK_CONDITION(Window::AllowWindowCreation());
}

Window::~Window() {
    gui_ = nullptr;
    LOGI("Window %s terminated.", name_.c_str());
}

PWindow Window::CreateExternal(const std::string& name) {
    auto window = std::make_shared<ExternalWindow>(name);
    Window::AddWindow(window);
    return window;
}

PWindow Window::Create(const std::string& name, WindowFlags flags) {
    if (Window::AllowWindowCreation()) {
#if defined(EMSCRIPTEN)
        auto window = std::make_shared<EmscriptenWindow>(name, flags);
#elif defined(IS_TARGET_WINDOWS)
        auto window = std::make_shared<WinWindow>(name, flags);
#elif defined(IS_TARGET_LINUX)
        auto conf = Engine::GetAppConfiguration();
        PWindow window;
        if (conf.platform_ == AppConfiguration::Platform::UseGLX)
            window = std::make_shared<LinuxX11GLXWindow>(name, flags);
#if defined(EGL)
        else
            window = std::make_shared<LinuxX11EGLWindow>(name, flags);
#endif
#elif defined(IS_TARGET_OSX)
        auto window = std::make_shared<OSXWindow>(name, flags);
#elif defined(IS_TARGET_IOS)
        auto window = std::make_shared<IOSWindow>(name, flags);
#elif defined(IS_TARGET_ANDROID)
        auto window = std::make_shared<AndroidWindow>(name, flags);
#else
#error("Unknown platform!!!")
#endif
        Window::AddWindow(window);
        return window;
    }
    return nullptr;
}

PWindow Window::Create(const std::string& name, int x, int y, int width,
                       int height, WindowFlags flags) {
    if (Window::AllowWindowCreation()) {
#if defined(EMSCRIPTEN)
        auto window = std::make_shared<EmscriptenWindow>(name, flags);
#elif defined(IS_TARGET_WINDOWS)
        auto window =
            std::make_shared<WinWindow>(name, x, y, width, height, flags);
#elif defined(IS_TARGET_LINUX)
        auto conf = Engine::GetAppConfiguration();
        PWindow window;
        if (conf.platform_ == AppConfiguration::Platform::UseGLX)
            window = std::make_shared<LinuxX11GLXWindow>(name, x, y, width,
                                                         height, flags);
#if defined(EGL)
        else
            window = std::make_shared<LinuxX11EGLWindow>(name, x, y, width,
                                                         height, flags);
#endif
#elif defined(IS_TARGET_OSX)
        auto window =
            std::make_shared<OSXWindow>(name, x, y, width, height, flags);
#elif defined(IS_TARGET_IOS)
        auto window =
            std::make_shared<IOSWindow>(name, x, y, width, height, flags);
#elif defined(IS_TARGET_ANDROID)
        auto window =
            std::make_shared<AndroidWindow>(name, x, y, width, height, flags);
#else
#error("Unknown platform!!!")
#endif
        Window::AddWindow(window);
        return window;
    }
    return nullptr;
}

void Window::Close() {
    LOGI("Closing %s window.", name_.c_str());

    if (Window::mainWindow_ == this) {
        // context_->ResetCachedState();
        // destroy other windows
        auto windows = Window::GetWindows();
        for (auto& obj : windows) {
            PWindow window = obj.lock();
            if (window && window.get() != this)
                window->Destroy();
        }
    }
    Destroy();
}

void Window::AllocateResources() {
    renderer_ = Renderer::Create();
    context_ = RenderingContext::GetSharedPtr();
    gui_ = std::make_shared<GUI>();
}

void Window::ReleaseResources() {
    gui_ = nullptr;
    renderer_ = nullptr;
    context_ = nullptr;
}

void Window::SetSize(int width, int height) {
    if (width_ != width || height_ != height) {
        LOGI("WindowChanged: %d,%d", width, height);
        width_ = width;
        height_ = height;
        signalViewChanged_->Run(width, height);
    }
}

void Window::ViewChanged(int width, int height) { SetSize(width, height); }

void Window::OnMouseMove(float x, float y) { signalFloatFloat_->Run(x, y); }

void Window::OnMouseMove(int x, int y) {
    signalMouseMoved_->Run(x, y);
    auto mx = (float)x;
    auto my = (float)y;
    auto width = GetWidth();
    auto height = GetHeight();
    OnMouseMove(-1 + 2 * mx / width, 1 + -2 * my / height);
}

void Window::OnMouseDown(int button, float x, float y) {
    signalMouseDown_->Run(button, x, y);
}

void Window::OnMouseDown(int button, int x, int y) {
    signalMouseDownInt_->Run(button, x, y);
    float mx = (float)x;
    float my = (float)y;
    auto width = GetWidth();
    auto height = GetHeight();
    OnMouseDown(button, -1 + 2 * mx / width, 1 + -2 * my / height);
}

void Window::OnMouseUp(int button, float x, float y) {
    signalMouseUp_->Run(button, x, y);
}

void Window::OnMouseUp(int button, int x, int y) {
    signalMouseUpInt_->Run(button, x, y);
    float mx = (float)x;
    float my = (float)y;
    auto width = GetWidth();
    auto height = GetHeight();
    OnMouseUp(button, -1 + 2 * mx / width, 1 + -2 * my / height);
}

void Window::OnMultiGesture(int timestamp, float x, float y, float dTheta,
                            float dDist, int numFingers) {
    signalMultiGesture_->Run(timestamp, x, y, dTheta, dDist, numFingers);
}

void Window::OnMouseWheel(float x, float y) { signalMouseWheel_->Run(x, y); }

void Window::OnKey(int key, int action, int modifier) {
    signalKey_->Run(key, action, modifier);
}

void Window::OnChar(unsigned int character) { signalUnsigned_->Run(character); }

void Window::OnText(const std::string& text) { signalText_->Run(text); }

void Window::OnJoystickDown(int joystickID, JoystickButton button) {
    SigJoystickDown()->Run(joystickID, button);
}

void Window::OnJoystickUp(int joystickID, JoystickButton button) {
    SigJoystickUp()->Run(joystickID, button);
}

void Window::OnJoystickAxisMotion(int joystickID, JoystickAxis axis,
                                  float position) {
    SigJoystickAxisMotion()->Run(joystickID, axis, position);
}

void Window::EnterBackground() { minimized_ = true; }

void Window::EnterForeground() { minimized_ = false; }

void Window::DropFile(const std::string& filePath) {
    signalDropFile_->Run(filePath);
}

Vector4 Window::GetViewport() const { return Vector4(0, 0, width_, height_); }

bool Window::AllowWindowCreation() {
#if defined(IS_TARGET_MOBILE) || defined(IS_TARGET_WEB)
    {
        if (windows_.size()) {
            LOGW("Only one window is allowed for this platform.");
            return false;
        }
    }
#endif
    return true;
}

void Window::SetMainWindow(Window* window) {
    if (Window::mainWindow_ != window)
        Window::mainWindow_ = window;
}

void Window::AddWindow(PWindow window) {
    CHECK_ASSERT(AllowWindowCreation());
    windows_.push_back(window);
}

void Window::UpdateScenes(float delta) {
    for (auto& obj : windows_) {
        auto window(obj.lock());
        if (window) {
            auto scene = window->scene_.lock();
            if (scene)
                scene->UpdateAll(delta);
        }
    }
}

void Window::SetRender(IRender* render) {
    CHECK_CONDITION(render_ == nullptr);
    render_ = render;
}

void Window::RemoveRender(IRender* render) {
    CHECK_CONDITION(render_ == render);
    render_ = nullptr;
}

bool Window::AreAllWindowsMinimized() {
    for (auto& obj : windows_) {
        auto window(obj.lock());
        if (!window || window->IsClosed())
            continue;
        if (!window->IsMinimized())
            return false;
    }
    return true;
}

void Window::RenderFrame() {
    if (IsReady()) {
        if (render_)
            render_->Render();
        else {
            auto scene = scene_.lock();
            renderer_->Render(this, scene.get());
            if (SigDrawIMGUI()->HasSlots()) {
                gui_->Render(SharedFromPointer(this),
                             [this]() { SigDrawIMGUI()->Run(); });
            }
        }
        SwapWindowBuffers();
    }
}

bool Window::RenderWindows() {
    auto windows = windows_;
    for (auto& obj : windows) {
        auto window(obj.lock());
        if (!window || window->IsClosed())
            continue;
        if (!window->IsMinimized())
            window->RenderFrame();
    }

    while (nWindows2Remove_) {
        windows_.erase(std::remove_if(windows_.begin(), windows_.end(),
                                      [&](PWeakWindow window) {
                                          if (!window.lock() ||
                                              window.lock()->IsClosed()) {
                                              --nWindows2Remove_;
                                              return true;
                                          }
                                          return false;
                                      }),
                       windows_.end());
    }

    if (!Window::mainWindow_)
        return false;
    else if (Window::mainWindow_->IsMinimized()) {
        std::this_thread::sleep_for(Milliseconds(100));
    }
    return true;
}

void Window::SetScene(PScene scene) { scene_ = scene; }

void Window::NotifyOneWindow2Remove() { ++nWindows2Remove_; }

void Window::SetupImgui() {
    // Keyboard mapping. ImGui will use those indices to peek into the
    // io.KeyDown[] array.
    ImGuiIO& io = ImGui::GetIO();
    io.KeyMap[ImGuiKey_Tab] = NSG_KEY_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = NSG_KEY_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = NSG_KEY_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = NSG_KEY_UP;
    io.KeyMap[ImGuiKey_DownArrow] = NSG_KEY_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = NSG_KEY_PAGEUP;
    io.KeyMap[ImGuiKey_PageDown] = NSG_KEY_PAGEDOWN;
    io.KeyMap[ImGuiKey_Home] = NSG_KEY_HOME;
    io.KeyMap[ImGuiKey_End] = NSG_KEY_END;
    io.KeyMap[ImGuiKey_Delete] = NSG_KEY_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = NSG_KEY_BACKSPACE;
    io.KeyMap[ImGuiKey_Enter] = NSG_KEY_ENTER;
    io.KeyMap[ImGuiKey_Escape] = NSG_KEY_ESC;
    io.KeyMap[ImGuiKey_A] = NSG_KEY_A;
    io.KeyMap[ImGuiKey_C] = NSG_KEY_C;
    io.KeyMap[ImGuiKey_V] = NSG_KEY_V;
    io.KeyMap[ImGuiKey_X] = NSG_KEY_X;
    io.KeyMap[ImGuiKey_Y] = NSG_KEY_Y;
    io.KeyMap[ImGuiKey_Z] = NSG_KEY_Z;
}
}
