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

namespace NSG
{
	class CameraControl
	{
	public:
		CameraControl(PCamera camera);
		~CameraControl();
		void SetWindow(Window* window);
		void AutoZoom();
		void OnUpdate(float deltaTime);
		void OnKey(int key, int action, int modifier);
		void OnMultiGesture(int timestamp, float x, float y, float dTheta, float dDist, int numFingers);
		void OnMousewheel(float x, float y);
		void OnMouseUp(int button, float x, float y);
		void OnMouseDown(int button, float x, float y);
		void OnMousemoved(float x, float y);
		SignalMouseMoved::PSlot slotMouseMoved_;
		SignalMouseDown::PSlot slotMouseDown_;
		SignalMouseUp::PSlot slotMouseUp_;
		SignalMouseWheel::PSlot slotMouseWheel_;
		SignalMultiGesture::PSlot slotMultiGesture_;
		SignalKey::PSlot slotKey_;
		SignalUpdate::PSlot slotUpdate_;
	private:
		void SetPosition(const Vertex3& position);
		void SetSphereCenter(bool centerObj);
		void Move(float x, float y);
		float lastX_;
		float lastY_;
		bool leftButtonDown_;
		bool altKeyDown_;
		bool shiftKeyDown_;
		PPointOnSphere pointOnSphere_;
		PCamera camera_;
		bool updateOrientation_;
		Window* window_;
		SignalWindow::PSlot slotWindowCreated_;
	};
}