/*
-------------------------------------------------------------------------------
This file is part of nsg-library.
http://github.com/woodjazz/nsg-library

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
float speedFactor = 0;
float jumpFactor = 0;
int NSG_MAIN(int argc, char* argv[])
{
    using namespace NSG;
    auto window = Window::Create();
    auto resource = Resource::GetOrCreateClass<ResourceFile>("data/scene.xml");
    auto data = AppData::Create();
    data->Load(resource);
    auto scene = data->scenes_[0];
    window->SetScene(scene.get());
    auto camera = scene->GetChild<Camera>("Camera", false);
    //auto control = std::make_shared<CameraControl>(camera);
    auto followCamera = std::make_shared<FollowCamera>(camera);
    auto player = scene->GetChild<SceneNode>("RigMomo", true);
    auto rigidBody = player->GetRigidBody();
    followCamera->Track(player);
    followCamera->SetOffset(Vector3(0, 20, -40));
    auto playerControl = std::make_shared<PlayerControl>();

    float speed = 0;
    float turn = 0;
    float turnFactor = 2;
    auto slotMoved = playerControl->SigMoved()->Connect([&](float x, float z)
    {
        speed = z;
        turn = -x;
    });

    bool buttonA = false;
    auto slotButtonA = playerControl->SigButtonA()->Connect([&](bool pressed)
    {
        buttonA = pressed;
    });

    struct State : FSM::State
    {
        bool loop_;
        float time_;
        PScene scene_;
        PSceneNode player_;
        PRigidBody rigidBody_;
        PAnimation animation_;
        State(const char* animName)
            : loop_(true), time_(0)
        {
            scene_ = AppData::GetPtr()->scenes_[0];
            player_ = scene_->GetChild<SceneNode>("RigMomo", true);
            rigidBody_ = player_->GetRigidBody();
            animation_ = scene_->GetAnimationFor(animName, player_);
        }
        void Begin() override
        {
            scene_->PlayAnimation(animation_, loop_);
        }
        void Stay() override
        {
            time_ += Engine::GetPtr()->GetDeltaTime();
        }
        void End() override
        {
            time_ = 0;
            scene_->StopAnimation(animation_);
        }
    };

    struct Idle : State
    {
        Idle() : State("Momo_IdleNasty")
        {
        }
        void Begin() override
        {
            State::Begin();
            speedFactor = 0;
        }
    } idle;

    struct Walk : State
    {
        Walk() : State("Momo_Walk")
        {
        }
        void Begin() override
        {
            State::Begin();
            speedFactor = -5;
        }
    } walk;

    struct WalkBack : State
    {
        WalkBack() : State("Momo_WalkBack")
        {
        }
        void Begin() override
        {
            State::Begin();
            speedFactor = 3;
        }
    } walkBack;

    struct Run : State
    {
        Run() : State("Momo_Run")
        {
        }
        void Begin() override
        {
            State::Begin();
            speedFactor = -15;
        }
    } run;

    struct TurnL : State
    {
        TurnL() : State("Momo_Turn.R")
        {
        }
        void Begin() override
        {
            State::Begin();
            speedFactor = 0;
        }
    } turnL;

    struct TurnR : State
    {
        TurnR() : State("Momo_Turn.L")
        {
        }
        void Begin() override
        {
            State::Begin();
            speedFactor = 0;
        }
    } turnR;

    struct Jump : State
    {
        Jump() : State("Momo_Jump")
        {
            loop_ = false;
        }
        void Begin() override
        {
            State::Begin();
            jumpFactor = 30;
        }
        void End() override
        {
            State::End();
            jumpFactor = 0;
        }
    } jump;

    struct Glide : State
    {
        Vector3 gravity_;
        Glide() : State("Momo_Glide")
        {
            loop_ = false;
        }
        void Begin() override
        {
            gravity_ = scene_->GetPhysicsWorld()->GetGravity();
            rigidBody_->SetGravity(VECTOR3_ZERO);
            State::Begin();
        }
        void End() override
        {
            rigidBody_->SetGravity(gravity_);
            State::End();
        }
    } glide;

    struct Fall : State
    {
        Fall() : State("Momo_FallUp")
        {
        }
        void Begin() override
        {
            State::Begin();
        }
        void End() override
        {
            State::End();
        }
    } fall;

    bool isFalling = false;
    FSM::Machine fsm(idle);
    idle.AddTransition(walk).When([&]() { return speed > 0; });
    idle.AddTransition(walkBack).When([&]() { return speed < 0; });
    idle.AddTransition(turnL).When([&]() { return turn < 0; });
    idle.AddTransition(turnR).When([&]() { return turn > 0; });
    idle.AddTransition(jump).When([&]() { return buttonA; });
    idle.AddTransition(fall).When([&]() { return isFalling; });
    jump.AddTransition(fall).When([&]() { return isFalling; });
    jump.AddTransition(glide).When([&]() { return jump.time_ > 0.7f; });
    glide.AddTransition(fall).When([&]() { return glide.time_ > 2.5f; });
    fall.AddTransition(idle).When([&]() { return !isFalling; });
    turnL.AddTransition(idle).When([&]() { return turn == 0; });
    turnL.AddTransition(turnR).When([&]() { return turn > 0; });
    turnL.AddTransition(walk).When([&]() { return speed > 0; });
    turnR.AddTransition(idle).When([&]() { return turn == 0; });
    turnR.AddTransition(turnL).When([&]() { return turn < 0; });
    turnR.AddTransition(walk).When([&]() { return speed > 0; });
    walk.AddTransition(run).When([&]() { return speed > 0 && walk.time_ > 1.3f; });
    walk.AddTransition(idle).When([&]() { return speed == 0 && walk.time_ > 1; });
    walk.AddTransition(idle).When([&]() { return speed < 0; });
    walkBack.AddTransition(idle).When([&]() { return speed >= 0; });
    run.AddTransition(walk).When([&]() { return speed == 0 && run.time_ > 1; });

    auto engine = Engine::Create();
    auto slotUpdate = engine->SigUpdate()->Connect([&](float deltaTime)
    {
        fsm.Update();
        auto forwardDir = player->GetGlobalOrientation() * VECTOR3_UP;
        auto upDir = VECTOR3_UP;
        rigidBody->SetLinearVelocity(speedFactor * forwardDir +  jumpFactor * upDir);
        rigidBody->SetAngularVelocity(Vector3(0, turnFactor * turn, 0));
        auto v = rigidBody->GetLinearVelocity();
        isFalling = v.y < 0;
        //LOGI("%s", ToString(v).c_str());
    });

    return engine->Run();
}
