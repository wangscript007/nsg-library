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
using namespace NSG;
static void Test0()
{
	auto resource = Resource::GetOrCreate<ResourceFile>("data/scene.xml");
	AppData data(resource);
	auto scene = data.scenes_.at(0);
	auto parent = scene->GetChild<SceneNode>("CubeParent", false);
	CHECK_CONDITION(Distance(parent->GetGlobalScale(), Vector3(1, 3, 1)) < 0.001f, __FILE__, __LINE__);
	auto child = parent->GetChild<SceneNode>("CubeChild", false);
	CHECK_CONDITION(Distance(child->GetGlobalScale(), Vector3(1, 3, 1)) < 0.001f, __FILE__, __LINE__);
	CHECK_CONDITION(Distance(child->GetGlobalPosition(), Vector3(1.963f, 2.111f, -4.177f)) < 0.01f, __FILE__, __LINE__);
}

int NSG_MAIN(int argc, char* argv[])
{
	Test0();
	return 0;
}