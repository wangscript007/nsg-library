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
#include "Skeleton.h"
#include "Node.h"
#include "Check.h"
#include "App.h"
#include "ModelMesh.h"
#include "Scene.h"
#include "Util.h"
#include "pugixml.hpp"

namespace NSG
{
    Skeleton::Skeleton(PModelMesh mesh)
        : mesh_(mesh)
    {

    }

    Skeleton::~Skeleton()
    {

    }

    unsigned Skeleton::GetBoneIndex(const std::string& name) const
    {
        unsigned result = -1;
        unsigned idx = 0;
        for (auto& obj : bones_)
        {
            if (obj->GetName() == name)
            {
                result = idx;
                break;
            }
            ++idx;
        }
        return result;
    }

    void Skeleton::SetBlendData(const std::vector<std::vector<unsigned>>& blendIndices, const std::vector<std::vector<float>>& blendWeights)
    {
        CHECK_CONDITION(blendIndices.size() == blendWeights.size(), __FILE__, __LINE__);
        blendIndices_ = blendIndices;
        blendWeights_ = blendWeights;
		mesh_.lock()->SetBlendData(blendIndices, blendWeights);
    }

    void Skeleton::Save(pugi::xml_node& node)
    {
        pugi::xml_node child = node.append_child("Skeleton");
        child.append_attribute("meshName") = mesh_.lock()->GetName().c_str();
        child.append_attribute("rootName") = root_->GetName().c_str();

        {
            pugi::xml_node childBones = child.append_child("Bones");
            for (auto& obj : bones_)
            {
                pugi::xml_node childBone = childBones.append_child("Bone");
                childBone.append_attribute("boneName") = obj->GetName().c_str();

				{
					const Matrix4& offset = obj->GetBoneOffsetMatrix();

					std::stringstream ss;
					ss << ToString(offset);
					childBone.append_attribute("offsetMatrix") = ss.str().c_str();
				}

            }
        }
#if 0
		CHECK_CONDITION(blendIndices_.size() == blendWeights_.size(), __FILE__, __LINE__);

        unsigned nVertexes = blendIndices_.size();
        if (nVertexes)
        {
			pugi::xml_node childVertexes = child.append_child("Vertexes");
            for (unsigned i = 0; i < nVertexes; i++)
            {
                pugi::xml_node childVertex = childVertexes.append_child("Vertex");
                {
                    pugi::xml_node childBoneIndexes = childVertex.append_child("BoneIndexes");
                    std::stringstream ss;
                    for (auto index : blendIndices_[i])
						ss << index << " ";
                    childBoneIndexes.append_child(pugi::node_pcdata).set_value(ss.str().c_str());
                }
                {
                    pugi::xml_node childBoneWeights = childVertex.append_child("BoneWeights");
                    std::stringstream ss;
                    for (auto weight : blendWeights_[i])
						ss << weight << " ";
                    childBoneWeights.append_child(pugi::node_pcdata).set_value(ss.str().c_str());
                }
            }
        }
#endif
    }

    void Skeleton::Load(const pugi::xml_node& node)
    {
        std::string meshName = node.attribute("meshName").as_string();
        const std::vector<PMesh>& meshes = App::this_->GetMeshes();
		mesh_ = App::this_->GetModelMesh(meshName);
		CHECK_CONDITION(mesh_.lock(), __FILE__, __LINE__);
        std::string rootName = node.attribute("rootName").as_string();
        PScene scene = App::this_->GetCurrentScene();
        root_ = scene->GetChild<Node>(rootName, true);
        CHECK_ASSERT(root_, __FILE__, __LINE__);
        bones_.clear();
        pugi::xml_node childBones = node.child("Bones");
        if (childBones)
        {
            pugi::xml_node child = childBones.child("Bone");
            while (child)
            {
                std::string boneName = child.attribute("boneName").as_string();
                PNode bone = scene->GetChild<Node>(boneName, true);
                CHECK_ASSERT(bone, __FILE__, __LINE__);
				bone->SetBoneOffsetMatrix(GetMatrix4(child.attribute("offsetMatrix").as_string()));
                bones_.push_back(bone);
                child = child.next_sibling("Bone");
            }
        }
#if 0
        blendIndices_.clear();
        blendWeights_.clear();
        pugi::xml_node childVertexes = node.child("Vertexes");
        if (childVertexes)
        {
            pugi::xml_node child = childVertexes.child("Vertex");
            while (child)
            {
				pugi::xml_node childBoneIndexes = child.child("BoneIndexes");
				if (childBoneIndexes)
                {
					std::vector<unsigned> indexes;
                    std::string data = childBoneIndexes.child_value();
                    std::stringstream ss;
                    ss << data;
                    unsigned index;
                    for (;;)
                    {
                        ss >> index;
                        if (ss.eof()) break;
						indexes.push_back(index);
                    }
					if (indexes.size())
						blendIndices_.push_back(indexes);
                }
                
				pugi::xml_node childBoneWeights = child.child("BoneWeights");
				if (childBoneWeights)
                {
					std::vector<float> weights;
                    std::string data = childBoneWeights.child_value();
                    std::stringstream ss;
                    ss << data;
                    float weight;
                    for (;;)
                    {
                        ss >> weight;
                        if (ss.eof()) break;
						weights.push_back(weight);
                    }
					if (weights.size())
						blendWeights_.push_back(weights);
                }
                child = child.next_sibling("Vertex");
            }
        }

        mesh_.lock()->SetBlendData(blendIndices_, blendWeights_);

		//CHECK_CONDITION(blendIndices_.size() == blendWeights_.size(), __FILE__, __LINE__);
		//CHECK_CONDITION(mesh_.lock()->GetVertexsData().size() == blendWeights_.size(), __FILE__, __LINE__);
#endif
    }

}