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
//#define DAE_FILE
int NSG_MAIN(int argc, char* argv[])
{
    using namespace NSG;
    App app;
    auto window = app.GetOrCreateWindow("window", 0, 100, 10, 10);
    #ifdef DAE_FILE
    auto resource = app.GetOrCreateResourceFile("data/scene.xml");
    #else
    auto resource = app.GetOrCreateResourceFile("data/bscene.xml");
    #endif
    auto scenes = app.Load(resource);
    auto scene = scenes.at(0);
    scene->SetAmbientColor(Color(0.0f));

    auto object = scene->GetChild<SceneNode>("Bone", true);
    auto plane = scene->GetChild<SceneNode>("Plane", false);
    plane->GetMaterial()->SetShininess(10);
    //plane->GetMaterial()->SetDisplacementMap(nullptr);

    auto camera = scene->GetChild<Camera>("Camera", false);
    camera->SetWindow(window);
    auto control = std::make_shared<CameraControl>(camera);
    control->SetWindow(window);
    auto lamp = scene->GetChild<Light>("Lamp", false);
    auto ball = scene->GetChild<SceneNode>("Earth", false);
    auto ramp1 = scene->GetChild<SceneNode>("Ramp1", false);
    auto ramp2 = scene->GetChild<SceneNode>("Ramp2", false);

    auto ballRigidBody = ball->GetOrCreateRigidBody();

    #ifdef DAE_FILE
    auto planeRigidBody = plane->GetOrCreateRigidBody();
    planeRigidBody->SetShape(SH_TRIMESH);

    auto ramp1RigidBody = ramp1->GetOrCreateRigidBody();
    ramp1RigidBody->SetShape(SH_CONVEX_TRIMESH);

    auto ramp2RigidBody = ramp2->GetOrCreateRigidBody();
    ramp2RigidBody->SetShape(SH_CONVEX_TRIMESH);

    ballRigidBody->SetMass(1);
    ballRigidBody->SetShape(SH_SPHERE);
    #endif

    {
        auto animations = scene->GetAnimationsFor(object);
        auto animation = animations[0];
        animation->Play(true);
    }

    auto updateSlot = window->signalUpdate_->Connect([&](float deltaTime)
    {
        scene->Update(deltaTime);
    });

    auto renderSlot = window->signalRender_->Connect([&]()
    {
        scene->Render(camera.get());
    });

    control->slotMouseDown_ = window->signalMouseDown_->Connect([&](int button, float x, float y)
    {
        if (button == NSG_BUTTON_LEFT)
        {
            control->OnMouseDown(button, x, y);
        }
        else
        {
            Ray ray = camera->GetScreenRay(x, y);
            RayNodeResult closest;
            if (scene->GetClosestRayNodeIntersection(ray, closest))
            {
                Vertex3 pos = ray.GetPoint(closest.distance_);
                pos.y = 7;
                ball->SetGlobalPosition(pos);
                ballRigidBody->SetLinearVelocity(Vector3(0));
                ballRigidBody->SyncWithNode();
            }
        }

    });

    return app.Run();
}
