#include "Player.h"
#include "Engine.h"
#include "Textures.h"
#include "Audio.h"
#include "Input.h"
#include "Render.h"
#include "Scene.h"
#include "Log.h"
#include "Physics.h"
#include "EntityManager.h"
#include "Map.h"
#include <cstring>

static const int SPR_MARGIN_X = 0;
static const int SPR_MARGIN_Y = 0;

Player::Player() : Entity(EntityType::PLAYER)
{
    name = "Player";
}

Player::~Player() {}

bool Player::Awake()
{
    position = Vector2D(100, 200);
    initialSpawn = position;
    checkpointPos = position;
    return true;
}

bool Player::Start()
{
    // Ajustar spawn al suelo de la capa "Collisions"
    float groundY = Engine::GetInstance().map->GetGroundYBelow(respawnPos.getX(), respawnPos.getY());
    respawnPos.setY(groundY - (float)(texH / 2));

    position = respawnPos;
    initialSpawn = respawnPos;
    checkpointPos = respawnPos;

    std::unordered_map<int, std::string> aliases = { {0,"idle"},{11,"move"},{22,"jump"},{33,"die"} };
    anims.LoadFromTSX("Assets/Textures/player_spritesheet.tsx", aliases);
    anims.SetCurrent("idle");
    anims.SetCurrent("move");
    anims.SetCurrent("jump");
    anims.SetCurrent("die");

    texture = Engine::GetInstance().textures->Load("Assets/Textures/player_spritesheet.png");
    texW = 32;
    texH = 32;

    respawnPos = position;
    {
        float groundY = Engine::GetInstance().map->GetGroundYBelow(respawnPos.getX(), respawnPos.getY());
        respawnPos.setY(groundY - (float)(texH / 2));
        position = respawnPos;
        initialSpawn = respawnPos;
        checkpointPos = respawnPos;
    }

    pbody = Engine::GetInstance().physics->CreateCircle((int)position.getX(), (int)position.getY(), texW / 2, bodyType::DYNAMIC);
    pbody->listener = this;
    pbody->ctype = ColliderType::PLAYER;

    pickCoinFxId = Engine::GetInstance().audio->LoadFx("Assets/Audio/Fx/coin-collision-sound-342335.wav");
    jumpTimer.Start();

    return true;
}

bool Player::Update(float dt)
{
    GetPhysicsValues();

    if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_M) == KEY_DOWN && !godMode)
    {
        LOG("DEBUG: Forced Die() by key M");
        Die();
    }

    // Manejar pantalla de victoria
    if (isWinning)
    {
        if (winTimer.ReadMSec() >= winDelayMs)
        {
            // Reiniciar el juego después de ganar
            ResetToInitialSpawn();
            Respawn();
            isWinning = false;
        }

        Draw(dt);
        return true;
    }

    // Manejar pantalla de muerte
    if (isDying)
    {
        if (dieTimer.ReadMSec() >= dieDelayMs)
        {
            Respawn();
            isDying = false;
        }

        Draw(dt);
        return true;
    }

    CheckOutOfBounds();

    Move();
    Jump();
    Teleport();
    ApplyPhysics();

    HandleGodMode(dt);
    Draw(dt);

    return true;
}

void Player::CheckOutOfBounds()
{
    Vector2D mapSize = Engine::GetInstance().map->GetMapSizeInPixels();

    int x, y;
    pbody->GetPosition(x, y);

    if (y > mapSize.getY() + 100)
    {
        Die();
    }
}

void Player::Respawn()
{
    isJumping = false;
    jumpCount = 0;
    anims.SetCurrent("idle");

    // Decidir dónde aparecer según las muertes
    Vector2D spawnPoint = hasCheckpoint ? checkpointPos : initialSpawn;

    float groundY = Engine::GetInstance().map->GetGroundYBelow(spawnPoint.getX(), spawnPoint.getY());
    float spawnY = groundY - (float)(texH / 2);

    pbody->SetPosition((int)spawnPoint.getX(), (int)spawnY);
    Engine::GetInstance().physics->SetLinearVelocity(pbody, { 0, 0 });

    jumpTimer.Start();

    platformContacts = 0;
    grounded = false;
}

void Player::Die()
{
    if (isDying || isWinning) return;

    isDying = true;
    deathCount++;

    LOG("Player died! Death count: %d/%d", deathCount, maxDeaths);

    // Si supera el límite de muertes, reiniciar el juego
    if (deathCount > maxDeaths)
    {
        LOG("Max deaths reached! Resetting game...");
        ResetToInitialSpawn();
    }

    anims.SetCurrent("die");
    Engine::GetInstance().physics->SetLinearVelocity(pbody, { 0.0f, 0.0f });
    dieTimer.Start();
}

