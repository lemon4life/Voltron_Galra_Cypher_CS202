#pragma once

#include "Core/Level/ILevelProvider.h"
#include "Core/Level/Tilemap.h"
#include "Core/Constants.h"
#include <memory>

class ProceduralLevelProvider : public ILevelProvider {
public:
    /// Creates a ProceduralLevelProvider instance from the supplied configuration.
    ProceduralLevelProvider(
        std::shared_ptr<RoomTemplate>& activeRoom,
        Vector2& roomOffset,
        Texture2D floorTileset,
        Texture2D wallTileset,
        Texture2D prop1Texture,
        Texture2D prop2Texture,
        Texture2D boxTexture,
        Texture2D gateTexture,
        const LevelMap& levelMap
    );

    /// Renders base.
    void DrawBase() override;
    /// Returns the current depth render items.
    void GetDepthRenderItems(std::vector<DepthRenderItem>& items) override;
    /// Reports whether the solid collision condition is satisfied.
    bool IsSolidCollision(Rectangle box) const override;
    /// Appends static blocking colliders for tile.
    void AppendStaticBlockingCollidersForTile(
        int tileX,
        int tileY,
        std::vector<Rectangle>& output
    ) const override;

private:
    std::shared_ptr<RoomTemplate>& activeRoom;
    Vector2& roomOffset;
    Texture2D floorTileset;
    Texture2D wallTileset;
    Texture2D prop1Texture;
    Texture2D prop2Texture;
    Texture2D boxTexture;
    Texture2D gateTexture;
    const LevelMap& levelMap;
};
