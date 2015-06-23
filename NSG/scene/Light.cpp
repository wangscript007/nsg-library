#include "Light.h"
#include "Log.h"
#include "Check.h"
#include "Constants.h"
#include "Resource.h"
#include "Material.h"
#include "Util.h"
#include "Scene.h"
#include "Frustum.h"
#include "Renderer.h"
#include "Window.h"
#include "Graphics.h"
#include "Sphere.h"
#include "Texture.h"
#include "ShadowCamera.h"
#include "FrameBuffer.h"
#include "pugixml.hpp"
#include <assert.h>
#include <algorithm>
#include <sstream>

namespace NSG
{
    Light::Light(const std::string& name)
        : SceneNode(name),
          range_(-1),
          type_(LightType::POINT),
          energy_(1),
          color_(1),
          diffuse_(true),
          specular_(true),
          spotCutOff_(45),
          diffuseColor_(1),
          specularColor_(1),
          shadowColor_(0, 0, 0, 1),
          distance_(30),
          shadows_(true),
          shadowClipStart_(0.1f), // same minimum as blender
          shadowClipEnd_(30.f), // same as distance_
          onlyShadow_(false),
          shadowBias_(0.005f),
          shadowSplits_(1)
    {
        FrameBuffer::Flags flags((unsigned int)(FrameBuffer::COLOR | FrameBuffer::COLOR_USE_TEXTURE | FrameBuffer::COLOR_CUBE_TEXTURE | FrameBuffer::DEPTH));
        for (int i = 0; i < MAX_SHADOW_SPLITS; i++)
        {
            shadowCamera_[i] = std::make_shared<ShadowCamera>(this);
            shadowFrameBuffer_[i] = std::make_shared<FrameBuffer>(GetUniqueName("LightCubeFrameBuffer"), flags);
            CHECK_ASSERT(TextureWrapMode::CLAMP_TO_EDGE == GetShadowMap(i)->GetWrapMode(), __FILE__, __LINE__);
            shadowFrameBuffer_[i]->EnableAutoSize(false);
        }

        CalculateRange();
    }

    Light::~Light()
    {
        if (Graphics::this_)
            SignalBeingDestroy()->Run(this);
    }

    void Light::SetEnergy(float energy)
    {
        if (energy_ != energy)
        {
            energy_ = energy;
            CalculateColor();
            SetUniformsNeedUpdate();
        }
    }

    void Light::SetColor(Color color)
    {
        if (color_ != color)
        {
            color_ = color;
            CalculateColor();
            SetUniformsNeedUpdate();
        }
    }

    void Light::SetShadowColor(Color color)
    {
        if (shadowColor_ != color)
        {
            shadowColor_ = color;
            SetUniformsNeedUpdate();
        }
    }

    void Light::EnableDiffuseColor(bool enable)
    {
        if (diffuse_ != enable)
        {
            diffuse_ = enable;
            CalculateColor();
            SetUniformsNeedUpdate();
        }
    }

    void Light::EnableSpecularColor(bool enable)
    {
        if (specular_ != enable)
        {
            specular_ = enable;
            CalculateColor();
            SetUniformsNeedUpdate();
        }
    }

    void Light::SetSpotCutOff(float spotCutOff)
    {
        if (spotCutOff_ != spotCutOff)
        {
            spotCutOff_ = spotCutOff;
            SetUniformsNeedUpdate();
        }
    }

    void Light::SetType(LightType type)
    {
        if (type_ != type)
        {
            for (int i = 0; i < MAX_SHADOW_SPLITS; i++)
            {
                if (type == LightType::POINT)
                {
                    FrameBuffer::Flags flags((unsigned int)(FrameBuffer::COLOR | FrameBuffer::COLOR_USE_TEXTURE | FrameBuffer::COLOR_CUBE_TEXTURE | FrameBuffer::DEPTH));
                    shadowFrameBuffer_[i] = std::make_shared<FrameBuffer>(GetUniqueName("LightCubeFrameBuffer"), flags);
                }
                else if (type_ == LightType::POINT)
                {
                    FrameBuffer::Flags flags((unsigned int)(FrameBuffer::COLOR | FrameBuffer::COLOR_USE_TEXTURE | FrameBuffer::DEPTH));
                    shadowFrameBuffer_[i] = std::make_shared<FrameBuffer>(GetUniqueName("Light2DFrameBuffer"), flags);
                }

                //GetShadowMap(i)->SetWrapMode(TextureWrapMode::REPEAT);
                CHECK_ASSERT(TextureWrapMode::CLAMP_TO_EDGE == GetShadowMap(i)->GetWrapMode(), __FILE__, __LINE__);
            }

            type_ = type;
            CalculateRange();
            OnDirty();
            SetUniformsNeedUpdate();
        }
    }

