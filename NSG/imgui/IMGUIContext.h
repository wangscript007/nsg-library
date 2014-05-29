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
#include "Singleton.h"
#include "SharedPointers.h"
#include <set>

namespace NSG
{
	namespace IMGUI
	{
		struct Context : public Singleton<Context>
		{
			PState state_;
			PSkin pSkin_;
			PCamera pCamera_;
			PNode pCurrentNode_;
			PNode pRootNode_;
			PFrameColorSelection pFrameColorSelection_;		
			PTextManager pTextManager_;
			PLayoutManager pLayoutManager_;

			Context();
			~Context();
			void Begin();
			void End();
			bool IsStable() const;
			PTextMesh GetCurrentTextMesh(GLushort item);

			// Used to avoid the same ID when more than one control lays in the same line
			GLushort GetValidId(GLushort id);

		private:
			GLushort lastId_;

		};
	}
}