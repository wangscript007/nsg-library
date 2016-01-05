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
#include "Window.h"
#include "Log.h"
#include "Check.h"
#include "Engine.h"
#include "AppConfiguration.h"
#include "SignalSlots.h"
#include "TextMesh.h"
#include "Scene.h"
#include "Graphics.h"
#include "Music.h"
#include "FrameBuffer.h"
#include "Program.h"
#include "Material.h"
#include "ShowTexture.h"
#include "SDLWindow.h"
#include "Keys.h"
#include "Texture.h"
#include "UTF8String.h"
#include "Renderer.h"
#include "GUI.h"
#include "Pass.h"
#include "QuadMesh.h"
#include "imgui.h"
#include <algorithm>
#include <thread>

#ifndef __GNUC__
#include <codecvt>
#endif

namespace NSG
{
    std::vector<PWeakWindow> Window::windows_;
    Window* Window::mainWindow_ = nullptr;
    int Window::nWindows2Remove_ = 0;

    Window::Window(const std::string& name)
        : name_(name),
          isClosed_(false),
          minimized_(false),
          isMainWindow_(true),
          width_(0),
          height_(0),
          filtersEnabled_(true),
          scene_(nullptr),
          signalViewChanged_(new Signal<int, int>()),
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
          signalDropFile_(new Signal<const std::string & >()),
          signalJoystickDown_(new SignalJoystickButton),
          signalJoystickUp_(new SignalJoystickButton),
          signalJoystickAxisMotion_(new Signal<int, JoystickAxis, float>),
          signalDrawIMGUI_(new Signal<>),
          signalTouchFinger_(new SignalTouchFinger),
          pixelFormat_(PixelFormat::UNKNOWN),
          render_(nullptr)
    {
        CHECK_CONDITION(Window::AllowWindowCreation());
    }

    Window::~Window()
    {
        gui_ = nullptr;
        LOGI("Window %s terminated.", name_.c_str());
    }

    PWindow Window::Create(const std::string& name, WindowFlags flags)
    {
        if (Window::AllowWindowCreation())
        {
            auto window = std::make_shared<SDLWindow>(name, flags);
            Window::AddWindow(window);
            return window;
        }
        return nullptr;
    }

    PWindow Window::Create(const std::string& name, int x, int y, int width, int height, WindowFlags flags)
    {
        if (Window::AllowWindowCreation())
        {
            auto window = std::make_shared<SDLWindow>(name, x, y, width, height, flags);
            Window::AddWindow(window);
            return window;
        }
        return nullptr;
    }

    void Window::CreateFrameBuffer()
    {
        CHECK_ASSERT(!frameBuffer_);
        FrameBuffer::Flags frameBufferFlags((unsigned int)(FrameBuffer::COLOR | FrameBuffer::COLOR_USE_TEXTURE | FrameBuffer::DEPTH | FrameBuffer::Flag::DEPTH_USE_TEXTURE));
        //frameBufferFlags |= FrameBuffer::STENCIL;
        frameBuffer_ = std::make_shared<FrameBuffer>(GetUniqueName("WindowFrameBuffer"), frameBufferFlags);
        frameBuffer_->SetWindow(this);

        CHECK_ASSERT(!filterFrameBuffer_);
        filterFrameBuffer_ = std::make_shared<FrameBuffer>(GetUniqueName("WindowFilterFrameBuffer"), frameBufferFlags);
        filterFrameBuffer_->SetDepthTexture(frameBuffer_->GetDepthTexture());
        filterFrameBuffer_->SetWindow(this);

        CHECK_ASSERT(!showMap_);
        showMap_ = std::make_shared<ShowTexture>();
        showMap_->SetColortexture(frameBuffer_->GetColorTexture());
    }

    void Window::ShowMap(PTexture texture)
    {
        showTexture_ = texture;
        if (texture)
            showMap_->SetColortexture(texture);
        else
            showMap_->SetColortexture(frameBuffer_->GetColorTexture());
    }

    bool Window::UseFrameRender()
    {
        if (!frameBuffer_->IsReady() || !filterFrameBuffer_->IsReady())
            return false;
        graphics_->SetFrameBuffer(frameBuffer_.get());
        return true;
    }


    bool Window::BeginFrameRender()
    {
        if (HasFilters())
            return UseFrameRender();
        else
            graphics_->SetFrameBuffer(nullptr);
        return true;
    }

