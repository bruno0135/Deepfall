#pragma once

#include "Input.h"
#include "Render.h"

enum class EntityType
{
    PLAYER,
    ITEM,
    UNKNOWN
};

class PhysBody;

class Entity : public std::enable_shared_from_this<Entity>
{
public:

    Entity() {}
    Entity(EntityType type) : type(type), active(true) {}

    virtual bool Awake()
    {
        return true;
    }

    virtual bool Start()
    {
        return true;
    }

    virtual bool Update(float dt)
    {
        return true;
    }

    virtual bool CleanUp()
    {
        return true;
    }

    virtual bool Destroy()
    {
        return true;
    }

    void Enable()
    {
        if (!active)
        {
            active = true;
            Start();
        }
    }

    void Disable()
    {
        if (active)
        {
            active = false;
            CleanUp();
        }
    }

    virtual void OnCollision(PhysBody* physA, PhysBody* physB) {}
    virtual void OnCollisionEnd(PhysBody* physA, PhysBody* physB) {}

public:

    std::string name = "";
    EntityType type = EntityType::UNKNOWN;  // inicializado

    bool active = true;

    // Possible properties
    Vector2D position = { 0, 0 };
    bool renderable = true;
};
