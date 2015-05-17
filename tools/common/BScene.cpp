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
#include "BScene.h"
#include "BlenderDefines.h"
#include "UtilConverter.h"
#include "pugixml.hpp"
#include "bMain.h"
#include "Blender.h"
#include "bBlenderFile.h"
#include "ResourceConverter.h"
#include "NSG.h"
#include <cstdio>

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

namespace BlenderConverter
{
    using namespace NSG;
    BScene::BScene(const Path& path, const Path& outputDir, bool embedResources)
        : path_(path),
          outputDir_(outputDir),
          embedResources_(embedResources)
    {
    }

    BScene::~BScene()
    {
    }

    void BScene::Load()
    {
        bParse::bBlenderFile obj(path_.GetFullAbsoluteFilePath().c_str());
        obj.parse(false);
        bParse::bMain* data = obj.getMain();
        auto materials = LoadMaterials(data);
        sounds_ = LoadSounds(data);
        CreateScenes(data);
        CreateAnimations(data);
    }

    void BScene::CreateScenes(bParse::bMain* data)
    {
        bParse::bListBasePtr* it = data->getScene();
        auto n = it->size();
        for (int i = 0; i < n; i++)
        {
            armatureLinker_.clear();
            const Blender::Scene* bscene = (const Blender::Scene*)it->at(i);
            auto scene = CreateScene(bscene);
            scenes_.push_back(scene);
            bscenes_.push_back(bscene);
            const Blender::Base* base = (const Blender::Base*)bscene->base.first;
            while (base)
            {
                const Blender::Object* obj = base->object;
				ConvertObject(obj, scene, bscene);
                base = base->next;
            }
            for (auto& objArmature : armatureLinker_)
                CreateSkeleton(scene, objArmature);
        }
    }

    PScene BScene::CreateScene(const Blender::Scene* bscene)
    {
        auto scene = std::make_shared<Scene>(B_IDNAME(bscene));
        scene->SetOrientation(glm::angleAxis<float>(-PI / 2.f, Vertex3(1, 0, 0)));
        const Blender::World* world = bscene->world;
        scene->GetPhysicsWorld()->SetGravity(Vector3(0, -world->gravity, 0));
        Color ambient(world->ambr, world->ambg, world->ambb, 1);
        scene->SetAmbientColor(ambient);
        return scene;
    }

    std::vector<NSG::PSound> BScene::LoadSounds(bParse::bMain* data)
    {
        std::vector<PSound> result;
        bParse::bListBasePtr* it = data->getSound();
        int n = it->size();
        for (int i = 0; i < n; i++)
        {
            auto sound = (Blender::bSound*)it->at(i);
            result.push_back(LoadSound(sound));
        }
        return result;
    }

	NSG::PSound BScene::LoadSound(Blender::bSound* sound)
    {
		std::string soundName = B_IDNAME(sound);
		if (!sound->packedfile)
		{
			Path path;
			path.SetPath(path_.GetPath());
			path.SetFileName(sound->name);
			auto resource = Resource::GetOrCreate<ResourceFile>(path.GetFilePath());
			auto sound = Sound::Create(soundName);
			sound->Set(resource);
			return sound;
		}
		else
		{
			auto resource = Resource::GetOrCreate<ResourceConverter>(sound->name);
			resource->SetData((const char*)sound->packedfile->data, sound->packedfile->size);
			auto sound = Sound::Create(soundName);
			sound->Set(resource);
			return sound;
		}
    }

    std::vector<NSG::PMaterial> BScene::LoadMaterials(bParse::bMain* data)
    {
        std::vector<PMaterial> result;
        bParse::bListBasePtr* it = data->getMat();
        int n = it->size();
        for (int i = 0; i < n; i++)
        {
            auto material = (Blender::Material*)it->at(i);
            result.push_back(LoadMaterial(material));
        }
        return result;
    }

    PTexture BScene::CreateTexture(const Blender::Image* ima)
    {
        std::string imageName = B_IDNAME(ima);
        if (!ima->packedfile)
        {
            Path path;
            path.SetPath(path_.GetPath());
            path.SetFileName(imageName);
            auto resource = Resource::GetOrCreate<ResourceFile>(path.GetFilePath());
            return std::make_shared<Texture>(resource, (int)TextureFlag::GENERATE_MIPMAPS | (int)TextureFlag::INVERT_Y);
        }
        else
        {
            auto resource = Resource::GetOrCreate<ResourceConverter>(imageName);
            resource->SetData((const char*)ima->packedfile->data, ima->packedfile->size);
            return std::make_shared<Texture>(resource, (int)TextureFlag::GENERATE_MIPMAPS | (int)TextureFlag::INVERT_Y);
        }
    }

