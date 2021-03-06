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
#include "BoundingBox.h"
#include "Mesh.h"
#include "Plane.h"
#include "Types.h"
#include <algorithm>
namespace NSG {
enum FrustumPlane {
    PLANE_NEAR = 0,
    PLANE_LEFT,
    PLANE_RIGHT,
    PLANE_UP,
    PLANE_DOWN,
    PLANE_FAR,
    MAX_PLANES
};

enum FrustumFace {
    FACE_RIGHT,
    FACE_LEFT,
    FACE_TOP,
    FACE_BOTTOM,
    FACE_FAR,
    FACE_NEAR
};

static const int NUM_FRUSTUM_VERTICES = 8;
static const int NUM_FRUSTUM_FACES = 6;

class Frustum {
public:
    Frustum(const Matrix4& VP);
    Intersection IsPointInside(const Vector3& point) const;
    Intersection IsSphereInside(const Vertex3& center, float radius) const;
    Intersection IsInside(const BoundingBox& box) const;
    bool IsVisible(const SceneNode& node) const;
    bool IsVisible(const Node& node, Mesh& mesh) const;
    const Plane& GetPlane(FrustumPlane idx) const { return planes_[idx]; }
    const Vector3* GetVertices() const { return vertices_; }
    std::vector<Vector3> GetVerticesTransform(const Matrix4& m) const;
    const Matrix4& GetMatrix() const { return m_; }
    struct Face {
        Vector3 vertices[4];
    };
    const Face& GetFace(FrustumFace index) const;
    void Debug(const Vector3& origin, DebugRenderer* debugRenderer,
               const Color& color);
    const Vector3& GetOrigin() const { return origin_; }

private:
    void Define();
    void BuildFaces();
    Vector3 IntersectionPoint(const Plane& a, const Plane& b, const Plane& c);
    Matrix4 m_;
    Plane planes_[FrustumPlane::MAX_PLANES];
    Vector3 vertices_[NUM_FRUSTUM_VERTICES];
    Face faces_[NUM_FRUSTUM_FACES];
    Vector3 origin_;
};
}
