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
#include "Scene.h"
#include "assimp/IOSystem.hpp"
#include <string>
#include <vector>

struct aiScene;
struct aiNode;

namespace pugi
{
	class xml_node;
}

namespace NSG
{
	class SceneConverter : public NSG::Scene, public Assimp::IOSystem
	{
	public:
		SceneConverter(PResourceFile resource);
		~SceneConverter();
        bool Exists(const char* filename) const override;
		char getOsSeparator() const override;
	    Assimp::IOStream* Open(const char* filename, const char* mode = "rb") override;
		void Close(Assimp::IOStream* pFile) override;
		bool Save(const std::string& filename);
	private:
		void SaveMeshes(pugi::xml_node& node);
		void SaveMaterials(pugi::xml_node& node);
		void LoadLights(const aiScene* sc);
		void LoadMeshesAndMaterials(const aiScene* sc);
		void RecursiveLoad(const aiScene *sc, const aiNode* nd, PSceneNode sceneNode);
		PResourceFile pResource_;
		PSceneNode root_;
		std::vector<PMesh> meshes_;
		std::vector<PMaterial> materials_;
	};
}