    PMaterial BScene::LoadMaterial(const Blender::Material* mt)
    {
        std::string name = B_IDNAME(mt);
        auto material = Material::GetOrCreate(name);
        auto diffuseIntensity = mt->ref;
        material->SetDiffuseColor(Color(diffuseIntensity * mt->r, diffuseIntensity * mt->g, diffuseIntensity * mt->b, mt->alpha));
        auto specularIntensity = mt->spec;
        material->SetSpecularColor(Color(specularIntensity * mt->specr, specularIntensity * mt->specg, specularIntensity * mt->specb, mt->alpha));
        material->SetAmbientColor(Color(mt->ambr, mt->ambg, mt->ambb, mt->alpha));
        
        material->SetShininess(mt->har);

        if (mt->mode & MA_WIRE)
            material->SetFillMode(FillMode::WIREFRAME);
        else
            material->SetFillMode(FillMode::SOLID);

		bool shadeless = mt->mode & MA_SHLESS ? true : false;
		material->SetShadeless(shadeless);

        if (mt->game.alpha_blend & GEMAT_ALPHA)
            material->SetBlendMode(BLEND_MODE::ALPHA);
        else
            material->SetBlendMode(BLEND_MODE::NONE);

        if (mt->game.flag & GEMAT_BACKCULL)
            material->SetCullFaceMode(CullFaceMode::BACK);
        else if (mt->game.flag & GEMAT_INVISIBLE)
            material->SetCullFaceMode(CullFaceMode::FRONT_AND_BACK);
        else 
            material->SetCullFaceMode(CullFaceMode::DISABLED);

        if (mt->game.flag & GEMAT_TEXT)
            material->SetRenderPass(RenderPass::TEXT);
        else
            material->SetRenderPass(RenderPass::PERPIXEL);

        if(mt->game.face_orientation & GEMAT_HALO)
            material->SetBillboardType(BillboardType::SPHERICAL);
        else if(mt->game.face_orientation & GEMAT_BILLBOARD)
            material->SetBillboardType(BillboardType::CYLINDRICAL);
        else 
            material->SetBillboardType(BillboardType::NONE);

        // textures
        if (mt->mtex != 0)
        {
            for (int i = 0; i < MAX_MTEX; i++)
            {
                if (!mt->mtex[i] || !mt->mtex[i]->tex)
                    continue;

                if (mt->mtex[i]->tex->type == TEX_IMAGE)
                {
                    const Blender::MTex* mtex = mt->mtex[i];
                    const Blender::Image* ima = mtex->tex->ima;
                    if (!ima) continue;
                    auto texture = CreateTexture(ima);
					if (mtex->uvname)
						texture->SetUVName(mtex->uvname);

					switch (mtex->blendtype)
					{
                        case MTEX_BLEND:
							texture->SetBlendType(TextureBlend::MIX);
                            break;
                        case MTEX_MUL:
							texture->SetBlendType(TextureBlend::MUL);
                            break;
                        case MTEX_ADD:
							texture->SetBlendType(TextureBlend::ADD);
                            break;
                        case MTEX_SUB:
							texture->SetBlendType(TextureBlend::SUB);
                            break;
                        case MTEX_DIV:
							texture->SetBlendType(TextureBlend::DIV);
                            break;
                        case MTEX_DARK:
							texture->SetBlendType(TextureBlend::DARK);
                            break;
                        case MTEX_DIFF:
							texture->SetBlendType(TextureBlend::DIFF);
                            break;
                        case MTEX_LIGHT:
							texture->SetBlendType(TextureBlend::LIGHT);
                            break;
                        case MTEX_SCREEN:
							texture->SetBlendType(TextureBlend::SCREEN);
                            break;
                        case MTEX_OVERLAY:
							texture->SetBlendType(TextureBlend::OVERLAY);
                            break;
                        case MTEX_BLEND_HUE:
							texture->SetBlendType(TextureBlend::BLEND_HUE);
                            break;
                        case MTEX_BLEND_SAT:
							texture->SetBlendType(TextureBlend::BLEND_SAT);
                            break;
                        case MTEX_BLEND_VAL:
							texture->SetBlendType(TextureBlend::BLEND_VAL);
                            break;
                        case MTEX_BLEND_COLOR:
							texture->SetBlendType(TextureBlend::BLEND_COLOR);
                            break;
                        default:
                            break;
					}

					if ((mtex->mapto & MAP_EMIT) || (mtex->maptoneg & MAP_EMIT))
					{
						texture->SetMapType(TextureType::EMIT);
						material->SetTexture(texture);
					}
					else if ((mtex->mapto & MAP_NORM) || (mtex->maptoneg & MAP_NORM))
					{
						texture->SetMapType(TextureType::NORM);
						material->SetTexture(texture);
					}
					else if ((mtex->mapto & MAP_SPEC) || (mtex->maptoneg & MAP_SPEC))
					{
						texture->SetMapType(TextureType::SPEC);
						material->SetTexture(texture);
					}
					else if ((mtex->mapto & MAP_AMB) || (mtex->maptoneg & MAP_AMB))
					{
						texture->SetMapType(TextureType::AMB);
						material->SetTexture(texture);
					}
					else if ((mtex->mapto & MAP_COL) || (mtex->maptoneg & MAP_COL))
					{
						texture->SetMapType(TextureType::COL);
						material->SetTexture(texture);
					}
                }
            }
        }

        return material;
    }

	void BScene::ConvertObject(const Blender::Object* obj, PScene scene, const Blender::Scene* bscene)
    {
		PSceneNode parent = scene;

        if (obj)
        {
            if (obj->type == OB_MESH && obj->parent != 0 && obj->parent->type == OB_ARMATURE)
                armatureLinker_.push_back(obj);
            
            if (obj->parent)
            {
                auto parentName = B_IDNAME(obj->parent);
				parent = scene->GetChild<SceneNode>(parentName, true);
            }
            
            switch (obj->type)
            {
                case OB_EMPTY:
                    CreateSceneNode(obj, parent);
                    break;
                case OB_LAMP:
                    CreateLight(obj, parent);
                    break;
                case OB_CAMERA:
                    CreateCamera(obj, parent, bscene);
                    break;
                case OB_MESH:
                    CreateMesh(obj, parent);
                    break;
                case OB_ARMATURE:   // SceneNode + Skeleton
                    CreateSkeletonBones(obj, parent);
                    break;
                case OB_CURVE:
                    break;
                default:
                    break;
            }
        }
    }

    void BScene::GetFrames(const Blender::bAction* action, std::vector<float>& fra)
    {
        const Blender::FCurve* fcu = (const Blender::FCurve*)action->curves.first;
        for (; fcu; fcu = fcu->next)
        {
            for (int i = 0; i < fcu->totvert; i++)
            {
                float f = fcu->bezt[i].vec[1][0];
                if (std::find(fra.begin(), fra.end(), f) == fra.end())
                    fra.push_back(f);
            }
        }
        std::sort(fra.begin(), fra.end()); // keep the keys in ascending order
    }

