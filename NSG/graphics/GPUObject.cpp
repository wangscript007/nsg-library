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
#include "GPUObject.h"
#include "Check.h"
#include "App.h"

namespace NSG 
{
	GPUObject::GPUObject()
	: isValid_(false),
	releaseCalled_(false),
	resourcesAllocated_(false)
	{
		Context::this_->Add(this);
	}
		
	GPUObject::~GPUObject()
	{
		CHECK_ASSERT(releaseCalled_ && "You shall call Release in the destructor!", __FILE__, __LINE__);	
	}

	void GPUObject::Invalidate()
	{
		isValid_ = false;
		if(resourcesAllocated_)
		{
			CHECK_GL_STATUS(__FILE__, __LINE__);
			ReleaseResources();
			CHECK_GL_STATUS(__FILE__, __LINE__);
			resourcesAllocated_ = false;
		}
	}

	bool GPUObject::IsReady()
	{
		auto viewSize = App::this_->GetViewSize();

		if(viewSize.first <= 0 || viewSize.second <= 0)
			return false;
		
		if(!isValid_)
		{
			CHECK_GL_STATUS(__FILE__, __LINE__);

			isValid_ = IsValid();

			if(isValid_)
			{
				CHECK_ASSERT(!resourcesAllocated_, __FILE__, __LINE__);
				AllocateResources();
				resourcesAllocated_ = true;
			}

			CHECK_GL_STATUS(__FILE__, __LINE__);
		}

		return isValid_;
	}

	void GPUObject::Release()
	{
		releaseCalled_ = true;

		if(isValid_)
		{
			ReleaseResources();
		}
	}
}