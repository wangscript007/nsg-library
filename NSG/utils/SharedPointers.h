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
#include <memory>
#include <vector>

namespace NSG
{
	struct AnimationTrack;
	typedef std::shared_ptr<AnimationTrack> PAnimationTrack;

	class Worker;
	typedef std::shared_ptr<Worker> PWorker;

	class Window;
	typedef std::shared_ptr<Window> PWindow;
	typedef std::weak_ptr<Window> PWeakWindow;

	class RigidBody;
	typedef std::shared_ptr<RigidBody> PRigidBody;

	class PhysicsWorld;
	typedef std::shared_ptr<PhysicsWorld> PPhysicsWorld;
	
	class Skeleton;
	typedef std::shared_ptr<Skeleton> PSkeleton;
	
	class AnimationState;
	typedef std::shared_ptr<AnimationState> PAnimationState;

	class Animation;
	typedef std::shared_ptr<Animation> PAnimation;
	
	class PointOnSphere;
	typedef std::shared_ptr<PointOnSphere> PPointOnSphere;

	class Ray;
	typedef std::shared_ptr<Ray> PRay;

	class Octree;
	typedef std::shared_ptr<Octree> POctree;

	struct BoundingBox;
	typedef std::shared_ptr<BoundingBox> PBoundingBox;

	class Scene;
	typedef std::shared_ptr<Scene> PScene;
	typedef std::weak_ptr<Scene> PWeakScene;

	class Graphics;
	typedef std::unique_ptr<Graphics> PGraphics;

	class Frustum;
	typedef std::shared_ptr<Frustum> PFrustum;
	
	class Audio;
	typedef std::unique_ptr<Audio> PAudio;

	class Music;
	typedef std::unique_ptr<Music> PMusic;
	
	class Sound;
	typedef std::shared_ptr<Sound> PSound;

	class Buffer;
	typedef std::shared_ptr<Buffer> PBuffer;

	struct Context;
	typedef std::unique_ptr<Context> PContext;
	
	struct AppConfiguration;
	typedef std::shared_ptr<AppConfiguration> PAppConfiguration;
	
	class Technique;
	typedef std::shared_ptr<Technique> PTechnique;
	typedef std::weak_ptr<Technique> PWeakTechnique;

	class PassFilter;
	typedef std::shared_ptr<PassFilter> PPassFilter;

	class FilterBlend;
	typedef std::shared_ptr<FilterBlend> PFilterBlend;

	class Pass2Texture;
	typedef std::shared_ptr<Pass2Texture> PPass2Texture;

	class Pass;
	typedef std::shared_ptr<Pass> PPass;

	class Pass2Stencil;
	typedef std::shared_ptr<Pass2Stencil> PPass2Stencil;

	typedef std::vector<PPass> PASSES;

	namespace FSM
	{
		class Machine;
		typedef std::shared_ptr<Machine> PMachine;
	}

	class Batch;
	typedef std::shared_ptr<Batch> PBatch;
	typedef std::weak_ptr<Batch> PWeakBatch;
	
	class ResourceFile;
	typedef std::shared_ptr<ResourceFile> PResourceFile;

	class ResourceMemory;
	typedef std::shared_ptr<ResourceMemory> PResourceMemory;
	
	class ShowTexture;
	typedef std::shared_ptr<ShowTexture> PShowTexture;

    class Mesh;
    typedef std::shared_ptr<Mesh> PMesh;

    class Texture;
    typedef std::shared_ptr<Texture> PTexture;

    class Material;
    typedef std::shared_ptr<Material> PMaterial;
    typedef std::weak_ptr<Material> PWeakMaterial;

    class FontAtlas;
    typedef std::shared_ptr<FontAtlas> PFontAtlas;

    class FontAtlasTextureManager;
    typedef std::unique_ptr<FontAtlasTextureManager> PFontAtlasTextureManager;

    class App;
	typedef std::unique_ptr<App> PApp;

	struct InternalApp;
	typedef std::unique_ptr<InternalApp> PInternalApp;

	struct Behavior;
	typedef std::shared_ptr<Behavior> PBehavior;

	class CameraControl;
	typedef std::shared_ptr<CameraControl> PCameraControl;

	class BoxMesh;
	typedef std::shared_ptr<BoxMesh> PBoxMesh;

	class Camera;
	typedef std::shared_ptr<Camera> PCamera;
	typedef std::weak_ptr<Camera> PWeakCamera;

	class CircleMesh;
	typedef std::shared_ptr<CircleMesh> PCircleMesh;

	class EllipseMesh;
	typedef std::shared_ptr<EllipseMesh> PEllipseMesh;

	class Filter;
	typedef std::shared_ptr<Filter> PFilter;

