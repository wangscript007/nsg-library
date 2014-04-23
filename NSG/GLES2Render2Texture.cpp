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
#include "GLES2Render2Texture.h"
#include "Log.h"
#include "Check.h"
#include "App.h"
#include <assert.h>
#include <algorithm>

namespace NSG
{
	GLES2Render2Texture::GLES2Render2Texture(PGLES2Texture pTexture, bool createDepthBuffer)
	: pTexture_(pTexture),
    pApp_(App::GetPtrInstance()),
    createDepthBuffer_(createDepthBuffer)
	{
        CHECK_ASSERT(glGetError() == GL_NO_ERROR, __FILE__, __LINE__);
        CHECK_ASSERT(pTexture != nullptr, __FILE__, __LINE__);

        glGenFramebuffers(1, &framebuffer_);

        if(createDepthBuffer_)
        {
            glGenRenderbuffers(1, &depthRenderBuffer_);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);

		//////////////////////////////////////////////////////////////////////////////////
        // The color buffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pTexture_->GetID(), 0);

        if(createDepthBuffer_)
        {
            //////////////////////////////////////////////////////////////////////////////////
            // The depth buffer
            glBindRenderbuffer(GL_RENDERBUFFER, depthRenderBuffer_);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, pTexture_->GetWidth(), pTexture_->GetHeight());
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderBuffer_);
            //////////////////////////////////////////////////////////////////////////////////
        }

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if(GL_FRAMEBUFFER_COMPLETE != status)
        {
            TRACE_LOG("Frame buffer failed with error=" << status);
            CHECK_ASSERT(!"Frame buffer failed", __FILE__, __LINE__);
        }

        CHECK_ASSERT(glGetError() == GL_NO_ERROR, __FILE__, __LINE__);
	}

	GLES2Render2Texture::~GLES2Render2Texture()
	{
        if(createDepthBuffer_)
        {
            glDeleteRenderbuffers(1, &depthRenderBuffer_);
        }

		glDeleteFramebuffers(1, &framebuffer_);
	}

	void GLES2Render2Texture::Begin()
	{
        glGetFloatv(GL_COLOR_CLEAR_VALUE, &clear_color_[0]);
        glGetFloatv(GL_DEPTH_CLEAR_VALUE, &clear_depth_);
        glGetBooleanv(GL_DEPTH_TEST, &depth_enable_);
        glGetIntegerv(GL_VIEWPORT, &viewport_[0]);

		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
        glViewport(0, 0, pTexture_->GetWidth(), pTexture_->GetHeight());
        glEnable(GL_DEPTH_TEST);
        glClearColor(0, 0, 0, 0);
        glClearDepth(1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
	}

	void GLES2Render2Texture::End()
	{
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(viewport_.x, viewport_.y, viewport_.z, viewport_.w); 

        if(!depth_enable_)
        {
            glDisable(GL_DEPTH_TEST);
        }

        glClearColor(clear_color_[0], clear_color_[1], clear_color_[2], clear_color_[3]);
        glClearDepth(clear_depth_);
        //glClear(GL_DEPTH_BUFFER_BIT);
        
	}

}