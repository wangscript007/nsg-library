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
#include <string>
#include "ft2build.h"
#include FT_FREETYPE_H
#include "GLES2Includes.h"
#include "GLES2Program.h"
#include "GLES2VertexBuffer.h"
#include "GLES2FontAtlasTexture.h"
#include "GLES2Mesh.h"
#include "Node.h"
#include "Types.h"

namespace NSG
{
	class App;
	class GLES2Text : public GLES2Mesh
	{
	public:
		GLES2Text(const char* filename, int fontSize, GLenum usage);
		~GLES2Text();
		void SetText(const std::string& text);
		GLfloat GetWidth() const { return screenWidth_; }
		GLfloat GetHeight() const { return screenHeight_; }
		GLfloat GetWidthForCharacterPosition(unsigned int charPos) const;
		unsigned int GetCharacterPositionForWidth(float width) const;
		PGLES2Texture GetAtlas() const { return pAtlas_; }
		PGLES2Program GetProgram() const { return pProgram_; }
		int GetFontSize() const { return fontSize_; }
		static void ReleaseAtlasCollection();
		GLenum GetWireFrameDrawMode() const;
		GLenum GetSolidDrawMode() const;

	private:
		App* pApp_;
		PGLES2FontAtlasTexture pAtlas_;
		PGLES2Program pProgram_;

		std::string lastText_;
		GLfloat screenWidth_;
		GLfloat screenHeight_;
		int32_t width_;
		int32_t height_;
		int fontSize_;
	};

	typedef std::shared_ptr<GLES2Text> PGLES2Text;
}