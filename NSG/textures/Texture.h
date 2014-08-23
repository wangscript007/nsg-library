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
#include "GLES2Includes.h"
#include "SharedPointers.h"
#include "GPUObject.h"

namespace NSG
{
	class Texture : public GPUObject
	{
	public:
		Texture(); 
		virtual ~Texture();
		GLuint GetID() const { return texture_; }
		GLsizei GetWidth() const { return width_; }
		GLsizei GetHeight() const { return height_; }
		GLint GetFormat() const { return format_; }
		int GetChannels() const { return channels_; }
		virtual bool IsValid() override;
		virtual void AllocateResources() override;
		virtual void ReleaseResources() override;
	protected:
		GLuint texture_;
		PResource pResource_;
		GLsizei width_;
		GLsizei height_;
		GLint format_;
		GLenum type_;
		int channels_;
	};
}