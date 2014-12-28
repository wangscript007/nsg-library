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

    auto resourceManager = std::make_shared<ResourceFileManager>();

    auto window = app.GetOrCreateWindow("window", 100, 100, 50, 30);
    auto scene = std::make_shared<Scene>("scene000");
    auto resource(resourceManager->GetOrCreate("data/duck.xml"));
    scene->SceneNode::Load(resource);
    auto objNode = scene->GetOrCreateChild<SceneNode>("LOD3sp");
    auto objPos = objNode->GetGlobalPosition();
    auto objBB = objNode->GetWorldBoundingBox();
    objBB.max_ *= 1.75f;
    objBB.min_ *= 1.75f;
    auto camera = scene->GetOrCreateChild<Camera>("camera1");
    camera->SetGlobalPosition(Vector3(0, objBB.max_.y, objBB.max_.z));
    camera->SetGlobalLookAt(objPos);

    camera->SetAspectRatio(window->GetWidth(), window->GetHeight());
    auto resizeSlot = window->signalViewChanged_->Connect([&](int width, int height)
    {
        camera->SetAspectRatio(width, height);
    });

    auto animation = scene->GetOrCreateAnimation("anim0");
    AnimationTrack track;
    track.node_ = camera;
    track.channelMask_ = (int)AnimationChannel::POSITION | (int)AnimationChannel::ROTATION;

    {
        AnimationKeyFrame key(0, camera.get());
        track.keyFrames_.push_back(key);
    }

    {
        auto node = std::make_shared<Node>("node0");
        node->SetParent(camera->GetParent());
        node->SetGlobalPosition(Vector3(objBB.max_.x, objBB.max_.y, 0));
        node->SetGlobalLookAt(objPos);
        AnimationKeyFrame key(2, node.get());
        track.keyFrames_.push_back(key);
    }

    {
        auto node = std::make_shared<Node>("node1");
        node->SetParent(camera->GetParent());
        node->SetGlobalPosition(Vector3(0, objBB.max_.y, objBB.min_.z));
        node->SetGlobalLookAt(objPos);
        AnimationKeyFrame key(4, node.get());
        track.keyFrames_.push_back(key);
    }

    {
        auto node = std::make_shared<Node>("node2");
        node->SetParent(camera->GetParent());
        node->SetGlobalPosition(Vector3(objBB.min_.x, objBB.max_.y, 0));
        node->SetGlobalLookAt(objPos);
        AnimationKeyFrame key(6, node.get());
        track.keyFrames_.push_back(key);
    }

    animation->AddTrack(track);
    animation->SetLength(8);

    scene->PlayAnimation(animation, false);

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

