/*
-------------------------------------------------------------------------------
This file is part of nsg-library.
http://github.com/woodjazz/nsg-library

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
#include "Level1.h"
#include "Planet.h"
#include "Sun.h"
#include "Enemy.h"
#include "Player.h"

Level1::Level1(PWindow window)
	: Level(window),
	totalEnemies_(0)
{
	scene_ = std::make_shared<Scene>("Level1");
	scene_->SetAmbientColor(ColorRGB(0.1f));
	AddObject(std::make_shared<Planet>(scene_));
	AddObject(std::make_shared<Sun>(scene_));
	player_ = std::make_shared<Player>(scene_);
	auto enemy0 = std::make_shared<Enemy>(scene_);
	auto enemy1 = std::make_shared<Enemy>(scene_);
	enemy0->SetPosition(-PI10, 0);
	enemy1->SetPosition(PI10, 0);
	AddObject(enemy0);
	AddObject(enemy1);
	totalEnemies_ = 2;
	camera_ = player_->GetCameraNode()->CreateChild<Camera>();
	camera_->SetPosition(Vertex3(0, 0, 10));

	slotPlayerDestroyed_ = player_->SigDestroyed()->Connect([this]()
	{
		camera_->SetParent(nullptr);
		player_ = std::make_shared<Player>(scene_);
		camera_->SetParent(player_->GetCameraNode());
		SigFailed()->Run();
	});

	slotEnemyDestroyed_ = GameObject::SigOneDestroyed()->Connect([this](GameObject* obj)
	{
		if (dynamic_cast<Enemy*>(obj) != nullptr)
		{
			RemoveObject(obj);
			if (--totalEnemies_ == 0)
				SigNextLevel()->Run();
		}
	});

	window->SetScene(scene_.get());

}

Level1::~Level1()
{
	
}
