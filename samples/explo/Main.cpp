/*
-------------------------------------------------------------------------------
This file is part of nsg-library.
http://github.com/woodjazz/nsg-library
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

#include "NSG.h"
int NSG_MAIN(int argc, char* argv[])
{
    using namespace NSG;

	Engine engine;
    auto window = Window::Create();
    auto resource = Resource::GetOrCreate<ResourceFile>("data/explo1.png");
    auto scene = std::make_shared<Scene>();
    auto camera = scene->CreateChild<Camera>();
    auto control = std::make_shared<CameraControl>(camera);
    auto mesh = Mesh::Create<QuadMesh>();
    auto material = std::make_shared<Material>();
    material->SetDiffuseMap(std::make_shared<Texture>(resource));
    auto pass = material->GetTechnique()->GetPass(0);
    pass->SetBlendMode(BLEND_MODE::BLEND_ALPHA);
    auto program = pass->GetProgram();
    program->EnableFlags((int)ProgramFlag::UNLIT | (int)ProgramFlag::SPHERICAL_BILLBOARD);
    auto sprite = scene->CreateChild<SceneNode>();
    sprite->SetMesh(mesh);
    sprite->SetMaterial(material);
    auto texSize = 1 / 7.f;
    Vector4 uvTransform(texSize, texSize, 0, 0);
    material->SetUVTransform(uvTransform);
    control->AutoZoom();
    auto totalTime = 0.f;
    auto fps = 1 / 24.f;
    Color color(1);
    auto slotUpdate = engine.signalUpdate_->Connect([&](float deltaTime)
    {
        if (totalTime > fps)
        {
            totalTime = 0;
            if (uvTransform.z >= 1)
            {
                uvTransform.z = 0;
                uvTransform.w += texSize;
                if (uvTransform.w >= 1)
                {
                    uvTransform.w = 0;
                    color.w = 1;
                }
            }
            material->SetUVTransform(uvTransform);
            uvTransform.z += texSize;
            if (uvTransform.w >= 0.95f-texSize)
                color.w -= texSize;
            material->SetColor(color);
        }
        else
            totalTime += deltaTime;
    });

	return engine.Run();
}