/*
-------------------------------------------------------------------------------
This file is part of nsg-library.
http://github.com/woodjazz/nsg-library

Copyright (c) 2014-2017 Néstor Silveira Gorski

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
#include "LevelResources.h"

LevelResources::LevelResources(PWindow window,
                               std::vector<const char*> resourceNames)
    : Level(window), scene_(std::make_shared<Scene>()) {
    loadingNode_ = scene_->CreateOverlay("loadingNode");
    loadingNode_->SetMaterial(Material::Create());
    auto xml = Resource::GetOrCreate<ResourceFile>("data/AnonymousPro132.xml");
    auto atlas = std::make_shared<FontXMLAtlas>();
    atlas->SetXML(xml);
    auto atlasResource =
        Resource::GetOrCreate<ResourceFile>("data/AnonymousPro132.png");
    auto atlasTexture = std::make_shared<Texture2D>(atlasResource);
    atlas->SetTexture(atlasTexture);
    loadingNode_->GetMaterial()->SetFontAtlas(atlas);
    window->SetScene(scene_);

    auto resource = Resource::Create("LevelResources");
    std::string data = "<App><Resources>";
    for (auto name : resourceNames)
        data += "<Resource name = \"" + std::string(name) + "\"/>";
    data += "</Resources></App>";
    resource->SetBuffer(data);

    loader_ = LoaderXML::GetOrCreate("loader");

    slotLoaded_ = loader_->Load(resource)->Connect(
        [this]() { Level::Load(GetIndex() + 1, window_); });

    slotPercentage_ = loader_->SigProgress()->Connect([this](float percentage) {
        loadingNode_->SetMesh(
            loadingNode_->GetMaterial()->GetFontAtlas()->GetOrCreateMesh(
                "Loading " + ToString((int)percentage)));
    });
}

LevelResources::~LevelResources() {}
