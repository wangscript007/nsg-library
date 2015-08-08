/*
-------------------------------------------------------------------------------
This file is part of nsg-library.
http://github.com/woodjazz/nsg-library

Copyright (c) 2014-2015 NÃ©stor Silveira Gorski

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
#include "VertexArrayObj.h"
#include "Program.h"
#include "Material.h"
#include "Mesh.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "InstanceBuffer.h"
#include "Graphics.h"
#include "Check.h"

namespace NSG
{
	bool VAOKey::operator < (const VAOKey& obj) const
	{
		return program < obj.program ||
			(!(obj.program < program) && mesh < obj.mesh) ||
			(!(obj.program < program) && !(obj.mesh < mesh) && allowInstancing < obj.allowInstancing) ||
            (!(obj.program < program) && !(obj.mesh < mesh) && !(obj.allowInstancing < allowInstancing) && solid < obj.solid);
	}

	std::string VAOKey::GetName() const
	{
		if(allowInstancing)
			return program->GetName() + mesh->GetName() + "_VAOI";
		else
			return program->GetName() + mesh->GetName() + "_VAO";
	}

	VertexArrayObj::VAOMap VertexArrayObj::vaoMap_;

    VertexArrayObj::VertexArrayObj(const VAOKey& key)
        : Object(key.GetName()),
          vao_(0),
          key_(key)
    {
    }

    VertexArrayObj::~VertexArrayObj()
    {
    }

    bool VertexArrayObj::IsValid()
    {
        return key_.program->IsReady() && key_.mesh->IsReady();
    }

    void VertexArrayObj::AllocateResources()
    {
        CHECK_GL_STATUS(__FILE__, __LINE__);
	
		auto vBuffer = key_.mesh->GetVertexBuffer();
		auto iBuffer = key_.mesh->GetIndexBuffer(key_.solid);
		auto program = key_.program;
        auto mesh = key_.mesh;

		CHECK_ASSERT(!vBuffer->IsDynamic() && !iBuffer->IsDynamic(), __FILE__, __LINE__);

        glGenVertexArrays(1, &vao_);

        CHECK_ASSERT(vao_ != 0, __FILE__, __LINE__);

        Graphics::this_->SetVertexArrayObj(this);

        Graphics::this_->SetVertexBuffer(vBuffer, true);

        GLuint position_loc = program->GetAttPositionLoc();
        GLuint texcoord_loc0 = program->GetAttTextCoordLoc0();
        GLuint texcoord_loc1 = program->GetAttTextCoordLoc1();
        GLuint normal_loc = program->GetAttNormalLoc();
        GLuint color_loc = program->GetAttColorLoc();
        GLuint tangent_loc = program->GetAttTangentLoc();
        GLuint bones_id_loc = program->GetAttBonesIDLoc();
        GLuint bones_weight = program->GetAttBonesWeightLoc();

        if (position_loc != -1)
            glEnableVertexAttribArray((int)AttributesLoc::POSITION);

        if (normal_loc != -1)
            glEnableVertexAttribArray((int)AttributesLoc::NORMAL);

        if (texcoord_loc0 != -1)
            glEnableVertexAttribArray((int)AttributesLoc::TEXTURECOORD0);

        if (texcoord_loc1 != -1)
            glEnableVertexAttribArray((int)AttributesLoc::TEXTURECOORD1);

        if (color_loc != -1)
            glEnableVertexAttribArray((int)AttributesLoc::COLOR);

        if (tangent_loc != -1)
            glEnableVertexAttribArray((int)AttributesLoc::TANGENT);

        if (bones_id_loc != -1)
            glEnableVertexAttribArray((int)AttributesLoc::BONES_ID);

        if (bones_weight != -1)
            glEnableVertexAttribArray((int)AttributesLoc::BONES_WEIGHT);

        Graphics::this_->SetVertexAttrPointers();

        Graphics::this_->SetIndexBuffer(iBuffer, true);

        if (key_.allowInstancing && program->GetMaterial()->IsBatched())
            Graphics::this_->SetInstanceAttrPointers(program);

        CHECK_GL_STATUS(__FILE__, __LINE__);

        slotProgramReleased_ = program->SigReleased()->Connect([this]()
        {
            Invalidate();
        });

        slotMeshReleased_ = mesh->SigReleased()->Connect([this]()
        {
            Invalidate();
        });
    }

    void VertexArrayObj::ReleaseResources()
    {
        if (Graphics::this_->GetVertexArrayObj() == this)
            Graphics::this_->SetVertexArrayObj(nullptr);
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }

    void VertexArrayObj::Use()
    {
        if (IsReady())
            Graphics::this_->SetVertexArrayObj(this);
    }

    void VertexArrayObj::Bind()
    {
        glBindVertexArray(vao_);
    }

    void VertexArrayObj::Unbind()
    {
        glBindVertexArray(0);
    }

    PVertexArrayObj VertexArrayObj::GetOrCreate(const VAOKey& key)
    {
        auto it = vaoMap_.find(key);
        if (it != vaoMap_.end())
        	return it->second;
		auto vao = std::make_shared<VertexArrayObj>(key);
        CHECK_CONDITION(vaoMap_.insert(VAOMap::value_type(key, vao)).second, __FILE__, __LINE__);
        return vao;
    }

    void VertexArrayObj::Clear()
    {
    	vaoMap_.clear();
    }
}
