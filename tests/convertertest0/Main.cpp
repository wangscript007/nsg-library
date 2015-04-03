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
    auto window = Window::Create();
    auto resource = Resource::GetOrCreate<ResourceFile>("data/momo1.xml");
    resource->Load();
	auto scenes = resource->Load();
	CHECK_CONDITION(scenes.size() == 1, __FILE__, __LINE__);
	auto scene = scenes.at(0);
	auto object = scene->GetChild<SceneNode>("Bone.main", true);
	CHECK_CONDITION(object, __FILE__, __LINE__);
	auto animations = scene->GetAnimationsFor(object);
	CHECK_CONDITION(animations.size() == 1, __FILE__, __LINE__);
	{
		auto animations = scene->GetAnimations();
		CHECK_CONDITION(animations.size() == 1, __FILE__, __LINE__);
	}
}