    void Window::RenderFilters()
    {
        if (!showTexture_ && HasFilters())
        {
            auto it = filters_.begin();
            while (it != filters_.end())
            {
                auto filter = (*it).lock();
                if (filter)
                {
                    graphics_->SetFrameBuffer(filterFrameBuffer_.get());
                    filter->SetTexture(MaterialTexture::DIFFUSE_MAP, frameBuffer_->GetColorTexture());
                    filter->FlipYTextureCoords(true);
                    Renderer::GetPtr()->Render(showMap_->GetPass().get(), QuadMesh::GetNDC().get(), filter.get());
                    graphics_->SetFrameBuffer(frameBuffer_.get());
                    showMap_->SetColortexture(filterFrameBuffer_->GetColorTexture());
                    showMap_->Show();
                    ++it;
                }
                else
                {
                    it = filters_.erase(it);
                }
            }
        }
    }

    void Window::ShowMap()
    {
        auto current = graphics_->SetFrameBuffer(nullptr); //use system framebuffer to show the texture
        if (showTexture_)
            showMap_->Show();
        else if (current == frameBuffer_.get())
        {
            showMap_->SetColortexture(frameBuffer_->GetColorTexture());
            showMap_->Show();
        }
    }

    void Window::Close()
    {
        LOGI("Closing %s window.", name_.c_str());

        if (Window::mainWindow_ == this)
        {
            //graphics_->ResetCachedState();
            // destroy other windows
            auto windows = Window::GetWindows();
            for (auto& obj : windows)
            {
                PWindow window = obj.lock();
                if (window && window.get() != this)
                    window->Destroy();
            }
        }
        Destroy();
    }

    void Window::OnReady()
    {
        graphics_ = Graphics::Create();
        renderer_ = Renderer::Create();
        graphics_->SetWindow(this);
        CreateFrameBuffer(); // used when filters are enabled
    }

    void Window::SetSize(int width, int height)
    {
        if (width_ != width || height_ != height)
        {
            LOGI("WindowChanged: %d,%d", width, height);
            width_ = width;
            height_ = height;

            if (graphics_ && graphics_->GetWindow() == this)
                graphics_->SetViewport(GetViewport(), true);

            signalViewChanged_->Run(width, height);
        }
    }

    void Window::ViewChanged(int width, int height)
    {
        SetSize(width, height);
    }

    void Window::OnMouseMove(float x, float y)
    {
        signalFloatFloat_->Run(x, y);
    }

    void Window::OnMouseMove(int x, int y)
    {
        signalMouseMoved_->Run(x, y);
    }

    void Window::OnMouseDown(int button, float x, float y)
    {
        signalMouseDown_->Run(button, x, y);
    }

    void Window::OnMouseDown(int button, int x, int y)
    {
        signalMouseDownInt_->Run(button, x, y);
    }

    void Window::OnMouseUp(int button, float x, float y)
    {
        signalMouseUp_->Run(button, x, y);
    }

    void Window::OnMouseUp(int button, int x, int y)
    {
        signalMouseUpInt_->Run(button, x, y);
    }

    void Window::OnMultiGesture(int timestamp, float x, float y, float dTheta, float dDist, int numFingers)
    {
        signalMultiGesture_->Run(timestamp, x, y, dTheta, dDist, numFingers);
    }

    void Window::OnMouseWheel(float x, float y)
    {
        signalMouseWheel_->Run(x, y);
    }

    void Window::OnKey(int key, int action, int modifier)
    {
        signalKey_->Run(key, action, modifier);
    }

    void Window::OnChar(unsigned int character)
    {
        signalUnsigned_->Run(character);
    }

    void Window::OnText(const std::string& text)
    {
        signalText_->Run(text);
    }

    void Window::OnJoystickDown(int joystickID, JoystickButton button)
    {
        SigJoystickDown()->Run(joystickID, button);
    }

    void Window::OnJoystickUp(int joystickID, JoystickButton button)
    {
        SigJoystickUp()->Run(joystickID, button);
    }

    void Window::OnJoystickAxisMotion(int joystickID, JoystickAxis axis, float position)
    {
        SigJoystickAxisMotion()->Run(joystickID, axis, position);
    }

