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

Player::Player() : Entity(EntityType::PLAYER)
{
	name = "Player";
}

Player::~Player() {}

bool Player::Awake() {

	//L03: TODO 2: Initialize Player parameters
	position = Vector2D(100, 200);
	return true;
}

bool Player::Start() {

	// save respawn
	respawnPos = position;

	// load
	std::unordered_map<int, std::string> aliases = { {0,"idle"},{11,"move"},{22,"jump"} };
	anims.LoadFromTSX("Assets/Textures/player1Spritesheet.tsx", aliases);
	anims.SetCurrent("idle");
	anims.SetCurrent("move");
	anims.SetCurrent("jump");

	//L03: TODO 2: Initialize Player parameters
	texture = Engine::GetInstance().textures->Load("Assets/Textures/player1_spritesheet.png");

	// L08 TODO 5: Add physics to the player - initialize physics body
	texW = 32;
	texH = 32;
	pbody = Engine::GetInstance().physics->CreateCircle((int)position.getX(), (int)position.getY(), texW / 2, bodyType::DYNAMIC);

	// L08 TODO 6: Assign player class (using "this") to the listener of the pbody. This makes the Physics module to call the OnCollision method
	pbody->listener = this;

	// L08 TODO 7: Assign collider type
	pbody->ctype = ColliderType::PLAYER;

	//initialize audio effect
	pickCoinFxId = Engine::GetInstance().audio->LoadFx("Assets/Audio/Fx/coin-collision-sound-342335.wav");

	// preparar cooldown de salto
	jumpTimer.Start();

	return true;
}

bool Player::Update(float dt)
{
	GetPhysicsValues();
	CheckOutOfBounds();

	Move();
	Jump();
	Teleport();
	ApplyPhysics();

	// --- GOD MODE (solo WASD) ---
	HandleGodMode(dt);

	Draw(dt);

	return true;
}

void Player::CheckOutOfBounds()
{
	Vector2D mapSize = Engine::GetInstance().map->GetMapSizeInPixels();

	int x, y;
	pbody->GetPosition(x, y);

	// Si cae fuera del mapa por abajo -> respawn
	if (y > mapSize.getY() + 100)
		Respawn();
}

void Player::Respawn()
{
	// Reset jump state
	isJumping = false;
	jumpCount = 0;

	// Reposicionar cuerpo
	pbody->SetPosition((int)respawnPos.getX(), (int)respawnPos.getY());

	// Eliminar velocidad
	Engine::GetInstance().physics->SetLinearVelocity(pbody, { 0, 0 });

	// Reset anim
	anims.SetCurrent("idle");

	// al reaparecer permitir salto de inmediato
	jumpTimer.Start();

	// reset de estado de suelo
	platformContacts = 0;
	grounded = false;
}

void Player::Teleport() {
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_T) == KEY_DOWN) {
		Respawn();
	}
}

void Player::GetPhysicsValues() {
	velocity = Engine::GetInstance().physics->GetLinearVelocity(pbody);
	velocity = { 0.0f, velocity.y };
}

void Player::Move() {

	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT) {
		velocity.x = -speed;
		anims.SetCurrent("move");
	}
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT) {
		velocity.x = speed;
		anims.SetCurrent("move");
	}
}

void Player::Jump() {
	// cooldown mínimo entre saltos
	const bool cooldownOk = (jumpTimer.ReadMSec() >= jumpCooldownMs);

	// condición de salto: permitir en suelo o doble salto en aire
	const bool canJump = (grounded && jumpCount < maxJumps) || (!grounded && jumpCount < maxJumps);

	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN
		&& canJump
		&& cooldownOk)
	{
		// fuerza distinta: suelo vs aire
		const float force = grounded ? jumpForceGround : jumpForceAir;

		Engine::GetInstance().physics->ApplyLinearImpulseToCenter(pbody, 0.0f, -force, true);
		anims.SetCurrent("jump");
		isJumping = true;
		jumpCount++;

		// limitar velocidad vertical ascendente para no “techar”
		float vy = Engine::GetInstance().physics->GetYVelocity(pbody);
		if (vy < -maxRiseSpeed) {
			Engine::GetInstance().physics->SetYVelocity(pbody, -maxRiseSpeed);
		}

		// tras saltar ya no estamos en suelo
		grounded = false;

		// reiniciar cooldown
		jumpTimer.Start();
	}
}

void Player::ApplyPhysics() {

	if (isJumping == true) {
		velocity.y = Engine::GetInstance().physics->GetYVelocity(pbody);
	}

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

	if (mapSize.getY() > cameraH) {
		if (desiredCameraY < minCameraY)
			desiredCameraY = minCameraY;
	}

	Engine::GetInstance().render->camera.y = desiredCameraY;

	Engine::GetInstance().render->DrawTexture(texture, x - texW / 2, y - texH / 2, &animFrame);
}

bool Player::CleanUp()
{
	LOG("Cleanup player");
	Engine::GetInstance().textures->UnLoad(texture);
	return true;
}

void Player::OnCollision(PhysBody* physA, PhysBody* physB) {
	if (godMode) return;

	switch (physB->ctype)
	{
	case ColliderType::PLATFORM:
	{
		// solo consideramos “suelo” si no vamos subiendo (evita reset por techo/pared)
		float vy = Engine::GetInstance().physics->GetYVelocity(pbody);

		if (vy >= -0.1f) {
			platformContacts++;

			// transición a grounded: de 0 contactos a 1 o más
			if (!grounded) {
				grounded = true;
				isJumping = false;
				jumpCount = 0;
				anims.SetCurrent("idle");

				// permitir salto inmediato tras aterrizar
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
		if (platformContacts > 0) {
			platformContacts--;
		}
		if (platformContacts == 0) {
			grounded = false;
		}
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
