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
#include "ShadowMapDebug.h"
#include "Check.h"
#include "Color.h"
#include "DebugRenderer.h"
#include "Keys.h"
#include "Light.h"
#include "Renderer.h"
#include "ShadowCamera.h"
#include "SharedFromPointer.h"
#include "Window.h"

namespace NSG {
ShadowMapDebug::ShadowMapDebug(PWindow window, PLight light)
    : light_(light), debugRendererEnabled_(false) {
    CHECK_ASSERT(light_);

    SetWindow(window);

    slotDebugRenderer_ = Renderer::GetPtr()->SigDebugRenderer()->Connect([&](
        DebugRenderer* renderer) {
        if (debugRendererEnabled_) {
            auto splits = light_->GetShadowSplits();
            light_->GetShadowCamera(0)->Debug(renderer, Color::Red);
            if (splits > 1) {
                light_->GetShadowCamera(1)->Debug(renderer, Color::Green);
                if (splits > 2) {
                    light_->GetShadowCamera(2)->Debug(renderer, Color::Blue);
                    if (splits > 3) {
                        light_->GetShadowCamera(3)->Debug(renderer,
                                                          Color::Yellow);
                    }
                }
            }
        }
    });
}

ShadowMapDebug::~ShadowMapDebug() {}

void ShadowMapDebug::SetWindow(PWindow window) {
    if (window_.lock() != window) {
        window_ = window;

        if (window) {
            slotKey_ = window->SigKey()->Connect(
                [&](int key, int action, int modifier) {
                    OnKey(key, action, modifier);
                });
        } else {
            slotKey_ = nullptr;
        }
    }
}

void ShadowMapDebug::OnKey(int key, int action, int modifier) {
#if 0
        if(modifier)
            return;

		auto window = window_.lock().get();
		if (!window)
			return;
        
        if (NSG_KEY_0 == key && action)
            window->ShowMap(nullptr);
        else if (NSG_KEY_1 == key && action)
        {
            window->ShowMap(nullptr);
			window->ShowMap(light_->GetShadowMap(0));
        }
        else if (NSG_KEY_2 == key && action)
        {
            window->ShowMap(nullptr);
			window->ShowMap(light_->GetShadowMap(1));
        }
        else if (NSG_KEY_3 == key && action)
        {
            window->ShowMap(nullptr);
			window->ShowMap(light_->GetShadowMap(2));
        }
        else if (NSG_KEY_4 == key && action)
        {
            window->ShowMap(nullptr);
			window->ShowMap(light_->GetShadowMap(3));
        }
		else if (NSG_KEY_M == key && action)
			debugRendererEnabled_ = !debugRendererEnabled_;
#endif
}
}
