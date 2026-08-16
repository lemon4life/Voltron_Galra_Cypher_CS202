#pragma once

#include "Core/Level/ILevelProvider.h"
#include "Core/Level/Tilemap.h"
#include "Entities/GameObject.h"
#include "Core/Constants.h"
#include <memory>

class ProceduralLevelProvider : public ILevelProvider {
public:
    ProceduralLevelProvider(
        std::shared_ptr<RoomTemplate>& activeRoom,
        Vector2& roomOffset,
        const std::vector<GameObject*>& levelEntities,
        Texture2D floorTileset,
        Texture2D wallTileset,
        Texture2D prop1Texture,
        Texture2D prop2Texture,
        Texture2D boxTexture,
        Texture2D gateTexture,
        const LevelMap& levelMap
    );

    void DrawBase() override;
    void GetDepthRenderItems(std::vector<DepthRenderItem>& items) override;
    bool IsSolidCollision(Rectangle box) const override;

private:
    std::shared_ptr<RoomTemplate>& activeRoom;
    Vector2& roomOffset;
    const std::vector<GameObject*>& levelEntities;
    Texture2D floorTileset;
    Texture2D wallTileset;
    Texture2D prop1Texture;
    Texture2D prop2Texture;
    Texture2D boxTexture;
    Texture2D gateTexture;
    const LevelMap& levelMap;
};