    float BScene::GetTracks(const Blender::bAction* action, float animfps, BTracks& tracks)
    {
        std::string name(B_IDNAME(action));
        float start, end;
        GetActionStartEnd(action, start, end);
        float trackLength = (end - start) / animfps;

        const Blender::FCurve* bfc = (const Blender::FCurve*)action->curves.first;

        while (bfc)
        {
            PTrackData trackData;
            std::string rnap(bfc->rna_path);
            std::string chan_name;
            std::string transform_name;

            // Pose action
            if (rnap.substr(0, 10) == "pose.bones")
            {
                size_t i = rnap.rfind('\"');
                chan_name = rnap.substr(12, i - 12);
                transform_name = rnap.substr(i + 3, rnap.length() - i + 3);
            }
            else
            {
                transform_name = rnap;
                chan_name = "NSGMainObjectChannel";
            }

            auto it = tracks.find(chan_name);
            if (it == tracks.end())
                trackData = tracks[chan_name] = std::make_shared<TrackData>();
            else
                trackData = it->second;

            if (bfc->bezt)
            {
                SPLINE_CHANNEL_CODE code = SPLINE_CHANNEL_CODE::NONE;
                if (transform_name == "rotation_quaternion")
                {
                    if (bfc->array_index == 0) code = SC_ROT_QUAT_W;
                    else if (bfc->array_index == 1) code = SC_ROT_QUAT_X;
                    else if (bfc->array_index == 2) code = SC_ROT_QUAT_Y;
                    else if (bfc->array_index == 3) code = SC_ROT_QUAT_Z;
                }
                else if (transform_name == "rotation_euler")// && obj->rotmode == ROT_MODE_EUL)
                {
                    if (bfc->array_index == 0) code = SC_ROT_EULER_X;
                    else if (bfc->array_index == 1) code = SC_ROT_EULER_Y;
                    else if (bfc->array_index == 2) code = SC_ROT_EULER_Z;
                }
                else if (transform_name == "location")
                {
                    if (bfc->array_index == 0) code = SC_LOC_X;
                    else if (bfc->array_index == 1) code = SC_LOC_Y;
                    else if (bfc->array_index == 2) code = SC_LOC_Z;
                }
                else if (transform_name == "scale")
                {
                    if (bfc->array_index == 0) code = SC_SCL_X;
                    else if (bfc->array_index == 1) code = SC_SCL_Y;
                    else if (bfc->array_index == 2) code = SC_SCL_Z;
                }

                // ignore any other codes
                if (code != -1 && bfc->totvert > 0)
                {
                    auto spline = ConvertSpline(bfc->bezt, code, bfc->bezt->ipo, bfc->totvert, -start, 1.0f / animfps, 0, 1);
                    trackData->keyframes.push_back(spline);
                }
            }

            if (bfc->next == 0 || bfc->next->prev != bfc)
                break;

            bfc = bfc->next;
        }

        return trackLength;
    }

    void BScene::CreateAnimations(bParse::bMain* data)
    {
        CHECK_ASSERT(scenes_.size(), __FILE__, __LINE__);
        CHECK_ASSERT(scenes_.size() == bscenes_.size(), __FILE__, __LINE__);
        const Blender::Scene* firstBScene(bscenes_.at(0));
        PScene firstScene(scenes_.at(0));
        //float animfps = firstBScene->r.frs_sec / firstBScene->r.frs_sec_base;
        bParse::bListBasePtr* it = data->getAction();
        auto n = it->size();
        for (int i = 0; i < n; i++)
        {
            const Blender::bAction* action = (const Blender::bAction*)it->at(i);
            CreateAnimation(action, firstBScene, firstScene);
        }
    }

    void BScene::CreateAnimation(const Blender::bAction* action, const Blender::Scene* bscene, PScene scene)
    {
        float animfps = bscene->r.frs_sec / bscene->r.frs_sec_base;
        std::string name(B_IDNAME(action));
        if (!scene->HasAnimation(name))
        {
            auto anim = scene->GetOrCreateAnimation(name);
            BTracks tracks;
            auto length = GetTracks(action, animfps, tracks);
            ConvertTracks(scene, action, anim, tracks, length);
            anim->SetLength(length);
        }
    }

    void BScene::ConvertTracks(PScene scene, const Blender::bAction* action, PAnimation anim, BTracks& tracks, float length)
    {
        for (auto& btrack : tracks)
        {
            std::string channelName = btrack.first;
            AnimationTrack track;
            if (scene->GetName() == channelName)
                track.node_ = scene;
            else
                track.node_ = scene->GetChild<Node>(channelName, true);

            if (!track.node_.lock())
            {
				TRACE_PRINTF("Warning: skipping animation track %s whose scene node was not found", channelName.c_str());
                continue;
            }

            track.channelMask_ = ConvertChannel(action, btrack.second, track, length);
            anim->AddTrack(track);
        }
    }

