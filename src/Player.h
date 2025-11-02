#pragma once

#include "Entity.h"
#include "Animation.h"
#include "Timer.h"
#include <box2d/box2d.h>
#include <SDL3/SDL.h>

struct SDL_Texture;

class Player : public Entity
{
public:

	Player();

	virtual ~Player();

	bool Awake();

	bool Start();

	bool Update(float dt);

	bool CleanUp();

	// L08 TODO 6: Define OnCollision function for the player. 
	void OnCollision(PhysBody* physA, PhysBody* physB);
	void OnCollisionEnd(PhysBody* physA, PhysBody* physB);

	// --- GOD MODE (público para que el motor pueda hacer toggle) ---
	void ToggleGodMode();
	void HandleGodMode(float dt);

private:

	void GetPhysicsValues();
	void Move();
	void Jump();
	void Teleport();
	void ApplyPhysics();
	void Draw(float dt);
	void CheckOutOfBounds();
	void Respawn();

public:

	//Declare player parameters
	float speed = 4.0f;
	SDL_Texture* texture = NULL;

	int texW = 0, texH = 0;

	//Audio fx
	int pickCoinFxId = 0;

	// L08 TODO 5: Add physics to the player - declare a Physics body
	PhysBody* pbody = nullptr;
	float jumpForce = 2.5f; // The force to apply when jumping
	bool isJumping = false; // Flag to check if the player is currently jumping

	// Doble salto
	int jumpCount = 0;
	const int maxJumps = 2;

	// Control de salto (anti-spam y altura)
	float jumpForceGround = 2.2f;     // fuerza del salto en suelo (ligeramente menor que antes)
	float jumpForceAir = 1.6f;     // fuerza del salto en el aire (más baja para no “techar”)
	float maxRiseSpeed = 6.0f;     // límite a la velocidad vertical hacia arriba
	float jumpCooldownMs = 200.0f;   // tiempo mínimo entre saltos (ms)
	Timer jumpTimer;                  // cronómetro de cooldown

	// Estado de suelo para evitar spam por colisiones laterales/techo
	int   platformContacts = 0;       // número de contactos simultáneos con plataformas
	bool  grounded = false;   // verdadero si hay apoyo “real” en suelo

	// NUEVO  punto de reaparición
	Vector2D respawnPos = { 100, 200 };

private:
	b2Vec2 velocity = { 0.0f, 0.0f };
	AnimationSet anims;

	// --- GOD MODE ---
	bool godMode = false;
};
