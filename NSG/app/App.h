/*
-------------------------------------------------------------------------------
This file is part of nsg-library.
http://nsg-library.googlecode.com/

Copyright (c) 2014-2015 N�stor Silveira Gorski

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
#pragma once

#include "Types.h"
#include "Tick.h"
#include "Context.h"
#include "Singleton.h"
#include "IMGUI.h"
#include "AppListeners.h"
#include <vector>

class AAssetManager;
class ANativeActivity;
namespace pp
{
    class Var;
}

namespace NSG
{
    struct InternalApp;
    class App : public IMGUI::IWindow, public Singleton<App>
    {
    public:
        App();
        App(PAppConfiguration configuration);
        virtual ~App();
        virtual void Start() {}
        virtual void Update() {}
        virtual void RenderFrame() {};
        virtual void ViewChanged(int32_t width, int32_t height) {};
        virtual void OnMouseMove(float x, float y) {}
        virtual void OnMouseDown(float x, float y) {}
        virtual void OnMouseWheel(float x, float y) {}
        virtual void OnMouseUp(float x, float y) {}
        virtual void OnKey(int key, int action, int modifier) {}
        virtual void OnChar(unsigned int character) {}
        virtual bool ShallExit() const
        {
            return false;
        }
        void DoTick(float delta);
        void SetViewSize(int32_t width, int32_t height);
        std::pair<int32_t, int32_t> GetViewSize() const;
        float GetDeltaTime() const
        {
            return deltaTime_;
        }
        virtual void HandleMessage(const pp::Var& var_message);
        void SetAssetManager(AAssetManager* pAAssetManager)
        {
            pAAssetManager_ = pAAssetManager;
        }
        AAssetManager* GetAssetManager()
        {
            return pAAssetManager_;
        }

        static void Add(IViewChangedListener* listener);
        static void Remove(IViewChangedListener* listener);

    private:
        void AddListener(IViewChangedListener* listener);
        void RemoveListener(IViewChangedListener* listener);

        AAssetManager* pAAssetManager_;
        int32_t width_;
        int32_t height_;
        float deltaTime_; // The time in seconds it took to complete the last frame
        PContext context_;
        PAppConfiguration configuration_;
        friend struct InternalApp;
        std::vector<IViewChangedListener*> viewChangedListeners_;
    };

    struct InternalApp : public Tick
    {
        PApp pApp_;
        float screenX_;
        float screenY_;

        InternalApp(App* pApp);
        ~InternalApp();
        virtual void InitializeTicks() override;
        virtual void BeginTicks() override;
        virtual void DoTick(float delta) override;
        virtual void EndTicks() override;
        void SetViewSize(int32_t width, int32_t height);
        void ViewChanged(int32_t width, int32_t height);
        void OnMouseMove(float x, float y);
        void OnMouseWheel(float x, float y);
        void OnMouseDown(float x, float y);
        void OnMouseUp(float x, float y);
        void OnKey(int key, int action, int modifier);
        void OnChar(unsigned int character);
        void RenderFrame();
        bool ShallExit() const;
        void InvalidateGPUContext();
        void ReleaseResourcesFromMemory();
        void HandleMessage(const pp::Var& var_message);
        void SetAssetManager(AAssetManager* pAAssetManager);
        void SetActivity(ANativeActivity* pActivity);
    };
}

