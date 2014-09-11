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
#include "Types.h"
#include "UniformsUpdate.h"
#include "NonCopyable.h"
#include <vector>
#include <string>

namespace NSG
{
	class Node : NonCopyable, public UniformsUpdate
	{
	public:
		Node(const std::string& name = "");
		virtual ~Node();
		IdType GetId() const { return id_;  }
		virtual void OnDirty() const {};
		void Translate(const Vector3& delta, TransformSpace space = TS_LOCAL);
		void SetPosition(const Vertex3& position);
		const Vertex3& GetPosition() const { return position_; }
		void SetOrientation(const Quaternion& q);
		const Quaternion& GetOrientation() const { return q_; };
		void SetScale(const Vertex3& scale);
		const Vertex3& GetScale() const { return scale_; }
		void SetGlobalPosition(const Vertex3& position);
		void SetGlobalOrientation(const Quaternion& q);
		void SetGlobalScale(const Vertex3& scale);
		const Vertex3& GetGlobalPosition() const;
		const Quaternion& GetGlobalOrientation() const;
		Vertex3 GetGlobalScale() const;
		const Matrix4& GetGlobalModelMatrix() const;
		const Matrix3& GetGlobalModelInvTranspMatrix() const;
		const Matrix4& GetGlobalModelInvMatrix() const;
		static Vertex3 UP;
		void SetLookAt(const Vertex3& lookAtPosition, const Vertex3& up = UP);
		void SetGlobalPositionAndLookAt(const Vertex3& newPosition, const Vertex3& lookAtPosition, const Vertex3& up = UP);
		const Vertex3& GetLookAtDirection() const;
		void SetInheritScale(bool inherit);
		bool IsPointInsideBB(const Vertex3& point) const;
		PNode GetChild(size_t idx) const { return children_.at(idx); }
		bool IsDirty() const { return dirty_; }
		virtual void Save(pugi::xml_node& node);
		void SetName(const std::string& name) { name_ = name_; }
		const std::string& GetName() const { return name_; }
		void SetParent(PNode parent);
		void SetParent(Node* parent);
		Node* GetParent() const { return parent_; }
		void AddChild(PNode node);
		void ClearAllChildren();
    protected:
		Node* parent_;
        std::vector<PNode> children_;
	private:
		void RemoveChild(Node* node);
		void RemoveFromParent();
		void MarkAsDirty();
		void Update(bool updateChildren = true) const;
		IdType id_;	
        mutable Matrix4 globalModel_;	
        mutable Matrix4 globalModelInv_;
		mutable Matrix3 globalModelInvTransp_;
		Vertex3 position_;
		Quaternion q_;
		Vertex3 scale_;
		mutable Vertex3 globalPosition_;
		mutable Quaternion globalOrientation_;
		mutable Vertex3 globalScale_;
		mutable Vertex3 lookAtDirection_;
		bool inheritScale_;
		mutable bool dirty_;
		std::string name_;
	};
}