    AnimationChannelMask BScene::ConvertChannel(const Blender::bAction* action, PTrackData trackData, AnimationTrack& track, float timeFrameLength)
    {
        AnimationChannelMask mask = 0;
        float inc = timeFrameLength;

        if (trackData->keyframes.size())
            inc = timeFrameLength / trackData->keyframes[0]->getNumVerts();

        float start, end;
        GetActionStartEnd(action, start, end);
        float totalFramesLength = end - start;

        std::vector<float> frames;
        GetFrames(action, frames);
        std::vector<float> framesTime;
        for (auto& f : frames)
        {
            float t = timeFrameLength * (f / totalFramesLength);
            framesTime.push_back(t);
        }
        if (framesTime.size())
            framesTime[0] = 0;
        if (framesTime.size() > 1)
            framesTime[framesTime.size() - 1] = timeFrameLength;

        for (auto t : framesTime)
        {
            float delta = t / timeFrameLength;
            AnimationKeyFrame keyframe;
            keyframe.time_ = t;
            Vector3 pos;
            Quaternion q;
            Vector3 scale(1);
            Vector3 eulerAngles;

            for (auto& spline : trackData->keyframes)
            {
                SPLINE_CHANNEL_CODE code = spline->GetCode();

                switch (code)
                {
                    case SC_ROT_QUAT_W:
                        q.w = spline->interpolate(delta, t);
                        break;
                    case SC_ROT_QUAT_X:
                        q.x = spline->interpolate(delta, t);
                        break;
                    case SC_ROT_QUAT_Y:
                        q.y = spline->interpolate(delta, t);
                        break;
                    case SC_ROT_QUAT_Z:
                        q.z = spline->interpolate(delta, t);
                        break;
                    case SC_ROT_EULER_X:
                        eulerAngles.x = spline->interpolate(delta, t);
                        break;
                    case SC_ROT_EULER_Y:
                        eulerAngles.y = spline->interpolate(delta, t);
                        break;
                    case SC_ROT_EULER_Z:
                        eulerAngles.z = spline->interpolate(delta, t);
                        break;
                    case SC_LOC_X:
                        pos.x = spline->interpolate(delta, t);
                        break;
                    case SC_LOC_Y:
                        pos.y = spline->interpolate(delta, t);
                        break;
                    case SC_LOC_Z:
                        pos.z = spline->interpolate(delta, t);
                        break;
                    case SC_SCL_X:
                        scale.x = spline->interpolate(delta, t);
                        break;
                    case SC_SCL_Y:
                        scale.y = spline->interpolate(delta, t);
                        break;
                    case SC_SCL_Z:
                        scale.z = spline->interpolate(delta, t);
                        break;
                    default:
                        break;
                }
            }

            if (glm::length2(eulerAngles))
            {
                CHECK_ASSERT(q == QUATERNION_IDENTITY, __FILE__, __LINE__);
                q = Quaternion(eulerAngles);
            }
            q = glm::normalize(q);
            scale = glm::normalize(scale);

            Matrix4 transform = glm::translate(glm::mat4(), pos) * glm::mat4_cast(q) * glm::scale(glm::mat4(1.0f), scale);
            Matrix4 totalTransform = track.node_.lock()->GetTransform() * transform;
            DecomposeMatrix(totalTransform, keyframe.position_, keyframe.rotation_, keyframe.scale_);
            track.keyFrames_.push_back(keyframe);
        }

        for (auto& kf : track.keyFrames_)
        {
            if (glm::epsilonNotEqual(kf.position_, VECTOR3_ZERO, Vector3(0.0001f)) != glm::bvec3(false))
                mask |= (int)AnimationChannel::POSITION;

            if (glm::epsilonNotEqual(kf.scale_, VECTOR3_ONE, Vector3(0.0001f)) != glm::bvec3(false))
                mask |= (int)AnimationChannel::POSITION;

            Vector3 angles = glm::eulerAngles(kf.rotation_);
            if (glm::epsilonNotEqual(angles, VECTOR3_ZERO, Vector3(0.001f)) != glm::bvec3(false))
                mask |= (int)AnimationChannel::ROTATION;
        }

        return mask;
    }

    PBSpline BScene::ConvertSpline(const Blender::BezTriple* bez, SPLINE_CHANNEL_CODE access, int mode, int totvert, float xoffset, float xfactor, float yoffset, float yfactor)
    {
        auto spline = std::make_shared<BSpline>(access);

        switch (mode)
        {
            case 0://BEZT_IPO_CONST:
                spline->setInterpolationMethod(BSpline::BEZ_CONSTANT);
                break;
            case 1://BEZT_IPO_LIN:
                spline->setInterpolationMethod(BSpline::BEZ_LINEAR);
                break;
            case 2://BEZT_IPO_BEZ:
                spline->setInterpolationMethod(BSpline::BEZ_CUBIC);
                break;
            default:
                return nullptr;
        }

        const Blender::BezTriple* bezt = bez;
        for (int c = 0; c < totvert; c++, bezt++)
        {
            BezierVertex v;

            v.h1[0] = (bezt->vec[0][0] + xoffset) * xfactor;
            v.h1[1] = (bezt->vec[0][1] + yoffset) * yfactor;
            v.cp[0] = (bezt->vec[1][0] + xoffset) * xfactor;
            v.cp[1] = (bezt->vec[1][1] + yoffset) * yfactor;
            v.h2[0] = (bezt->vec[2][0] + xoffset) * xfactor;
            v.h2[1] = (bezt->vec[2][1] + yoffset) * yfactor;

            spline->addVertex(v);
        }

        return spline;
    }


    void BScene::GetActionStartEnd(const Blender::bAction* action, float& start, float& end)
    {
        start = std::numeric_limits<float>::max();
        end = -start;
        float tstart, tend;
        Blender::FCurve* bfc = (Blender::FCurve*)action->curves.first;
        while (bfc)
        {
            GetSplineStartEnd(bfc->bezt, bfc->totvert, tstart, tend);
            if (start > tstart) start = tstart;
            if (end < tend) end = tend;

            if (bfc->next == 0 || bfc->next->prev != bfc)
                break; //FIX: Momo_WalkBack fcurve is broken in uncompressed 256a.

            bfc = bfc->next;
        }
    }

    void BScene::GetSplineStartEnd(const Blender::BezTriple* bez, int totvert, float& start, float& end)
    {
        start = std::numeric_limits<float>::max();
        end = -start;
        const Blender::BezTriple* bezt = bez;
        for (int c = 0; c < totvert; c++, bezt++)
        {
            if (start > bezt->vec[1][0])
                start = bezt->vec[1][0];
            if (end < bezt->vec[1][0])
                end = bezt->vec[1][0];
        }
    }

