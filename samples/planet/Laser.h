/*
-------------------------------------------------------------------------------
This file is part of nsg-library.
http://github.com/woodjazz/nsg-library

Copyright (c) 2014-2015 Néstor Silveira Gorski

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
#include "NSG.h"
#include "GameObject.h"
using namespace NSG;
class Explo;
class Laser : public GameObject
{
public:
    Laser(PScene scene);
    ~Laser();
    void SetOrientation(const Quaternion& q0, const Quaternion& q1);
    void SetPosition(const Vector3& position);
    void Destroyed() override;
private:
    PNode node_;
    PSceneNode child_;
    PRigidBody body_;
    int collisionGroup_;
    int collisionMask_;
    SignalUpdate::PSlot updateSlot_;
    SignalCollision::PSlot slotCollision_;
    float totalTime_;
    bool destroyed_;
};