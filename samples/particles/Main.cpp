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
#include <deque>
int NSG_MAIN(int argc, char* argv[])
{
    using namespace NSG;
    App app;

    auto window = app.GetOrCreateWindow("window", 100, 100, 1024, 768);

    auto scene = std::make_shared<Scene>("scene");

    auto camera = scene->GetOrCreateChild<Camera>("camera");
	camera->SetWindow(window);
	camera->SetPosition(Vertex3(0, 8, 15));

    auto control = std::make_shared<CameraControl>(camera);
    control->SetWindow(window);

	auto ps = scene->GetOrCreateChild<ParticleSystem>("ps");
    auto meshEmitter(app.CreateBoxMesh());
    ps->SetMesh(meshEmitter);
    ps->SetPosition(Vertex3(0, 10, 0));
	ps->SetMaterial(app.GetOrCreateMaterial());
	ps->GetMaterial()->GetTechnique()->GetPass(0)->SetDrawMode(DrawMode::WIREFRAME);

	auto planeMesh = app.CreateBoxMesh(20, 0.1f, 20);
    auto floorObj = scene->GetOrCreateChild<SceneNode>("floor");
	floorObj->SetMesh(planeMesh);
	floorObj->SetMaterial(app.GetOrCreateMaterial());
	floorObj->GetMaterial()->SetColor(Color(1, 0, 0, 1));
	floorObj->GetOrCreateRigidBody()->SetShape(SH_TRIMESH);

    //auto light = scene->GetOrCreateChild<Light>("light");
    //light->SetPosition(Vertex3(0, 0.0, 5.0));

    auto updateSlot = window->signalUpdate_->Connect([&](float deltaTime)
    {
        scene->Update(deltaTime);
    });

	auto renderSlot = window->signalRender_->Connect([&]()
	{
		scene->Render(camera.get());
	});

    return app.Run();
}

