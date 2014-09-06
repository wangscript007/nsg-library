/*
-------------------------------------------------------------------------------
This file is part of nsg-library.
http://nsg-library.googlecode.com/

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
#include "NSG.h"
using namespace NSG;

static void Test01()
{
	{
		Vertex3 v(1, 2, 3);
		BoundingBox bb(v);
		CHECK_ASSERT(bb.min_ == bb.max_, __FILE__, __LINE__);
		CHECK_ASSERT(bb.min_ == v, __FILE__, __LINE__);
	}

	{
		Vertex3 v0(1, 2, 3);
		Vertex3 v1(1, 2, 3);
		BoundingBox bb(v0, v1);
		CHECK_ASSERT(bb.min_ == bb.max_, __FILE__, __LINE__);
		CHECK_ASSERT(bb.min_ == v0, __FILE__, __LINE__);
	}

	{
		Vertex3 v0(-1, -2, -3);
		Vertex3 v1(1, 2, 3);
		Vertex3 v2(-1, -3, -3);
		BoundingBox bb(v0, v1);
		CHECK_ASSERT(bb.min_ == v0, __FILE__, __LINE__);
		CHECK_ASSERT(bb.max_ == v1, __FILE__, __LINE__);
		CHECK_ASSERT(bb.Size() == (v1 - v0), __FILE__, __LINE__);
		CHECK_ASSERT(bb.Center() == Vertex3(0), __FILE__, __LINE__);
		CHECK_ASSERT(bb.IsInside(Vertex3(0)), __FILE__, __LINE__);
		CHECK_ASSERT(bb.IsInside(v0), __FILE__, __LINE__);
		CHECK_ASSERT(bb.IsInside(v1), __FILE__, __LINE__);
		CHECK_ASSERT(!bb.IsInside(v2), __FILE__, __LINE__);
		CHECK_ASSERT(bb.IsInside(bb) == Intersection::INSIDE, __FILE__, __LINE__);
		CHECK_ASSERT(bb.IsInside(BoundingBox(v2, v1)) == Intersection::INTERSECTS, __FILE__, __LINE__);
		CHECK_ASSERT(bb.IsInside(BoundingBox(Vertex3(-5), Vertex3(5))) == Intersection::INTERSECTS, __FILE__, __LINE__);
		CHECK_ASSERT(bb.IsInside(BoundingBox(Vertex3(-6), Vertex3(-5))) == Intersection::OUTSIDE, __FILE__, __LINE__);
	}

	{
		Vertex3 v0(-1, -1, -1);
		Vertex3 v1(1, 1, 1);
		BoundingBox bb(v0, v1);
		CHECK_ASSERT(bb.Size() == Vertex3(2), __FILE__, __LINE__);
		bb.Merge(Vertex3(0));
		CHECK_ASSERT(bb.Size() == Vertex3(2), __FILE__, __LINE__);
		bb.Merge(Vertex3(1));
		CHECK_ASSERT(bb.Size() == Vertex3(2), __FILE__, __LINE__);
		bb.Merge(Vertex3(2));
		CHECK_ASSERT(bb.Size() == Vertex3(3), __FILE__, __LINE__);
	}

	{
		Vertex3 v0(-1, -0.5f, 0);
		Vertex3 v1(1, 0.5f, 0);
		BoundingBox bb(v0, v1);
		CHECK_ASSERT(bb.Size() == Vertex3(2, 1, 0), __FILE__, __LINE__);

		Node node;
		node.SetOrientation(glm::angleAxis(glm::pi<float>()/2, Vertex3(0, 1, 0)));
		bb.Transform(node);
		CHECK_ASSERT(glm::distance(bb.Size(), Vertex3(0, 1, 2)) < 2 * glm::epsilon<float>(), __FILE__, __LINE__);
		bb.Transform(node);
		CHECK_ASSERT(glm::distance(bb.Size(), Vertex3(2, 1, 0)) < 3 * glm::epsilon<float>(), __FILE__, __LINE__);
	}

	{
		Vertex3 v0(-1, -0.5f, 0);
		Vertex3 v1(1, 0.5f, 0);
		BoundingBox bb(v0, v1);
		CHECK_ASSERT(bb.Size() == Vertex3(2, 1, 0), __FILE__, __LINE__);

		Node node;
		node.SetScale(Vertex3(2));
		bb.Transform(node);
		CHECK_ASSERT(glm::distance(bb.Size(), Vertex3(4, 2, 0)) < 3 * glm::epsilon<float>(), __FILE__, __LINE__);
		bb.Transform(node);
		CHECK_ASSERT(glm::distance(bb.Size(), Vertex3(8, 4, 0)) < 3 * glm::epsilon<float>(), __FILE__, __LINE__);
		node.SetOrientation(glm::angleAxis(glm::pi<float>() / 2, Vertex3(0, 1, 0)));
		bb.Transform(node);
		CHECK_ASSERT(glm::distance(bb.Size(), Vertex3(0, 8, 16)) < 0.1f, __FILE__, __LINE__);
		CHECK_ASSERT(bb.Center() == Vertex3(0), __FILE__, __LINE__);
		node.SetPosition(Vertex3(2,2,2));
		bb.Transform(node);
		CHECK_ASSERT(bb.Center() == Vertex3(2), __FILE__, __LINE__);
	}

}

void BBTest()
{
	Test01();
}