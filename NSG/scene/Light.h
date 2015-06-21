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
#include "SceneNode.h"

namespace NSG
{	
	class Light : public SceneNode
	{
	public:
		Light(const std::string& name);
		~Light();
		void SetEnergy(float energy);
		void SetColor(Color color);
		void EnableDiffuseColor(bool enable);
		void EnableSpecularColor(bool enable);
		void SetSpotCutOff(float spotCutOff); // angle in degrees
		float GetSpotCutOff() const { return spotCutOff_; }
		LightType GetType() const { return type_; }
		void SetType(LightType type);
		void Save(pugi::xml_node& node) const override;
		void Load(const pugi::xml_node& node) override;
		void FillShaderDefines(std::string& defines, PassType passType, Material* material) const;
		static SignalLight::PSignal SignalBeingDestroy();
		const Color& GetDiffuseColor() const { return diffuseColor_; }
		const Color& GetSpecularColor() const { return specularColor_; }
		void SetShadowColor(Color color);
		const Color& GetShadowColor() const { return shadowColor_; }
		void SetDistance(float distance);
		float GetDistance() const { return distance_; }
		float GetRange() const { return range_; }
		void EnableShadows(bool enable) { shadows_ = enable; }
		bool DoShadows() const;
		PTexture GetShadowMap(int idx) const;
		float GetShadowClipStart() const { return shadowClipStart_; }
		float GetShadowClipEnd() const { return shadowClipEnd_; }
		void SetShadowClipStart(float value);
		void SetShadowClipEnd(float value);
		void SetOnlyShadow(bool onlyShadow) { onlyShadow_ = onlyShadow; }
		bool GetOnlyShadow() const { return onlyShadow_; }
		void SetBias(float shadowBias) { shadowBias_ = shadowBias; }
		float GetBias() const { return shadowBias_; }
		ShadowCamera* GetShadowCamera(int idx) const;
		void GenerateShadowMaps(const Camera* camera);
		bool HasSpecularColor() const;
	private:
		int CalculateSplits(const Camera* camera, float splits[MAX_SHADOW_SPLITS]) const;
		FrameBuffer* GetShadowFrameBuffer(int idx) const;
		void CalculateColor();
		void CalculateRange();
		void Generate2DShadowMap(int split);
		void GenerateShadowMapCubeFace(int split);
		int GetShadowFrameBufferSize(int split) const;
		void GenerateCubeShadowMap(int split, const Camera* camera);

		LightType type_;
		float energy_;
		Color color_;
		bool diffuse_;
		bool specular_;
        float spotCutOff_; // angle in degrees
        Color diffuseColor_; // calculated
        Color specularColor_; // calculated
        Color shadowColor_;
        float distance_;
        float range_; // calculated
        bool shadows_;
        float shadowClipStart_;
        float shadowClipEnd_;
        bool onlyShadow_;
        PFrameBuffer shadowFrameBuffer_[MAX_SHADOW_SPLITS];
        float shadowBias_; // Bias is used to add a slight offset distance between an object and the shadows cast by it.
		PShadowCamera shadowCamera_[MAX_SHADOW_SPLITS];
	};
}