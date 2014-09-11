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
#include "Camera.h"
#include "App.h"
#include "Frustum.h"
#include "Graphics.h"

namespace NSG
{
    Camera* activeCamera = nullptr;

	Camera::Camera(const std::string& name, Scene* scene)
        : SceneNode(name, scene),
          fovy_(45),
          zNear_(0.1f),
          zFar_(250),
          xo_(0),
          yo_(0),
          xf_(1),
          yf_(1),
          isOrtho_(false),
          viewWidth_(0),
          viewHeight_(0),
          aspectRatio_(1),
          cameraIsDirty_(false)
    {
        UpdateProjection();
        App::Add(this);
    }

    Camera::~Camera()
    {
        App::Remove(this);
    }

    void Camera::EnableOrtho()
    {
        if (!isOrtho_)
        {
            isOrtho_ = true;
            UpdateProjection();
        }
    }

    void Camera::DisableOrtho()
    {
        if (isOrtho_)
        {
            isOrtho_ = false;
            UpdateProjection();
        }
    }

    void Camera::SetFov(float fovy)
    {
        if (fovy_ != fovy)
        {
            fovy_ = fovy;
            UpdateProjection();
        }
    }

    void Camera::SetNearClip(float zNear)
    {
        if (zNear_ != zNear)
        {
            zNear_ = zNear;
            UpdateProjection();
        }
    }

    void Camera::SetFarClip(float zFar)
    {
        if (zFar_ != zFar)
        {
            zFar_ = zFar;
            UpdateProjection();
        }
    }

    Camera* Camera::Deactivate()
    {
        Camera* pCurrent = activeCamera;
        activeCamera = nullptr;
        return pCurrent;
    }

    Camera* Camera::Activate(Camera* pCamera)
    {
        Camera* pCurrent = Camera::Deactivate();

        if (pCamera)
            pCamera->Activate();

        return pCurrent;

    }

    Camera* Camera::GetActiveCamera()
    {
        return activeCamera;
    }

    void Camera::SetViewportFactor(float xo, float yo, float xf, float yf)
    {
        xo_ = xo;
        yo_ = yo;
        xf_ = xf;
        yf_ = yf;
    }

    Recti Camera::GetViewport() const
    {
        return Recti((GLsizei)(viewWidth_ * xo_), (GLsizei)(viewHeight_ * yo_), (GLsizei)(viewWidth_ * xf_), (GLsizei)(viewHeight_ * yf_));
    }

    void Camera::Activate()
    {
        activeCamera = this;

        Graphics::this_->SetViewport(GetViewport());
    }

    void Camera::UpdateProjection() const
    {
        if (isOrtho_)
        {
            matProjection_ = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f,  zNear_, zFar_);
        }
        else
        {
            CHECK_ASSERT(zNear_ > 0, __FILE__, __LINE__);

            matProjection_ = glm::perspective(fovy_, aspectRatio_, zNear_, zFar_);
        }

        UpdateViewProjection();

        SetUniformsNeedUpdate();
    }

    void Camera::UpdateViewProjection() const
    {
        cameraIsDirty_ = false;

        matViewInverse_ = GetGlobalModelMatrix();
        matView_ = glm::inverse(matViewInverse_);

        matViewProjection_ = matProjection_ * matView_;
        matViewProjectionInverse_ = glm::inverse(matViewProjection_);

        frustum_ = PFrustum(new Frustum(this));
    }

    const PFrustum Camera::GetFrustum() const
    {
        if (cameraIsDirty_)
            UpdateProjection();

        return frustum_;
    }

    const Frustum* Camera::GetFrustumPointer() const
    {
        if (cameraIsDirty_)
            UpdateProjection();

        return frustum_.get();

    }

	const Matrix4& Camera::GetMatViewProjection() const
	{
		if (cameraIsDirty_)
			UpdateProjection();

		return matViewProjection_;
	}


    const Matrix4& Camera::GetMatViewProj()
    {
		if (activeCamera)
		{
			return activeCamera->GetMatViewProjection();
		}
		else
		{
			return IDENTITY_MATRIX;
		}
    }

    Matrix4 Camera::GetModelViewProjection(const Node* pNode)
    {
        if (activeCamera)
        {
            return activeCamera->GetMatViewProjection() * pNode->GetGlobalModelMatrix();
        }
        else
        {
            // if no Camera then position is in screen coordinates
            return pNode->GetGlobalModelMatrix();
        }
    }

    const Matrix4& Camera::GetMatProjection() const
    {
        if (cameraIsDirty_)
            UpdateProjection();

        return matProjection_;
    }

    Matrix4 Camera::GetInverseView()
    {
        if (activeCamera)
        {
            return activeCamera->GetInverseViewMatrix();
        }
        else
        {
            return IDENTITY_MATRIX;
        }
    }

	const Matrix4& Camera::GetViewMatrix()
	{
		if (activeCamera)
		{
			return activeCamera->GetView();
		}
		else
		{
			return IDENTITY_MATRIX;
		}

	}

    const Matrix4& Camera::GetView() const
    {
        if (cameraIsDirty_)
            UpdateProjection();

        return matView_;
    }

    const Matrix4& Camera::GetInverseViewMatrix() const
    {
        if (cameraIsDirty_)
            UpdateProjection();

        return matViewInverse_;
    }

    const Matrix4& Camera::GetViewProjectionMatrix() const
    {
        if (cameraIsDirty_)
            UpdateProjection();

        return matViewProjection_;
    }

    const Matrix4& Camera::GetViewProjectionInverseMatrix() const
    {
        if (cameraIsDirty_)
            UpdateProjection();

        return matViewProjectionInverse_;
    }

    Vertex3 Camera::ScreenToWorld(const Vertex3& screenXYZ) const
    {
        Vertex4 worldCoord = GetViewProjectionInverseMatrix() * Vertex4(screenXYZ, 1);
        return Vertex3(worldCoord.x / worldCoord.w, worldCoord.y / worldCoord.w, worldCoord.z / worldCoord.w);
    }

    Vertex3 Camera::WorldToScreen(const Vertex3& worldXYZ) const
    {
        Vertex4 screenCoord = GetViewProjectionMatrix() * Vertex4(worldXYZ, 1);
        return Vertex3(screenCoord.x / screenCoord.w, screenCoord.y / screenCoord.w, screenCoord.z / screenCoord.w);
    }

    void Camera::OnViewChanged(int32_t width, int32_t height)
    {
        if (viewWidth_ != width || viewHeight_ != height)
        {
            viewWidth_ = width;
            viewHeight_ = height;
            if (viewHeight_ > 0)
                aspectRatio_ = static_cast<float>(viewWidth_) / viewHeight_;
            else
                aspectRatio_ = 1;

            UpdateProjection();
        }
    }

    bool Camera::IsVisible(const Node& node, Mesh& mesh) const
    {
        return GetFrustum()->IsVisible(node, mesh);
    }

    void Camera::OnDirty() const
    {
        cameraIsDirty_ = true;
    }
}