    void BScene::ExtractGeneral(const Blender::Object* obj, PSceneNode sceneNode)
    {
        Matrix4 m = ToMatrix(obj->obmat);
        Quaternion q;
        Vector3 pos;
        Vector3 scale;
        DecomposeMatrix(m, pos, q, scale);

        Matrix4 parentinv = ToMatrix(obj->parentinv);
        Quaternion parent_q;
        Vector3 parent_pos;
        Vector3 parent_scale;
        DecomposeMatrix(parentinv, parent_pos, parent_q, parent_scale);
        
        sceneNode->SetPosition(parent_pos + parent_q * (parent_scale * pos));
        sceneNode->SetOrientation(parent_q * q);
        sceneNode->SetScale(parent_scale * scale);
    }

    void BScene::LoadPhysics(const Blender::Object* obj, PSceneNode sceneNode)
    {
        if (obj->body_type == OB_BODY_TYPE_NO_COLLISION)
            return;

        PhysicsShape shapeType = PhysicsShape::SH_UNKNOWN;
        int boundtype = obj->collision_boundtype;

        if (obj->type != OB_MESH)
        {
            if (!(obj->gameflag & OB_ACTOR))
                boundtype = 0;
        }

        if (!(obj->gameflag & OB_BOUNDS))
        {
            if (obj->body_type == OB_BODY_TYPE_STATIC)
            {
                boundtype = OB_BOUND_TRIANGLE_MESH;
            }
            else
                boundtype = OB_BOUND_CONVEX_HULL;
        }

/*        Blender::Object* parent = obj->parent;
        while (parent && parent->parent)
            parent = parent->parent;

        if (parent && (obj->gameflag & OB_CHILD) == 0)
            boundtype = 0;

        if (!boundtype)
            return;
*/
        auto shape = sceneNode->GetMesh()->GetShape();
        auto rigBody = sceneNode->GetOrCreateRigidBody();
        rigBody->SetShape(shape);
        rigBody->SetLinearDamp(obj->damping);
        rigBody->SetAngularDamp(obj->rdamping);
        shape->SetMargin(obj->margin);
        shape->SetScale(sceneNode->GetGlobalScale());

        if(obj->body_type == OB_BODY_TYPE_CHARACTER)
            rigBody->SetKinematic(true);

        if (obj->type == OB_MESH)
        {
            const Blender::Mesh* me = (const Blender::Mesh*)obj->data;
            if (me)
            {
                const Blender::Material* ma = GetMaterial(obj, 0);
                if (ma)
                {
                    rigBody->SetRestitution(ma->reflect);
                    rigBody->SetFriction(ma->friction);
                }
            }
        }

        if (obj->body_type == OB_BODY_TYPE_STATIC || boundtype == OB_BOUND_TRIANGLE_MESH)
            rigBody->SetMass(0);
        else
            rigBody->SetMass(obj->mass);

        switch (boundtype)
        {
            case OB_BOUND_BOX:
                shapeType = PhysicsShape::SH_BOX;
                break;
            case OB_BOUND_SPHERE:
                shapeType = PhysicsShape::SH_SPHERE;
                break;
            case OB_BOUND_CONE:
                shapeType = PhysicsShape::SH_CONE;
                break;
            case OB_BOUND_CYLINDER:
                shapeType = PhysicsShape::SH_CYLINDER;
                break;
            case OB_BOUND_CONVEX_HULL:
                shapeType = PhysicsShape::SH_CONVEX_TRIMESH;
                break;
            case OB_BOUND_TRIANGLE_MESH:
                if (obj->type == OB_MESH)
                    shapeType = PhysicsShape::SH_TRIMESH;
                else
                    shapeType = PhysicsShape::SH_SPHERE;
                break;
            case OB_BOUND_CAPSULE:
                shapeType = PhysicsShape::SH_CAPSULE;
                break;
        }

        shape->SetType(shapeType);
    }


    PSceneNode BScene::CreateSceneNode(const Blender::Object* obj, PSceneNode parent)
    {
        auto sceneNode = parent->CreateChild<SceneNode>(B_IDNAME(obj));
        ExtractGeneral(obj, sceneNode);
        return sceneNode;
    }

    void BScene::CreateSkeletonBones(const Blender::Object* obj, PSceneNode parent)
    {
        auto sceneNode = CreateSceneNode(obj, parent);
        const Blender::bArmature* ar = static_cast<const Blender::bArmature*>(obj->data);
        //CHECK_ASSERT(ar->flag & ARM_RESTPOS && "Armature has to be in rest position. Go to blender and change it.", __FILE__, __LINE__);
        std::string armatureName = B_IDNAME(ar);
        armatureBones_.clear();
        // create bone lists && transforms
        const Blender::Bone* bone = static_cast<const Blender::Bone*>(ar->bonebase.first);
        while (bone)
        {
            if (!bone->parent)
                BuildBoneTree(armatureName, bone, sceneNode);
            bone = bone->next;
        }
    }

    void BScene::BuildBoneTree(const std::string& armatureName, const Blender::Bone* cur, PSceneNode parent)
    {
        auto& list = armatureBones_[armatureName];

        auto it = std::find_if(list.begin(), list.end(), [&](PWeakNode node)
        {
            auto p = node.lock();
            return p == parent;
        });

        if (it == list.end())
            list.push_back(parent);

        Matrix4 parBind = IDENTITY_MATRIX;
        if (cur->parent)
            parBind = glm::inverse(ToMatrix(cur->parent->arm_mat));

        CHECK_ASSERT(!parent->GetChild<SceneNode>(cur->name, false), __FILE__, __LINE__);
        PSceneNode bone = parent->CreateChild<SceneNode>(cur->name);

        Matrix4 bind = parBind * ToMatrix(cur->arm_mat);

        Quaternion rot; Vector3 loc, scl;
        DecomposeMatrix(bind, loc, rot, scl);

        bone->SetPosition(loc);
        bone->SetOrientation(rot);
        bone->SetScale(scl);

        Blender::Bone* chi = static_cast<Blender::Bone*>(cur->childbase.first);
        while (chi)
        {
            BuildBoneTree(armatureName, chi, bone);
            chi = chi->next;
        }
    }

