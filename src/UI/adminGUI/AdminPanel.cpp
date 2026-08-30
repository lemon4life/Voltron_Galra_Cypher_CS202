#include "UI/adminGUI/AdminPanel.h"

#include "Core/Manager/AssetManager.h"
#include "Core/Constants.h"
#include "Core/Diagnostics/FramePerformanceStats.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/LevelManager.h"
#include "Core/Manager/TeamManager.h"
#include "Entities/Enemy.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>

namespace {
/// Renders text admin.
void DrawTextAdmin(const char* text, int x, int y, int fontSize, Color color) {
    DrawTextEx(AssetManager::GetInstance().GetCustomFont("PixeloidSans"), text, {(float)x, (float)y}, fontSize, 1.0f, color);
}

constexpr float PANEL_X = 10.0f;
constexpr float PANEL_Y = 10.0f;
constexpr float PANEL_WIDTH = 460.0f;
constexpr float PANEL_PADDING = 14.0f;
constexpr float TOGGLE_HEIGHT = 32.0f;
constexpr float BUTTON_HEIGHT = 38.0f;
constexpr float PROPERTY_ROW_HEIGHT = 58.0f;
constexpr float TOGGLE_START_Y = 106.0f;
constexpr float SPAWN_HEADING_Y = 258.0f;
constexpr float SPAWN_BUTTON_START_Y = 285.0f;
constexpr float SPAWN_BUTTON_GAP_Y = 6.0f;
constexpr int TEXT_SIZE = 18;
constexpr int SMALL_TEXT_SIZE = 16;
constexpr int TITLE_TEXT_SIZE = 26;
constexpr float SLIDER_TRACK_HEIGHT = 8.0f;
constexpr float SLIDER_THUMB_RADIUS = 9.0f;

constexpr Color PANEL_COLOR = { 18, 22, 31, 242 };
constexpr Color PANEL_BORDER_COLOR = { 112, 132, 164, 255 };
constexpr Color BUTTON_COLOR = { 45, 55, 72, 255 };
constexpr Color BUTTON_HOVER_COLOR = { 64, 78, 101, 255 };
constexpr Color BUTTON_SELECTED_COLOR = { 40, 112, 92, 255 };
constexpr Color SLIDER_TRACK_COLOR = { 55, 65, 82, 255 };
constexpr Color SLIDER_FILL_COLOR = { 62, 173, 142, 255 };

constexpr std::array<MapObjectId, 5> SPAWN_TYPES = {
    MapObjectId::Chaser,
    MapObjectId::Range,
    MapObjectId::Diver,
    MapObjectId::Boss,
    MapObjectId::DemonTHA
};
constexpr std::size_t SPAWN_COLUMN_COUNT = 2;
constexpr std::size_t SPAWN_ROW_COUNT =
    (SPAWN_TYPES.size() + SPAWN_COLUMN_COUNT - 1) / SPAWN_COLUMN_COUNT;
constexpr float ACTION_BUTTON_Y = SPAWN_BUTTON_START_Y +
    (float)SPAWN_ROW_COUNT * (BUTTON_HEIGHT + SPAWN_BUTTON_GAP_Y) + 2.0f;
constexpr float ACTION_ROW_GAP = 6.0f;
constexpr float STATUS_TEXT_Y = ACTION_BUTTON_Y +
    BUTTON_HEIGHT * 2.0f + ACTION_ROW_GAP + 8.0f;
constexpr float PROPERTY_HEADING_Y = STATUS_TEXT_Y + 22.0f;
constexpr float PROPERTY_START_Y = PROPERTY_HEADING_Y + 21.0f;

enum class EnemyProperty {
    Health,
    MaxHealth,
    Speed,
    Damage,
    BaseAttackCooldown,
    CurrentAttackCooldown,
    DazeDuration,
    KnockbackResistance,
    HitboxWidth,
    HitboxHeight,
    NavigationWidth,
    NavigationHeight,
    NavigationOffsetX,
    NavigationOffsetY
};

struct PropertyDefinition {
    EnemyProperty property;
    const char* label;
    float step;
    float minimum;
    float maximum;
    int decimals;
};

constexpr std::array<PropertyDefinition, 14> PROPERTIES = {{
    { EnemyProperty::Health, "Spawn health", 10.0f, 1.0f, 2000.0f, 0 },
    { EnemyProperty::MaxHealth, "Maximum health", 10.0f, 1.0f, 2000.0f, 0 },
    { EnemyProperty::Speed, "Movement speed", 5.0f, 0.0f, 500.0f, 0 },
    { EnemyProperty::Damage, "Attack damage", 5.0f, 0.0f, 500.0f, 0 },
    { EnemyProperty::BaseAttackCooldown, "Base attack cooldown", 0.1f, 0.0f, 10.0f, 1 },
    { EnemyProperty::CurrentAttackCooldown, "Initial attack cooldown", 0.1f, 0.0f, 10.0f, 1 },
    { EnemyProperty::DazeDuration, "Daze duration", 0.1f, 0.0f, 10.0f, 1 },
    { EnemyProperty::KnockbackResistance, "Push resistance", 0.05f, 0.0f, 1.0f, 2 },
    { EnemyProperty::HitboxWidth, "Hitbox width", 1.0f, 1.0f, 256.0f, 0 },
    { EnemyProperty::HitboxHeight, "Hitbox height", 1.0f, 1.0f, 256.0f, 0 },
    { EnemyProperty::NavigationWidth, "Collision width", 1.0f, 1.0f, 256.0f, 0 },
    { EnemyProperty::NavigationHeight, "Collision height", 1.0f, 1.0f, 256.0f, 0 },
    { EnemyProperty::NavigationOffsetX, "Collision offset X", 1.0f, -128.0f, 128.0f, 0 },
    { EnemyProperty::NavigationOffsetY, "Collision offset Y", 1.0f, -128.0f, 128.0f, 0 }
}};

constexpr std::array<std::array<float, 14>, 5> DEFAULT_SPAWN_VALUES = {{
    { 80.0f, 80.0f, 150.0f, 15.0f, 1.0f, 1.0f, 2.0f, 0.25f,
      20.0f, 20.0f, 16.0f, 8.0f, 0.0f, 8.0f },
    { 70.0f, 70.0f, 120.0f, 12.0f, 1.0f, 1.0f, 2.0f, 0.10f,
      24.0f, 24.0f, 18.0f, 8.0f, 0.0f, 8.0f },
    { 200.0f, 200.0f, 160.0f, 70.0f, 2.5f, 2.5f, 2.0f, 0.50f,
      24.0f, 24.0f, 18.0f, 8.0f, 0.0f, 8.0f },
    { 500.0f, 500.0f, 75.0f, 25.0f, 0.8f, 0.8f, 2.0f, 1.0f,
      47.0f, 69.0f, 32.0f, 12.0f, 0.0f, 28.5f },
    { 120.0f, 120.0f, 100.0f, 15.0f, 0.4f, 0.0f, 2.0f, 0.25f,
      34.0f, 30.0f, 14.0f, 8.0f, 0.0f, 10.5f }
}};

/// Reports whether the point inside condition is satisfied.
bool IsPointInside(Rectangle bounds, Vector2 point) {
    return CheckCollisionPointRec(point, bounds);
}

/// Implements the was button pressed behavior for this component.
bool WasButtonPressed(Rectangle bounds, Vector2 mousePosition) {
    return IsPointInside(bounds, mousePosition) &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

/// Renders button.
void DrawButton(
    Rectangle bounds,
    const char* text,
    Vector2 mousePosition,
    bool selected = false
) {
    Color color = selected
        ? BUTTON_SELECTED_COLOR
        : (IsPointInside(bounds, mousePosition)
            ? BUTTON_HOVER_COLOR
            : BUTTON_COLOR);
    DrawRectangleRec(bounds, color);
    DrawRectangleLinesEx(bounds, 1.0f, PANEL_BORDER_COLOR);
    int textWidth = MeasureTextEx(AssetManager::GetInstance().GetCustomFont("PixeloidSans"), text, TEXT_SIZE, 1.0f).x;
    DrawTextAdmin(
        text,
        (int)(bounds.x + (bounds.width - textWidth) * 0.5f),
        (int)(bounds.y + (bounds.height - TEXT_SIZE) * 0.5f),
        TEXT_SIZE,
        RAYWHITE
    );
}

/// Calculates and returns enemy type name.
const char* EnemyTypeName(MapObjectId type) {
    switch (type) {
        case MapObjectId::Chaser: return "Chaser";
        case MapObjectId::Range: return "Ranger";
        case MapObjectId::Diver: return "Diver";
        case MapObjectId::Boss: return "Boss";
        case MapObjectId::DemonTHA: return "Demon_THA";
        default: return "Unknown";
    }
}

/// Implements the slider value from mouse behavior for this component.
float SliderValueFromMouse(
    Rectangle track,
    Vector2 mousePosition,
    const PropertyDefinition& definition
) {
    float ratio = std::clamp(
        (mousePosition.x - track.x) / track.width,
        0.0f,
        1.0f
    );
    float rawValue = definition.minimum +
        ratio * (definition.maximum - definition.minimum);
    float steppedValue = definition.minimum + std::round(
        (rawValue - definition.minimum) / definition.step
    ) * definition.step;
    return std::clamp(
        steppedValue,
        definition.minimum,
        definition.maximum
    );
}

/// Renders toggle row.
void DrawToggleRow(
    Rectangle row,
    const char* label,
    bool enabled,
    Vector2 mousePosition
) {
    Rectangle box = { row.x, row.y + 4.0f, 23.0f, 23.0f };
    DrawRectangleRec(box, enabled ? BUTTON_SELECTED_COLOR : BUTTON_COLOR);
    DrawRectangleLinesEx(box, 1.0f, PANEL_BORDER_COLOR);
    if (enabled) {
        DrawTextAdmin("X", (int)box.x + 5, (int)box.y + 1, TEXT_SIZE, RAYWHITE);
    }
    DrawTextAdmin(
        label,
        (int)row.x + 34,
        (int)row.y + 6,
        TEXT_SIZE,
        IsPointInside(row, mousePosition) ? WHITE : LIGHTGRAY
    );
}
}

/// Creates a AdminPanel instance from the supplied configuration.
AdminPanel::AdminPanel()
    : open(false),
      placementArmed(false),
      spawnType(MapObjectId::Chaser),
      spawnValues(DEFAULT_SPAWN_VALUES),
      propertyScroll(0.0f),
      statusMessage("Select a type, tune values, then click the world"),
      pathFlowBuildsPerSecond(0),
      pathFlowProfiles(0),
      pathFlowAverageMilliseconds(0.0f),
      pathSearchesPerSecond(0),
      pathAverageMilliseconds(0.0f),
      pathMaximumMilliseconds(0.0f) {
}

/// Returns the current panel bounds.
Rectangle AdminPanel::GetPanelBounds() const {
    return {
        PANEL_X,
        PANEL_Y,
        std::min(PANEL_WIDTH, std::max(260.0f, (float)GetScreenWidth() - 20.0f)),
        std::max(430.0f, (float)GetScreenHeight() - PANEL_Y * 2.0f)
    };
}

/// Returns the current property viewport.
Rectangle AdminPanel::GetPropertyViewport() const {
    Rectangle panel = GetPanelBounds();
    return {
        panel.x + PANEL_PADDING,
        panel.y + PROPERTY_START_Y,
        panel.width - PANEL_PADDING * 2.0f,
        std::max(40.0f, panel.height - PROPERTY_START_Y - PANEL_PADDING)
    };
}

/// Returns the current spawn type index.
std::size_t AdminPanel::GetSpawnTypeIndex() const {
    switch (spawnType) {
        case MapObjectId::Chaser: return 0;
        case MapObjectId::Range: return 1;
        case MapObjectId::Diver: return 2;
        case MapObjectId::Boss: return 3;
        case MapObjectId::DemonTHA: return 4;
        default: return 0;
    }
}

/// Spawns selected type.
void AdminPanel::SpawnSelectedType(
    Vector2 worldMousePosition,
    LevelManager& levelManager,
    TeamManager* teamManager
) {
    if (!teamManager) {
        statusMessage = "Team is not initialized";
        return;
    }

    ObjectManager& objectManager =
        GameManager::GetInstance().GetObjectManager();
    GameObject* object = objectManager.Spawn(
        spawnType,
        worldMousePosition
    );
    if (!object) {
        statusMessage = "Selected enemy type could not be created";
        return;
    }

    Enemy* enemy = dynamic_cast<Enemy*>(object);
    if (!enemy) {
        statusMessage = "Selected object is not an enemy";
        return;
    }
    const std::array<float, SPAWN_PROPERTY_COUNT>& values =
        spawnValues[GetSpawnTypeIndex()];

    enemy->SetMaxHealth((int)std::round(values[1]));
    enemy->SetHealth((int)std::round(std::min(values[0], values[1])));
    enemy->SetSpeed(values[2]);
    enemy->SetDamage((int)std::round(values[3]));
    enemy->SetBaseAttackCooldown(values[4]);
    enemy->SetAttackCooldown(values[5]);
    enemy->SetDazeDuration(values[6]);
    enemy->SetKnockbackResistance(values[7]);
    enemy->SetSize({ values[8], values[9] });
    enemy->SetCollisionProfile({
        { values[10], values[11] },
        { values[12], values[13] }
    });

    if (levelManager.IsSolidCollision(enemy->GetCollisionBox())) {
        objectManager.QueueRemoval(object);
        objectManager.CommitPendingChanges();
        statusMessage = "Spawn blocked by level collision";
        return;
    }

    statusMessage = std::string("Spawned configured ") +
        EnemyTypeName(spawnType);
}

/// Deletes all enemies.
void AdminPanel::DeleteAllEnemies(LevelManager& levelManager) {
    (void)levelManager;
    ObjectManager& objectManager =
        GameManager::GetInstance().GetObjectManager();
    int enemyCount = static_cast<int>(objectManager.GetEnemyCount());
    objectManager.DeleteAllEnemies();

    statusMessage = enemyCount > 0
        ? std::string("Removing ") + std::to_string(enemyCount) +
            " enemies"
        : "No enemies to delete";
}

/// Updates property editor.
void AdminPanel::UpdatePropertyEditor(Vector2 mousePosition) {
    Rectangle viewport = GetPropertyViewport();
    float totalHeight = (float)PROPERTIES.size() * PROPERTY_ROW_HEIGHT;
    float maximumScroll = std::max(0.0f, totalHeight - viewport.height);
    if (IsPointInside(viewport, mousePosition)) {
        propertyScroll = std::clamp(
            propertyScroll - GetMouseWheelMove() * PROPERTY_ROW_HEIGHT * 1.5f,
            0.0f,
            maximumScroll
        );
    }

    std::array<float, SPAWN_PROPERTY_COUNT>& values =
        spawnValues[GetSpawnTypeIndex()];
    for (std::size_t index = 0; index < PROPERTIES.size(); ++index) {
        const PropertyDefinition& definition = PROPERTIES[index];
        Rectangle row = {
            viewport.x,
            viewport.y + (float)index * PROPERTY_ROW_HEIGHT - propertyScroll,
            viewport.width,
            PROPERTY_ROW_HEIGHT - 2.0f
        };
        Rectangle sliderTrack = {
            row.x + 9.0f,
            row.y + 39.0f,
            row.width - 18.0f,
            SLIDER_TRACK_HEIGHT
        };
        Rectangle sliderInput = {
            sliderTrack.x - SLIDER_THUMB_RADIUS,
            sliderTrack.y - SLIDER_THUMB_RADIUS,
            sliderTrack.width + SLIDER_THUMB_RADIUS * 2.0f,
            sliderTrack.height + SLIDER_THUMB_RADIUS * 2.0f
        };
        if (!IsPointInside(viewport, mousePosition) ||
            !IsPointInside(sliderInput, mousePosition) ||
            !IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            continue;
        }

        values[index] = SliderValueFromMouse(
            sliderTrack,
            mousePosition,
            definition
        );
        if (definition.property == EnemyProperty::Health) {
            values[index] = std::min(values[index], values[1]);
        } else if (definition.property == EnemyProperty::MaxHealth) {
            values[0] = std::min(values[0], values[index]);
        }
    }
}

/// Advances this component's state for the current frame.
void AdminPanel::Update(
    Vector2 worldMousePosition,
    LevelManager& levelManager,
    TeamManager* teamManager,
    GameState gameState
) {
    if (!Constants::ENABLE_ADMIN_GUI) {
        open = false;
        return;
    }

    if (IsKeyPressed(KEY_F1)) {
        open = !open;
    }
    if (!open) return;

    const EnemyPathProfilingStats& pathStats =
        GameManager::GetInstance()
            .GetPathFindingManager()
            .GetProfilingStats();
    pathFlowBuildsPerSecond = pathStats.flowFieldBuildsLastSecond;
    pathFlowProfiles = pathStats.activeFlowFieldProfiles;
    pathFlowAverageMilliseconds =
        pathStats.averageFlowFieldMilliseconds;
    pathSearchesPerSecond = pathStats.searchesLastSecond;
    pathAverageMilliseconds = pathStats.averageSearchMilliseconds;
    pathMaximumMilliseconds = pathStats.maximumSearchMilliseconds;

    Vector2 mousePosition = GetMousePosition();
    Rectangle panel = GetPanelBounds();
    float contentX = panel.x + PANEL_PADDING;
    float contentWidth = panel.width - PANEL_PADDING * 2.0f;

    Rectangle collisionToggle = {
        contentX, panel.y + TOGGLE_START_Y, contentWidth, TOGGLE_HEIGHT
    };
    Rectangle pathToggle = {
        contentX, panel.y + TOGGLE_START_Y + 36.0f, contentWidth, TOGGLE_HEIGHT
    };
    Rectangle lineOfSightToggle = {
        contentX, panel.y + TOGGLE_START_Y + 72.0f, contentWidth, TOGGLE_HEIGHT
    };
    Rectangle immunityToggle = {
        contentX, panel.y + TOGGLE_START_Y + 108.0f, contentWidth, TOGGLE_HEIGHT
    };
    if (WasButtonPressed(collisionToggle, mousePosition)) {
        Constants::DEBUG_DRAW_ENTITY_COLLISION_BOXES =
            !Constants::DEBUG_DRAW_ENTITY_COLLISION_BOXES;
    }
    if (WasButtonPressed(pathToggle, mousePosition)) {
        Constants::DEBUG_DRAW_ENEMY_PATHS =
            !Constants::DEBUG_DRAW_ENEMY_PATHS;
    }
    if (WasButtonPressed(lineOfSightToggle, mousePosition)) {
        Constants::DEBUG_DRAW_LINE_OF_SIGHT =
            !Constants::DEBUG_DRAW_LINE_OF_SIGHT;
    }
    if (WasButtonPressed(immunityToggle, mousePosition)) {
        Constants::DEBUG_PLAYER_IMMUNITY =
            !Constants::DEBUG_PLAYER_IMMUNITY;
    }

    float buttonWidth = (contentWidth - 8.0f) * 0.5f;
    for (std::size_t index = 0; index < SPAWN_TYPES.size(); ++index) {
        Rectangle button = {
            contentX + (index % SPAWN_COLUMN_COUNT) *
                (buttonWidth + 8.0f),
            panel.y + SPAWN_BUTTON_START_Y +
                (index / SPAWN_COLUMN_COUNT) *
                    (BUTTON_HEIGHT + SPAWN_BUTTON_GAP_Y),
            buttonWidth,
            BUTTON_HEIGHT
        };
        if (WasButtonPressed(button, mousePosition)) {
            spawnType = SPAWN_TYPES[index];
            placementArmed = true;
            propertyScroll = 0.0f;
            statusMessage = std::string("Editing next ") +
                EnemyTypeName(spawnType) + " spawn";
        }
    }

    float actionButtonWidth = (contentWidth - 8.0f) * 0.5f;
    Rectangle cancelButton = {
        contentX,
        panel.y + ACTION_BUTTON_Y,
        actionButtonWidth,
        BUTTON_HEIGHT
    };
    Rectangle deleteAllButton = {
        contentX + actionButtonWidth + 8.0f,
        panel.y + ACTION_BUTTON_Y,
        actionButtonWidth,
        BUTTON_HEIGHT
    };
    Rectangle skipRoomButton = {
        contentX,
        panel.y + ACTION_BUTTON_Y + BUTTON_HEIGHT + ACTION_ROW_GAP,
        actionButtonWidth,
        BUTTON_HEIGHT
    };
    Rectangle skipFloorButton = {
        contentX + actionButtonWidth + 8.0f,
        panel.y + ACTION_BUTTON_Y + BUTTON_HEIGHT + ACTION_ROW_GAP,
        actionButtonWidth,
        BUTTON_HEIGHT
    };
    if (WasButtonPressed(cancelButton, mousePosition)) {
        placementArmed = false;
        statusMessage = "Placement cancelled; values are preserved";
    }
    if (WasButtonPressed(deleteAllButton, mousePosition)) {
        DeleteAllEnemies(levelManager);
    }
    if (WasButtonPressed(skipRoomButton, mousePosition)) {
        bool skipped = GameManager::GetInstance()
            .GetWaveManager()
            .SkipCurrentRoom(teamManager, &levelManager);
        statusMessage = skipped
            ? "Current room cleared"
            : "Enter a locked combat room first";
    }
    if (WasButtonPressed(skipFloorButton, mousePosition)) {
        if (gameState != GameState::GAMEPLAY) {
            statusMessage = "Skip floor is available during gameplay only";
        } else {
            GameManager& gameManager = GameManager::GetInstance();
            placementArmed = false;
            gameManager.AdvanceFloorCount();
            gameManager.ClearProjectiles();

            if (gameManager.GetCurrentFloor() > GameManager::MAX_FLOORS) {
                gameManager.SetState(GameState::VICTORY);
                statusMessage = "Final floor skipped; victory triggered";
            } else {
                gameManager.GenerateDungeon();
                gameManager.GetWaveManager().Reset(0, 0, 0);
                statusMessage = std::string("Teleported to floor ") +
                    std::to_string(gameManager.GetCurrentFloor());
            }
        }
    }

    UpdatePropertyEditor(mousePosition);

    bool worldState = gameState == GameState::HUB ||
        gameState == GameState::GAMEPLAY;
    if (!worldState || IsPointInside(panel, mousePosition)) return;

    if (placementArmed && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        SpawnSelectedType(worldMousePosition, levelManager, teamManager);
    }
}

/// Renders property editor.
void AdminPanel::DrawPropertyEditor() const {
    Rectangle viewport = GetPropertyViewport();
    DrawRectangleRec(viewport, Color{ 12, 16, 23, 230 });

    BeginScissorMode(
        (int)viewport.x,
        (int)viewport.y,
        (int)viewport.width,
        (int)viewport.height
    );

    const std::array<float, SPAWN_PROPERTY_COUNT>& values =
        spawnValues[GetSpawnTypeIndex()];
    for (std::size_t index = 0; index < PROPERTIES.size(); ++index) {
        const PropertyDefinition& definition = PROPERTIES[index];
        Rectangle row = {
            viewport.x,
            viewport.y + (float)index * PROPERTY_ROW_HEIGHT - propertyScroll,
            viewport.width,
            PROPERTY_ROW_HEIGHT - 2.0f
        };
        if (row.y + row.height < viewport.y ||
            row.y > viewport.y + viewport.height) {
            continue;
        }

        if (index % 2 == 0) {
            DrawRectangleRec(row, Color{ 25, 31, 43, 180 });
        }
        DrawTextAdmin(
            definition.label,
            (int)row.x + 8,
            (int)row.y + 7,
            TEXT_SIZE,
            LIGHTGRAY
        );

        char valueText[32] = {};
        std::snprintf(
            valueText,
            sizeof(valueText),
            definition.decimals == 0 ? "%.0f" :
                (definition.decimals == 1 ? "%.1f" : "%.2f"),
            values[index]
        );
        int valueWidth = MeasureTextEx(AssetManager::GetInstance().GetCustomFont("PixeloidSans"), valueText, TEXT_SIZE, 1.0f).x;
        DrawTextAdmin(
            valueText,
            (int)(row.x + row.width - valueWidth - 10.0f),
            (int)row.y + 7,
            TEXT_SIZE,
            RAYWHITE
        );

        Rectangle sliderTrack = {
            row.x + 9.0f,
            row.y + 39.0f,
            row.width - 18.0f,
            SLIDER_TRACK_HEIGHT
        };
        float ratio = (values[index] - definition.minimum) /
            (definition.maximum - definition.minimum);
        ratio = std::clamp(ratio, 0.0f, 1.0f);
        DrawRectangleRec(sliderTrack, SLIDER_TRACK_COLOR);
        DrawRectangleRec(
            { sliderTrack.x, sliderTrack.y, sliderTrack.width * ratio,
              sliderTrack.height },
            SLIDER_FILL_COLOR
        );
        DrawCircleV(
            { sliderTrack.x + sliderTrack.width * ratio,
              sliderTrack.y + sliderTrack.height * 0.5f },
            SLIDER_THUMB_RADIUS,
            RAYWHITE
        );
    }
    EndScissorMode();

    float totalHeight = (float)PROPERTIES.size() * PROPERTY_ROW_HEIGHT;
    if (totalHeight > viewport.height) {
        float trackHeight = viewport.height;
        float thumbHeight = trackHeight * viewport.height / totalHeight;
        float maximumScroll = totalHeight - viewport.height;
        float thumbY = viewport.y + propertyScroll / maximumScroll *
            (trackHeight - thumbHeight);
        DrawRectangle(
            (int)(viewport.x + viewport.width - 4.0f),
            (int)thumbY,
            4,
            (int)thumbHeight,
            PANEL_BORDER_COLOR
        );
    }
}

/// Renders this component using its current state and visual resources.
void AdminPanel::Draw() const {
    if (!Constants::ENABLE_ADMIN_GUI || !open) return;

    Rectangle panel = GetPanelBounds();
    Vector2 mousePosition = GetMousePosition();
    float contentX = panel.x + PANEL_PADDING;
    float contentWidth = panel.width - PANEL_PADDING * 2.0f;

    DrawRectangleRec(panel, PANEL_COLOR);
    DrawRectangleLinesEx(panel, 1.0f, PANEL_BORDER_COLOR);
    DrawTextAdmin(
        "ADMIN PANEL",
        (int)contentX,
        (int)panel.y + 10,
        TITLE_TEXT_SIZE,
        GOLD
    );
    DrawTextAdmin(
        "F1: hide",
        (int)(panel.x + panel.width - 78.0f),
        (int)panel.y + 17,
        SMALL_TEXT_SIZE,
        GRAY
    );
    const FramePerformanceSnapshot& frameStats =
        FramePerformanceStats::GetInstance().GetSnapshot();
    DrawTextAdmin(
        TextFormat(
            "FPS %.0f | avg %.1f | min %.1f | max %.0f",
            frameStats.currentFps,
            frameStats.averageFps,
            frameStats.lowestFps,
            frameStats.highestFps
        ),
        (int)contentX,
        (int)panel.y + 38,
        13,
        LIME
    );
    DrawTextAdmin(
        TextFormat(
            "1%% low %.1f | 0.1%% low %.1f | below target %.1f%% | hitch %.1f%%",
            frameStats.onePercentLowFps,
            frameStats.pointOnePercentLowFps,
            frameStats.belowTargetPercent,
            frameStats.hitchPercent
        ),
        (int)contentX,
        (int)panel.y + 54,
        13,
        SKYBLUE
    );
    DrawTextAdmin(
        TextFormat(
            "Frame ms: now %.2f avg %.2f P95 %.2f P99 %.2f max %.2f",
            frameStats.currentFrameMilliseconds,
            frameStats.averageFrameMilliseconds,
            frameStats.p95FrameMilliseconds,
            frameStats.p99FrameMilliseconds,
            frameStats.maximumFrameMilliseconds
        ),
        (int)contentX,
        (int)panel.y + 70,
        13,
        LIGHTGRAY
    );
    if (Constants::DEBUG_SHOW_PATHFINDING_PROFILING) {
        DrawTextAdmin(
            TextFormat(
                "Flow: %i/s (%i) %.2fms | A*: %i/s %.2fms",
                pathFlowBuildsPerSecond,
                pathFlowProfiles,
                pathFlowAverageMilliseconds,
                pathSearchesPerSecond,
                pathAverageMilliseconds
            ),
            (int)contentX,
            (int)panel.y + 86,
            13,
            SKYBLUE
        );
    }

    DrawToggleRow(
        { contentX, panel.y + TOGGLE_START_Y, contentWidth, TOGGLE_HEIGHT },
        "Hitbox / collision boxes",
        Constants::DEBUG_DRAW_ENTITY_COLLISION_BOXES,
        mousePosition
    );
    DrawToggleRow(
        { contentX, panel.y + TOGGLE_START_Y + 36.0f, contentWidth, TOGGLE_HEIGHT },
        "Enemy paths and target points",
        Constants::DEBUG_DRAW_ENEMY_PATHS,
        mousePosition
    );
    DrawToggleRow(
        { contentX, panel.y + TOGGLE_START_Y + 72.0f, contentWidth, TOGGLE_HEIGHT },
        "All line-of-sight queries",
        Constants::DEBUG_DRAW_LINE_OF_SIGHT,
        mousePosition
    );
    DrawToggleRow(
        { contentX, panel.y + TOGGLE_START_Y + 108.0f, contentWidth, TOGGLE_HEIGHT },
        "Player immunity",
        Constants::DEBUG_PLAYER_IMMUNITY,
        mousePosition
    );

    DrawTextAdmin(
        "SPAWN ENEMY",
        (int)contentX,
        (int)panel.y + SPAWN_HEADING_Y,
        TEXT_SIZE,
        GOLD
    );
    float buttonWidth = (contentWidth - 8.0f) * 0.5f;
    for (std::size_t index = 0; index < SPAWN_TYPES.size(); ++index) {
        Rectangle button = {
            contentX + (index % SPAWN_COLUMN_COUNT) *
                (buttonWidth + 8.0f),
            panel.y + SPAWN_BUTTON_START_Y +
                (index / SPAWN_COLUMN_COUNT) *
                    (BUTTON_HEIGHT + SPAWN_BUTTON_GAP_Y),
            buttonWidth,
            BUTTON_HEIGHT
        };
        DrawButton(
            button,
            EnemyTypeName(SPAWN_TYPES[index]),
            mousePosition,
            spawnType == SPAWN_TYPES[index]
        );
    }

    float actionButtonWidth = (contentWidth - 8.0f) * 0.5f;
    Rectangle cancelButton = {
        contentX,
        panel.y + ACTION_BUTTON_Y,
        actionButtonWidth,
        BUTTON_HEIGHT
    };
    Rectangle deleteAllButton = {
        contentX + actionButtonWidth + 8.0f,
        panel.y + ACTION_BUTTON_Y,
        actionButtonWidth,
        BUTTON_HEIGHT
    };
    Rectangle skipRoomButton = {
        contentX,
        panel.y + ACTION_BUTTON_Y + BUTTON_HEIGHT + ACTION_ROW_GAP,
        actionButtonWidth,
        BUTTON_HEIGHT
    };
    Rectangle skipFloorButton = {
        contentX + actionButtonWidth + 8.0f,
        panel.y + ACTION_BUTTON_Y + BUTTON_HEIGHT + ACTION_ROW_GAP,
        actionButtonWidth,
        BUTTON_HEIGHT
    };
    DrawButton(cancelButton, "Cancel placement", mousePosition);
    DrawButton(deleteAllButton, "Delete enemies", mousePosition);
    DrawButton(skipRoomButton, "Skip room", mousePosition);
    DrawButton(skipFloorButton, "Skip floor", mousePosition);

    DrawTextAdmin(
        statusMessage.c_str(),
        (int)contentX,
        (int)(panel.y + STATUS_TEXT_Y),
        SMALL_TEXT_SIZE,
        placementArmed ? YELLOW : LIGHTGRAY
    );

    std::string heading = std::string("NEXT ") +
        EnemyTypeName(spawnType) + " SPAWN VALUES";
    DrawTextAdmin(
        heading.c_str(),
        (int)contentX,
        (int)(panel.y + PROPERTY_HEADING_Y),
        TEXT_SIZE,
        GOLD
    );

    DrawPropertyEditor();
}
