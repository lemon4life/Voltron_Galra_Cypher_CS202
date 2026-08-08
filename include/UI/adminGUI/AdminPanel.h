#pragma once

#include "Core/LevelAccess.h"
#include "raylib.h"

#include <array>
#include <cstddef>
#include <string>

class LevelManager;
class TeamManager;
enum class GameState;

class AdminPanel {
public:
    AdminPanel();

    void Update(
        Vector2 worldMousePosition,
        LevelManager& levelManager,
        TeamManager* teamManager,
        GameState gameState
    );
    void Draw() const;

    bool IsOpen() const { return open; }
    bool IsMouseOverPanel() const;

private:
    static constexpr std::size_t SPAWN_TYPE_COUNT = 4;
    static constexpr std::size_t SPAWN_PROPERTY_COUNT = 14;

    Rectangle GetPanelBounds() const;
    Rectangle GetPropertyViewport() const;
    void SpawnSelectedType(
        Vector2 worldMousePosition,
        LevelManager& levelManager,
        TeamManager* teamManager
    );
    void DeleteAllEnemies(LevelManager& levelManager);
    void UpdatePropertyEditor(Vector2 mousePosition);
    void DrawPropertyEditor() const;
    std::size_t GetSpawnTypeIndex() const;

    bool open;
    bool placementArmed;
    MapObjectId spawnType;
    std::array<
        std::array<float, SPAWN_PROPERTY_COUNT>,
        SPAWN_TYPE_COUNT
    > spawnValues;
    float propertyScroll;
    std::string statusMessage;
    int pathSearchesPerSecond;
    float pathAverageMilliseconds;
    float pathMaximumMilliseconds;
};
