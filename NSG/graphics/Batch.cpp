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
#include "Batch.h"
#include "Check.h"
#include "Material.h"
#include "Mesh.h"
#include "Pass.h"
#include "RenderingCapabilities.h"
#include "RenderingContext.h"
#include "RigidBody.h"
#include "SceneNode.h"

namespace NSG {
Batch::Batch() : material_(nullptr), mesh_(nullptr), allowInstancing_(true) {}

Batch::Batch(Material* material, Mesh* mesh)
    : material_(material), mesh_(mesh), allowInstancing_(true) {
    CHECK_ASSERT(material_ && mesh_);
}

Batch::~Batch() {}

bool Batch::operator==(const Batch& obj) const {
    return material_ == obj.material_ && mesh_ == obj.mesh_ &&
           nodes_ == obj.nodes_;
}

bool Batch::IsReady() { return material_->IsReady() && mesh_->IsReady(); }

void Batch::Add(SceneNode* node) {
    allowInstancing_ &= !node->GetArmature();
    nodes_.push_back(node);
}

bool Batch::AllowInstancing() const {
    return RenderingCapabilities::GetPtr()->HasInstancedArrays() &&
           allowInstancing_ && mesh_->IsStatic();
}

void Batch::Clear() {
    material_ = nullptr;
    mesh_ = nullptr;
    nodes_.clear();
    allowInstancing_ = true;
}
}