	void BScene::CreateCamera(const Blender::Object* obj, PSceneNode parent, const Blender::Scene* bscene)
    {
        CHECK_ASSERT(obj->data, __FILE__, __LINE__);
        auto camera = parent->CreateChild<Camera>(B_IDNAME(obj));
        ExtractGeneral(obj, camera);
        Blender::Camera* bcamera = static_cast<Blender::Camera*>(obj->data);

		if (bscene->camera == obj)
			camera->GetScene()->SetMainCamera(camera);

        if (bcamera->type == CAM_ORTHO)
            camera->EnableOrtho();

        float fov = 2.f * atan(0.5f * bcamera->sensor_x / bcamera->lens);
        if(bcamera->sensor_fit == '\x02')
        {
            camera->SetSensorFit(CameraSensorFit::VERTICAL);
            fov = 2.f * atan(0.5f * bcamera->sensor_y / bcamera->lens);
        }
        else if(bcamera->sensor_fit == '\x01')
            camera->SetSensorFit(CameraSensorFit::HORIZONTAL);
        else
            camera->SetSensorFit(CameraSensorFit::AUTOMATIC);

        camera->SetOrthoScale(bcamera->ortho_scale);

        camera->SetNearClip(bcamera->clipsta);
        camera->SetFarClip(bcamera->clipend);
        
        camera->SetFOV(glm::degrees(fov));
    }

    void BScene::CreateLight(const Blender::Object* obj, PSceneNode parent)
    {
        CHECK_ASSERT(obj->data, __FILE__, __LINE__);
        auto light = parent->CreateChild<Light>(B_IDNAME(obj));
        ExtractGeneral(obj, light);
        Blender::Lamp* la = static_cast<Blender::Lamp*>(obj->data);
		if (la->mode & LA_NEG)
			light->SetEnergy(-la->energy);
		else 
			light->SetEnergy(la->energy);
        light->SetColor(Color(la->r, la->g, la->b, 1.f));
		light->EnableDiffuseColor(!(la->mode & LA_NO_DIFF));
		light->EnableSpecularColor(!(la->mode & LA_NO_SPEC));
        LightType type = LightType::POINT;
        if (la->type != LA_LOCAL)
            type = la->type == LA_SPOT ? LightType::SPOT : LightType::DIRECTIONAL;
        light->SetType(type);
        float cutoff = glm::degrees<float>(la->spotsize);
        light->SetSpotCutOff(cutoff > 128 ? 128 : cutoff);
        light->SetDistance(la->dist);
        //falloff = 128.f * la->spotblend;
    }

    void BScene::CreateMesh(const Blender::Object* obj, PSceneNode parent)
    {
        CHECK_ASSERT(obj->data, __FILE__, __LINE__);

        auto sceneNode = parent->CreateChild<SceneNode>(B_IDNAME(obj));
        ExtractGeneral(obj, sceneNode);
        const Blender::Mesh* me = (const Blender::Mesh*)obj->data;
        std::string meshName = B_IDNAME(me);
        auto mesh = Mesh::Get<ModelMesh>(B_IDNAME(me));
        if (!mesh)
        {
            mesh = Mesh::GetOrCreate<ModelMesh>(B_IDNAME(me));
            ConvertMesh(obj, me, mesh);
        }
        sceneNode->SetMesh(mesh);
        SetMaterial(obj, sceneNode);
        LoadPhysics(obj, sceneNode);
    }

    const Blender::Object* BScene::GetAssignedArmature(const Blender::Object* obj) const
    {
        Blender::Object* ob_arm = nullptr;

        if (obj->parent && obj->partype == PARSKEL && obj->parent->type == OB_ARMATURE)
            ob_arm = obj->parent;
        else
        {
            const Blender::ModifierData* mod = (const Blender::ModifierData*)obj->modifiers.first;
            for (; mod; mod = mod->next)
                if (mod->type == eModifierType_Armature)
                    ob_arm = ((const Blender::ArmatureModifierData*)mod)->object;
        }
        return ob_arm;
    }

    void BScene::GetBoneIndexByDeformGroupIndex(const Blender::Object* obj, const Blender::Object* obAr, std::vector<std::pair<int, std::string>>& list)
    {
        list.clear();
        int idx = 0;
        const Blender::bDeformGroup* def = (const Blender::bDeformGroup*)obj->defbase.first;
        while (def)
        {
            std::pair<int, std::string> data = { -1, def->name };
            if (IsBoneDefGroup(obAr, def))
                data.first = idx++;
            list.push_back(data);
            def = def->next;
        }
    }

