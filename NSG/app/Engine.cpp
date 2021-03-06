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
#include "Engine.h"
#include "RenderingContext.h"
#include "UniformsUpdate.h"
#include "Window.h"
#if defined(IS_TARGET_IOS)
#include "IOSWindow.h"
#endif
#include "Animation.h"
#include "FileSystem.h"
#include "LoaderXML.h"
#include "Material.h"
#include "Mesh.h"
#include "Program.h"
#include "Resource.h"
#include "Shape.h"
#include "Skeleton.h"

#if EMSCRIPTEN
#include "SDL.h"
#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/html5.h>
#endif
#include <thread>

namespace NSG {
AppConfiguration Engine::conf_;

Engine::Engine() : Tick(conf_.fps_), deltaTime_(0) { Tick::Initialize(); }

Engine::~Engine() {}

void Engine::InitializeTicks() {}

void Engine::BeginTicks() {
    auto mainWindow = Window::GetMainWindow();
    if (mainWindow)
        mainWindow->HandleEvents();
}

void Engine::DoTick(float delta) {
    deltaTime_ = delta;
    Window::UpdateScenes(delta);
    Engine::SigUpdate()->Run(delta);
}

void Engine::EndTicks() {
    if (!Window::AreAllWindowsMinimized())
        RenderFrame();
}

void Engine::RenderFrame() {
    Engine::SigBeginFrame()->Run();
    Window::RenderWindows();
}

int Engine::Run() {
    FileSystem::Initialize();
    Tick::Initialize();
#if EMSCRIPTEN
    {
        SDL_StartTextInput();
        auto saveSlot = FileSystem::SigSaved()->Connect([] {
            emscripten_run_script(
                "setTimeout(function() { window.close() }, 2000)");
        });
        bool saved = false;

        auto runframe = [](void* arg) {
            bool& saved = *(bool*)arg;
            Engine::GetPtr()->PerformTicks();
            if (!Window::GetMainWindow() && !saved)
                saved = FileSystem::Save();
        };
        emscripten_set_main_loop_arg(runframe, &saved, 0, 1);
    }
#elif defined(IS_TARGET_IOS)
    IOSWindow::RunApplication();
    FileSystem::Save();
#else
    {
        for (;;) {
            PerformTicks();
            if (!Window::GetMainWindow())
                break;
        }
        FileSystem::Save();
    }
#endif
    return 0;
}

SignalUpdate::PSignal Engine::SigUpdate() {
    static SignalUpdate::PSignal signalUpdate(new SignalUpdate);
    return signalUpdate;
}

SignalEmpty::PSignal Engine::SigBeginFrame() {
    static SignalEmpty::PSignal signalBeginFrame(new SignalEmpty);
    return signalBeginFrame;
}

void Engine::ReleaseMemory() {
    Resource::Clear();
    Mesh::Clear();
    Material::Clear();
    Shape::Clear();
    Skeleton::Clear();
    Animation::Clear();
    Program::Clear();
    LoaderXML::Clear();
    auto ctx = RenderingContext::GetPtr();
    if (ctx)
        ctx->ResetCachedState();
}
}
