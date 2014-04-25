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
#include "GLES2Texture.h"
#include "GLES2Material.h"
#include "GLES2Mesh.h"
#include "GLES2FShader.h"
#include "GLES2Render2Texture.h"
#include <memory>

namespace NSG
{
	class GLES2Filter
	{
	public:
		GLES2Filter(PGLES2Texture input, PGLES2Texture output, const char* fragment);
		~GLES2Filter();
		void Render();
		PGLES2Material GetMaterial() const { return pMaterial_; }
	private:
        PGLES2Material pMaterial_; 
		PGLES2Mesh pMesh_;
		PGLES2Render2Texture pRender2Texture_;
		PGLES2Texture input_;
	};

	typedef std::shared_ptr<GLES2Filter> PGLES2Filter;
}