    void BScene::AssignBonesAndWeights(const Blender::Object* obj, const Blender::Mesh* me, VertexsData& vertexes)
    {
        const Blender::Object* obAr = GetAssignedArmature(obj);
        if (me->dvert && obAr)
        {
            std::vector<std::pair<int, std::string>> jointList;
            GetBoneIndexByDeformGroupIndex(obj, obAr, jointList);
            const Blender::bDeformGroup* dg = (const Blender::bDeformGroup*)obj->defbase.first;
            while (dg)
            {
                if (IsBoneDefGroup(obAr, dg))
                {
                    const Blender::MDeformVert* dvert = me->dvert;
                    for (int n = 0; n < me->totvert; n++)
                    {
                        float sumw = 0.0f;
                        std::map<int, float> boneWeightPerVertex;
                        const Blender::MDeformVert& dv = dvert[n];
                        int nWeights = glm::clamp(dv.totweight, 0, (int)MAX_BONES_PER_VERTEX);
                        for (int w = 0; w < nWeights; w++)
                        {
                            const Blender::MDeformWeight& deform = dv.dw[w];
                            if (deform.def_nr >= jointList.size())
                                continue; // it looks like we can have out of bound indexes in blender
                            else if (deform.def_nr >= 0)
                            {
                                int joint_index = jointList[deform.def_nr].first;
                                if (joint_index != -1 && deform.weight > 0.0f)
                                {
                                    if (boneWeightPerVertex.size() == MAX_BONES_PER_VERTEX && boneWeightPerVertex.end() == boneWeightPerVertex.find(joint_index))
                                    {
										TRACE_PRINTF("Warning detected vertex with more than %d bones assigned. New bones will be ignored!!!", MAX_BONES_PER_VERTEX);
                                    }
                                    else
                                    {
                                        boneWeightPerVertex[joint_index] += deform.weight;
                                        sumw += deform.weight;
                                    }
                                }
                            }
                        }
                        if (sumw > 0.0f)
                        {
                            auto invsumw = 1.0f / sumw;
                            int idx = 0;
                            for (auto& bw : boneWeightPerVertex)
                            {
                                auto boneIndex(bw.first);
                                vertexes[n].bonesID_[idx] = boneIndex;
                                vertexes[n].bonesWeight_[idx] = bw.second * invsumw;
                                ++idx;
                                if (idx > MAX_BONES_PER_VERTEX)
                                    break;
                            }
                        }
                    }
                }
                dg = dg->next;
            }
        }
    }

    const void* BScene::FindByString(const Blender::ListBase* listbase, const char* id, int offset) const
    {
        const Blender::Link* link = (const Blender::Link*)listbase->first;
        const char* id_iter;
        while (link)
        {
            id_iter = ((const char*)link) + offset;
            if (id[0] == id_iter[0] && strcmp(id, id_iter) == 0)
                return link;
            link = link->next;
        }
        return nullptr;
    }

    const Blender::bPoseChannel* BScene::GetPoseChannelByName(const Blender::bPose* pose, const char* name) const
    {
        if (!name || (name[0] == '\0'))
            return nullptr;

        return (const Blender::bPoseChannel*)FindByString(&(pose)->chanbase, name, offsetof(Blender::bPoseChannel, name));
    }

    const Blender::Bone* BScene::GetBoneFromDefGroup(const Blender::Object* obj, const Blender::bDeformGroup* def) const
    {
        const Blender::bPoseChannel* pchan = GetPoseChannelByName(obj->pose, def->name);
        return pchan ? pchan->bone : nullptr;
    }

    bool BScene::IsBoneDefGroup(const Blender::Object* obj, const Blender::bDeformGroup* def) const
    {
        return GetBoneFromDefGroup(obj, def) != nullptr;
    }

    void BScene::CreateSkeleton(PScene scene, const Blender::Object* obj)
    {
        const Blender::Object* obAr = GetAssignedArmature(obj);
        CHECK_ASSERT(obAr, __FILE__, __LINE__);
        CHECK_ASSERT(obj->type == OB_MESH, __FILE__, __LINE__);
        const Blender::Mesh* me = (Blender::Mesh*)obj->data;
        auto mesh = Mesh::Get<ModelMesh>(B_IDNAME(me));
        CHECK_ASSERT(mesh, __FILE__, __LINE__);

        const Blender::bArmature* arm = static_cast<const Blender::bArmature*>(obAr->data);
        std::string armatureName = B_IDNAME(arm);
        if (!(arm->flag & ARM_RESTPOS))
        {
			TRACE_PRINTF("!!! Cannot create skeleton: %s. Armature has to be in rest position. Go to blender and change it.", armatureName.c_str());
            return;
        }

        auto skeleton(std::make_shared<Skeleton>(mesh));
        PSceneNode armatureNode = scene->GetChild<SceneNode>(B_IDNAME(obAr), true);

        std::vector<NSG::PWeakNode> boneList;
        std::vector<std::pair<int, std::string>> jointList;
        GetBoneIndexByDeformGroupIndex(obj, obAr, jointList);
        for (auto& joint : jointList)
        {
            if (joint.first != -1)
            {
                auto bone = armatureNode->GetChild<SceneNode>(joint.second, true);
                boneList.push_back(bone);
            }
        }
        skeleton->SetRoot(armatureNode);
        mesh->SetSkeleton(skeleton);
        skeleton->SetBones(boneList);
        CreateOffsetMatrices(obj, armatureNode);
    }

    void BScene::CreateOffsetMatrices(const Blender::Object* obj, PSceneNode armatureNode)
    {
        const Blender::Object* obAr = GetAssignedArmature(obj);
        CHECK_ASSERT(obAr, __FILE__, __LINE__);
        const Blender::bPose* pose = obAr->pose;
        const Blender::bDeformGroup* def = (const Blender::bDeformGroup*)obj->defbase.first;
        while (def)
        {
            if (IsBoneDefGroup(obAr, def))
            {
                const Blender::bPoseChannel* pchan = GetPoseChannelByName(pose, def->name);
                PSceneNode bone = armatureNode->GetChild<SceneNode>(pchan->bone->name, true);

                Matrix4 offsetMatrix = Matrix4(glm::inverse(ToMatrix(obj->obmat) * ToMatrix(pchan->bone->arm_mat)));
                Matrix4 bindShapeMatrix = armatureNode->GetTransform();
                bone->SetBoneOffsetMatrix(offsetMatrix * bindShapeMatrix);
            }
            def = def->next;
        }
    }

