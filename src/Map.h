#pragma once

#include "Module.h"
#include <list>
#include <vector>

struct Properties
{
    struct Property
    {
        std::string name = "";
        bool value = false;
    };

    std::list<Property*> propertyList;

    ~Properties()
    {
        for (const auto& property : propertyList) delete property;
        propertyList.clear();
    }

    Property* GetProperty(const char* name)
    {
        for (const auto& property : propertyList)
            if (property->name == name) return property;
        return nullptr;
    }
};

struct MapLayer
{
    int id = 0;
    std::string name = "";
    int width = 0;
    int height = 0;
    std::vector<int> tiles;
    Properties properties;

    unsigned int Get(int i, int j) const
    {
        return tiles[(i * width) + j];
    }
};

struct TileSet
{
    int firstGid = 0;
    std::string name = "";
    int tileWidth = 0;
    int tileHeight = 0;
    int spacing = 0;
    int margin = 0;
    int tileCount = 0;
    int columns = 0;
    SDL_Texture* texture = nullptr;

    SDL_Rect GetRect(unsigned int gid)
    {
        SDL_Rect rect = { 0 };
        int relativeIndex = (int)gid - firstGid;
        rect.w = tileWidth;
        rect.h = tileHeight;
        rect.x = margin + (tileWidth + spacing) * (relativeIndex % columns);
        rect.y = margin + (tileHeight + spacing) * (relativeIndex / columns);
        return rect;
    }
};

struct MapData
{
    int width = 0;
    int height = 0;
    int tileWidth = 0;
    int tileHeight = 0;
    std::list<TileSet*> tilesets;
    std::list<MapLayer*> layers;
};

class Map : public Module
{
public:
    Map();
    virtual ~Map();

    bool Awake();
    bool Start();
    bool Update(float dt);
    bool CleanUp();

    bool Load(std::string path, std::string mapFileName);

    Vector2D MapToWorld(int i, int j) const;
    TileSet* GetTilesetFromTileId(int gid) const;
    bool LoadProperties(pugi::xml_node& node, Properties& properties);
    Vector2D GetMapSizeInPixels();

    // NUEVO: devuelve la Y (mundo) de la parte superior del primer tile sólido ("Collisions") bajo worldX,worldY
    float GetGroundYBelow(float worldX, float worldY) const;

private:
    int WorldToMapX(float worldX) const;
    int WorldToMapY(float worldY) const;

public:
    std::string mapFileName = "";
    std::string mapPath = "";

private:
    bool mapLoaded = false;
    MapData mapData;
};
