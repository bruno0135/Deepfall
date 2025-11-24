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
    return true;
}

bool Player::Start()
{
    // Ajustar spawn al suelo de la capa "Collisions"
    float groundY = Engine::GetInstance().map->GetGroundYBelow(respawnPos.getX(), respawnPos.getY());
    // colocar al jugador apoyado: top del tile - mitad del alto del sprite
    respawnPos.setY(groundY - (float)(texH / 2));

    // Igualamos la posición lógica al spawn ajustado antes de crear el cuerpo físico
    position = respawnPos;

    std::unordered_map<int, std::string> aliases = { {0,"idle"},{11,"move"},{22,"jump"},{33,"die"} };
    anims.LoadFromTSX("Assets/Textures/player1Spritesheet.tsx", aliases);
    anims.SetCurrent("idle");
    anims.SetCurrent("move");
    anims.SetCurrent("jump");
    // Carga de textura y dimensiones del tile antes de ajustar spawn
    texture = Engine::GetInstance().textures->Load("Assets/Textures/player1_spritesheet.png");
    texW = 32;
    texH = 32;

    // Guardar spawn base y ajustarlo al suelo usando la capa "Collisions"
    respawnPos = position;
    {
        float groundY = Engine::GetInstance().map->GetGroundYBelow(respawnPos.getX(), respawnPos.getY());
        respawnPos.setY(groundY - (float)(texH / 2)); // apoyar "pies"
        position = respawnPos;
    }

    // Crear cuerpo físico en el spawn ajustado
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
    // Recalcular el suelo por si el respawn está sobre vacío
    float groundY = Engine::GetInstance().map->GetGroundYBelow(respawnPos.getX(), respawnPos.getY());
    float spawnY = groundY - (float)(texH / 2);

    pbody->SetPosition((int)respawnPos.getX(), (int)spawnY);
    Engine::GetInstance().physics->SetLinearVelocity(pbody, { 0, 0 });

    jumpTimer.Start();

    platformContacts = 0;
    grounded = false;
}

void Player::Die()
{
    if (isDying) return;

    isDying = true;
    Engine::GetInstance().physics->SetLinearVelocity(pbody, { 0.0f, 0.0f });
    dieTimer.Start();
}

void Player::Teleport()
{
    if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_T) == KEY_DOWN)
        Respawn();
}

void Player::GetPhysicsValues()
{
    velocity = Engine::GetInstance().physics->GetLinearVelocity(pbody);
    velocity = { 0.0f, velocity.y };
}

void Player::Move()
{
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
                anims.SetCurrent("idle");
                jumpTimer.Start();
            }
        }
    }
    break;

    case ColliderType::ITEM:
        LOG("Collision ITEM");
        Engine::GetInstance().audio->PlayFx(pickCoinFxId);
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