    void BScene::ConvertMesh(const Blender::Object* obj, const Blender::Mesh* me, PModelMesh mesh)
    {
        CHECK_ASSERT(!me->mface && "Legacy conversion is not allowed", __FILE__, __LINE__);
        CHECK_ASSERT(me->mvert && me->mpoly, __FILE__, __LINE__);

        // UV-Layer-Data
        Blender::MLoopUV* muvs[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
		char* uvNames[8] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
		int totlayer = GetUVLayersBMmesh(me, muvs, uvNames);
		for (int i = 0; i < MAX_UVS; i++)
			if (uvNames[i])
				mesh->SetUVName(i, uvNames[i]);

        int nVertexes = me->totvert;
        int nuvs = glm::clamp(totlayer, 0, 2);

        VertexsData vertexData(nVertexes);
        for (int i = 0; i < nVertexes; i++)
        {
            vertexData[i].position_ = Vertex3(me->mvert[i].co[0], me->mvert[i].co[1], me->mvert[i].co[2]);
            vertexData[i].normal_ = glm::normalize(Vertex3(me->mvert[i].no[0], me->mvert[i].no[1], me->mvert[i].no[2]));
        }

        AssignBonesAndWeights(obj, me, vertexData);

        bool hasColorVertex = me->mloopcol && me->totcol;
        for (int fi = 0; fi < me->totpoly; fi++)
        {
            const Blender::MPoly& curpoly = me->mpoly[fi];
            int nloops = curpoly.totloop;
            int indexBase = curpoly.loopstart;

            // skip if face is not a triangle || quad
            if (nloops < 3 || nloops > 4)
            {
                TRACE_PRINTF("*** Only triangles or quads are converted! (loops = %d)!!!\n", nloops);
                continue;
            }

            for (int i = 0; i < nloops; i++)
            {
                int li = indexBase + i;
                int vi = me->mloop[li].v;
                for (int j = 0; j < nuvs; j++)
                    vertexData[vi].uv_[j] = Vertex2(muvs[j][li].uv[0], muvs[j][li].uv[1]);
            }

            if (hasColorVertex)
            {
                for (int i = 0; i < nloops; i++)
                {
                    int li = indexBase + i;
                    int vi = me->mloop[li].v;
                    unsigned r = glm::clamp<unsigned>(me->mloopcol[li].r, 0, 255);
                    unsigned g = glm::clamp<unsigned>(me->mloopcol[li].g, 0, 255);
                    unsigned b = glm::clamp<unsigned>(me->mloopcol[li].b, 0, 255);
                    unsigned a = glm::clamp<unsigned>(me->mloopcol[li].a, 0, 255);
                    Color color(r, g, b, a);
                    vertexData[vi].color_ = color / 255.f;
                }
            }

            int index[4];
            for (int i = 0; i < nloops; i++)
                index[i] = me->mloop[indexBase + i].v;

            bool isQuad = nloops == 4;
            bool calcFaceNormal = !(curpoly.flag & ME_SMOOTH);

            if (isQuad)
                mesh->AddQuad(vertexData[index[0]], vertexData[index[1]], vertexData[index[2]], vertexData[index[3]], calcFaceNormal);
            else
                mesh->AddTriangle(vertexData[index[0]], vertexData[index[1]], vertexData[index[2]], calcFaceNormal);
        }
    }

    int BScene::GetUVLayersBMmesh(const Blender::Mesh* mesh, Blender::MLoopUV** uvEightLayerArray, char** uvNames)
    {
        CHECK_ASSERT(mesh, __FILE__, __LINE__);

        int validLayers = 0;
        Blender::CustomDataLayer* layers = (Blender::CustomDataLayer*)mesh->ldata.layers;
        if (layers)
        {
            for (int i = 0; i < mesh->ldata.totlayer && validLayers < 8; i++)
            {
				Blender::CustomDataLayer& layer = layers[i];
                if (layers[i].type == CD_MLOOPUV && uvEightLayerArray)
                {
					Blender::MLoopUV* mtf = (Blender::MLoopUV*)layer.data;
					if (mtf)
					{
						uvNames[validLayers] = layer.name;
						uvEightLayerArray[validLayers++] = mtf;
					}
                }
            }
        }
        return validLayers;
    }

    const Blender::Material* BScene::GetMaterial(const Blender::Object* ob, int index) const
    {
        if (!ob || ob->totcol == 0) return 0;

        index = glm::clamp<int>(index, 0, ob->totcol - 1);
        Blender::Material* ma = nullptr;

        int inObject = ob->matbits && ob->matbits[index] ? 1 : 0;

        if (!inObject)
            inObject = ob->colbits & (1 << index);

        if (inObject)
            ma = (Blender::Material*)ob->mat[index];
        else
        {
            Blender::Mesh* me = (Blender::Mesh*)ob->data;
            if (me && me->mat && me->mat[index])
                ma = me->mat[index];
        }

        return ma;
    }

    void BScene::SetMaterial(const Blender::Object* obj, PSceneNode sceneNode)
    {
        const Blender::Material* mt = GetMaterial(obj, 0);
        std::string name = B_IDNAME(mt);
        auto material = Material::Get(name);
        sceneNode->SetMaterial(material);
    }

    bool BScene::Save(bool compress)
    {
        Path outputFile(outputDir_);
        outputFile.SetName("b" + path_.GetName());
        outputFile.SetExtension("xml");
        pugi::xml_document doc;
		GenerateXML(doc);
        return FileSystem::SaveDocument(outputFile, doc, compress);
    }

	void BScene::GenerateXML(pugi::xml_document& doc)
    {
        pugi::xml_node appNode = doc.append_child("App");
        if (embedResources_)
            Resource::SaveResources(appNode);
        else
            Resource::SaveResourcesExternally(appNode, path_, outputDir_);
        Sound::SaveSounds(appNode);
        Mesh::SaveMeshes(appNode);
        Material::SaveMaterials(appNode);
        for (auto& scene : scenes_)
            scene->Save(appNode);
    }


}
