/*
-------------------------------------------------------------------------------
This file is part of nsg-library.
http://github.com/woodjazz/nsg-library

Copyright (c) 2014-2017 N�stor Silveira Gorski

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
#include "ProceduralMesh.h"

namespace NSG {
class CylinderMesh : public ProceduralMesh {
public:
    CylinderMesh(const std::string& name);
    ~CylinderMesh();
    void Set(float radius = 0.5f, float height = 1, int rings = 8,
             bool capped = true);
    void SetOrientation(const Quaternion& q);
    GLenum GetWireFrameDrawMode() const override;
    GLenum GetSolidDrawMode() const override;
    size_t GetNumberOfTriangles() const override;
    void AllocateResources() override;
    PhysicsShape GetShapeType() const override { return SH_CYLINDER_Y; }

private:
    float radius_;
    float height_;
    int rings_;
    bool capped_;
    Quaternion q_;
    friend class App;
};
}
