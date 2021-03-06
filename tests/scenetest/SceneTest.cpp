/*
-------------------------------------------------------------------------------
This file is part of nsg-library.
http://github.com/woodjazz/nsg-library

Copyright (c) 2014-2017 Néstor Silveira Gorski

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
using namespace NSG;

static void Test01() {
    auto scene = std::make_shared<Scene>();

    auto sphereMesh(Mesh::Create<SphereMesh>());
    auto boxMesh(Mesh::Create<BoxMesh>());

    auto node1s = scene->CreateChild<SceneNode>();
    node1s->SetMesh(sphereMesh);
    node1s->SetPosition(Vertex3(0, 0, 1));

    auto node1b = scene->CreateChild<SceneNode>();
    node1b->SetMesh(boxMesh);
    node1b->SetPosition(Vertex3(0, 0, 1));

    auto camera = scene->CreateChild<Camera>();

    std::vector<SceneNode*> visibles;
    scene->GetVisibleNodes(camera.get(), visibles);
    CHECK_CONDITION(visibles.size() == 0);

    node1s->SetPosition(Vertex3(0, 0, -10));

    scene->GetVisibleNodes(camera.get(), visibles);
    CHECK_CONDITION(visibles.size() == 1);

    node1b->SetPosition(Vertex3(0, 0, -100));
    scene->GetVisibleNodes(camera.get(), visibles);
    CHECK_CONDITION(visibles.size() == 2);

    node1s->SetPosition(Vertex3(0, 0, 1));
    scene->GetVisibleNodes(camera.get(), visibles);
    CHECK_CONDITION(visibles.size() == 1);

    node1s->SetPosition(Vertex3(0, 0, 0.8f));
    scene->GetVisibleNodes(camera.get(), visibles);
    CHECK_CONDITION(visibles.size() == 2);

    camera->SetGlobalLookAtPosition(Vector3(0, 0, 1));
    scene->GetVisibleNodes(camera.get(), visibles);
    CHECK_CONDITION(visibles.size() == 1);

    node1s->SetPosition(Vertex3(0, 0, -0.8f));
    scene->GetVisibleNodes(camera.get(), visibles);
    CHECK_CONDITION(visibles.size() == 1);

    camera->SetPosition(Vertex3(0, 0, 1));

    scene->GetVisibleNodes(camera.get(), visibles);
    CHECK_CONDITION(visibles.size() == 0);
}

static void Test02() {
    auto scene = std::make_shared<Scene>("scene2");

    auto sphereMesh(Mesh::Create<SphereMesh>());
    auto boxMesh(Mesh::Create<BoxMesh>());

    auto node1s = scene->GetOrCreateChild<SceneNode>("node 1");
    node1s->SetMesh(sphereMesh);
    node1s->SetPosition(Vertex3(100, 0, -100));

    auto node1b = scene->GetOrCreateChild<SceneNode>("node 2");
    node1b->SetMesh(boxMesh);
    node1b->SetPosition(Vertex3(-100, 0, -100));

    std::vector<SceneNode*> nodes;
    Vertex3 origin(0);
    Vector3 direction1s(node1s->GetGlobalPosition() - origin);
    Ray ray(origin, direction1s);
    CHECK_CONDITION(scene->GetFastRayNodesIntersection(ray, nodes));
    CHECK_CONDITION(nodes.size() == 1);
    CHECK_CONDITION(nodes[0] == node1s.get());

    {
        Ray ray(origin, direction1s, 140);
        CHECK_CONDITION(!scene->GetFastRayNodesIntersection(ray, nodes));
    }

    {
        Ray ray(origin, direction1s, 141);
        CHECK_CONDITION(scene->GetFastRayNodesIntersection(ray, nodes));
        CHECK_CONDITION(nodes.size() == 1);
        CHECK_CONDITION(nodes[0] == node1s.get());
    }

    ray = Ray(origin, -direction1s);
    CHECK_CONDITION(!scene->GetFastRayNodesIntersection(ray, nodes));

    Vector3 direction1b(node1b->GetGlobalPosition() - origin);
    ray = Ray(origin, direction1b);
    CHECK_CONDITION(scene->GetFastRayNodesIntersection(ray, nodes));
    CHECK_CONDITION(nodes.size() == 1);
    CHECK_CONDITION(nodes[0] == node1b.get());

    ray = Ray(origin, -direction1b);
    CHECK_CONDITION(!scene->GetFastRayNodesIntersection(ray, nodes));
}

static void Test03() {
    auto scene = std::make_shared<Scene>("scene3");

    {
        auto sphereMesh(Mesh::Create<SphereMesh>());
        auto node1s = scene->GetOrCreateChild<SceneNode>("node 1");
        node1s->SetMesh(sphereMesh);

        Vertex3 origin(0, 0, 100);
        Vector3 direction1s(node1s->GetGlobalPosition() - origin);
        Ray ray(origin, direction1s);
        std::vector<RayNodeResult> result;
        CHECK_CONDITION(scene->GetPreciseRayNodesIntersection(ray, result));
        CHECK_CONDITION(result.size() == 1);
        CHECK_CONDITION(result[0].node_ == node1s.get());
        CHECK_CONDITION(Abs(result[0].distance_ - 99) < 0.1f);

        direction1s =
            node1s->GetGlobalPosition() - Vector3(0.45f, 0, 0) - origin;
        ray = Ray(origin, direction1s);

        CHECK_CONDITION(scene->GetPreciseRayNodesIntersection(ray, result));
        CHECK_CONDITION(result.size() == 1);
        CHECK_CONDITION(result[0].node_ == node1s.get());
        CHECK_CONDITION(result[0].distance_ > 99);
    }
}

static void Test04() {
    auto scene = std::make_shared<Scene>("scene4");

    {
        const float RADIUS = 0.5f;
        auto sphereMesh(Mesh::Create<SphereMesh>());
        sphereMesh->Set(RADIUS, 64);
        std::vector<PSceneNode> nodes;
        for (int i = 0; i < 100; i++) {
            std::stringstream ss;
            ss << i;
            auto node = scene->GetOrCreateChild<SceneNode>(ss.str());
            nodes.push_back(node);
            node->SetPosition(Vertex3(0, 0, -i));
            node->SetMesh(sphereMesh);
        }

        Vertex3 origin(0, 0, 100);
        Vector3 direction1s(Vector3(0, 0, -1));
        Ray ray(origin, direction1s);
        RayNodeResult closest;
        CHECK_CONDITION(scene->GetClosestRayNodeIntersection(ray, closest));
        CHECK_CONDITION(closest.node_->GetName() == "0");
        CHECK_CONDITION(Abs(closest.distance_ - (99 + RADIUS)) < 0.01f);

        ray = Ray(Vertex3(0, 0, -101), -direction1s);
        CHECK_CONDITION(scene->GetClosestRayNodeIntersection(ray, closest));
        CHECK_CONDITION(closest.node_->GetName() == "99");
        CHECK_CONDITION(Abs(closest.distance_ - (1 + RADIUS)) < 0.01f);

        origin = Vertex3(5000, 0, 100);
        direction1s = Vertex3(0, 0, -50) - origin;
        ray = Ray(origin, direction1s);
        CHECK_CONDITION(scene->GetClosestRayNodeIntersection(ray, closest));
        CHECK_CONDITION(closest.node_->GetName() == "50");
        float d = direction1s.Length() - RADIUS;
        CHECK_CONDITION(Abs(closest.distance_ - d) < 0.01f);

        origin = Vertex3(5000, 0, -50);
        direction1s = Vertex3(0, 0, -50) - origin;
        ray = Ray(origin, direction1s);
        CHECK_CONDITION(scene->GetClosestRayNodeIntersection(ray, closest));
        CHECK_CONDITION(closest.node_->GetName() == "50");
        d = direction1s.Length() - RADIUS;
        CHECK_CONDITION(Abs(closest.distance_ - d) < 0.01f);

        origin = Vertex3(5000, 0, -50);
        direction1s = Vertex3(0, 0, -50.49f) - origin;
        ray = Ray(origin, direction1s);
        CHECK_CONDITION(scene->GetClosestRayNodeIntersection(ray, closest));
        CHECK_CONDITION(closest.node_->GetName() == "50");
        d = direction1s.Length() - 0.01f;
        CHECK_CONDITION(Abs(closest.distance_ - d) < 0.1f);

        origin = Vertex3(5000, 0, -50);
        direction1s = Vertex3(0, 0, -50.51f) - origin;
        ray = Ray(origin, direction1s);
        CHECK_CONDITION(scene->GetClosestRayNodeIntersection(ray, closest));
        CHECK_CONDITION(closest.node_->GetName() == "51");
        d = direction1s.Length() - 0.01f;
        CHECK_CONDITION(Abs(closest.distance_ - d) < 0.1f);

        origin = Vertex3(5000, 0, -50);
        direction1s = Vertex3(0, 0, -49.51f) - origin;
        ray = Ray(origin, direction1s);
        CHECK_CONDITION(scene->GetClosestRayNodeIntersection(ray, closest));
        CHECK_CONDITION(closest.node_->GetName() == "50");
        d = direction1s.Length() - 0.01f;
        CHECK_CONDITION(Abs(closest.distance_ - d) < 0.1f);
    }
}

static void Test05() {
    auto scene = std::make_shared<Scene>("scene5");

    {
        const float RADIUS = 0.5f;
        auto sphereMesh(Mesh::Create<SphereMesh>());
        sphereMesh->Set(RADIUS, 64);
        auto node = scene->GetOrCreateChild<SceneNode>("0");
        node->SetPosition(Vertex3(0, 0, 0));
        const float SCALE = 0.1f;
        node->SetScale(Vertex3(SCALE));
        node->SetMesh(sphereMesh);

        Vertex3 origin(0, 0, 1);
        Vector3 direction1s(Vector3(0, 0, -1));
        Ray ray(origin, direction1s);
        RayNodeResult closest;
        CHECK_CONDITION(scene->GetClosestRayNodeIntersection(ray, closest));
        CHECK_CONDITION(closest.node_->GetName() == "0");
        CHECK_CONDITION(Abs(closest.distance_ - (1 - RADIUS * SCALE)) < 0.01f);
    }
}

static void Test06() {
    auto scene = std::make_shared<Scene>();
    auto quad = Mesh::Create<QuadMesh>();
    quad->Set(0.5f);
    auto nodeLU = scene->CreateChild<SceneNode>();
    nodeLU->SetMesh(quad);
    nodeLU->SetPosition(Vector3(-0.5f, 0.5f, 0));
    auto nodeRU = scene->CreateChild<SceneNode>();
    nodeRU->SetMesh(quad);
    nodeRU->SetPosition(Vector3(0.5f, 0.5f, 0));
    auto nodeLB = scene->CreateChild<SceneNode>();
    nodeLB->SetMesh(quad);
    nodeLB->SetPosition(Vector3(-0.5f, -0.5f, 0));
    auto nodeRB = scene->CreateChild<SceneNode>();
    nodeRB->SetMesh(quad);
    nodeRB->SetPosition(Vector3(0.5f, -0.5f, 0));

    {
        Ray ray = Camera::GetRay(0, 0);
        std::vector<RayNodeResult> result;
        CHECK_CONDITION(!scene->GetPreciseRayNodesIntersection(ray, result));
    }

    {
        Ray ray = Camera::GetRay(-0.5f, 0.5f);
        std::vector<RayNodeResult> result;
        CHECK_CONDITION(scene->GetPreciseRayNodesIntersection(ray, result));
        CHECK_CONDITION(result.size() == 1);
        CHECK_CONDITION(result[0].node_ == nodeLU.get());
    }

    {
        Ray ray = Camera::GetRay(0.5f, 0.5f);
        std::vector<RayNodeResult> result;
        CHECK_CONDITION(scene->GetPreciseRayNodesIntersection(ray, result));
        CHECK_CONDITION(result.size() == 1);
        CHECK_CONDITION(result[0].node_ == nodeRU.get());
    }

    {
        Ray ray = Camera::GetRay(-0.5f, -0.5f);
        std::vector<RayNodeResult> result;
        CHECK_CONDITION(scene->GetPreciseRayNodesIntersection(ray, result));
        CHECK_CONDITION(result.size() == 1);
        CHECK_CONDITION(result[0].node_ == nodeLB.get());
    }

    {
        Ray ray = Camera::GetRay(0.5f, -0.5f);
        std::vector<RayNodeResult> result;
        CHECK_CONDITION(scene->GetPreciseRayNodesIntersection(ray, result));
        CHECK_CONDITION(result.size() == 1);
        CHECK_CONDITION(result[0].node_ == nodeRB.get());
    }
}

static void Test07() {
    auto xml = Resource::GetOrCreate<ResourceFile>("data/AnonymousPro132.xml");
    auto atlas = std::make_shared<FontXMLAtlas>();
    atlas->SetXML(xml);
    auto atlasResource =
        Resource::GetOrCreate<ResourceFile>("data/AnonymousPro132.png");
    auto atlasTexture = std::make_shared<Texture2D>(atlasResource);
    atlas->SetTexture(atlasTexture);

    auto textCenter =
        atlas->GetOrCreateMesh("A", CENTER_ALIGNMENT, MIDDLE_ALIGNMENT);
    auto textLeftTop =
        atlas->GetOrCreateMesh("B", LEFT_ALIGNMENT, TOP_ALIGNMENT);
    auto textRightTop =
        atlas->GetOrCreateMesh("C", RIGHT_ALIGNMENT, TOP_ALIGNMENT);
    auto textLeftBottom =
        atlas->GetOrCreateMesh("D", LEFT_ALIGNMENT, BOTTOM_ALIGNMENT);
    auto textRightBottom =
        atlas->GetOrCreateMesh("E", RIGHT_ALIGNMENT, BOTTOM_ALIGNMENT);

    auto scene = std::make_shared<Scene>("scene");

    auto nodeCenter = scene->GetOrCreateChild<SceneNode>("nodeCenter");
    auto nodeLeftTop = scene->GetOrCreateChild<SceneNode>("nodeLeftTop");
    auto nodeRightTop = scene->GetOrCreateChild<SceneNode>("nodeRightTop");
    auto nodeLeftBottom = scene->GetOrCreateChild<SceneNode>("nodeLeftBottom");
    auto nodeRightBottom =
        scene->GetOrCreateChild<SceneNode>("nodeRightBottom");

    nodeCenter->SetMesh(textCenter);
    nodeLeftTop->SetMesh(textLeftTop);
    nodeRightTop->SetMesh(textRightTop);
    nodeLeftBottom->SetMesh(textLeftBottom);
    nodeRightBottom->SetMesh(textRightBottom);

    auto material = Material::Create();
    material->SetFontAtlas(atlas);

    nodeCenter->SetMaterial(material);
    nodeLeftTop->SetMaterial(material);
    nodeRightTop->SetMaterial(material);
    nodeLeftBottom->SetMaterial(material);
    nodeRightBottom->SetMaterial(material);

    atlas->SetViewSize(256, 256);

    {
        Ray ray = Camera::GetRay(0, 0.f);
        std::vector<RayNodeResult> result;
        CHECK_CONDITION(scene->GetPreciseRayNodesIntersection(ray, result));
        CHECK_CONDITION(result.size() == 1);
        CHECK_CONDITION(result[0].node_ == nodeCenter.get());
    }

    {
        Ray ray = Camera::GetRay(-0.9f, 0.9f);
        std::vector<RayNodeResult> result;
        CHECK_CONDITION(scene->GetPreciseRayNodesIntersection(ray, result));
        CHECK_CONDITION(result.size() == 1);
        CHECK_CONDITION(result[0].node_ == nodeLeftTop.get());
    }

    {
        Ray ray = Camera::GetRay(0.9f, 0.9f);
        std::vector<RayNodeResult> result;
        CHECK_CONDITION(scene->GetPreciseRayNodesIntersection(ray, result));
        CHECK_CONDITION(result.size() == 1);
        CHECK_CONDITION(result[0].node_ == nodeRightTop.get());
    }

    {
        Ray ray = Camera::GetRay(-0.9f, -0.9f);
        std::vector<RayNodeResult> result;
        CHECK_CONDITION(scene->GetPreciseRayNodesIntersection(ray, result));
        CHECK_CONDITION(result.size() == 1);
        CHECK_CONDITION(result[0].node_ == nodeLeftBottom.get());
    }

    {
        Ray ray = Camera::GetRay(0.9f, -0.9f);
        std::vector<RayNodeResult> result;
        CHECK_CONDITION(scene->GetPreciseRayNodesIntersection(ray, result));
        CHECK_CONDITION(result.size() == 1);
        CHECK_CONDITION(result[0].node_ == nodeRightBottom.get());
    }
}

static void Test08() {
    auto xml = Resource::GetOrCreate<ResourceFile>("data/AnonymousPro132.xml");
    auto atlas = std::make_shared<FontXMLAtlas>();
    atlas->SetXML(xml);
    auto atlasResource =
        Resource::GetOrCreate<ResourceFile>("data/AnonymousPro132.png");
    auto atlasTexture = std::make_shared<Texture2D>(atlasResource);
    atlas->SetTexture(atlasTexture);

    auto textCenter =
        atlas->GetOrCreateMesh("A", CENTER_ALIGNMENT, MIDDLE_ALIGNMENT);
    auto textLeftTop =
        atlas->GetOrCreateMesh("B", LEFT_ALIGNMENT, TOP_ALIGNMENT);
    auto textRightTop =
        atlas->GetOrCreateMesh("C", RIGHT_ALIGNMENT, TOP_ALIGNMENT);
    auto textLeftBottom =
        atlas->GetOrCreateMesh("D", LEFT_ALIGNMENT, BOTTOM_ALIGNMENT);
    auto textRightBottom =
        atlas->GetOrCreateMesh("E", RIGHT_ALIGNMENT, BOTTOM_ALIGNMENT);

    auto scene = std::make_shared<Scene>("scene");
    auto camera = scene->CreateChild<Camera>();
    camera->EnableOrtho();
    camera->SetNearClip(-0.1f);

    auto nodeCenter = scene->GetOrCreateChild<SceneNode>("nodeCenter");
    auto nodeLeftTop = scene->GetOrCreateChild<SceneNode>("nodeLeftTop");
    auto nodeRightTop = scene->GetOrCreateChild<SceneNode>("nodeRightTop");
    auto nodeLeftBottom = scene->GetOrCreateChild<SceneNode>("nodeLeftBottom");
    auto nodeRightBottom =
        scene->GetOrCreateChild<SceneNode>("nodeRightBottom");

    nodeCenter->SetMesh(textCenter);
    nodeLeftTop->SetMesh(textLeftTop);
    nodeRightTop->SetMesh(textRightTop);
    nodeLeftBottom->SetMesh(textLeftBottom);
    nodeRightBottom->SetMesh(textRightBottom);

    auto material = Material::Create();
    material->SetFontAtlas(atlas);

    nodeCenter->SetMaterial(material);
    nodeLeftTop->SetMaterial(material);
    nodeRightTop->SetMaterial(material);
    nodeLeftBottom->SetMaterial(material);
    nodeRightBottom->SetMaterial(material);

    atlas->SetViewSize(256, 256);
    camera->SetAspectRatio(256, 256);

    {
        Ray ray = Camera::GetRay(0, 0.f);
        std::vector<RayNodeResult> result;
        CHECK_CONDITION(scene->GetPreciseRayNodesIntersection(ray, result));
        CHECK_CONDITION(result.size() == 1);
        CHECK_CONDITION(result[0].node_ == nodeCenter.get());
    }

    {
        Ray ray = Camera::GetRay(-0.9f, 0.9f);
        std::vector<RayNodeResult> result;
        CHECK_CONDITION(scene->GetPreciseRayNodesIntersection(ray, result));
        CHECK_CONDITION(result.size() == 1);
        CHECK_CONDITION(result[0].node_ == nodeLeftTop.get());
    }

    {
        Ray ray = Camera::GetRay(0.9f, 0.9f);
        std::vector<RayNodeResult> result;
        CHECK_CONDITION(scene->GetPreciseRayNodesIntersection(ray, result));
        CHECK_CONDITION(result.size() == 1);
        CHECK_CONDITION(result[0].node_ == nodeRightTop.get());
    }

    {
        Ray ray = Camera::GetRay(-0.9f, -0.9f);
        std::vector<RayNodeResult> result;
        CHECK_CONDITION(scene->GetPreciseRayNodesIntersection(ray, result));
        CHECK_CONDITION(result.size() == 1);
        CHECK_CONDITION(result[0].node_ == nodeLeftBottom.get());
    }

    {
        Ray ray = Camera::GetRay(0.9f, -0.9f);
        std::vector<RayNodeResult> result;
        CHECK_CONDITION(scene->GetPreciseRayNodesIntersection(ray, result));
        CHECK_CONDITION(result.size() == 1);
        CHECK_CONDITION(result[0].node_ == nodeRightBottom.get());
    }
}

static void Test09() {
    auto scene = std::make_shared<Scene>();

    auto sphereMesh(Mesh::Create<SphereMesh>());
    auto boxMesh(Mesh::Create<BoxMesh>());
    auto material1 = Material::Create();

    std::vector<PSceneNode> spheres;
    const int Spheres = 100;
    for (int i = 0; i < Spheres; i++) {
        auto node = scene->CreateChild<SceneNode>();
        node->SetMesh(sphereMesh);
        node->SetPosition(Vertex3(0, 0, i + 1));
        node->SetMaterial(material1);
        spheres.push_back(node);
    }
    std::vector<PSceneNode> boxes;
    const int Boxes = 50;
    for (int i = 0; i < Boxes; i++) {
        auto node = scene->CreateChild<SceneNode>();
        node->SetMesh(boxMesh);
        node->SetPosition(Vertex3(0, 0, -(i + 1)));
        node->SetMaterial(material1);
        boxes.push_back(node);
    }

    auto camera = scene->CreateChild<Camera>();
    camera->SetFarClip(200);

    std::vector<SceneNode*> visibles;
    scene->GetVisibleNodes(camera.get(), visibles);
    CHECK_CONDITION(visibles.size() == Boxes);
    std::vector<Batch> batches;
    Renderer::GetPtr()->GenerateBatches(visibles, batches);
    CHECK_CONDITION(batches.size() == 1);
    CHECK_CONDITION(batches[0].GetMesh() == boxMesh.get());

    auto material2 = Material::Create();
    boxes[0]->SetMaterial(material2);
    Renderer::GetPtr()->GenerateBatches(visibles, batches);
    CHECK_CONDITION(batches.size() == 2);
    CHECK_CONDITION(batches[0].GetMesh() == boxMesh.get());
    CHECK_CONDITION(batches[1].GetMesh() == boxMesh.get());
    CHECK_CONDITION(batches[0].GetMaterial() != batches[1].GetMaterial());

    camera->SetGlobalLookAtPosition(Vector3(0, 0, 1));
    scene->GetVisibleNodes(camera.get(), visibles);
    CHECK_CONDITION(visibles.size() == Spheres);
    Renderer::GetPtr()->GenerateBatches(visibles, batches);
    CHECK_CONDITION(batches.size() == 1);
    CHECK_CONDITION(batches[0].GetMesh() == sphereMesh.get());
    for (int i = 0; i < Spheres / 2; i++)
        spheres[i]->SetMaterial(material2);
    Renderer::GetPtr()->GenerateBatches(visibles, batches);
    CHECK_CONDITION(batches.size() == 2);
    CHECK_CONDITION(batches[0].GetMesh() == sphereMesh.get());
    CHECK_CONDITION(batches[1].GetMesh() == sphereMesh.get());
    CHECK_CONDITION(batches[0].GetMaterial() != batches[1].GetMaterial());
}

static void Test0A() {
    auto scene = std::make_shared<Scene>();
    auto camera = scene->CreateChild<Camera>();
    camera->SetFarClip(200);
    camera->SetGlobalLookAtPosition(Vector3(0, 0, 1));

    auto sphereMesh(Mesh::Create<SphereMesh>());
    auto boxMesh(Mesh::Create<BoxMesh>());
    auto material1 = Material::Create();

    const int Spheres = 100;
    for (int i = 0; i < Spheres; i++) {
        auto node = scene->CreateChild<SceneNode>();
        node->SetMesh(sphereMesh);
        node->SetPosition(Vertex3(0, 0, i + 2));
        node->SetMaterial(material1);
    }
    const int Boxes = 50;
    for (int i = 0; i < Boxes; i++) {
        auto node = scene->CreateChild<SceneNode>();
        node->SetMesh(boxMesh);
        node->SetPosition(Vertex3(0, 0, (i + 1)));
        node->SetMaterial(material1);
    }

#if 0
	std::vector<SceneNode*> visibles;
	scene->GetVisibleNodes(camera.get(), visibles);
	CHECK_CONDITION(visibles.size() == Spheres + Boxes);
	Renderer::GetPtr()->SortTransparentBackToFront(visibles);
	CHECK_CONDITION(visibles[0]->GetMesh().get() == sphereMesh.get());
	CHECK_CONDITION(visibles[0]->GetPosition().z > 100);
	Renderer::GetPtr()->SortSolidFrontToBack(visibles);
	CHECK_CONDITION(visibles[0]->GetMesh().get() == boxMesh.get());
	CHECK_CONDITION(visibles[0]->GetPosition().z < 1.01f);
#endif
}

void Tests() {
    auto window = Window::Create("window", 0, 0, 1, 1, (int)WindowFlag::HIDDEN);
    //    CHECK_CONDITION(window->IsReady());
    Test01();
    Test02();
    Test03();
    Test04();
    Test05();
    Test06();
    Test07();
    Test08();
    Test09();
    Test0A();
}
