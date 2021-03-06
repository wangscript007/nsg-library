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
#include "TextMesh.h"
#include "FontAtlas.h"
#include "Program.h"
#include "RenderingContext.h"
#include "SignalSlots.h"
#include "Window.h"
#include <algorithm>

namespace NSG {
template <>
std::map<std::string, PWeakTextMesh>
    WeakFactory<std::string, TextMesh>::objsMap_ =
        std::map<std::string, PWeakTextMesh>{};

TextMesh::TextMesh(const std::string& name)
    : Mesh(name), screenWidth_(0), screenHeight_(0),
      hAlignment_(LEFT_ALIGNMENT), vAlignment_(BOTTOM_ALIGNMENT) {
    SetSerializable(false);
}

TextMesh::~TextMesh() {}

void TextMesh::SetAtlas(PFontAtlas atlas) {
    if (pAtlas_.lock() != atlas) {
        pAtlas_ = atlas;
        Invalidate();
    }
}

bool TextMesh::IsValid() {
    return !text_.empty() && pAtlas_.lock() && pAtlas_.lock()->IsReady();
}

void TextMesh::AllocateResources() {
    pAtlas_.lock()->GenerateMeshData(text_, vertexsData_, indexes_,
                                     screenWidth_, screenHeight_);

    float alignmentOffsetX;
    float alignmentOffsetY;

    if (hAlignment_ == CENTER_ALIGNMENT)
        alignmentOffsetX = -screenWidth_ / 2;
    else if (hAlignment_ == RIGHT_ALIGNMENT)
        alignmentOffsetX = 1 - screenWidth_;
    else
        alignmentOffsetX = -1;

    if (vAlignment_ == MIDDLE_ALIGNMENT)
        alignmentOffsetY = -screenHeight_ / 2;
    else if (vAlignment_ == TOP_ALIGNMENT)
        alignmentOffsetY = 1 - screenHeight_;
    else
        alignmentOffsetY = -1; // + screenHeight_;

    for (auto& obj : vertexsData_) {
        obj.position_.x += alignmentOffsetX;
        obj.position_.y += alignmentOffsetY;
    }

    Mesh::AllocateResources();
}

void TextMesh::SetText(const std::string& text, HorizontalAlignment hAlign,
                       VerticalAlignment vAlign) {
    if (text_ != text) {
        text_ = text;
        SetAlignment(hAlign, vAlign);
        Invalidate();
    }
}

void TextMesh::SetAlignment(HorizontalAlignment hAlign,
                            VerticalAlignment vAlign) {
    if (hAlignment_ != hAlign || vAlignment_ != vAlign) {
        hAlignment_ = hAlign;
        vAlignment_ = vAlign;
        Invalidate();
    }
}

void TextMesh::GetAlignment(HorizontalAlignment& hAlign,
                            VerticalAlignment& vAlign) {
    hAlign = hAlignment_;
    vAlign = vAlignment_;
}

GLenum TextMesh::GetWireFrameDrawMode() const { return GL_LINE_LOOP; }

GLenum TextMesh::GetSolidDrawMode() const { return GL_TRIANGLES; }

size_t TextMesh::GetNumberOfTriangles() const { return indexes_.size() / 3; }
}
