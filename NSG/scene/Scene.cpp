#include "Scene.h"
#include "SceneNode.h"
#include "Camera.h"
#include "Light.h"
#include "Light.h"
#include "Octree.h"
#include "OctreeQuery.h"
#include "Graphics.h"
#include "Constants.h"
#include "Material.h"
#include "Mesh.h"
#include "Util.h"
#include "Skeleton.h"
#include "ModelMesh.h"
#include "Animation.h"
#include "AnimationState.h"
#include "PhysicsWorld.h"
#include "ParticleSystem.h"
#include "Window.h"
#include "pugixml.hpp"
#include <algorithm>
#include <functional>
#include <thread>

namespace NSG
{
    Scene::Scene(const std::string& name)
        : SceneNode(name),
          mainCamera_(nullptr),
          ambient_(0.3f, 0.3f, 0.3f, 1),
          horizon_(0.f, 0.f, 0.f, 1.f),
          octree_(std::make_shared<Octree>()),
          physicsWorld_(new PhysicsWorld),
          window_(nullptr),
          signalNodeMouseMoved_(new Signal<SceneNode *, float, float>()),
          signalNodeMouseDown_(new Signal<SceneNode *, int, float, float>()),
          signalNodeMouseUp_(new Signal<SceneNode *, int, float, float>()),
          signalNodeMouseWheel_(new Signal<SceneNode *, float, float>())
    {
        if (Graphics::this_)
        {
            auto window = Graphics::this_->GetWindow();
            if (window)
                SetWindow(window);
        }

        slotWindowCreated_ = Window::SigReady()->Connect([this](Window * window)
        {
            if (!window_)
                SetWindow(window);
        });

        slotLightBeingDestroy_ = Light::SignalBeingDestroy()->Connect([this](Light * light)
        {
			auto it = std::find(lights_.begin(), lights_.end(), light);
			if (it != lights_.end())
				lights_.erase(it);
        });

        slotCameraBeingDestroy_ = Camera::SignalBeingDestroy()->Connect([this](Camera * camera)
        {
            if(mainCamera_ == camera)
                mainCamera_ = nullptr;
			auto it = std::find(cameras_.begin(), cameras_.end(), camera);
			if (it != cameras_.end())
				cameras_.erase(it);
        });

        slotPSBeingDestroy_ = ParticleSystem::SignalBeingDestroy()->Connect([this](ParticleSystem * ps)
        {
			auto it = std::find(particleSystems_.begin(), particleSystems_.end(), ps);
			if (it != particleSystems_.end())
				particleSystems_.erase(it);
        });
    }

    Scene::~Scene()
    {
        slotLightBeingDestroy_ = nullptr;
        slotCameraBeingDestroy_ = nullptr;
        slotPSBeingDestroy_ = nullptr;
        if (window_ && window_->GetScene() == this)
            window_->SetScene(nullptr);
    }

    void Scene::SetWindow(Window* window)
    {
        if (window_ != window)
        {
            if (window)
            {
                if (!window->GetScene())
                    window->SetScene(this);

                slotMouseMoved_ = window->SigFloatFloat()->Connect([&](float x, float y)
                {
                    if (signalNodeMouseMoved_->HasSlots())
                    {
                        SceneNode* node = GetClosestNode(x, y);
                        if (node)
                            signalNodeMouseMoved_->Run(node, x, y);
                    }
                });

                slotMouseDown_ = window->SigMouseDown()->Connect([&](int button, float x, float y)
                {
                    if (signalNodeMouseDown_->HasSlots())
                    {
                        SceneNode* node = GetClosestNode(x, y);
                        if (node)
                            signalNodeMouseDown_->Run(node, button, x, y);
                    }
                });

                slotMouseUp_ = window->SigMouseUp()->Connect([&](int button, float x, float y)
                {
                    if (signalNodeMouseUp_->HasSlots())
                    {
                        SceneNode* node = GetClosestNode(x, y);
                        if (node)
                            signalNodeMouseUp_->Run(node, button, x, y);
                    }
                });

                slotMouseWheel_ = window->SigMouseWheel()->Connect([&](float x, float y)
                {
                    if (signalNodeMouseWheel_->HasSlots())
                    {
                        SceneNode* node = GetClosestNode(x, y);
                        if (node)
                            signalNodeMouseWheel_->Run(node, x, y);
                    }
                });

            }
            else
            {
                if (window_->GetScene() == this)
                    window_->SetScene(nullptr);
                slotMouseMoved_ = nullptr;
                slotMouseDown_ = nullptr;
                slotMouseUp_ = nullptr;
                slotMouseWheel_ = nullptr;
            }

            window_ = window;
        }
    }

