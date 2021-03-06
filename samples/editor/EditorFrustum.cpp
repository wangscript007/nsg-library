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
#include "EditorFrustum.h"
#include "Camera.h"
#include "Frustum.h"
#include "FrustumMesh.h"
#include "Material.h"

EditorFrustum::EditorFrustum(const std::string& name) : EditorSceneNode(name) {
    SetMesh(Mesh::GetOrCreate<FrustumMesh>("NSGEditorFrustum"));
    SetMaterial(Material::GetOrCreate("NSGEditorFrustum"));
    material_->SetRenderPass(RenderPass::VERTEXCOLOR);
    material_->CastShadow(false);
    material_->ReceiveShadows(false);
}

EditorFrustum::~EditorFrustum() {}

void EditorFrustum::SetCamera(PCamera camera) {
    camera_ = camera;
    SetTransform(camera->GetTransform().Inverse());
    std::dynamic_pointer_cast<FrustumMesh>(GetMesh())->SetFrustum(
        camera->GetFrustum());
    slotUpdated_ = camera->SigUpdated()->Connect([this]() {
        if (CanBeVisible()) {
            auto camera = camera_.lock();
            if (camera) {
                SetTransform(camera->GetTransform().Inverse());
                std::dynamic_pointer_cast<FrustumMesh>(GetMesh())->SetFrustum(
                    camera->GetFrustum());
            }
        }
    });
}