void Player::Win()
{
    if (isDying || isWinning) return;

    isWinning = true;
    LOG("Player reached the WIN ZONE!");

    anims.SetCurrent("idle");
    Engine::GetInstance().physics->SetLinearVelocity(pbody, { 0.0f, 0.0f });
    winTimer.Start();
}

void Player::Teleport()
{
    if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_T) == KEY_DOWN)
        Respawn();
}

void Player::SetCheckpoint(Vector2D newCheckpoint)
{
    checkpointPos = newCheckpoint;
    hasCheckpoint = true;
    deathCount = 0; // Resetear contador al coger moneda
    LOG("Checkpoint saved at (%.1f, %.1f). Deaths reset to 0.", checkpointPos.getX(), checkpointPos.getY());
}

void Player::ResetToInitialSpawn()
{
    // Reiniciar todas las variables del juego
    checkpointPos = initialSpawn;
    hasCheckpoint = false;
    deathCount = 0;

    LOG("Game reset to initial spawn. Coin will respawn on next scene reset.");
}

void Player::GetPhysicsValues()
{
    velocity = Engine::GetInstance().physics->GetLinearVelocity(pbody);
    velocity = { 0.0f, velocity.y };
}

void Player::Move()
{
    if (isDying) return;

    if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT)
    {
        velocity.x = -speed;
        anims.SetCurrent("move");
    }
    if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT)
    {
        velocity.x = speed;
        anims.SetCurrent("move");
    }
}

void Player::Jump()
{
    if (isDying) return;

    const bool cooldownOk = (jumpTimer.ReadMSec() >= jumpCooldownMs);
    const bool canJump = (grounded && jumpCount < maxJumps) || (!grounded && jumpCount < maxJumps);

    if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN && canJump && cooldownOk)
    {
        const float force = grounded ? jumpForceGround : jumpForceAir;

        Engine::GetInstance().physics->ApplyLinearImpulseToCenter(pbody, 0.0f, -force, true);
        anims.SetCurrent("jump");
        isJumping = true;
        jumpCount++;

        float vy = Engine::GetInstance().physics->GetYVelocity(pbody);
        if (vy < -maxRiseSpeed)
            Engine::GetInstance().physics->SetYVelocity(pbody, -maxRiseSpeed);

        grounded = false;
        jumpTimer.Start();
    }
}

void Player::ApplyPhysics()
{
    if (isJumping == true)
        velocity.y = Engine::GetInstance().physics->GetYVelocity(pbody);

    Engine::GetInstance().physics->SetLinearVelocity(pbody, velocity);
}

void Player::Draw(float dt)
{
    anims.Update(dt);
    const SDL_Rect& animFrame = anims.GetCurrentFrame();

    int x, y;
    pbody->GetPosition(x, y);
    position.setX((float)x);
    position.setY((float)y);

    Vector2D mapSize = Engine::GetInstance().map->GetMapSizeInPixels();
    int cameraW = Engine::GetInstance().render->camera.w;
    int cameraH = Engine::GetInstance().render->camera.h;

    float cameraOffsetX = cameraW / 4.0f;
    int desiredCameraX = (int)(-position.getX() + cameraOffsetX);

    if (desiredCameraX > 0)
        desiredCameraX = 0;

    int maxCameraX = (int)(mapSize.getX() - cameraW);
    int minCameraX = (int)-maxCameraX;

    if (mapSize.getX() > cameraW)
    {
        if (desiredCameraX < minCameraX)
            desiredCameraX = minCameraX;
    }

    Engine::GetInstance().render->camera.x = desiredCameraX;

    float cameraOffsetY = cameraH / 2.0f;
    int desiredCameraY = (int)(-position.getY() + cameraOffsetY);

    if (desiredCameraY > 0)
        desiredCameraY = 0;

    int maxCameraY = (int)(mapSize.getY() - cameraH);
    int minCameraY = (int)-maxCameraY;

    if (mapSize.getY() > cameraH)
    {
        if (desiredCameraY < minCameraY)
            desiredCameraY = minCameraY;
    }

    Engine::GetInstance().render->camera.y = desiredCameraY;

    Engine::GetInstance().render->DrawTexture(texture, x - texW / 2, y - texH / 2, &animFrame);

    // Pantalla de victoria
    if (isWinning)
    {
        SDL_Rect screen = { 0, 0, cameraW, cameraH };
        Engine::GetInstance().render->DrawRectangle(screen, 0, 0, 0, 180, true, false);

        const char* txt = "YOU WIN!";
        int scale = 4;
        int textLength = (int)strlen(txt);
        int charWidth = 8 * scale;
        int textWidth = textLength * charWidth;
        int textHeight = 16 * scale;
        int textX = (cameraW - textWidth) / 2;
        int textY = (cameraH - textHeight) / 2;

        Engine::GetInstance().render->DrawText(txt, textX, textY, scale, 255, 215, 0, 255, false);
    }

    // Pantalla de muerte
    if (isDying)
    {
        SDL_Rect screen = { 0, 0, cameraW, cameraH };
        Engine::GetInstance().render->DrawRectangle(screen, 0, 0, 0, 180, true, false);

        const char* txt = "YOU DIED";
        int scale = 4;
        int textLength = (int)strlen(txt);
        int charWidth = 8 * scale;
        int textWidth = textLength * charWidth;
        int textHeight = 16 * scale;
        int textX = (cameraW - textWidth) / 2;
        int textY = (cameraH - textHeight) / 2;

        Engine::GetInstance().render->DrawText(txt, textX, textY, scale, 255, 0, 0, 255, false);

        // Mostrar contador de muertes
        char deathText[64];
        snprintf(deathText, sizeof(deathText), "DEATHS: %d / %d", deathCount, maxDeaths);
        Engine::GetInstance().render->DrawText(deathText, textX - 50, textY + 60, 2, 255, 255, 255, 255, false);
    }
}

