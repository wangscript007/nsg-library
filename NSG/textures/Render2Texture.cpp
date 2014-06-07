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
#include "Render2Texture.h"
#include "Log.h"
#include "Check.h"
#include "App.h"
#include "Context.h"
#include "Texture.h"
#include "Graphics.h"
#include <algorithm>

namespace NSG
{
	Render2Texture::Render2Texture(PTexture pTexture, bool createDepthBuffer)
	: pTexture_(pTexture),
    createDepthBuffer_(createDepthBuffer),
    enabled_(false)
	{
	}

	Render2Texture::~Render2Texture()
	{
        Context::this_->Remove(this);
	}

    bool Render2Texture::IsValid()
    {
        return pTexture_->IsReady();
    }

    void Render2Texture::AllocateResources()
    {
        CHECK_GL_STATUS(__FILE__, __LINE__);

        auto windowSize = App::this_->GetViewSize();

        CHECK_ASSERT(windowSize.first > 0 && windowSize.second > 0, __FILE__, __LINE__);

        CHECK_ASSERT(pTexture_ != nullptr, __FILE__, __LINE__);

        glGenFramebuffers(1, &framebuffer_);

        if(createDepthBuffer_)
        {
            glGenRenderbuffers(1, &depthRenderBuffer_);
        }

        CHECK_GL_STATUS(__FILE__, __LINE__);
        
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);

        CHECK_GL_STATUS(__FILE__, __LINE__);

        //////////////////////////////////////////////////////////////////////////////////
        // The color buffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pTexture_->GetID(), 0);

        CHECK_GL_STATUS(__FILE__, __LINE__);

        if(createDepthBuffer_)
        {
            //////////////////////////////////////////////////////////////////////////////////
            // The depth buffer
            glBindRenderbuffer(GL_RENDERBUFFER, depthRenderBuffer_);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, pTexture_->GetWidth(), pTexture_->GetHeight());
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderBuffer_);
            //////////////////////////////////////////////////////////////////////////////////
        }

        CHECK_GL_STATUS(__FILE__, __LINE__);

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if(GL_FRAMEBUFFER_COMPLETE != status)
        {
            TRACE_LOG("Frame buffer failed with error=0x" << std::hex << status);
            CHECK_ASSERT(!"Frame buffer failed", __FILE__, __LINE__);
        }

        CHECK_GL_STATUS(__FILE__, __LINE__);
    }

    void Render2Texture::ReleaseResources()
    {
        CHECK_GL_STATUS(__FILE__, __LINE__);

        if(createDepthBuffer_)
        {
            glDeleteRenderbuffers(1, &depthRenderBuffer_);
        }

        glDeleteFramebuffers(1, &framebuffer_);

        CHECK_GL_STATUS(__FILE__, __LINE__);

        enabled_ = false;
    }

	void Render2Texture::Begin()
	{
        if(IsReady())
        {
            CHECK_GL_STATUS(__FILE__, __LINE__);

            glGetIntegerv(GL_VIEWPORT, &viewport_[0]);

    		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
            glViewport(0, 0, pTexture_->GetWidth(), pTexture_->GetHeight());

            ClearAllBuffers();

            CHECK_GL_STATUS(__FILE__, __LINE__);

            enabled_ = true;
        }
        
	}

	void Render2Texture::End()
	{
        if(IsReady() && enabled_)
        {
            CHECK_GL_STATUS(__FILE__, __LINE__);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(viewport_.x, viewport_.y, viewport_.z, viewport_.w); 

            CHECK_GL_STATUS(__FILE__, __LINE__);

            enabled_ = false;
        }        
	}
}