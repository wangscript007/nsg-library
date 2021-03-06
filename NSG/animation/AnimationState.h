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
#include "Animation.h"
#include "Types.h"

namespace NSG {
struct AnimationStateTrack : AnimationTrack {
    size_t currentKeyFrame_;
    AnimationStateTrack(const AnimationTrack& base);
};

class AnimationState {
public:
    AnimationState(PAnimation animation);
    ~AnimationState();
    void Update();
    void SetTime(float time);
    float GetTime() const { return timePosition_; }
    float GetLength() const;
    void AddTime(float delta);
    void SetLooped(bool looped);
    bool IsLooped() const { return looped_; }
    void SetWeight(float weight);
    float GetWeight() const { return weight_; }
    bool HasEnded() const;
    PAnimation GetAnimation() const { return animation_; }

private:
    void Blend(AnimationStateTrack& track, float weight);
    PAnimation animation_;
    float timePosition_;
    std::vector<AnimationStateTrack> tracks_;
    bool looped_;
    float weight_; // Blending weight.
};
}