    void Scene::SetAmbientColor(Color ambient)
    {
        if (ambient_ != ambient)
        {
            ambient_ = ambient;
            SetUniformsNeedUpdate();
        }
    }

    void Scene::SetHorizonColor(Color horizon)
    {
        if (horizon_ != horizon)
        {
            horizon_ = horizon;
            SetUniformsNeedUpdate();
        }
    }

    void Scene::UpdateAll(float deltaTime)
    {
        physicsWorld_->StepSimulation(deltaTime);
        UpdateAnimations(deltaTime);
        UpdateParticleSystems(deltaTime);
    }

    bool Scene::GetFastRayNodesIntersection(const Ray& ray, std::vector<SceneNode*>& nodes) const
    {
        RayOctreeQuery query(nodes, ray);
        octree_->Execute(query);
        return !nodes.empty();
    }

    bool Scene::GetPreciseRayNodesIntersection(const Ray& ray, std::vector<RayNodeResult>& result) const
    {
        std::vector<SceneNode*> tmpNodes;
        RayOctreeQuery query(tmpNodes, ray);
        octree_->Execute(query);
        result.clear();
        float maxDistance = ray.GetMaxDistance();
        for (auto& obj : tmpNodes)
        {
            float distance = ray.HitDistance(obj);
            if (distance < maxDistance)
            {
                RayNodeResult r {distance, obj};
                result.push_back(r);
            }
        }
        return !result.empty();
    }

    bool Scene::GetClosestRayNodeIntersection(const Ray& ray, RayNodeResult& closest) const
    {
        std::vector<RayNodeResult> results;
        if (GetPreciseRayNodesIntersection(ray, results))
        {
            int closestIdx = -1;
            int idx = 0;
            float distance = std::numeric_limits<float>::max();
            for (auto& result : results)
            {
                if (result.distance_ < distance)
                {
                    closestIdx = idx;
                    distance = result.distance_;
                }
                ++idx;
            }

            closest = results[closestIdx];

            return true;

        }
        return false;
    }

    void Scene::GetVisibleNodes(const Camera* camera, std::vector<SceneNode*>& visibles) const
    {
        for (auto& obj : needUpdate_)
            octree_->InsertUpdate(obj);
        needUpdate_.clear();
        FrustumOctreeQuery query(visibles, camera);
        octree_->Execute(query);
    }

    std::vector<Camera*> Scene::GetCameras() const
    {
        return cameras_;
    }

    void Scene::NeedUpdate(SceneNode* obj)
    {
        if(obj->GetMesh() != nullptr)
            needUpdate_.insert(obj);
    }

    void Scene::SavePhysics(pugi::xml_node& node) const
    {
        pugi::xml_node child = node.append_child("Physics");
        child.append_attribute("gravity").set_value(ToString(physicsWorld_->GetGravity()).c_str());
    }

    void Scene::LoadPhysics(const pugi::xml_node& node)
    {
        pugi::xml_node child = node.child("Physics");
        Vertex3 gravity = ToVertex3(child.attribute("gravity").as_string());
        physicsWorld_->SetGravity(gravity);
    }

    void Scene::SaveAnimations(pugi::xml_node& node) const
    {
        pugi::xml_node child = node.append_child("Animations");
        for (auto& obj : animations_)
            obj.second->Save(child);
    }

