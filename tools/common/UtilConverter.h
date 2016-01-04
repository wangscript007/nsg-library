/*
-------------------------------------------------------------------------------
This file is part of nsg-library.
http://github.com/woodjazz/nsg-library

Copyright (c) 2014-2016 Néstor Silveira Gorski

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
#include "Types.h"
#include <string>

#define B_IDNAME(x) ((x) && (x)->id.name[0] != '0' ? (x)->id.name + 2 : "")
namespace BlenderConverter
{
	NSG::Matrix4 ToMatrix(const float m[][4]);
	NSG::Matrix3 ToMatrix(const float m[][3]);

	enum SPLINE_CHANNEL_CODE
	{
		NONE,
		SC_ROT_QUAT_W,
		SC_ROT_QUAT_X,
		SC_ROT_QUAT_Y,
		SC_ROT_QUAT_Z,
		SC_ROT_EULER_X,
		SC_ROT_EULER_Y,
		SC_ROT_EULER_Z,
		SC_LOC_X,
		SC_LOC_Y,
		SC_LOC_Z,
		SC_SCL_X,
		SC_SCL_Y,
		SC_SCL_Z
	};

	NSG::Matrix4 GeneratePointTransformMatrix(const NSG::Plane& plane, const NSG::Vector3& center);
}