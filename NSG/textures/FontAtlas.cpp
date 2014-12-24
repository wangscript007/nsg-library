#include "FontAtlas.h"
#include "Check.h"
#include "TextureFileManager.h"
#include "Texture.h"
#include "TextMesh.h"
#include "ResourceFile.h"
#include "ResourceMemory.h"
#include "ResourceFileManager.h"
#include "Mesh.h"
#include "App.h"
#include "Window.h"
#include "UTF8String.h"
#include "pugixml.hpp"
#include "autogenerated/anonymous_pro_regular_14_png.inl"
#include "autogenerated/anonymous_pro_regular_14_xml.inl"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <assert.h>
#include <cmath>

namespace NSG
{
    FontAtlas::FontAtlas(int viewWidth, int viewHeight)
        : viewWidth_(viewWidth),
          viewHeight_(viewHeight)
    {
        PResourceMemory bitmapFont(new ResourceMemory((const char*)ANONYMOUS_PRO_REGULAR_14_PNG, ANONYMOUS_PRO_REGULAR_14_PNG_SIZE));
        texture_ = PTexture(new Texture(bitmapFont));
        xmlResource_ = PResource(new ResourceMemory((const char*)ANONYMOUS_PRO_REGULAR_14_XML, ANONYMOUS_PRO_REGULAR_14_XML_SIZE));
    }

    FontAtlas::FontAtlas(const Path& path, int viewWidth, int viewHeight)
        : path_(path),
          viewWidth_(viewWidth),
          viewHeight_(viewHeight)
    {
        CHECK_ASSERT(!path_.IsEmpty(), __FILE__, __LINE__);
        texture_ = TextureFileManager::this_->GetOrCreate(path_);
        texture_->SetFlags((int)TextureFlag::NONE);
        Path xmlFilename(path_.GetPathAndName() + ".xml"); //divo compatible (generated with font builder)
        xmlResource_ = App::this_->GetOrCreateResourceFile(xmlFilename);
    }

    FontAtlas::~FontAtlas()
    {
		Invalidate();
    }

    PTextMesh FontAtlas::GetOrCreateMesh(const std::string& text, HorizontalAlignment hAlign, VerticalAlignment vAlign)
    {
        auto mesh = meshes_.GetOrCreateClass<TextMesh>(text);
        mesh->SetAtlas(shared_from_this());
        mesh->SetText(text, hAlign, vAlign);
		return mesh;
    }

    bool FontAtlas::IsValid()
    {
        return xmlResource_ && xmlResource_->IsReady() && texture_->IsReady();
    }

    void FontAtlas::AllocateResources()
    {
        ParseXML();
    }

    void FontAtlas::ReleaseResources()
    {
        charsMap_.clear();
		xmlResource_->Invalidate();
    }

    void FontAtlas::SetViewSize(int width, int height)
    {
        if(viewWidth_ != width || viewHeight_ != height)
        {
            viewWidth_ = width;
            viewHeight_ = height;
            Invalidate();
            auto objs = meshes_.GetObjs();
            for(auto& obj: objs)
                obj->Invalidate();
        }
    }

    bool FontAtlas::ParseXML()
    {
        TRACE_LOG("FontAtlas::Parsing: " << (path_.IsEmpty() ? "internal font" : xmlResource_->GetPath().GetFilePath()));

        pugi::xml_document doc;
        pugi::xml_parse_result result = doc.load_buffer_inplace((void*)xmlResource_->GetData(), xmlResource_->GetBytes());
        if (!result)
        {
            TRACE_LOG("XML parsed with errors, attr value: [" << doc.child("node").attribute("attr").value() << "]");
            TRACE_LOG("Error description: " << result.description());
            TRACE_LOG("Error offset: " << result.offset << " (error at [..." << (result.offset) << "]");
            CHECK_ASSERT(false, __FILE__, __LINE__);
            return false;
        }
        else
        {
            pugi::xml_node node = doc.child("Font");
            int height = node.attribute("height").as_int();
            node = node.child("Char");
            while (node)
            {
                CharInfo charInfo;
                charInfo.height = height;

                charInfo.width = node.attribute("width").as_int();
                {
                    std::stringstream ss;
                    ss << node.attribute("offset").value();
                    ss >> charInfo.offset.x;
                    ss >> charInfo.offset.y;
                }

                {
                    std::stringstream ss;
                    ss << node.attribute("rect").value();
                    ss >> charInfo.rect.x;
                    ss >> charInfo.rect.y;
                    ss >> charInfo.rect.z;
                    ss >> charInfo.rect.w;
                }

                const char* code = node.attribute("code").value();
                UTF8String utf8(code);
                unsigned unicode = utf8.AtUTF8(0);
                charsMap_[unicode] = charInfo;
                node = node.next_sibling("Char");
            }

            TRACE_LOG("FontAtlas::Parsing done.");

            return true;
        }
    }

