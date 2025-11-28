#pragma once

#include "Module.h"
#include "Player.h"
#include "Item.h"
#include "Vector2D.h"
#include <memory>

struct SDL_Texture;

class Scene : public Module
{
public:

	Scene();

	// Destructor
	virtual ~Scene();

	// Called before render is available
	bool Awake();

	// Called before the first frame
	bool Start();

	// Called before all Updates
	bool PreUpdate();

	// Called each loop iteration
	bool Update(float dt);

	// Called before all Updates
	bool PostUpdate();

	// Called before quitting
	bool CleanUp();

	// Reset level (respawn coin)
	void ResetLevel();

private:

	//L03: TODO 3b: Declare a Player attribute
	std::shared_ptr<Player> player;

	// Item reference for respawning
	std::shared_ptr<Item> coin;
	Vector2D coinInitialPos = Vector2D(1300, 375);
};