    void Light::Save(pugi::xml_node& node) const
    {
        node.append_attribute("name").set_value(GetName().c_str());
        node.append_attribute("nodeType").set_value("Light");
        node.append_attribute("type").set_value(ToString(type_));
        node.append_attribute("energy").set_value(energy_);
        node.append_attribute("color").set_value(ToString(color_).c_str());
        node.append_attribute("diffuse").set_value(diffuse_);
        node.append_attribute("specular").set_value(specular_);
        node.append_attribute("spotCutOff").set_value(spotCutOff_);
        node.append_attribute("position").set_value(ToString(GetPosition()).c_str());
        node.append_attribute("orientation").set_value(ToString(GetOrientation()).c_str());
        node.append_attribute("distance").set_value(distance_);
        node.append_attribute("shadows").set_value(shadows_);
        node.append_attribute("shadowClipStart").set_value(shadowClipStart_);
        node.append_attribute("shadowClipEnd").set_value(shadowClipEnd_);
        node.append_attribute("onlyShadow").set_value(onlyShadow_);
        node.append_attribute("shadowColor").set_value(ToString(shadowColor_).c_str());
        node.append_attribute("shadowBias").set_value(shadowBias_);


        SaveChildren(node);
    }

    void Light::Load(const pugi::xml_node& node)
    {
        name_ = node.attribute("name").as_string();
        SetType(ToLightType(node.attribute("type").as_string()));
        SetEnergy(node.attribute("energy").as_float());
        SetColor(ToVertex4(node.attribute("color").as_string()));
        EnableDiffuseColor(node.attribute("diffuse").as_bool());
        EnableSpecularColor(node.attribute("specular").as_bool());
        SetSpotCutOff(node.attribute("spotCutOff").as_float());
        Vertex3 position = ToVertex3(node.attribute("position").as_string());
        SetPosition(position);
        Quaternion orientation = ToQuaternion(node.attribute("orientation").as_string());
        SetOrientation(orientation);
        SetDistance(node.attribute("distance").as_float());
        EnableShadows(node.attribute("shadows").as_bool());
        SetShadowClipStart(node.attribute("shadowClipStart").as_float());
        SetShadowClipEnd(node.attribute("shadowClipEnd").as_float());
        SetOnlyShadow(node.attribute("onlyShadow").as_bool());
        SetShadowColor(ToVertex4(node.attribute("shadowColor").as_string()));
        SetBias(node.attribute("shadowBias").as_float());
        LoadChildren(node);
    }

    void Light::FillShaderDefines(std::string& defines, PassType passType, Material* material) const
    {
        bool litPass =  PassType::LIT == passType;
        if (litPass)
        {
            if (LightType::POINT == type_)
                defines += "HAS_POINT_LIGHT\n";
            else if (LightType::DIRECTIONAL == type_)
                defines += "HAS_DIRECTIONAL_LIGHT\n";
            else
            {
                defines += "HAS_SPOT_LIGHT\n";
            }

            if (DoShadows() && material->ReceiveShadows())
            {
                if (LightType::POINT == type_)
                    defines += "CUBESHADOWMAP\n";
                else
                    defines += "SHADOWMAP\n";
            }
        }
    }

    SignalLight::PSignal Light::SignalBeingDestroy()
    {
        static SignalLight::PSignal sig(new SignalLight);
        return sig;
    }

    void Light::CalculateColor()
    {
        diffuseColor_ = diffuse_ ? color_ * energy_ : COLOR_BLACK;
        specularColor_ = specular_ ? color_ * energy_ : COLOR_BLACK;
    }

    bool Light::HasSpecularColor() const
    {
        return specularColor_.a && (specularColor_.r || specularColor_.g || specularColor_.b);
    }

    void Light::SetDistance(float distance)
    {
        if(distance_ != distance)
        {
            distance_ = distance;
            CalculateRange();
            SetUniformsNeedUpdate();
        }
    }

    bool Light::DoShadows() const
    {
        return shadows_;// && Graphics::this_->GetWindow()->GetPixelFormat() != PixelFormat::RGB565;
    }

    FrameBuffer* Light::GetShadowFrameBuffer(int idx) const
    {
        CHECK_ASSERT(idx < MAX_SHADOW_SPLITS, __FILE__, __LINE__);
        return shadowFrameBuffer_[idx].get();
    }

    PTexture Light::GetShadowMap(int idx) const
    {
        CHECK_ASSERT(idx < MAX_SHADOW_SPLITS, __FILE__, __LINE__);
        return shadowFrameBuffer_[idx]->GetColorTexture();
    }

