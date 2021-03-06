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
#pragma once
#include "GameObject.h"
#include "NSG.h"
using namespace NSG;
class Explo;
class Player : public GameObject {
public:
    Player(PScene scene, PWindow window);
    ~Player();
    PNode GetCameraNode() const { return node_; }
    void Destroyed() override;

private:
    PNode node_;
    PSceneNode child_;
    PlayerControl control_;
    SignalFloatFloat::PSlot moveSlot_;
    SignalFloatFloat::PSlot moveLeftStickSlot_;
    SignalFloatFloat::PSlot moveRightStickSlot_;
    SignalBool::PSlot buttonASlot_;
    SignalUpdate::PSlot updateSlot_;
    PRigidBody body_;
    SignalCollision::PSlot slotCollision_;
    shared_ptr<Explo> explo_;
    int collisionGroup_;
    int collisionMask_;
    bool shot_;
    bool buttonADown_;
    float lastShotTime_;
    float totalSpawningTime_;
    Quaternion lastShotOrientation_;
    bool spawning_;
};
