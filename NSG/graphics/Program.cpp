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

#include "Program.h"
#include "VertexShader.h"
#include "FragmentShader.h"
#include "Texture.h"
#include "Check.h"
#include "Light.h"
#include "Camera.h"
#include "Mesh.h"
#include "Scene.h"
#include "Skeleton.h"
#include "Material.h"
#include "Graphics.h"
#include "Constants.h"
#include "Util.h"
#include "Material.h"
#include "pugixml.hpp"
#include "autogenerated/Common_glsl.inl"
#include "autogenerated/Samplers_glsl.inl"
#include "autogenerated/Transforms_glsl.inl"
#include "autogenerated/Lighting_glsl.inl"
#include "autogenerated/PostProcess_glsl.inl"
#include "autogenerated/VS_glsl.inl"
#include "autogenerated/FS_glsl.inl"
#include <stdlib.h>
#include <stdio.h>
#if !defined(__APPLE__)
#include <malloc.h>
#endif
#include <assert.h>
#include <cstring>
#include <string>
#include <algorithm>
#include <sstream>

namespace NSG
{
    Program::Program(const std::string& defines)
        : Object(GetUniqueName("Program")),
          defines_(defines),
          id_(0),
          att_texcoordLoc0_(-1),
          att_texcoordLoc1_(-1),
          att_positionLoc_(-1),
          att_normalLoc_(-1),
          att_colorLoc_(-1),
          att_tangentLoc_(-1),
          att_bonesIDLoc_(-1),
          att_bonesWeightLoc_(-1),
          att_modelMatrixRow0Loc_(-1),
          att_normalMatrixCol0Loc_(-1),
          modelLoc_(-1),
          normalMatrixLoc_(-1),
          viewLoc_(-1),
          viewProjectionLoc_(-1),
          projectionLoc_(-1),
          sceneColorAmbientLoc_(-1),
          eyeWorldPosLoc_(-1),
          u_uvTransformLoc_(-1),
          blendMode_loc_(-1),
          nBones_(0),
          activeCamera_(nullptr),
          viewVariablesNeverSet_(true),
          materialVariablesNeverSet_(true),
          activeSkeleton_(nullptr),
          activeNode_(nullptr),
          activeScene_(nullptr),
          sceneColor_(-1),
          mesh_(nullptr),
          node_(nullptr),
          material_(nullptr),
          light_(nullptr)
    {
        memset(&textureLoc_, -1, sizeof(textureLoc_));
        memset(&materialLoc_, -1, sizeof(materialLoc_));
        memset(&blurFilterLoc_, -1, sizeof(blurFilterLoc_));
        memset(&wavesFilterLoc_, -1, sizeof(wavesFilterLoc_));
    }

    Program::~Program()
    {
    }

    void Program::ConfigureShaders(std::string& vertexShader, std::string& fragmentShader)
    {
        std::string preDefines;

        #ifdef GL_ES_VERSION_2_0
        preDefines = "#version 100\n#define GLES2\n";
        #else
        preDefines = "#version 120\n";
        #endif

        std::string::size_type pos0 = 0;
		std::string::size_type pos1 = defines_.find('\n', 0);
		while (pos1 != std::string::npos)
        {
			preDefines += "#define " + defines_.substr(pos0, pos1 - pos0 + 1);
			pos0 = pos1 + 1;
			pos1 = defines_.find('\n', pos0);
		} ;

		TRACE_PRINTF("Shader variation:\n%s", defines_.c_str());

        std::string vBuffer = preDefines + "#define COMPILEVS\n";
        vBuffer += COMMON_GLSL;
        vBuffer += TRANSFORMS_GLSL;
        vBuffer += LIGHTING_GLSL;
        vBuffer += VS_GLSL;
        vertexShader = vBuffer;

        std::string fBuffer = preDefines + "#define COMPILEFS\n";
        fBuffer += COMMON_GLSL;
        fBuffer += SAMPLERS_GLSL;
        fBuffer += LIGHTING_GLSL;
        fBuffer += POSTPROCESS_GLSL;
        fBuffer += FS_GLSL;
        fragmentShader = fBuffer;
    }

    bool Program::IsValid()
    {
        return material_ && material_->IsReady() && mesh_ && mesh_->IsReady();
    }

