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
#include "SharedPointers.h"

namespace pugi
{
	class xml_node;
}

namespace NSG
{
	class Technique
	{
	public:
		static const size_t MAX_PASSES = 10;
		Technique();
		~Technique();
		void Add(PPass pass);
		size_t GetNumPasses() const;
		const PASSES& GetConstPasses() const { return passes_; }
		PASSES& GetPasses() { return passes_; }
		void SetPass(unsigned int idx, PPass pass) { passes_.at(idx) = pass; }
		PPass GetPass(unsigned int idx) { return passes_.at(idx); }
		void Save(pugi::xml_node& node);
		void Load(const pugi::xml_node& node);
		bool Render();
	private:
		PASSES passes_;
	};
}