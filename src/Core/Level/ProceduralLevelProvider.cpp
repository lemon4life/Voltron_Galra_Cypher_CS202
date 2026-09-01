#include "Core/Level/ProceduralLevelProvider.h"
#include "Core/Level/Tilemap.h"
#include "Core/Level/RoomNode.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/LevelManager.h"
#include <cmath>

namespace {
    constexpr float COLLISION_EDGE_PADDING = 0.001f;
}

/// Creates a ProceduralLevelProvider instance from the supplied configuration.
ProceduralLevelProvider::ProceduralLevelProvider(
    std::shared_ptr<RoomTemplate>& activeRoom,
    Vector2& roomOffset,
    Texture2D floorTileset,
    Texture2D wallTileset,
    Texture2D prop1Texture,
    Texture2D prop2Texture,
    Texture2D boxTexture,
    Texture2D gateTexture,
    const LevelMap& levelMap
) : activeRoom(activeRoom), roomOffset(roomOffset),
    floorTileset(floorTileset), wallTileset(wallTileset),
    prop1Texture(prop1Texture), prop2Texture(prop2Texture),
    boxTexture(boxTexture), gateTexture(gateTexture), levelMap(levelMap) {}

/// Renders base.
void ProceduralLevelProvider::DrawBase() {
    if (activeRoom) {
        TilemapRenderer::DrawRoomBase(
            *activeRoom,
            roomOffset,
            floorTileset,
            wallTileset
        );
    }
}

/// Renders portals on a deterministic layer above corpses and below actors.
void ProceduralLevelProvider::DrawPortalLayer() {
    if (!activeRoom || gateTexture.id == 0) return;

    auto drawGate = [this](Vector2 center) {
        float tileW = Constants::RENDER_TILE_SIZE;
        Rectangle destination = {
            center.x - tileW * 2.0f,
            center.y - tileW * 2.0f,
            tileW * 4.0f,
            tileW * 4.0f
        };
        float frameWidth = gateTexture.width / 8.0f;
        int currentFrame = (int)(GetTime() * 10) % 8;
        Rectangle source = {
            currentFrame * frameWidth,
            0.0f,
            frameWidth,
            (float)gateTexture.height
        };
        DrawTexturePro(
            gateTexture,
            source,
            destination,
            { 0.0f, 0.0f },
            0.0f,
            WHITE
        );
    };

    for (const std::shared_ptr<RoomNode>& node : levelMap.generatedNodes) {
        if (!node || node->type != RoomType::EXIT) continue;
        Rectangle bounds = node->GetWorldBounds();
        drawGate({
            bounds.x + bounds.width * 0.5f,
            bounds.y + bounds.height * 0.5f
        });
    }

    LevelManager* levelManager =
        GameManager::GetInstance().GetLevelManager();
    if (levelManager && levelManager->IsBossExitGateActive()) {
        drawGate(levelManager->GetBossExitGatePosition());
    }
}

/// Returns the current depth render items.
void ProceduralLevelProvider::GetDepthRenderItems(std::vector<DepthRenderItem>& items) {
    if (activeRoom) {
        TilemapRenderer::GetRoomDepthRenderItems(
            *activeRoom,
            roomOffset,
            wallTileset,
            items
        );
    }
}

/// Reports whether the solid collision condition is satisfied.
bool ProceduralLevelProvider::IsSolidCollision(Rectangle box) const {
    if (!activeRoom) return false;

    const float tileSize = Constants::RENDER_TILE_SIZE;
    const int minimumTileX = (int)std::floor(
        (box.x + COLLISION_EDGE_PADDING - roomOffset.x) / tileSize
    );
    const int maximumTileX = (int)std::floor(
        (box.x + box.width - COLLISION_EDGE_PADDING - roomOffset.x) /
            tileSize
    );
    const int minimumTileY = (int)std::floor(
        (box.y + COLLISION_EDGE_PADDING - roomOffset.y) / tileSize
    );
    const int maximumTileY = (int)std::floor(
        (box.y + box.height - COLLISION_EDGE_PADDING - roomOffset.y) /
            tileSize
    );

    for (int tileY = minimumTileY; tileY <= maximumTileY; ++tileY) {
        for (int tileX = minimumTileX; tileX <= maximumTileX; ++tileX) {
            if (tileX < 0 || tileY < 0 ||
                tileX >= activeRoom->width || tileY >= activeRoom->height) {
                return true;
            }

            const int tile = activeRoom->layer0_tiles[tileY][tileX];
            if (tile == 1 || tile == 2) {
                return true;
            }
        }
    }

    return false;
}

/// Appends static blocking colliders for tile.
void ProceduralLevelProvider::AppendStaticBlockingCollidersForTile(
    int tileX,
    int tileY,
    std::vector<Rectangle>& output
) const {
    float tileSize = Constants::RENDER_TILE_SIZE;
    Rectangle fullTile = {
        roomOffset.x + tileX * tileSize,
        roomOffset.y + tileY * tileSize,
        tileSize,
        tileSize
    };
    if (!activeRoom || tileX < 0 || tileY < 0 ||
        tileX >= activeRoom->width || tileY >= activeRoom->height) {
        output.push_back(fullTile);
        return;
    }

    int tile = activeRoom->layer0_tiles[tileY][tileX];
    if (tile == 1 || tile == 2) {
        output.push_back(fullTile);
    }
}