    void Program::AllocateResources()
    {
        std::string vShader;
        std::string fShader;
        ConfigureShaders(vShader, fShader);
        pVShader_ = PVertexShader(new VertexShader(vShader.c_str()));
        pFShader_ = PFragmentShader(new FragmentShader(fShader.c_str()));
        if (Initialize())
        {
            Graphics::this_->SetProgram(this);
            SetUniformLocations();
        }
    }

    void Program::ReleaseResources()
    {
        if (pVShader_)
            glDetachShader(id_, pVShader_->GetId());
        if (pFShader_)
            glDetachShader(id_, pFShader_->GetId());

        pVShader_ = nullptr;
        pFShader_ = nullptr;
        glDeleteProgram(id_);

        if (Graphics::this_->GetProgram() == this)
            Graphics::this_->SetProgram(nullptr);

        nBones_ = 0;

        activeCamera_ = nullptr;
        viewVariablesNeverSet_ = true;
        materialVariablesNeverSet_ = true;
        activeSkeleton_ = nullptr;
        activeNode_ = nullptr;
        activeScene_ = nullptr;
        sceneColor_ = Color(-1);

        bonesBaseLoc_.clear();

        Graphics::this_->InvalidateVAOFor(this);
    }

    bool Program::ShaderCompiles(GLenum type, const std::string& buffer) const
    {
        CHECK_GL_STATUS(__FILE__, __LINE__);
        GLuint id = glCreateShader(type);
        const char* source = buffer.c_str();
        glShaderSource(id, 1, &source, nullptr);
        glCompileShader(id);
        GLint compile_status = GL_FALSE;
        glGetShaderiv(id, GL_COMPILE_STATUS, &compile_status);
        if (compile_status != GL_TRUE)
        {
            GLint logLength = 0;
            glGetShaderiv(id, GL_INFO_LOG_LENGTH, &logLength);
            if (logLength > 0)
            {
                std::string log;
                log.resize(logLength);
                glGetShaderInfoLog(id, logLength, &logLength, &log[0]);
                TRACE_PRINTF("%s", log.c_str());
            }
        }
        glDeleteShader(id);
        //glReleaseShaderCompiler(); // fails on osx
        CHECK_GL_STATUS(__FILE__, __LINE__);
        TRACE_PRINTF("Checking %s shader for material %s: %s", (type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT"), name_.c_str(), (compile_status == GL_TRUE ? "IS OK" : "HAS FAILED"));

        return compile_status == GL_TRUE;
    }

    void Program::SetUniformLocations()
    {
        CHECK_GL_STATUS(__FILE__, __LINE__);

        att_positionLoc_ = GetAttributeLocation("a_position");
        att_normalLoc_ = GetAttributeLocation("a_normal");
        att_texcoordLoc0_ = GetAttributeLocation("a_texcoord0");
        att_texcoordLoc1_ = GetAttributeLocation("a_texcoord1");
        att_colorLoc_ = GetAttributeLocation("a_color");
        att_tangentLoc_ = GetAttributeLocation("a_tangent");
        att_bonesIDLoc_ = GetAttributeLocation("a_boneIDs");
        att_bonesWeightLoc_ = GetAttributeLocation("a_weights");

        att_modelMatrixRow0Loc_ = GetAttributeLocation("a_mMatrixRow0");
        att_normalMatrixCol0Loc_ = GetAttributeLocation("a_normalMatrixCol0");

        modelLoc_ = GetUniformLocation("u_model");
        normalMatrixLoc_ = GetUniformLocation("u_normalMatrix");
        viewLoc_ = GetUniformLocation("u_view");
        viewProjectionLoc_ = GetUniformLocation("u_viewProjection");
        projectionLoc_  = GetUniformLocation("u_projection");
        sceneColorAmbientLoc_ = GetUniformLocation("u_sceneAmbientColor");
        eyeWorldPosLoc_ = GetUniformLocation("u_eyeWorldPos");
        u_uvTransformLoc_ = GetUniformLocation("u_uvTransform");
        for (size_t index = 0; index < MaterialTexture::MAX_TEXTURES_MAPS; index++)
            textureLoc_[index] = GetUniformLocation("u_texture" + ToString(index));
        materialLoc_.color_ = GetUniformLocation("u_material.color");
        materialLoc_.ambient_ = GetUniformLocation("u_material.ambient");
        materialLoc_.diffuse_ = GetUniformLocation("u_material.diffuse");
        materialLoc_.specular_ = GetUniformLocation("u_material.specular");
        materialLoc_.shininess_ = GetUniformLocation("u_material.shininess");

        for (size_t i = 0; i < nBones_; i++)
        {
            GLuint boneLoc = GetUniformLocation("u_bones[" + ToString(i) + "]");
            CHECK_ASSERT(boneLoc != -1, __FILE__, __LINE__);
            bonesBaseLoc_.push_back(boneLoc);
        }

        {
            directionalLightLoc_.base_.diffuse_ = GetUniformLocation("u_directionalLight.base.diffuse");
            directionalLightLoc_.base_.specular_ = GetUniformLocation("u_directionalLight.base.specular");
            directionalLightLoc_.direction_ = GetUniformLocation("u_directionalLight.direction");
        }

        {
            pointLightLoc_.base_.diffuse_ = GetUniformLocation("u_pointLight.base.diffuse");
            pointLightLoc_.base_.specular_ = GetUniformLocation("u_pointLight.base.specular");
            pointLightLoc_.position_ = GetUniformLocation("u_pointLight.position");
            pointLightLoc_.invRange_ = GetUniformLocation("u_pointLight.invRange");
        }

        {
            spotLightLoc_.base_.diffuse_ = GetUniformLocation("u_spotLight.base.diffuse");
            spotLightLoc_.base_.specular_ = GetUniformLocation("u_spotLight.base.specular");
            spotLightLoc_.position_ = GetUniformLocation("u_spotLight.position");
            spotLightLoc_.direction_ = GetUniformLocation("u_spotLight.direction");
            spotLightLoc_.cutOff_ = GetUniformLocation("u_spotLight.cutOff");
            spotLightLoc_.invRange_ = GetUniformLocation("u_spotLight.invRange");
        }

        blendMode_loc_ = GetUniformLocation("u_blendMode");
        blurFilterLoc_.blurDir_ = GetUniformLocation("u_blurDir");
        blurFilterLoc_.blurRadius_ = GetUniformLocation("u_blurRadius");
        blurFilterLoc_.sigma_ = GetUniformLocation("u_sigma");
        wavesFilterLoc_.factor_ = GetUniformLocation("u_waveFactor");
        wavesFilterLoc_.offset_ = GetUniformLocation("u_waveOffset");

        for (int index = 0; index < MaterialTexture::MAX_TEXTURES_MAPS; index++)
        {
            if (textureLoc_[index] != -1)
                glUniform1i(textureLoc_[index], index); //set fixed locations for samplers
        }

        CHECK_GL_STATUS(__FILE__, __LINE__);
    }

    bool Program::Initialize()
    {
        CHECK_GL_STATUS(__FILE__, __LINE__);
        TRACE_PRINTF("Creating program for material %s", name_.c_str());
        id_ = glCreateProgram();
        // Bind vertex attribute locations to ensure they are the same in all shaders
        glBindAttribLocation(id_, (int)AttributesLoc::POSITION, "a_position");
        glBindAttribLocation(id_, (int)AttributesLoc::NORMAL, "a_normal");
        glBindAttribLocation(id_, (int)AttributesLoc::TEXTURECOORD0, "a_texcoord0");
        glBindAttribLocation(id_, (int)AttributesLoc::TEXTURECOORD1, "a_texcoord1");
        glBindAttribLocation(id_, (int)AttributesLoc::COLOR, "a_color");
        glBindAttribLocation(id_, (int)AttributesLoc::TANGENT, "a_tangent");
        glBindAttribLocation(id_, (int)AttributesLoc::BONES_ID, "a_boneIDs");
        glBindAttribLocation(id_, (int)AttributesLoc::BONES_WEIGHT, "a_weights");
        glBindAttribLocation(id_, (int)AttributesLoc::MODEL_MATRIX_ROW0, "a_mMatrixRow0");
        glBindAttribLocation(id_, (int)AttributesLoc::MODEL_MATRIX_ROW1, "a_mMatrixRow1");
        glBindAttribLocation(id_, (int)AttributesLoc::MODEL_MATRIX_ROW2, "a_mMatrixRow2");
        glBindAttribLocation(id_, (int)AttributesLoc::NORMAL_MATRIX_COL0, "a_normalMatrixCol0");
        glBindAttribLocation(id_, (int)AttributesLoc::NORMAL_MATRIX_COL1, "a_normalMatrixCol1");
        glBindAttribLocation(id_, (int)AttributesLoc::NORMAL_MATRIX_COL2, "a_normalMatrixCol2");
        glAttachShader(id_, pVShader_->GetId());
        glAttachShader(id_, pFShader_->GetId());
        glLinkProgram(id_);
        GLint link_status = GL_FALSE;
        glGetProgramiv(id_, GL_LINK_STATUS, &link_status);
        if (link_status != GL_TRUE)
        {
            GLint logLength = 0;
            glGetProgramiv(id_, GL_INFO_LOG_LENGTH, &logLength);
            if (logLength > 0)
            {
                std::string log;
                log.resize(logLength);
                glGetProgramInfoLog(id_, logLength, &logLength, &log[0]);
                TRACE_PRINTF("Error in Program Creation: %s", log.c_str());
                //TRACE_PRINTF("VS: %s", pVShader_->GetSource());
                //TRACE_PRINTF("FS: %s" << pFShader_->GetSource());
            }
            TRACE_PRINTF("Linking program for material %s HAS FAILED!!!", name_.c_str());
            return false;
        }
        TRACE_PRINTF("Program for material %s OK.", name_.c_str());
        CHECK_GL_STATUS(__FILE__, __LINE__);
        return true;
    }

    GLuint Program::GetAttributeLocation(const std::string& name)
    {
        return glGetAttribLocation(id_, name.c_str());
    }

    GLuint Program::GetUniformLocation(const std::string& name)
    {
        return glGetUniformLocation(id_, name.c_str());
    }

    void Program::SetSceneVariables()
    {
        Scene* scene = Graphics::this_->GetScene();

        if (sceneColorAmbientLoc_ != -1)
        {
            if (scene)
            {
                if (activeScene_ != scene || scene->UniformsNeedUpdate())
                    glUniform4fv(sceneColorAmbientLoc_, 1, &scene->GetAmbientColor()[0]);
            }
            else if (activeScene_ != scene || sceneColor_ == Color(-1))
            {
                sceneColor_ = Color(0, 0, 0, 1);
                glUniform4fv(sceneColorAmbientLoc_, 1, &sceneColor_[0]);
            }

            activeScene_ = scene;
        }
    }

    void Program::SetNodeVariables()
    {
        if (node_ && (activeNode_ != node_ || node_->UniformsNeedUpdate()))
        {
            if (modelLoc_ != -1)
            {
                const Matrix4& m = node_->GetGlobalModelMatrix();
                glUniformMatrix4fv(modelLoc_, 1, GL_FALSE, glm::value_ptr(m));
            }

            if (normalMatrixLoc_ != -1)
            {
                const Matrix3& m = node_->GetGlobalModelInvTranspMatrix();
                glUniformMatrix3fv(normalMatrixLoc_, 1, GL_FALSE, glm::value_ptr(m));
            }

            activeNode_ = node_;
        }
    }

    void Program::SetMaterialVariables()
    {
        if (material_)
        {
            for (int index = 0; index < MaterialTexture::MAX_TEXTURES_MAPS; index++)
            {
                if (textureLoc_[index] != -1)
                {
                    MaterialTexture type = (MaterialTexture)index;
                    Graphics::this_->SetTexture(index, material_->GetTexture(type).get());
                }
            }

            if (materialVariablesNeverSet_ || material_->UniformsNeedUpdate())
            {
                materialVariablesNeverSet_ = false;

                if (materialLoc_.color_ != -1)
                    glUniform4fv(materialLoc_.color_, 1, &material_->color_[0]);

                if (materialLoc_.ambient_ != -1)
                    glUniform4fv(materialLoc_.ambient_, 1, &material_->ambient_[0]);

                if (materialLoc_.diffuse_ != -1)
                    glUniform4fv(materialLoc_.diffuse_, 1, &material_->diffuse_[0]);

                if (materialLoc_.specular_ != -1)
                    glUniform4fv(materialLoc_.specular_, 1, &material_->specular_[0]);

                if (materialLoc_.shininess_ != -1)
                    glUniform1f(materialLoc_.shininess_, material_->shininess_);

                if (blendMode_loc_ != -1)
                    glUniform1i(blendMode_loc_, (int)material_->GetFilterBlendMode());

                if (blurFilterLoc_.blurDir_ != -1)
                    glUniform2fv(blurFilterLoc_.blurDir_, 1, &material_->blurFilter_.blurDir_[0]);

                if (blurFilterLoc_.blurRadius_ != -1)
                    glUniform2fv(blurFilterLoc_.blurRadius_, 1, &material_->blurFilter_.blurRadius_[0]);

                if (blurFilterLoc_.sigma_ != -1)
                    glUniform1f(blurFilterLoc_.sigma_, material_->blurFilter_.sigma_);

                if (wavesFilterLoc_.factor_ != -1)
                    glUniform1f(wavesFilterLoc_.factor_, material_->waveFilter_.factor_);

                if (wavesFilterLoc_.offset_ != -1)
                    glUniform1f(wavesFilterLoc_.offset_, material_->waveFilter_.offset_);

                if (u_uvTransformLoc_ != -1)
                    glUniform4fv(u_uvTransformLoc_, 1, &material_->uvTransform_[0]);
            }
        }
    }

    void Program::SetSkeletonVariables()
    {
        auto skeleton = mesh_->GetSkeleton().get();

        if (skeleton)
        {
            const std::vector<PWeakNode>& bones = skeleton->GetBones();
            size_t nBones = bones.size();
            CHECK_CONDITION(nBones == nBones_ && "This shader has been used with a different number of bones.!!!", __FILE__, __LINE__);

            PNode rootNode = skeleton->GetRoot().lock();
            Matrix4 globalInverseModelMatrix(1);
            PNode parent = rootNode->GetParent();
            if (parent)
            {
                // In order to make all the bones relatives to the root's parent.
                // The model matrix and normal matrix for the active node is premultiplied in the shader (see Program::SetNodeVariables)
                // See in Transform.glsl: GetModelMatrix() and GetWorldNormal()
                globalInverseModelMatrix = parent->GetGlobalModelInvMatrix();
            }

            CHECK_GL_STATUS(__FILE__, __LINE__);

            for (unsigned idx = 0; idx < nBones; idx++)
            {
                GLuint boneLoc = bonesBaseLoc_[idx];

                Node* bone = bones[idx].lock().get();
                if (activeSkeleton_ != skeleton || bone->UniformsNeedUpdate())
                {
                    // Be careful, bones don't have normal matrix so their scale must be uniform (sx == sy == sz)
                    CHECK_ASSERT(bone->IsScaleUniform(), __FILE__, __LINE__);
                    const Matrix4& m = bone->GetGlobalModelMatrix();
                    const Matrix4& offsetMatrix = bone->GetBoneOffsetMatrix();
                    Matrix4 boneMatrix(globalInverseModelMatrix * m * offsetMatrix);
                    glUniformMatrix4fv(boneLoc, 1, GL_FALSE, glm::value_ptr(boneMatrix));
                }
            }

            CHECK_GL_STATUS(__FILE__, __LINE__);

        }

        activeSkeleton_ = skeleton;

    }

    void Program::SetCameraVariables()
    {
        if (viewLoc_ != -1 || viewProjectionLoc_ != -1 || projectionLoc_ != -1 || eyeWorldPosLoc_ != -1)
        {
            Camera* camera = Graphics::this_->GetCamera();

            bool update_camera = viewVariablesNeverSet_ || (activeCamera_ != camera || (camera && camera->UniformsNeedUpdate()));

            if (update_camera)
            {
                viewVariablesNeverSet_ = false;

                if (viewProjectionLoc_ != -1)
                {
                    const Matrix4& m = Camera::GetMatViewProj();
                    glUniformMatrix4fv(viewProjectionLoc_, 1, GL_FALSE, glm::value_ptr(m));
                }

                if (viewLoc_ != -1)
                {
                    const Matrix4& m = Camera::GetViewMatrix();
                    glUniformMatrix4fv(viewLoc_, 1, GL_FALSE, glm::value_ptr(m));
                }

                if (projectionLoc_ != -1)
                {
                    const Matrix4& m = Camera::GetProjectionMatrix();
                    glUniformMatrix4fv(projectionLoc_, 1, GL_FALSE, glm::value_ptr(m));
                }


                if (eyeWorldPosLoc_ != -1)
                {
                    Vertex3 position(0);
                    if (camera)
                        position = camera->GetGlobalPosition();
                    glUniform3fv(eyeWorldPosLoc_, 1, &position[0]);
                }
            }

            activeCamera_ = camera;
        }
    }

    void Program::SetBaseLightVariables(const BaseLightLoc& baseLoc)
    {
        if (baseLoc.diffuse_ != -1)
        {
            const Color& diffuse = light_->GetDiffuseColor();
            glUniform4fv(baseLoc.diffuse_, 1, &diffuse[0]);
        }

        if (baseLoc.specular_ != -1)
        {
            const Color& specular = light_->GetSpecularColor();
            glUniform4fv(baseLoc.specular_, 1, &specular[0]);
        }
    }

    void Program::SetLightVariables()
    {
        if (light_ && light_->UniformsNeedUpdate())
        {
            if (LightType::DIRECTIONAL == light_->GetType())
            {
                const DirectionalLightLoc& loc = directionalLightLoc_;

                SetBaseLightVariables(loc.base_);

                if (loc.direction_ != -1)
                {
                    const Vertex3& direction = light_->GetLookAtDirection();
                    glUniform3fv(loc.direction_, 1, &direction[0]);
                }
            }
            else if (LightType::POINT == light_->GetType())
            {
                const PointLightLoc& loc = pointLightLoc_;

                SetBaseLightVariables(loc.base_);

                if (loc.position_ != -1)
                {
                    const Vertex3& position = light_->GetGlobalPosition();
                    glUniform3fv(loc.position_, 1, &position[0]);
                }

                if (loc.invRange_ != -1)
                    glUniform1f(loc.invRange_, light_->GetInvRange());
            }
            else
            {
                CHECK_ASSERT(LightType::SPOT == light_->GetType(), __FILE__, __LINE__);

                const SpotLightLoc& loc = spotLightLoc_;

                SetBaseLightVariables(loc.base_);
                if (loc.position_ != -1)
                {
                    const Vertex3& position = light_->GetGlobalPosition();
                    glUniform3fv(loc.position_, 1, &position[0]);
                }

                if (loc.direction_ != -1)
                {
                    const Vertex3& direction = light_->GetLookAtDirection();
                    glUniform3fv(loc.direction_, 1, &direction[0]);
                }

                if (loc.cutOff_ != -1)
                {
                    float cutOff = light_->GetSpotCutOff() * 0.5f;
                    float value = glm::cos(glm::radians(cutOff));
                    glUniform1f(loc.cutOff_, value);
                }

                if (loc.invRange_ != -1)
                    glUniform1f(loc.invRange_, light_->GetInvRange());
            }
        }
    }

    void Program::SetVariables()
    {
        SetSkeletonVariables();
        SetSceneVariables();
        SetMaterialVariables();
        SetNodeVariables();
        SetCameraVariables();
        SetLightVariables();
    }

    void Program::Set(Mesh* mesh)
    {
        mesh_ = mesh;
    }

    void Program::Set(Node* node)
    {
        if (node_ != node)
        {
            if (node)
                node->SetUniformsNeedUpdate();
            node_ = node;
        }
    }

    void Program::Set(Material* material)
    {
        if (material_ != material)
        {
            material->SetUniformsNeedUpdate();
            material_ = material;
        }
    }

    void Program::Set(Light* light)
    {
        if (light_ != light)
        {
            light->SetUniformsNeedUpdate();
            light_ = light;
        }
    }

    void Program::SetNumberBones(size_t nBones)
    {
        if (nBones_ != nBones)
        {
            CHECK_CONDITION(!nBones_ && "This shader has been used with a different number of bones.!!!", __FILE__, __LINE__);
            nBones_ = nBones;
        }
    }
}
