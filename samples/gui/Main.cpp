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

#include "NSG.h"
int NSG_MAIN(int argc, char* argv[])
{
    using namespace NSG;

    App app;

    auto window = app.GetOrCreateWindow("window", 100, 100, 10, 10);

    auto xml = app.GetOrCreateResourceFile("data/GuiAnonymousPro32.xml");
    auto atlas = std::make_shared<FontAtlas>(xml);

    auto scene = std::make_shared<Scene>("scene");
    auto camera = scene->CreateChild<Camera>();
    camera->SetPosition(Vector3(0, 0, 2));
//    camera->EnableOrtho();
    auto control = std::make_shared<CameraControl>(camera);

    auto button1 = scene->CreateChild<Button>();
    button1->SetText(atlas, "Hello");

    auto slotButtonDown = button1->signalButtonMouseDown_->Connect([&](int button)
    {
        button1->SetText(atlas, "(DOWN)");
    });

    auto slotButtonUp = button1->signalButtonMouseUp_->Connect([&](int button)
    {
        button1->SetText(atlas, "Hello");
    });

    auto renderSlot = window->signalRender_->Connect([&]()
    {
        Camera::Render(scene);
    });

    return app.Run();
}