bool Player::CleanUp()
{
    LOG("Cleanup player");
    Engine::GetInstance().textures->UnLoad(texture);
    return true;
}

void Player::OnCollision(PhysBody* physA, PhysBody* physB)
{
    if (godMode) return;

    switch (physB->ctype)
    {
    case ColliderType::PLATFORM:
    {
        float vy = Engine::GetInstance().physics->GetYVelocity(pbody);
        if (vy >= -0.1f)
        {
            platformContacts++;
            if (!grounded)
            {
                grounded = true;
                isJumping = false;
                jumpCount = 0;

                if (!isDying)
                    anims.SetCurrent("idle");

                jumpTimer.Start();
            }
        }
    }
    break;

    case ColliderType::ITEM:
        LOG("Collision ITEM - Setting checkpoint!");
        Engine::GetInstance().audio->PlayFx(pickCoinFxId);

        // Guardar checkpoint en la posición de la moneda
        int coinX, coinY;
        physB->GetPosition(coinX, coinY);
        SetCheckpoint(Vector2D((float)coinX, (float)coinY));

        physB->listener->Destroy();
        break;

    case ColliderType::ENEMY:
        LOG("Collision ENEMY");
        Die();
        break;

    case ColliderType::DEATH_ZONE:
        LOG("Has tocado la zona de muerte");
        Die();
        break;

    case ColliderType::WIN_ZONE:
        LOG("Has llegado a la zona de victoria!");
        Win();
        break;

    case ColliderType::UNKNOWN:
        LOG("Collision UNKNOWN");
        break;

    default:
        break;
    }
}

void Player::OnCollisionEnd(PhysBody* physA, PhysBody* physB)
{
    switch (physB->ctype)
    {
    case ColliderType::PLATFORM:
        if (platformContacts > 0) platformContacts--;
        if (platformContacts == 0) grounded = false;
        break;

    case ColliderType::ITEM:
        LOG("End Collision ITEM");
        break;

    case ColliderType::UNKNOWN:
        LOG("End Collision UNKNOWN");
        break;

    default:
        break;
    }
}

void Player::ToggleGodMode()
{
    godMode = !godMode;
    LOG(godMode ? "God Mode: ON" : "God Mode: OFF");

    if (pbody && godMode)
        Engine::GetInstance().physics->SetLinearVelocity(pbody, { 0.0f, 0.0f });
}

void Player::HandleGodMode(float dt)
{
    if (!godMode || !pbody) return;

    const float GOD_SPEED = 8.0f;
    float vx = 0.0f;
    float vy = 0.0f;

    if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT) vx -= GOD_SPEED;
    if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT) vx += GOD_SPEED;
    if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_W) == KEY_REPEAT) vy -= GOD_SPEED;
    if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_S) == KEY_REPEAT) vy += GOD_SPEED;

    Engine::GetInstance().physics->SetLinearVelocity(pbody, { vx, vy });
}