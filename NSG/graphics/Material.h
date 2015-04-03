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
#include "Object.h"
#include "Batch.h"
#include "Util.h"
#include "MapAndVector.h"
#include "UniformsUpdate.h"
namespace NSG
{
    class Material : public Object, UniformsUpdate
    {
    public:
        Material(const std::string& name);
        ~Material();
        void SetProgramFlags(unsigned passIndex, const ProgramFlags& flags);
        void EnableProgramFlags(unsigned passIndex, const ProgramFlags& flags);
        void DisableProgramFlags(unsigned passIndex, const ProgramFlags& flags);
        PMaterial Clone(const std::string& name = GetUniqueName("Clone"));
        void SetName(const std::string& name) {name_ = name;}
        const std::string& GetName() const { return name_;  }
		bool SetTexture(size_t index, PTexture texture);
        void SetDiffuseMap(PTexture texture);
        void SetNormalMap(PTexture texture);
        void SetLightMap(PTexture texture);
        void SetSpecularMap(PTexture texture);
        void SetAOMap(PTexture texture);
        void SetDisplacementMap(PTexture texture);
		void SetTextMap(PTexture texture);
		PTexture GetTexture(size_t index) const 
		{ 
			CHECK_ASSERT(index >= 0 && index < MaterialTexture::MAX_TEXTURES_MAPS, __FILE__, __LINE__); 
			return texture_[index]; 
		}
		PTexture GetDiffuseMap() const { return texture_[MaterialTexture::DIFFUSE_MAP]; }
		PTexture GetNormalMap() const { return texture_[MaterialTexture::NORMAL_MAP]; }
		PTexture GetLightMap() const { return texture_[MaterialTexture::LIGHT_MAP]; }
		PTexture GetSpecularMap() const { return texture_[MaterialTexture::SPECULAR_MAP]; }
		PTexture GetAOMap() const { return texture_[MaterialTexture::AO_MAP]; }
		PTexture GetDisplacementMap() const { return texture_[MaterialTexture::DISPLACEMENT_MAP]; }
        void SetColor(Color color);
        Color GetColor() const { return color_; }
        void SetDiffuseColor(Color diffuse);
        Color GetDiffuseColor() const { return diffuse_; }
        void SetSpecularColor(Color specular);
        Color GetSpecularColor() const { return specular_; }
        void SetAmbientColor(Color ambient);
        Color GetAmbientColor() const { return ambient_; }
        void SetShininess(float shininess);
        float GetShininess() const { return shininess_; }
        void SetParallaxScale(float parallaxScale);
        float GetParallaxScale() const { return parallaxScale_; }
        void SetUniformValue(const char* name, int value);
        int GetUniformValue(const char* name) const;
        PTechnique GetTechnique() { return technique_; }
        void Save(pugi::xml_node& node);
        void SetSerializable(bool serializable) { serializable_ = serializable; }
        bool IsSerializable() const { return serializable_; }
        void SetFilterBlendMode(BlendFilterMode mode);
        BlendFilterMode GetFilterBlendMode() const { return blendFilterMode_; }
        void SetFilterBlur(const BlurFilter& data);
        void SetFilterWave(const WaveFilter& data);
        const BlurFilter& GetFilterBlur() const { return blurFilter_; }
        const WaveFilter& GetWaveFilter() const { return waveFilter_; }
        PTexture GetTextureWith(PResource resource) const;
		PInstanceBuffer GetInstanceBuffer() const { return instanceBuffer_; }
		bool IsTransparent() const;
		void SetSolid(bool solid);
		bool IsBatched();
		void UpdateBatchBuffer(const Batch& batch);
		void BachedNodeHasChanged();
		bool IsText() const;
		static PMaterial Create(const std::string& name = GetUniqueName("Material"), const ProgramFlags& flags = (int)ProgramFlag::NONE);
		static PMaterial GetOrCreate(const std::string& name = GetUniqueName("Material"), const ProgramFlags& flags = (int)ProgramFlag::NONE);
		static PMaterial Get(const std::string& name);
		static std::vector<PMaterial> GetMaterials();
		static PTexture GetTextureWithResource(PResource resource);
		static std::vector<PMaterial> LoadMaterials(PResource resource, const pugi::xml_node& node);
		static void SaveMaterials(pugi::xml_node& node);
        void Set(PResourceXMLNode xmlResource);
    private:
		void Load(PResource resource, const pugi::xml_node& node) override;
		void SetupBlur();
        bool IsValid() override;
		void AllocateResources() override;
		void ReleaseResources() override;
        PTexture texture_[MaterialTexture::MAX_TEXTURES_MAPS];
        Color ambient_;
        Color diffuse_;
        Color specular_;
        float shininess_;
        float parallaxScale_; //used with displacement map
        Color color_;
        PTechnique technique_;
        bool serializable_;
        BlendFilterMode blendFilterMode_;
        BlurFilter blurFilter_;
        WaveFilter waveFilter_;
		PInstanceBuffer instanceBuffer_;
		Batch lastBatch_;
		bool isBatched_;
        PResourceXMLNode xmlResource_;
		static MapAndVector<std::string, Material> materials_;
        friend class Program;
    };
}