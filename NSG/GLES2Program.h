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

#include "GLES2Includes.h"
#include "GLES2VShader.h"
#include "GLES2FShader.h"
#include "Resource.h"
#include "GLES2GPUObject.h"
#include "SharedPointers.h"
#include <memory>
#include <string>
#include <vector>

namespace NSG 
{
	struct ExtraUniforms;
	class UseProgram;
	class GLES2Mesh;
	class GLES2Program : public GLES2GPUObject
	{
	public:
		GLES2Program(PResource pRVShader, PResource pRFShader);
		GLES2Program(const char* vShader, const char* fShader);
		~GLES2Program();
		bool Initialize();
		void Set(ExtraUniforms* pExtraUniforms) { pExtraUniforms_ = pExtraUniforms; }

		GLuint GetAttributeLocation(const std::string& name);
		GLuint GetUniformLocation(const std::string& name);
		void Render(bool solid, GLES2Mesh* pMesh);
		virtual bool IsValid();
		virtual void AllocateResources();
		virtual void ReleaseResources();
	private:
		operator const GLuint() const { return id_; }
		GLuint GetId() const { return id_; }
		void Use() { glUseProgram(id_); }
		GLuint id_;
		PGLES2VShader pVShader_;
		PGLES2FShader pFShader_;
		PResource pRVShader_;
		PResource pRFShader_;
		const char* vShader_;
		const char* fShader_;

		ExtraUniforms* pExtraUniforms_;
		GLuint color_scene_ambient_loc_;
		GLuint color_ambient_loc_;
		GLuint color_diffuse_loc_;
		GLuint color_specular_loc_;
		GLuint shininess_loc_;
		GLuint texture0_loc_;
		GLuint texture1_loc_;
		GLuint texcoord_loc_;
		GLuint position_loc_;
		GLuint normal_loc_;
        GLuint color_loc_;
		GLuint model_inv_transp_loc_;
		GLuint v_inv_loc_;
        GLuint mvp_loc_;
        GLuint m_loc_;

        struct LightLoc
        {
        	GLuint type_loc;
        	GLuint position_loc;
	        GLuint diffuse_loc;
	        GLuint specular_loc;
	        GLuint constantAttenuation_loc;
	        GLuint linearAttenuation_loc;
	        GLuint quadraticAttenuation_loc;
	        GLuint spotCutoff_loc;
	        GLuint spotExponent_loc;
	        GLuint spotDirection_loc;
	    };

	    typedef std::vector<LightLoc> LightsLoc;
	    LightsLoc lightsLoc_;
        bool hasLights_;

		friend class UseProgram;
	};

	class UseProgram
	{
	public:
		UseProgram(GLES2Program& obj, GLES2Material* material, Node* node);
		~UseProgram();
	private:
		GLES2Program& obj_;
	};

}
