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

    void OnCollision(PhysBody* physA, PhysBody* physB);
    void OnCollisionEnd(PhysBody* physA, PhysBody* physB);

    void ToggleGodMode();
    void HandleGodMode(float dt);

    // Sistema de checkpoints
    void SetCheckpoint(Vector2D newCheckpoint);
    void Win();
    void ResetToInitialSpawn();
    int GetDeathCount() const { return deathCount; }

private:

    void GetPhysicsValues();
    void Move();
    void Jump();
    void Teleport();
    void ApplyPhysics();
    void Draw(float dt);
    void CheckOutOfBounds();
    void Respawn();
    void Die();

public:

    float speed = 4.0f;
    SDL_Texture* texture = NULL;

    int texW = 0, texH = 0;

    int pickCoinFxId = 0;

    PhysBody* pbody = nullptr;
    float jumpForce = 2.5f;
    bool isJumping = false;

    int jumpCount = 0;
    const int maxJumps = 2;

    float jumpForceGround = 2.2f;
    float jumpForceAir = 1.6f;
    float maxRiseSpeed = 6.0f;
    float jumpCooldownMs = 200.0f;
    Timer jumpTimer;

    int platformContacts = 0;
    bool grounded = false;

    Vector2D respawnPos = { 100, 200 };

private:
    b2Vec2 velocity = { 0.0f, 0.0f };
    AnimationSet anims;

    bool godMode = false;

    // Animación de muerte
    bool isDying = false;
    Timer dieTimer;
    float dieDelayMs = 1500.0f;

    //Animacion victoria
    bool isWinning = false;
    Timer winTimer;
    float winDelayMs = 3000.0f; 

    // Sistema de checkpoints
    Vector2D initialSpawn = { 100, 200 };  // Spawn inicial del juego
    Vector2D checkpointPos = { 1300, 375 }; // Último checkpoint (moneda)
    bool hasCheckpoint = false;            // Mira si ha cogido alguna moneda
    int deathCount = 0;                    // Contador de muertes
    const int maxDeaths = 3;               // Máximo de muertes antes de reiniciar
};