	class FilterBlur;
	typedef std::shared_ptr<FilterBlur> PFilterBlur;

	class FrameBuffer;
    typedef std::shared_ptr<FrameBuffer> PFrameBuffer;

	class FrameColorSelection;
    typedef std::shared_ptr<FrameColorSelection> PFrameColorSelection;

    class FragmentShader;
	typedef std::shared_ptr<FragmentShader> PFragmentShader;

	class IndexBuffer;
	typedef std::shared_ptr<IndexBuffer> PIndexBuffer;

	class VertexArrayObj;
	typedef std::shared_ptr<VertexArrayObj> PVertexArrayObj;

	class Light;
	typedef std::shared_ptr<Light> PLight;
	typedef std::weak_ptr<Light> PWeakLight;
	
	class PlaneMesh;
	typedef std::shared_ptr<PlaneMesh> PPlaneMesh;

	class Program;
	typedef std::shared_ptr<Program> PProgram;

	class RectangleMesh;
	typedef std::shared_ptr<RectangleMesh> PRectangleMesh;

	class Render2Texture;
	typedef std::shared_ptr<Render2Texture> PRender2Texture;

	class RoundedRectangleMesh;
	typedef std::shared_ptr<RoundedRectangleMesh> PRoundedRectangleMesh;

	class SphereMesh;
	typedef std::shared_ptr<SphereMesh> PSphereMesh;

	class ModelMesh;
	typedef std::shared_ptr<ModelMesh> PModelMesh;

	class StencilMask;
	typedef std::shared_ptr<StencilMask> PStencilMask;

	class TextMesh;
	typedef std::shared_ptr<TextMesh> PTextMesh;

	class VertexBuffer;
	typedef std::shared_ptr<VertexBuffer> PVertexBuffer;

	class InstanceBuffer;
	typedef std::shared_ptr<InstanceBuffer> PInstanceBuffer;

	class VertexShader;
	typedef std::shared_ptr<VertexShader> PVertexShader;

	namespace IMGUI
	{
		struct LayoutArea;
		typedef std::shared_ptr<LayoutArea> PLayoutArea;

		class LayoutManager;
		typedef std::unique_ptr<LayoutManager> PLayoutManager;

		struct Skin;
		typedef std::shared_ptr<Skin> PSkin;

		class TextManager;
	    typedef std::unique_ptr<TextManager> PTextManager;

	    struct Context;
	    typedef std::unique_ptr<Context> PContext;

	    struct State;
	    typedef std::shared_ptr<State> PState;

	    class Area;
	    typedef std::shared_ptr<Area> PArea;

	    class Title;
	    typedef std::shared_ptr<Title> PTitle;

	    class Window;
	    typedef std::shared_ptr<Window> PWindow;

		struct WindowManager;
		typedef std::shared_ptr<WindowManager> PWindowManager;

		struct Style;
		typedef std::shared_ptr<Style> PStyle;

		struct ButtonStyle;
		typedef std::shared_ptr<ButtonStyle> PButtonStyle;

		struct LabelStyle;
		typedef std::shared_ptr<LabelStyle> PLabelStyle;

		struct TextStyle;
		typedef std::shared_ptr<TextStyle> PTextStyle;

		struct TitleStyle;
		typedef std::shared_ptr<TitleStyle> PTitleStyle;

		struct WindowStyle;
		typedef std::shared_ptr<WindowStyle> PWindowStyle;

		struct AreaStyle;
		typedef std::shared_ptr<AreaStyle> PAreaStyle;

		struct SliderStyle;
		typedef std::shared_ptr<SliderStyle> PSliderStyle;

		struct SizerStyle;
		typedef std::shared_ptr<SizerStyle> PSizerStyle;

		struct LineStyle;
		typedef std::shared_ptr<LineStyle> PLineStyle;

		struct CheckButtonStyle;
		typedef std::shared_ptr<CheckButtonStyle> PCheckButtonStyle;
	}

	namespace NaCl 
	{
		class NaClURLLoader;
		typedef std::shared_ptr<NaClURLLoader> PNaClURLLoader;
	}

	class Node;
	typedef std::shared_ptr<Node> PNode;
	typedef std::weak_ptr<Node> PWeakNode;

	class Resource;
	typedef std::shared_ptr<Resource> PResource;

	class SceneNode;
	typedef std::shared_ptr<SceneNode> PSceneNode;
	typedef std::weak_ptr<SceneNode> PWeakSceneNode;

	class Keyboard;
	typedef std::unique_ptr<Keyboard> PKeyboard;

	class ParticleSystem;
	typedef std::shared_ptr<ParticleSystem> PParticleSystem;
	typedef std::weak_ptr<ParticleSystem> PWeakParticleSystem;

	class Particle;
	typedef std::shared_ptr<Particle> PParticle;
}