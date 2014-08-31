#include "FontAtlasTexture.h"
#include "Check.h"
#include "Context.h"
#include "TextureFile.h"
#include "ResourceFile.h"
#include "ResourceMemory.h"
#include "Mesh.h"
#include "App.h"
#include "UTF8String.h"
#include "pugixml.hpp"
#include "fonts/anonymous_pro_regular_14_png.inl"
#include "fonts/anonymous_pro_regular_14_xml.inl"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <assert.h>
#include <cmath>

namespace NSG
{
    FontAtlasTexture::FontAtlasTexture(const std::string& filename)
        : viewWidth_(0),
          viewHeight_(0),
          filename_(filename)
    {
        if (filename.empty())
        {
            PResource bitmapFont(new ResourceMemory((const char*)ANONYMOUS_PRO_REGULAR_14_PNG, ANONYMOUS_PRO_REGULAR_14_PNG_SIZE));
            texture_ = PTexture(new TextureFile(bitmapFont, Texture::Flag::NONE));
            xmlResource_ = PResource(new ResourceMemory((const char*)ANONYMOUS_PRO_REGULAR_14_XML, ANONYMOUS_PRO_REGULAR_14_XML_SIZE));
        }
        else
        {
            texture_ = PTexture(new TextureFile(filename.c_str(), Texture::Flag::NONE));
            std::string::size_type idx = filename.find_last_of(".");
            if (idx != std::string::npos)
            {
                std::string xmlFilename = filename;
                xmlFilename.replace(xmlFilename.begin() + idx, xmlFilename.end(), ".xml"); //divo compatible (generated with font builder)
                xmlResource_ = PResourceFile(new ResourceFile(xmlFilename.c_str()));
            }
        }

        App::Add(this);
    }

    FontAtlasTexture::~FontAtlasTexture()
    {
        App::Remove(this);
    }

    bool FontAtlasTexture::IsReady()
    {
        if (xmlResource_ && xmlResource_->IsLoaded() && texture_->IsReady())
        {
            if (charsMap_.empty())
                ParseXML();

            return true;
        }

        return false;
    }

    bool FontAtlasTexture::ParseXML()
    {
        TRACE_LOG("FontAtlasTexture::Parsing: " << (filename_.empty() ? "internal font" : filename_));

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

            TRACE_LOG("FontAtlasTexture::Parsing done.");

            return true;
        }
    }

    bool FontAtlasTexture::GenerateMesh(const std::string& text, VertexsData& vertexsData, Indexes& indexes, GLfloat& screenWidth, GLfloat& screenHeight)
    {
        if (!IsReady())
            return false;

        vertexsData.clear();
        indexes.clear();

        screenWidth = screenHeight = 0;

        CHECK_ASSERT(viewWidth_ > 0 && viewHeight_ > 0, __FILE__, __LINE__);

        float sx = 2.0f / viewWidth_;
        float sy = 2.0f / viewHeight_;

        GLsizei textureWidth = texture_->GetWidth();
        GLsizei textureHeight = texture_->GetHeight();

        float x = 0;
        float y = 0;

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
                vertex[0].uv_ = Vertex2(ux, uy);

                vertex[1].position_ = Vertex3(offsetX + w, offsetY, 0);
                vertex[1].uv_ = Vertex2(ux + uw, uy);

                vertex[2].position_ = Vertex3(offsetX, offsetY + h, 0);
                vertex[2].uv_ = Vertex2(ux, uy + uh);

                vertex[3].position_ = Vertex3(offsetX + w, offsetY + h, 0);
                vertex[3].uv_ = Vertex2(ux + uw, uy + uh);
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

        return true;
    }

    GLfloat FontAtlasTexture::GetWidthForCharacterPosition(const char* text, unsigned int charPos)
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

    unsigned int FontAtlasTexture::GetCharacterPositionForWidth(const char* text, float width)
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

    void FontAtlasTexture::OnViewChanged(int32_t width, int32_t height)
    {
        viewWidth_ = width;
        viewHeight_ = height;
    }
}