    void Scene::LoadAnimations(const pugi::xml_node& node)
    {
        pugi::xml_node childAnimations = node.child("Animations");
        if (childAnimations)
        {
            pugi::xml_node child = childAnimations.child("Animation");
            while (child)
            {
                std::string name = child.attribute("name").as_string();
                if (!HasAnimation(name))
                {
                    PAnimation animation = GetOrCreateAnimation(name);
                    animation->SetScene(std::dynamic_pointer_cast<Scene>(shared_from_this()));
                    animation->Load(child);
                }
                child = child.next_sibling("Animation");
            }
        }
    }

    void Scene::SaveSkeletons(pugi::xml_node& node) const
    {
        const std::vector<PMesh>& meshes = Mesh::GetMeshes();
        if (meshes.size())
        {
            pugi::xml_node child = node.append_child("Skeletons");
            for (auto& obj : meshes)
            {
                PSkeleton skeleton = obj->GetSkeleton();
                if (skeleton)
                    skeleton->Save(child);
            }
        }
    }

    void Scene::LoadSkeletons(const pugi::xml_node& node)
    {
        pugi::xml_node childSkeletons = node.child("Skeletons");
        if (childSkeletons)
        {
            pugi::xml_node child = childSkeletons.child("Skeleton");
            while (child)
            {
                std::string meshName = child.attribute("meshName").as_string();
                auto mesh = Mesh::Get<ModelMesh>(meshName);
                CHECK_CONDITION(mesh, __FILE__, __LINE__);
                if (!mesh->GetSkeleton())
                {
                    PSkeleton skeleton(new Skeleton(mesh));
                    skeleton->SetScene(std::dynamic_pointer_cast<Scene>(shared_from_this()));
                    skeleton->Load(child);
                    mesh->SetSkeleton(skeleton);
                }
                child = child.next_sibling("Skeleton");
            }
        }
    }

    void Scene::Load(const pugi::xml_node& node)
    {
        SetAmbientColor(ToVertex4(node.attribute("ambient").as_string()));
        SetHorizonColor(ToVertex4(node.attribute("horizon").as_string()));
        std::string mainCameraName = node.attribute("mainCamera").as_string();
        pugi::xml_node sceneNode = node.child("SceneNode");
        CHECK_ASSERT(sceneNode, __FILE__, __LINE__);
        SceneNode::Load(sceneNode);
		if (!mainCameraName.empty())
			SetMainCamera(GetChild<Camera>(mainCameraName, true));
        LoadAnimations(node);
        LoadSkeletons(node);
        LoadPhysics(node);
    }

    void Scene::Save(pugi::xml_node& node) const
    {
        pugi::xml_node scene = node.append_child("Scene");
        scene.append_attribute("ambient").set_value(ToString(ambient_).c_str());
        scene.append_attribute("horizon").set_value(ToString(horizon_).c_str());
        std::string mainCameraName;
        if(mainCamera_)
            mainCameraName = mainCamera_->GetName();
        scene.append_attribute("mainCamera").set_value(mainCameraName.c_str());
        pugi::xml_node sceneNode = scene.append_child("SceneNode");
        SceneNode::Save(sceneNode);
        SaveAnimations(scene);
        SaveSkeletons(scene);
        SavePhysics(scene);
    }

    bool Scene::GetVisibleBoundingBox(const Camera* camera, BoundingBox& bb) const
    {
        std::vector<SceneNode*> visibles;
        GetVisibleNodes(camera, visibles);
        if (!visibles.empty())
        {
            bb = BoundingBox();
            for (auto& obj : visibles)
                bb.Merge(obj->GetWorldBoundingBox());
            return true;
        }
        return false;
    }

    PAnimation Scene::GetOrCreateAnimation(const std::string& name)
    {
        auto it = animations_.find(name);
        if (it == animations_.end())
        {
            auto animation = std::make_shared<Animation>(name);
            animations_[name] = animation;
            return animation;
        }
        return it->second;
    }

