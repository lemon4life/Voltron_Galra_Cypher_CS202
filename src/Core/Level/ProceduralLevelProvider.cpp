#include "Core/Level/ProceduralLevelProvider.h"
#include "Core/Level/Tilemap.h"
#include "Core/Level/RoomNode.h"
#include <cmath>

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
        TilemapRenderer::DrawRoomBase(*activeRoom, roomOffset, floorTileset, wallTileset, prop1Texture, prop2Texture, boxTexture);
        
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
        TilemapRenderer::GetRoomDepthRenderItems(*activeRoom, roomOffset, wallTileset, prop1Texture, prop2Texture, boxTexture, items);
    }
}

bool ProceduralLevelProvider::IsSolidCollision(Rectangle box) const {
    // First: do a fast O(1) tile-based lookup for wall and void tiles.
    if (activeRoom) {
        float ts = Constants::RENDER_TILE_SIZE;
        float cx[5] = { box.x + 1.f, box.x + box.width - 1.f, box.x + 1.f, box.x + box.width - 1.f, box.x + box.width * 0.5f };
        float cy[5] = { box.y + 1.f, box.y + 1.f, box.y + box.height - 1.f, box.y + box.height - 1.f, box.y + box.height * 0.5f };
        for (int i = 0; i < 5; ++i) {
            int tx = (int)std::floor((cx[i] - roomOffset.x) / ts);
            int ty = (int)std::floor((cy[i] - roomOffset.y) / ts);
            if (tx < 0 || ty < 0 || tx >= activeRoom->width || ty >= activeRoom->height) {
                return true; // Out of map bounds = solid
            }
            int tile = activeRoom->layer0_tiles[ty][tx];
            if (tile == 1 || tile == 2) return true; // Wall or void
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