    void Light::CalculateRange()
    {
        float range = range_;
        if (LightType::SPOT == type_)
            range = glm::clamp((shadowClipEnd_ - shadowClipStart_), 0.f, distance_);
        else if (LightType::POINT == type_)
            range = std::max(distance_, glm::epsilon<float>());
        //For directional the range is calculated when we setup the shadow camera.
        //See ShadowCamera::SetupDirectional
        if(range != range_)
        {
            range_ = range;
            SetUniformsNeedUpdate();
        }
    }

    void Light::SetRange(float range)
    {
        CHECK_ASSERT(LightType::DIRECTIONAL == type_ && "SetRange only must be used from ShadowCamera::SetupDirectional!!!", __FILE__, __LINE__);
        if (range_ != range)
        {
            range_ = range;
            SetUniformsNeedUpdate();
        }
    }

    float Light::GetRange() const
    {
        CHECK_ASSERT(range_ != -1 && "Light or ShadowCamera range is incorrect!!!", __FILE__, __LINE__);
        return range_;
    }

    void Light::SetShadowClipStart(float value)
    {
        if(shadowClipStart_ != value)
        {
            shadowClipStart_ = value;
            CalculateRange();
        }
    }

    void Light::SetShadowClipEnd(float value)
    {
        if(shadowClipEnd_ != value)
        {
            shadowClipEnd_ = value;
            CalculateRange();
        }
    }

    ShadowCamera* Light::GetShadowCamera(int idx) const
    {
        CHECK_ASSERT(idx < MAX_SHADOW_SPLITS, __FILE__, __LINE__);
        return shadowCamera_[idx].get();
    }

    void Light::Generate2DShadowMap(int split)
    {
        auto frameBuffer = Graphics::this_->GetFrameBuffer();
        auto shadowFrameBuffer = GetShadowFrameBuffer(split);
        auto splitMapsize = GetShadowFrameBufferSize(split);
        shadowFrameBuffer->SetSize(splitMapsize, splitMapsize);
        if (shadowFrameBuffer->IsReady())
        {
            auto shadowCamera = GetShadowCamera(split);
            std::vector<SceneNode*> shadowCasters;
            shadowCamera->GetVisiblesShadowCasters(shadowCasters);
            Graphics::this_->SetFrameBuffer(shadowFrameBuffer);
            auto lastCamera = Graphics::this_->SetCamera(shadowCamera);
            Graphics::this_->ClearBuffers(true, true, false);
            std::vector<PBatch> batches;
            Renderer::GetPtr()->GenerateBatches(shadowCasters, batches);
            for (auto& batch : batches)
                if (batch->GetMaterial()->CastShadow())
                    Renderer::GetPtr()->DrawShadowPass(batch.get(), this);
            Graphics::this_->SetCamera(lastCamera);
            Graphics::this_->SetFrameBuffer(frameBuffer);
        }
    }

    void Light::GenerateShadowMapCubeFace(int split)
    {
        auto shadowCamera = GetShadowCamera(split);
        auto lastCamera = Graphics::this_->SetCamera(shadowCamera);
        std::vector<SceneNode*> shadowCasters;
        shadowCamera->GetVisiblesShadowCasters(shadowCasters);
        std::vector<PBatch> batches;
        auto renderer = Renderer::GetPtr();
        renderer->GenerateBatches(shadowCasters, batches);
        Graphics::this_->ClearBuffers(true, true, false);
        for (auto& batch : batches)
            if (batch->GetMaterial()->CastShadow())
                renderer->DrawShadowPass(batch.get(), this);
        Graphics::this_->SetCamera(lastCamera);
    }

    int Light::GetShadowFrameBufferSize(int split) const
    {
        CHECK_ASSERT(split < MAX_SHADOW_SPLITS, __FILE__, __LINE__);
        static const int SplitMapSize[MAX_SHADOW_SPLITS] = { 1024, 512, 256, 128 };
        //static const int SplitMapSize[MAX_SHADOW_SPLITS] = { 1024, 1024, 1024, 1024 };
        return SplitMapSize[split];
    }

