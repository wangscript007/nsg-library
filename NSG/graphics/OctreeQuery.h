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
#pragma once
#include "Types.h"
#include "Frustum.h"
#include <vector>

namespace NSG
{
    class OctreeQuery
    {
    public:
        OctreeQuery(std::vector<const SceneNode*>& result);
        virtual ~OctreeQuery();
        virtual Intersection TestOctant(const BoundingBox& box, bool inside) = 0;
		virtual void Test(const std::vector<SceneNode*>& objs, bool inside) = 0;
        std::vector<const SceneNode*>& result_;
    };

    class FrustumOctreeQuery : public OctreeQuery
    {
    public:
        FrustumOctreeQuery(std::vector<const SceneNode*>& result, const Frustum& frustum);
        virtual Intersection TestOctant(const BoundingBox& box, bool inside) override;
		virtual void Test(const std::vector<SceneNode*>& objs, bool inside) override;

        Frustum frustum_;
    };


}