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
#include "Types.h"
#include "Check.h"
#include "StringConverter.h"
#include "LinearMath/btTransform.h"
#include "LinearMath/btVector3.h"
#include "LinearMath/btQuaternion.h"

#include <string>
#include <algorithm>

namespace NSG
{
    btTransform ToTransform(const Vector3& pos, const Quaternion& rot);
    btVector3 ToBtVector3(const Vector3& obj);
    Vector3 ToVector3(const btVector3& obj);
    btQuaternion ToBtQuaternion(const Quaternion& q);
    Quaternion ToQuaternion(const btQuaternion& q);

    template<typename T>
	T Lerp(const T& lhs, const T& rhs, float t)
	{
		return lhs * (1.0f - t) + rhs * t;
	}

	bool IsNaN(const Quaternion& q);
	Quaternion QuaternionFromLookRotation(const Vector3& direction, const Vector3& upDirection);
	Vector3 Translation(const Matrix4& m);
	Vector3 Scale(const Matrix4& m);
	void DecomposeMatrix(const Matrix4& m, Vertex3& position, Quaternion& q, Vertex3& scale);
	inline unsigned NextPowerOfTwo(unsigned value)
	{
	    unsigned ret = 1;
	    while (ret < value && ret < 0x80000000)
	        ret <<= 1;
	    return ret;
	}
	std::string GetUniqueName(const std::string& name = "");
	void GetPowerOfTwoValues(int& width, int& height);
	bool IsPowerOfTwo(int value);
	bool IsZeroLength(const Vector3& obj);
	GLushort Transform(GLubyte selected[4]);
	Color Transform(GLushort id);
	std::string CompressBuffer(const std::string& buf);
	std::string DecompressBuffer(const std::string& buffer);
	bool IsScaleUniform(const Vector3& scale);
}