    void FontAtlas::GenerateMesh(const std::string& text, VertexsData& vertexsData, Indexes& indexes, GLfloat& screenWidth, GLfloat& screenHeight)
    {
        vertexsData.clear();
        indexes.clear();

        screenWidth = screenHeight = 0;

        CHECK_ASSERT(viewWidth_ > 0 && viewHeight_ > 0, __FILE__, __LINE__);

        float sx = 2.0f / viewWidth_;
        float sy = 2.0f / viewHeight_;

        GLsizei textureWidth = texture_->GetWidth();
        GLsizei textureHeight = texture_->GetHeight();

        float x = 0;
        //float y = 0;

        int index = 0;

        const char* p = text.c_str();

        while (*p)
        {
            int idx = (unsigned char)(*p++);

            const CharInfo& charInfo = charsMap_[idx];

            // Front Face CCW
            VertexData vertex[4];
            {
                float w = (float)charInfo.rect.z * sx;
                float h = -(float)charInfo.rect.w * sy;

                float offsetX = (float)charInfo.offset.x * sx;
                float offsetY = -(float)charInfo.offset.y * sy;

                float ux = (float)charInfo.rect.x / textureWidth;
                float uy = (float)charInfo.rect.y / textureHeight;
                float uw = (float)charInfo.rect.z / textureWidth;
                float uh = (float)charInfo.rect.w / textureHeight;


                vertex[0].position_ = Vertex3(offsetX, offsetY, 0);
                vertex[0].uv0_ = Vertex2(ux, uy);

                vertex[1].position_ = Vertex3(offsetX + w, offsetY, 0);
                vertex[1].uv0_ = Vertex2(ux + uw, uy);

                vertex[2].position_ = Vertex3(offsetX, offsetY + h, 0);
                vertex[2].uv0_ = Vertex2(ux, uy + uh);

                vertex[3].position_ = Vertex3(offsetX + w, offsetY + h, 0);
                vertex[3].uv0_ = Vertex2(ux + uw, uy + uh);
            }

            for (int i = 0; i < 4; i++)
            {
                vertex[i].position_.x += x;
                vertexsData.push_back(vertex[i]);
            }

            indexes.push_back(index + 0);
            indexes.push_back(index + 2);
            indexes.push_back(index + 1);

            indexes.push_back(index + 1);
            indexes.push_back(index + 2);
            indexes.push_back(index + 3);

            index += 4;

            x += charInfo.width * sx;
            //y += charInfo.height * sy;

            screenHeight = charInfo.height * sy;
        }

        screenWidth = x;
    }

    GLfloat FontAtlas::GetWidthForCharacterPosition(const char* text, unsigned int charPos)
    {
        GLfloat pos = 0;

        if (IsReady())
        {
            const char* p = text;

            float sx = 2.0f / viewWidth_;

            for (unsigned int i = 0; i < charPos && *p; i++)
            {
                int idx = (unsigned char)(*p++);
                const CharInfo& charInfo = charsMap_[idx];
                pos += charInfo.width * sx;
            }
        }
        return pos;
    }

    unsigned int FontAtlas::GetCharacterPositionForWidth(const char* text, float width)
    {
        unsigned int charPos = 0;

        if (IsReady())
        {
            GLfloat pos = 0;

            const char* p = text;

            float sx = 2.0f / viewWidth_;

            while (*p)
            {
                if (pos >= viewWidth_)
                    break;

                int idx = (unsigned char)(*p);

                const CharInfo& charInfo = charsMap_[idx];

                pos += charInfo.width * sx;

                ++p;

                ++charPos;
            }
        }
        else
        {
            charPos = strlen(text);
        }

        return charPos;
    }

}