    void Window::EnterBackground()
    {
        minimized_ = true;
        if (Window::mainWindow_ == this)
        {
            if (Music::GetPtr() && Engine::GetAppConfiguration().pauseMusicOnBackground_)
                Music::GetPtr()->Pause();
        }
    }

    void Window::EnterForeground()
    {
        if (Window::mainWindow_ == this)
        {
            if (Music::GetPtr() && Engine::GetAppConfiguration().pauseMusicOnBackground_)
                Music::GetPtr()->Resume();
        }

        minimized_ = false;
    }

    void Window::DropFile(const std::string& filePath)
    {
        signalDropFile_->Run(filePath);
    }

    Recti Window::GetViewport() const
    {
        return Recti(0, 0, width_, height_);
    }

    void Window::AddFilter(PMaterial filter)
    {
        filters_.push_back(filter);
    }

    void Window::RemoveFilter(PMaterial filter)
    {
        filters_.erase(std::remove_if(filters_.begin(), filters_.end(), [&](PWeakMaterial material)
        {
            return material.lock() == filter;
        }), filters_.end());

    }

    void Window::EnableFilters(bool enable)
    {
        if (filtersEnabled_ != enable)
        {
            filtersEnabled_ = enable;
            if (!enable)
            {
                frameBuffer_->Invalidate();
                filterFrameBuffer_->Invalidate();
            }
        }
    }

    bool Window::AllowWindowCreation()
    {
        #if defined(IS_TARGET_MOBILE) || defined(IS_TARGET_WEB)
        {
            if (windows_.size())
            {
                LOGW("Only one window is allowed for this platform.");
                return false;
            }
        }
        #endif
        return true;
    }

    void Window::SetMainWindow(Window* window)
    {
        if (Window::mainWindow_ != window)
            Window::mainWindow_ = window;
    }

    void Window::AddWindow(PWindow window)
    {
        CHECK_ASSERT(AllowWindowCreation());
        windows_.push_back(window);
        window->gui_ = std::make_shared<GUI>();
    }

    void Window::UpdateScenes(float delta)
    {
        for (auto& obj : windows_)
        {
            auto window(obj.lock());
            if (window)
            {
                if (window->scene_)
                    window->scene_->UpdateAll(delta);
            }
        }
    }

    void Window::SetRender(IRender* render)
    {
        CHECK_CONDITION(render_ == nullptr);
        render_ = render;
    }

    void Window::RemoveRender(IRender* render)
    {
        CHECK_CONDITION(render_ == render);
        render_ = nullptr;
    }

    void Window::RenderFrame()
    {
        if (BeginFrameRender())
        {
            if (render_)
                render_->Render();
            else
            {
                Renderer::GetPtr()->Render(this, scene_);
                if (SigDrawIMGUI()->HasSlots())
                    gui_->Render(this, [this]() { SigDrawIMGUI()->Run(); });
            }
            SwapWindowBuffers();
        }
    }

    bool Window::AreAllWindowsMinimized()
    {
        for (auto& obj : windows_)
        {
            auto window(obj.lock());
            if (!window || window->IsClosed())
                continue;
            if (!window->IsMinimized())
                return false;
        }
        return true;
    }

    bool Window::RenderWindows()
    {
        auto windows = windows_;
        for (auto& obj : windows)
        {
            auto window(obj.lock());
            if (!window || window->IsClosed())
                continue;
            if (!window->IsMinimized())
                window->RenderFrame();
        }

        while (nWindows2Remove_)
        {
            windows_.erase(std::remove_if(windows_.begin(), windows_.end(), [&](PWeakWindow window)
            {
                if (!window.lock() || window.lock()->IsClosed())
                {
                    --nWindows2Remove_;
                    return true;
                }
                return false;
            }), windows_.end());
        }

        if (!Window::mainWindow_)
            return false;
        else if (Window::mainWindow_->IsMinimized())
        {
            std::this_thread::sleep_for(Milliseconds(100));
        }
        return true;
    }

    void Window::HandleEvents()
    {
        #if SDL
        SDLWindow::HandleEvents();
        #else
        CHEKC_ASSERT(!"So far only support for SDL...!!!");
        #endif
    }

    void Window::SetScene(Scene* scene)
    {
        scene_ = scene;
    }

    void Window::NotifyOneWindow2Remove()
    {
        ++nWindows2Remove_;
    }
}