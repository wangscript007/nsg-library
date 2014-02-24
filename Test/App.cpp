/*
-------------------------------------------------------------------------------
This file is part of nsg-library.
http://nsg-library.googlecode.com/

Copyright (c) 2014-2015 N�stor Silveira Gorski

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
#include "App.h"
#include "NSG/Log.h"
#include "NSG/GLES2Texture.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>

extern void FSMExamples();
extern void FSMTest();
extern void TimedTaskTest();
extern void QueuedTaskTest();

using namespace NSG;

namespace 
{
	const float kFovY = 45.0f;
	const float kZNear = 1.0f;
	const float kZFar = 10.0f;
}

App::App() 
: x_angle_(0),
y_angle_(0),
pCamera1_(new Camera(kFovY, kZNear, kZFar)),
pCamera2_(new Camera(kFovY, kZNear, kZFar)),
width_(1),
height_(1),
x_(0),
y_(0),
buttonDown_(false),
buttonUp_(false),
selectedIndex_(0),
scale_(1,1,1)
{
}

App::~App() 
{
	thread_.join();
}

void App::InternalTask() 
{
	FSMExamples();
	FSMTest();
	TimedTaskTest();
	QueuedTaskTest();
}

static const char kFragShaderSource[] = {
#include "shaders/gles2FragmentShader.h"
};

static const char kVertexShaderSource[] = {
#include "shaders/gles2VertexShader.h"
};

void App::Start() 
{
	TRACE_LOG("Start");

	thread_ = std::thread([this](){InternalTask();});	

	pProgram_ = PGLES2Program(new GLES2Program(kVertexShaderSource, kFragShaderSource));

	pMesh_ = PBoxMesh(new BoxMesh(pProgram_, PGLES2Texture(new GLES2Texture("cube_example.png")), GL_STATIC_DRAW));

	Vertex2 v10(1, 0);
	Vertex2 v00(0, 0);
	Vertex2 v01(0, 1);
	Vertex2 v11(1, 1);

	const float kOffset = 0.333f;

	Vertex2 base(1, 0);
	pMesh_->SetFaceUVs(BoxMesh::FRONT, (base + v10) * kOffset, (base + v00) * kOffset, (base + v01) * kOffset, (base + v11) * kOffset, false);

	base = Vertex2(1, 2);
	pMesh_->SetFaceUVs(BoxMesh::BACK, (base + v10) * kOffset, (base + v00) * kOffset, (base + v01) * kOffset, (base + v11) * kOffset, false);

	pMesh_->Redo();


    pNode1_ = PNode(new Node);
    pNode2_ = PNode(new Node);

	pTextNode0_ = PNode(new Node);
    pTextNode1_ = PNode(new Node(pTextNode0_));

    pNode1_->SetPosition(Vertex3(-2, 0, 0));
    pNode2_->SetPosition(Vertex3(2, 0, 0));

    pCamera1_->SetLookAt(Vertex3(0,5,5), Vertex3(0,0,0), Vertex3(0,1,0));
    pCamera2_->SetLookAt(Vertex3(0,5,5), Vertex3(0,0,0), Vertex3(0,1,0));
    pCamera2_->SetViewport(0.75f, 0.75f, 0.25f, 0.25f);

    pText1_ = PText(new Text("font/FreeSans.ttf", 24, GL_DYNAMIC_DRAW));
    pText2_ = PText(new Text("font/bluebold.ttf", 24, GL_STATIC_DRAW));
    pText3_ = PText(new Text("font/FreeSans.ttf", 48, GL_STATIC_DRAW));
}

void App::Update(float delta) 
{
	//TRACE_LOG("App::Update delta = " << delta);
	x_angle_ += glm::pi<float>()/10.0f * delta;
	y_angle_ += glm::pi<float>()/10.0f * delta;
	pNode1_->SetRotation(glm::angleAxis(x_angle_, Vertex3(1, 0, 0)));
	pNode2_->SetRotation(glm::angleAxis(y_angle_, Vertex3(0, 0, 1)));
	pNode1_->SetScale(scale_);

	static float factor_scale = 1;
	scale_ -= delta * 0.1f * factor_scale;

	if(scale_.x < 0.1f && factor_scale > 0)
		factor_scale *= -1;
	else if(scale_.x > 1 && factor_scale < 0)
		factor_scale *= -1;

	pTextNode0_->SetPosition(Vertex3(-pText1_->GetWidth()/2, 0, 0));
	pTextNode1_->SetRotation(glm::angleAxis(x_angle_, Vertex3(0, 0, 1)));

	//pTextNode1_->SetScale(scale_);


}

void App::LateUpdate()
{
	//TRACE_LOG("App::LateUpdate");
}

void App::RenderFrame() 
{
	//TRACE_LOG("App::RenderFrame");

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glClearColor(1, 1, 1, 1);
	glClearDepth(1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

    pCamera1_->Activate();

    pMesh_->Render(pNode1_);
    pMesh_->Render(pNode2_);

    std::stringstream ss;
    ss << "Mouse x=" << x_ << " y=" << y_;

    if(buttonDown_)
    {
    	ss << " BDown";
    }
    else if(buttonUp_)
    {
    	ss << " BUp";
    }

	ss << " S=" << std::hex << selectedIndex_;

	pText1_->Render(pTextNode1_, Color(0,0,0,1), ss.str());

    //pText1_->RenderText(Color(1,0,0,1), "(-1,-1)");

    //pText2_->RenderText(Color(0,1,0,1), "(-1,1)");

//	screenWidth = pText2_->GetWidth();
//	screenHeight = pText2_->GetHeight();

    //pText3_->RenderText(Color(0,0,1,1), "(1,-1)");
    //pText3_->RenderText(Color(0,0,1,1), "(1,1)");

    pCamera2_->Activate();

    pMesh_->Render(pNode1_);
    pMesh_->Render(pNode2_);

	pFrameColorSelection_->Begin(x_, y_);

    pCamera1_->Activate();

    pFrameColorSelection_->Render(0xFF00, pMesh_, pNode1_);
    pFrameColorSelection_->Render(0x12AB, pMesh_, pNode2_);

    pCamera2_->Activate();

    pFrameColorSelection_->Render(0x0001, pMesh_, pNode1_);
    pFrameColorSelection_->Render(0x0002, pMesh_, pNode2_);

    pFrameColorSelection_->End();
}

void App::ViewChanged(int32_t width, int32_t height) 
{
	glViewport(0, 0, width, height);

	width_ = width;
	height_ = height;

	pCamera1_->ViewChanged(width, height);
	pCamera2_->ViewChanged(width, height);

    if(!pFrameColorSelection_)
        pFrameColorSelection_ = PFrameColorSelection(new FrameColorSelection());

	pFrameColorSelection_->ViewChanged(width, height);
}

void App::OnMouseMove(double x, double y)
{
	x_=x;
	y_=y;
}

void App::OnMouseDown(double x, double y)
{
	x_=x;
	y_=y;
	
	buttonDown_ = true;
	buttonUp_ = false;

	selectedIndex_ = pFrameColorSelection_->GetSelected();
}

void App::OnMouseUp()
{
	buttonDown_ = false;
	buttonUp_ = true;
}
