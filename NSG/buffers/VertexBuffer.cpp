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
#include "VertexBuffer.h"
#include "Graphics.h"
#include "Check.h"

namespace NSG 
{
	VertexBuffer::VertexBuffer(GLsizeiptr maxSize, GLsizeiptr size, const GLvoid* data, GLenum usage) 
	: Buffer(GL_ARRAY_BUFFER, maxSize, size, data, usage)
	{
		CHECK_GL_STATUS(__FILE__, __LINE__);
		SetVertexBuffer(this);
		CHECK_ASSERT(maxSize <= MAX_BUFFER_SIZE, __FILE__, __LINE__);
		glBufferData(type_, MAX_BUFFER_SIZE, nullptr, usage_);
		glBufferSubData(type_, 0, size, data);
		CHECK_GL_STATUS(__FILE__, __LINE__);
	}

	VertexBuffer::~VertexBuffer()
	{
		if(GetVertexBuffer() == this)
			SetVertexBuffer(nullptr);
	}

	bool VertexBuffer::ReallocateSpaceFor(GLsizeiptr maxSize, GLsizeiptr size, const GLvoid* data)
	{
		if(Buffer::ReallocateSpaceFor(maxSize, size, data))
		{
			const Data* obj = GetLastAllocation();

			CHECK_GL_STATUS(__FILE__, __LINE__);

			SetVertexBuffer(this);

			glBufferSubData(type_, obj->offset_, obj->size_, obj->data_);

			CHECK_GL_STATUS(__FILE__, __LINE__);

			return true;
		}

		return false;
	}

	void VertexBuffer::UnBind() 
	{ 
		glBindBuffer(GL_ARRAY_BUFFER, 0); 
	}

	void VertexBuffer::UpdateData(Buffer::Data& obj, const VertexsData& vertexes)
	{
		CHECK_GL_STATUS(__FILE__, __LINE__);

		GLsizeiptr bytes2Set = vertexes.size() * sizeof(VertexData);

		CHECK_ASSERT(bytes2Set <= obj.maxSize_, __FILE__, __LINE__);

		obj.size_ = bytes2Set;

		SetVertexBuffer(this);

		glBufferSubData(type_, obj.offset_, obj.size_, &vertexes[0]);

		CHECK_GL_STATUS(__FILE__, __LINE__);
	}
}
