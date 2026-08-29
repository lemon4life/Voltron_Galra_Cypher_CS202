#include "Core/Level/ProceduralLevelProvider.h"
#include "Core/Level/Tilemap.h"
#include "Core/Level/RoomNode.h"
#include <cmath>

namespace {
    constexpr float COLLISION_EDGE_PADDING = 0.001f;
}

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

void ProceduralLevelProvider::DrawBase() {
    if (activeRoom) {
        TilemapRenderer::DrawRoomBase(
            *activeRoom,
            roomOffset,
            floorTileset,
            wallTileset
        );
        
        // Draw EXIT gate if this room is an EXIT room
        for (const auto& node : levelMap.generatedNodes) {
            if (node->type == RoomType::EXIT) {
                Rectangle bounds = node->GetWorldBounds();
                float roomCenterX = bounds.x + bounds.width / 2.0f;
                float roomCenterY = bounds.y + bounds.height / 2.0f;
                float tileW = Constants::RENDER_TILE_SIZE;
                
                Rectangle destRec = {
                    roomCenterX - tileW * 2.0f,
                    roomCenterY - tileW * 2.0f,
                    tileW * 4.0f,
                    tileW * 4.0f
                };
                
                float frameWidth = gateTexture.width / 8.0f;
                int currentFrame = (int)(GetTime() * 10) % 8;
                Rectangle gateFrameSrc = {currentFrame * frameWidth, 0, frameWidth, (float)gateTexture.height};
                DrawTexturePro(gateTexture, gateFrameSrc, destRec, {0,0}, 0.0f, WHITE);
            }
        }
    }
}

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
