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
    /// Creates a AdminPanel instance from the supplied configuration.
    AdminPanel();

    /// Advances this component's state for the current frame.
    void Update(
        Vector2 worldMousePosition,
        LevelManager& levelManager,
        TeamManager* teamManager,
        GameState gameState
    );
    /// Renders this component using its current state and visual resources.
    void Draw() const;

    /// Reports whether the open condition is satisfied.
    bool IsOpen() const { return open; }
private:
    static constexpr std::size_t SPAWN_TYPE_COUNT = 5;
    static constexpr std::size_t SPAWN_PROPERTY_COUNT = 14;

    /// Returns the current panel bounds.
    Rectangle GetPanelBounds() const;
    /// Returns the current property viewport.
    Rectangle GetPropertyViewport() const;
    /// Spawns selected type.
    void SpawnSelectedType(
        Vector2 worldMousePosition,
        LevelManager& levelManager,
        TeamManager* teamManager
    );
    /// Deletes all enemies.
    void DeleteAllEnemies(LevelManager& levelManager);
    /// Updates property editor.
    void UpdatePropertyEditor(Vector2 mousePosition);
    /// Renders property editor.
    void DrawPropertyEditor() const;
    /// Returns the current spawn type index.
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
    int pathFlowBuildsPerSecond;
    int pathFlowProfiles;
    float pathFlowAverageMilliseconds;
    int pathSearchesPerSecond;
    float pathAverageMilliseconds;
    float pathMaximumMilliseconds;
};
