#include "Item.h"
#include "Engine.h"
#include "Textures.h"
#include "Audio.h"
#include "Input.h"
#include "Render.h"
#include "Scene.h"
#include "Log.h"
#include "Physics.h"
#include "EntityManager.h"

Item::Item() : Entity(EntityType::ITEM)
{
	name = "item";
}

Item::~Item() {}

bool Item::Awake() {
	return true;
}

bool Item::Start() {

	// Cargar animaciones desde el .tsx
	std::unordered_map<int, std::string> aliases = { {0, "spin"} };
	anims.LoadFromTSX("Assets/Textures/checkpoint_coin.tsx", aliases);
	anims.SetCurrent("spin");

	// Initialize textures
	texture = Engine::GetInstance().textures->Load("Assets/Textures/checkpoint_coin.png");

	// Obtener dimensiones del sprite
	const SDL_Rect& frame = anims.GetCurrentFrame();
	texW = frame.w;
	texH = frame.h;

	// Usar CreateRectangleSensor para que sea un sensor sin colisiones físicas
	pbody = Engine::GetInstance().physics->CreateRectangleSensor(
		(int)position.getX(),
		(int)position.getY(),
		texW,
		texH,
		bodyType::STATIC  // STATIC para que no caiga por gravedad
	);

	// L08 TODO 7: Assign collider type
	pbody->ctype = ColliderType::ITEM;

	// Set this class as the listener of the pbody
	pbody->listener = this;   // so Begin/EndContact can call back to Item

	return true;
}

bool Item::Update(float dt)
{
	if (!active) return true;

	// Actualizar animación
	anims.Update(dt);
	const SDL_Rect& animFrame = anims.GetCurrentFrame();

	// L08 TODO 4: Add a physics to an item - update the position of the object from the physics.  
	int x, y;
	pbody->GetPosition(x, y);
	position.setX((float)x);
	position.setY((float)y);

	// Dibujar con el frame de animación actual
	Engine::GetInstance().render->DrawTexture(texture, x - texW / 2, y - texH / 2, &animFrame);

	return true;
}

bool Item::CleanUp()
{
	Engine::GetInstance().textures->UnLoad(texture);
	Engine::GetInstance().physics->DeletePhysBody(pbody);
	return true;
}

bool Item::Destroy()
{
	LOG("Destroying item");
	active = false;
	Engine::GetInstance().entityManager->DestroyEntity(shared_from_this());
	return true;
}