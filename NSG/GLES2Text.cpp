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
#include "GLES2Text.h"
#include "Log.h"
#include "App.h"
#include "GLES2Camera.h"
#include <algorithm>
#include <vector>
#include <map>

const char s_fragShaderSource[] = {
#include "shaders/gles2TextFragmentShader.h"
};

const char s_vertexShaderSource[] = {
#include "shaders/gles2TextVertexShader.h"
};

namespace NSG
{
	typedef std::pair<std::string, int> Key;
	typedef std::map<Key, PGLES2Texture> Atlas;
	Atlas fontAtlas;

	GLES2Text::GLES2Text(const char* filename, int fontSize, GLenum usage)
	: pProgram_(new GLES2Program(s_vertexShaderSource, s_fragShaderSource)),
	texture_loc_(pProgram_->GetUniformLocation("u_texture")),
	position_loc_(pProgram_->GetAttributeLocation("a_position")),
	color_loc_(pProgram_->GetUniformLocation("u_color")),
	mvp_loc_(pProgram_->GetUniformLocation("u_mvp")),
	screenWidth_(0),
	screenHeight_(0),
	usage_(usage),
	width_(0),
	height_(0)
    {
    	Key k(filename, fontSize);
    	auto it = fontAtlas.find(k);
    	if(it != fontAtlas.end())
    	{
    		pAtlas_ = it->second;
    	}
    	else
    	{
    		pAtlas_ = PGLES2Texture(new GLES2Texture(filename, true, fontSize));
			fontAtlas.insert(Atlas::value_type(k, pAtlas_));
    	}
	}

	GLES2Text::~GLES2Text() 
	{
	}

	void GLES2Text::Render(PNode pNode, Color color, const std::string& text) 
	{
		Render(pNode.get(), color, text);
	}

	void GLES2Text::Render(Node* pNode, Color color, const std::string& text) 
	{
		if(!pAtlas_->IsReady() || text.empty())
			return;

		auto viewSize = App::GetPtrInstance()->GetViewSize();

		if(lastText_ != text || viewSize.first != width_ || viewSize.second != height_)
		{
			width_ = viewSize.first;
			height_ = viewSize.second;
			lastText_.clear();
			pVBuffer_ = nullptr;
		}

		if(!pVBuffer_ && width_ > 0 && height_ > 0)
		{
			float x = 0;
			float y = 0;

			float sx = 2.0/width_;
		    float sy = 2.0/height_;    

	        size_t length = 6 * text.size();

	        coords_.clear();

	        coords_.resize(length);

            screenHeight_ = 0;

			int c = 0;

			const GLES2Texture::CharsInfo& charInfo = pAtlas_->GetCharInfo();
			int atlasWidth = pAtlas_->GetAtlasWidth();
			int atlasHeight = pAtlas_->GetAtlasHeight();

			for(const char *p = text.c_str(); *p; p++) 
			{ 
				float x2 =  x + charInfo[*p].bl * sx;
				float y2 = -y - charInfo[*p].bt * sy;
				float w = charInfo[*p].bw * sx;
				float h = charInfo[*p].bh * sy;

				/* Advance the cursor to the start of the next character */
				x += charInfo[*p].ax * sx;
				y += charInfo[*p].ay * sy;

				/* Skip glyphs that have no pixels */
				if(!w || !h)
					continue;

				int idx = (int)*p;

		        Point point1 = {x2, -y2, charInfo[*p].tx, charInfo[*p].ty};
				Point point2 = {x2 + w, -y2, charInfo[*p].tx + charInfo[*p].bw / atlasWidth, charInfo[*p].ty};
				Point point3 = {x2, -y2 - h, charInfo[*p].tx, charInfo[*p].ty + charInfo[*p].bh / atlasHeight};
				Point point4 = {x2 + w, -y2, charInfo[*p].tx + charInfo[*p].bw / atlasWidth, charInfo[*p].ty};
				Point point5 = {x2, -y2 - h, charInfo[*p].tx, charInfo[*p].ty + charInfo[*p].bh / atlasHeight};
				Point point6 = {x2 + w, -y2 - h, charInfo[*p].tx + charInfo[*p].bw / atlasWidth, charInfo[*p].ty + charInfo[*p].bh / atlasHeight};

	            coords_[c++] = point1;
	            coords_[c++] = point2;
	            coords_[c++] = point3;
	            coords_[c++] = point4;
	            coords_[c++] = point5;
	            coords_[c++] = point6;

                screenHeight_ = std::max(screenHeight_, h);
			}

            screenWidth_ = x;

			pVBuffer_ = PGLES2VertexBuffer(new GLES2VertexBuffer(sizeof(Point) * coords_.size(), &coords_[0], usage_));

			lastText_ = text;
		}

		if(pVBuffer_ != nullptr)
		{
            //bool isDepthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
            //glEnable(GL_DEPTH_TEST);

            assert(glGetError() == GL_NO_ERROR);

			GLboolean isBlendEnabled = glIsEnabled(GL_BLEND);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);	

			UseProgram useProgram(*pProgram_);

			Matrix4 mvp = GLES2Camera::GetModelViewProjection(pNode);
			glUniformMatrix4fv(mvp_loc_, 1, GL_FALSE, glm::value_ptr(mvp));

			glActiveTexture(GL_TEXTURE0);

			BindTexture bindTexture(*pAtlas_);
			glUniform1i(texture_loc_, 0);
			glUniform4fv(color_loc_, 1, &color[0]);

			BindBuffer bindVBuffer(*pVBuffer_);

			glEnableVertexAttribArray(position_loc_);
			glVertexAttribPointer(position_loc_, 4, GL_FLOAT, GL_FALSE, 0, 0);

			glDrawArrays(GL_TRIANGLES, 0, coords_.size());

			if(!isBlendEnabled)
				glDisable(GL_BLEND);

            //if(!isDepthTestEnabled)
            //    glDisable(GL_DEPTH_TEST);
            
            assert(glGetError() == GL_NO_ERROR);
		}
	}	
}
