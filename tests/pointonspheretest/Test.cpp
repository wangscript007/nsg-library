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
#include "NSG.h"
using namespace NSG;


static void Test01()
{
	PointOnSphere sphere;

	{
		Vertex3 position(0, 0, 10);
		Vertex3 center(0);

		sphere.SetCenterAndPoint(center, position);

		CHECK_CONDITION(Distance(position, sphere.GetPoint()) < PRECISION);
		CHECK_CONDITION(sphere.GetUp().y > 0.8f);
	}

	{
		Vertex3 position(0, 0, 10);
		Vertex3 center(0, 0, 9);

		sphere.SetCenterAndPoint(center, position);

		CHECK_CONDITION(Distance(position, sphere.GetPoint()) < PRECISION);
		CHECK_CONDITION(sphere.GetUp().y > 0.8f);

		sphere.IncAngles(PI, 0);

		CHECK_CONDITION(Distance(Vertex3(0, 0, 8), sphere.GetPoint()) < 0.05f);
		CHECK_CONDITION(sphere.GetUp().y > 0.8f);

		sphere.IncAngles(PI, 0);

		CHECK_CONDITION(Distance(position, sphere.GetPoint()) < 0.05f);
		CHECK_CONDITION(sphere.GetUp().y > 0.8f);
	}

	{
		Vertex3 position(0, 0, 10);
		Vertex3 center(0, 0, 9);

		sphere.SetCenterAndPoint(center, position);

		CHECK_CONDITION(Distance(position, sphere.GetPoint()) < 0.05f);
		CHECK_CONDITION(sphere.GetUp().y > 0.8f);
		CHECK_CONDITION(sphere.GetUp().z < -0.4f);

		sphere.IncAngles(0, PI / 2);

		CHECK_CONDITION(Distance(Vertex3(0, -1, 9), sphere.GetPoint()) < 0.05f);
		CHECK_CONDITION(sphere.GetUp().y > 0.4f);
		CHECK_CONDITION(sphere.GetUp().z > 0.8f);

		sphere.IncAngles(0, PI / 2);

		CHECK_CONDITION(Distance(Vertex3(0, 0, 8), sphere.GetPoint()) < 0.05f);
		CHECK_CONDITION(sphere.GetUp().y < -0.8f);
		CHECK_CONDITION(sphere.GetUp().z > 0.4f);

		sphere.IncAngles(0, PI / 2);

		CHECK_CONDITION(Distance(Vertex3(0, 1, 9), sphere.GetPoint()) < 0.05f);
		CHECK_CONDITION(sphere.GetUp().y < -0.4f);
		CHECK_CONDITION(sphere.GetUp().z < -0.8f);

		sphere.IncAngles(0, PI / 2);

		CHECK_CONDITION(Distance(position, sphere.GetPoint()) < 0.05f);
		CHECK_CONDITION(sphere.GetUp().y > 0.8f);
		CHECK_CONDITION(sphere.GetUp().z < -0.4f);

	}
#if 0
	{
		for (int i = 0; i < 5000; i++)
		{
			Vertex3 position(rand() - rand(), rand() - rand(), rand() - rand());
			Vertex3 center(rand() - rand(), rand() - rand(), rand() - rand());

			sphere.SetCenterAndPoint(center, position);

			CHECK_CONDITION(Distance(position, sphere.GetPoint()) < 0.09f);
		}
	}
#endif
	{
		Vertex3 position(0, 0, 10);
		Vertex3 center(1, 0, 10);

		sphere.SetCenterAndPoint(center, position);

		CHECK_CONDITION(Distance(position, sphere.GetPoint()) < 0.05f);
		CHECK_CONDITION(sphere.GetUp().y > 0.8f);

		sphere.IncAngles(PI, 0);

		CHECK_CONDITION(Distance(Vertex3(2, 0, 10), sphere.GetPoint()) < 0.05f);
		CHECK_CONDITION(sphere.GetUp().y > 0.8f);

		sphere.IncAngles(PI, 0);

		CHECK_CONDITION(Distance(position, sphere.GetPoint()) < 0.05f);
		CHECK_CONDITION(sphere.GetUp().y > 0.8f);

	}


	{
		Vertex3 position(0, 0, 10);
		Vertex3 center(0, 0, 9);

		sphere.SetCenterAndPoint(center, position);

		CHECK_CONDITION(Distance(position, sphere.GetPoint()) < 0.05f);
		CHECK_CONDITION(sphere.GetUp().y > 0.8f);

		sphere.IncAngles(PI/2, 0);

		CHECK_CONDITION(Distance(Vertex3(-1, 0, 9), sphere.GetPoint()) < 0.05f);
		CHECK_CONDITION(sphere.GetUp().y > 0.8f);

		sphere.IncAngles(PI / 2, 0);

		CHECK_CONDITION(Distance(Vertex3(0, 0, 8), sphere.GetPoint()) < 0.05f);
		CHECK_CONDITION(sphere.GetUp().y > 0.8f);
	}



	{
		Vertex3 position(1, 0, 10);
		Vertex3 center(-20, 0, 5);

		sphere.SetCenterAndPoint(center, position);

		CHECK_CONDITION(Distance(position, sphere.GetPoint()) < 0.05f);
		CHECK_CONDITION(sphere.GetUp().y > 0.8f);
	}


	{
		Vertex3 position = Vector3(0.1f, 0, 10);
		Vertex3 center(0);

		sphere.SetCenterAndPoint(center, position);

		CHECK_CONDITION(Distance(position, sphere.GetPoint()) < 0.05f);
		CHECK_CONDITION(sphere.GetUp().y > 0.8f);
	}

	{
		Vertex3 position = Vector3(1, 0, -10);
		Vertex3 center = Vector3(10, 10, 10);

		sphere.SetCenterAndPoint(center, position);

		CHECK_CONDITION(Distance(position, sphere.GetPoint()) < 0.05f);
		CHECK_CONDITION(sphere.GetUp().y > 0.8f);
	}

	{
		Vertex3 position = Vector3(10, 1, 5);
		Vertex3 center = Vector3(0);

		sphere.SetCenterAndPoint(center, position);

		CHECK_CONDITION(Distance(position, sphere.GetPoint()) < 0.05f);
		CHECK_CONDITION(sphere.GetUp().y > 0.8f);
	}

	{
		Vertex3 position = Vector3(0, 0, 5);
		Vertex3 center = Vector3(0);

		sphere.SetCenterAndPoint(center, position);

		CHECK_CONDITION(Distance(position, sphere.GetPoint()) < 0.05f);
		CHECK_CONDITION(sphere.GetUp().y > 0.8f);
	}



}

void Tests()
{
    Test01();
}