    void Light::GenerateCubeShadowMap(int split, const Camera* camera)
    {
        auto frameBuffer = Graphics::this_->GetFrameBuffer();
        auto shadowFrameBuffer = GetShadowFrameBuffer(split);
        auto splitMapsize = GetShadowFrameBufferSize(split);
        shadowFrameBuffer->SetSize(splitMapsize, splitMapsize);
        if (shadowFrameBuffer->IsReady())
        {
            auto shadowCamera = GetShadowCamera(split);
            for (unsigned i = 0; i < (unsigned)CubeMapFace::MAX_CUBEMAP_FACES; i++)
            {
                TextureTarget face = (TextureTarget)(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);
                shadowCamera->SetCurrentCubeShadowMapFace(face);
                auto camFrustum = camera->GetFrustum();
                auto intersection = camFrustum->IsPointInside(GetGlobalPosition());
                bool genShadowMap = Intersection::OUTSIDE != intersection ||
                                    Intersection::OUTSIDE != camFrustum->IsInside(BoundingBox(*shadowCamera->GetFrustum()));
                if (genShadowMap)
                {
                    Graphics::this_->SetFrameBuffer(shadowFrameBuffer, face);
                    GenerateShadowMapCubeFace(split);
                }
            }
            Graphics::this_->SetFrameBuffer(frameBuffer);
        }
    }

    int Light::CalculateSplits(const Camera* camera, float splits[MAX_SHADOW_SPLITS], const BoundingBox& camFrustumViewBox, const BoundingBox& receiversViewBox) const
    {
        auto camNear = camera->GetZNear();
        auto camFar  = camera->GetZFar();
        auto frustumDepth = camFar - camNear;
        float shadowSplitLogFactor = camera->GetShadowSplitLogFactor();
        int nSplits = camera->GetMaxShadowSplits();

        if (camera->AutomaticSplits())
        {
            auto viewBox(receiversViewBox);
            viewBox.Transform(camera->GetView());
            viewBox.Clip(camFrustumViewBox);
            auto receiversDepth = viewBox.Size().z;
            auto frustumVisibilityFactor = receiversDepth / frustumDepth;
            frustumVisibilityFactor = glm::clamp(frustumVisibilityFactor, 0.f, 1.f);
            nSplits = (int)round(frustumVisibilityFactor * nSplits);
        }

        nSplits = glm::clamp(nSplits, 1, MAX_SHADOW_SPLITS);
        float zDistance = camFar - camNear;
        for (int i = 0; i < nSplits - 1; i++)
        {
            float factor = (i + 1.f) / (float)nSplits;
            splits[i] = Lerp(camNear + factor * zDistance,
                             camNear * powf(camFar / camNear, factor),
                             shadowSplitLogFactor);
        }
        splits[nSplits - 1] = camFar;
        return nSplits;
    }

    void Light::GenerateShadowMaps(const Camera* camera)
    {
        switch (type_)
        {
            case LightType::POINT:
                {
                    std::vector<SceneNode*> shadowCasters;
                    auto shadowCamera = shadowCamera_[0].get();
                    shadowCamera->SetupPoint(camera);
                    GenerateCubeShadowMap(0, camera);
                    shadowSplits_ = 1;
                }
                break;

            case LightType::SPOT:
                {
                    auto shadowCamera = shadowCamera_[0].get();
                    shadowCamera->SetupSpot(camera);
                    auto intersection = camera->GetFrustum()->IsPointInside(GetGlobalPosition());
                    if (Intersection::OUTSIDE != intersection)
                        Generate2DShadowMap(0);
                    else if (Intersection::OUTSIDE != camera->GetFrustum()->IsInside(BoundingBox(*shadowCamera->GetFrustum())))
                        Generate2DShadowMap(0);
                    shadowSplits_ = 1;
                }
                break;

            default: //DIRECTIONAL
                {
                    auto farZ = camera->GetZFar();
                    auto nearSplit = camera->GetZNear();
                    auto camFrustum = camera->GetFrustum();
                    BoundingBox camFullFrustumViewBox(*camFrustum);
                    auto receiversViewBox = Camera::GetViewBox(camFrustum.get(), GetScene().get(), true, false);
                    float splits[MAX_SHADOW_SPLITS];
                    shadowSplits_ = CalculateSplits(camera, splits, camFullFrustumViewBox, receiversViewBox);
                    auto farSplit = farZ;
                    int split = 0;
                    receiversViewBox.Clip(camFullFrustumViewBox);
                    // Setup the sahdow camera for each split
                    while (split < shadowSplits_)
                    {
                        if (nearSplit > farZ)
                            break;
                        farSplit = std::min(farZ, splits[split]);
                        auto shadowCamera = shadowCamera_[split].get();
                        shadowCamera->SetupDirectional(split, camera, nearSplit, farSplit, receiversViewBox);
                        nearSplit = farSplit;
                        ++split;
                    }
                    for (int i = 0; i < shadowSplits_; i++)
                        Generate2DShadowMap(i);
                }
                break;
        }
    }
}