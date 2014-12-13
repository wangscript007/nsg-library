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
#include "ResourceMemory.h"
#include "Check.h"
#include "Path.h"

namespace NSG
{
    ResourceMemory::ResourceMemory(const char* staticBuffer, size_t bytes)
        : staticBuffer_(staticBuffer),
          bytes_(bytes)
    {
        IsReady(); // since it is just memory then for the first time force load in constructor
    }

    ResourceMemory::~ResourceMemory()
    {
        Invalidate();
    }

	bool ResourceMemory::IsValid()
	{
		return true;
	}

    void ResourceMemory::AllocateResources()
    {
        if (staticBuffer_ != nullptr)
        {
            CHECK_ASSERT(bytes_ > 0, __FILE__, __LINE__);
            buffer_.resize(bytes_);
            memcpy(&buffer_[0], staticBuffer_, bytes_);
        }
    }
}