    std::vector<PAnimation> Scene::GetAnimationsFor(PNode node) const
    {
        std::vector<PAnimation> result;
        for (auto& animation : animations_)
        {
            auto& tracks = animation.second->GetTracks();
            for (auto& track : tracks)
            {
                auto trackNode = track.node_.lock();
                if (trackNode == node)
                {
                    result.push_back(animation.second);
                    break;
                }
            }
        }
        return result;
    }

    bool Scene::HasAnimation(const std::string& name) const
    {
        return animations_.find(name) != animations_.end();
    }

    bool Scene::PlayAnimation(const std::string& name, bool looped)
    {
        PAnimation animation = animations_.find(name)->second;
        if (animation)
        {
            PlayAnimation(animation, looped);
            return true;
        }
        return false;
    }

    void Scene::PlayAnimation(const PAnimation& animation, bool looped)
    {
        std::string name = animation->GetName();

        PAnimationState animationState;
        auto it = animationStateMap_.find(name);
        if (it != animationStateMap_.end())
            animationState = it->second;
        else
        {
            animationState = PAnimationState(new AnimationState(animation));
            animationStateMap_.insert(AnimationStateMap::value_type(name, animationState));
        }
        animationState->SetLooped(looped);
    }

    void Scene::UpdateAnimations(float deltaTime)
    {
        auto it = animationStateMap_.begin();
        while (it != animationStateMap_.end())
        {
            auto& animState = it->second;
            if (!animState->HasEnded())
            {
                animState->AddTime(deltaTime);
                animState->Update();
                ++it;
            }
            else
                it = animationStateMap_.erase(it);
        }
    }

    void Scene::UpdateParticleSystems(float deltaTime)
    {
        for (auto ps : particleSystems_)
            ps->Update(deltaTime);
    }

    bool Scene::SetAnimationSpeed(const std::string& name, float speed)
    {
        PAnimationState animationState;
        auto it = animationStateMap_.find(name);
        if (it != animationStateMap_.end())
            animationState = it->second;
        else
            return false;

        animationState->SetSpeed(speed);
        return true;
    }

    void Scene::AddLight(Light* light)
    {
        lights_.push_back(light);

        // we want the lights sorted by type for rendering/batching purposes
        std::sort(lights_.begin(), lights_.end(), [&](const Light * a, const Light * b) -> bool
        {
            return (int)a->GetType() < (int)b->GetType();
        });
    }

    const std::vector<Light*>& Scene::GetLights() const
    {
        return lights_;
    }

    void Scene::AddCamera(Camera* camera)
    {
        cameras_.push_back(camera);
        if(!mainCamera_)
            mainCamera_ = camera;
    }

    void Scene::AddParticleSystem(ParticleSystem* ps)
    {
        particleSystems_.push_back(ps);
    }

    void Scene::UpdateOctree(SceneNode* node)
    {
        octree_->InsertUpdate(node);
    }

    void Scene::RemoveFromOctree(SceneNode* node)
    {
        needUpdate_.erase(node);
        octree_->Remove(node);
    }

    SceneNode* Scene::GetClosestNode(float screenX, float screenY) const
    {
        Ray ray = Camera::GetRay(screenX, screenY);
        RayNodeResult closest{ 0, nullptr };
        GetClosestRayNodeIntersection(ray, closest);
        if (closest.node_)
            return closest.node_;
        return nullptr;
    }

    unsigned Scene::GetDrawablesNumber() const
    {
        return octree_->GetNumDrawables();
    }

	PCamera Scene::GetMainCamera() const
    { 
		if (mainCamera_)
		{
			auto p = mainCamera_->shared_from_this();
			return std::dynamic_pointer_cast<Camera>(p);
		}
        return nullptr; 
    }

    void Scene::SetMainCamera(PCamera camera)
    {
        mainCamera_ = camera.get();
    }

    const std::vector<SceneNode*>& Scene::GetDrawables() const
    {
        return octree_->GetDrawables();
    }
}
