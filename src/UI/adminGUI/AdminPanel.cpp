#include "UI/adminGUI/AdminPanel.h"

#include "Core/Constants.h"
#include "Core/EntityFactory.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/LevelManager.h"
#include "Core/Manager/TeamManager.h"
#include "Entities/Enemy.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>

namespace {
constexpr float PANEL_X = 10.0f;
constexpr float PANEL_Y = 10.0f;
constexpr float PANEL_WIDTH = 460.0f;
constexpr float PANEL_PADDING = 14.0f;
constexpr float TOGGLE_HEIGHT = 32.0f;
constexpr float BUTTON_HEIGHT = 38.0f;
constexpr float PROPERTY_ROW_HEIGHT = 58.0f;
constexpr float PROPERTY_START_Y = 374.0f;
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

constexpr std::array<std::array<float, 14>, 4> DEFAULT_SPAWN_VALUES = {{
    { 80.0f, 80.0f, 150.0f, 15.0f, 1.0f, 1.0f, 2.0f, 0.25f,
      20.0f, 20.0f, 16.0f, 8.0f, 0.0f, 8.0f },
    { 70.0f, 70.0f, 120.0f, 12.0f, 1.0f, 1.0f, 2.0f, 0.10f,
      24.0f, 24.0f, 18.0f, 8.0f, 0.0f, 8.0f },
    { 200.0f, 200.0f, 160.0f, 70.0f, 2.5f, 2.5f, 2.0f, 0.50f,
      24.0f, 24.0f, 18.0f, 8.0f, 0.0f, 8.0f },
    { 500.0f, 500.0f, 75.0f, 25.0f, 0.8f, 0.8f, 2.0f, 1.0f,
      96.0f, 124.0f, 56.0f, 18.0f, 0.0f, 53.0f }
}};

bool IsPointInside(Rectangle bounds, Vector2 point) {
    return CheckCollisionPointRec(point, bounds);
}

bool WasButtonPressed(Rectangle bounds, Vector2 mousePosition) {
    return IsPointInside(bounds, mousePosition) &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

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
    int textWidth = MeasureText(text, TEXT_SIZE);
    DrawText(
        text,
        (int)(bounds.x + (bounds.width - textWidth) * 0.5f),
        (int)(bounds.y + (bounds.height - TEXT_SIZE) * 0.5f),
        TEXT_SIZE,
        RAYWHITE
    );
}

const char* EnemyTypeName(MapObjectId type) {
    switch (type) {
        case MapObjectId::Chaser: return "Chaser";
        case MapObjectId::Range: return "Ranger";
        case MapObjectId::Diver: return "Diver";
        case MapObjectId::Boss: return "Boss";
        default: return "Unknown";
    }
}

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
        DrawText("X", (int)box.x + 5, (int)box.y + 1, TEXT_SIZE, RAYWHITE);
    }
    DrawText(
        label,
        (int)row.x + 34,
        (int)row.y + 6,
        TEXT_SIZE,
        IsPointInside(row, mousePosition) ? WHITE : LIGHTGRAY
    );
}
}

AdminPanel::AdminPanel()
    : open(Constants::ENABLE_ADMIN_GUI),
      placementArmed(false),
      spawnType(MapObjectId::Chaser),
      spawnValues(DEFAULT_SPAWN_VALUES),
      propertyScroll(0.0f),
      statusMessage("Select a type, tune values, then click the world"),
      pathSearchesPerSecond(0),
      pathAverageMilliseconds(0.0f),
      pathMaximumMilliseconds(0.0f) {
}

Rectangle AdminPanel::GetPanelBounds() const {
    return {
        PANEL_X,
        PANEL_Y,
        std::min(PANEL_WIDTH, std::max(260.0f, (float)GetScreenWidth() - 20.0f)),
        std::max(430.0f, (float)GetScreenHeight() - PANEL_Y * 2.0f)
    };
}

Rectangle AdminPanel::GetPropertyViewport() const {
    Rectangle panel = GetPanelBounds();
    return {
        panel.x + PANEL_PADDING,
        panel.y + PROPERTY_START_Y,
        panel.width - PANEL_PADDING * 2.0f,
        std::max(40.0f, panel.height - PROPERTY_START_Y - PANEL_PADDING)
    };
}

std::size_t AdminPanel::GetSpawnTypeIndex() const {
    switch (spawnType) {
        case MapObjectId::Chaser: return 0;
        case MapObjectId::Range: return 1;
        case MapObjectId::Diver: return 2;
        case MapObjectId::Boss: return 3;
        default: return 0;
    }
}

bool AdminPanel::IsMouseOverPanel() const {
    return Constants::ENABLE_ADMIN_GUI && open &&
        IsPointInside(GetPanelBounds(), GetMousePosition());
}

void AdminPanel::SpawnSelectedType(
    Vector2 worldMousePosition,
    LevelManager& levelManager,
    TeamManager* teamManager
) {
    if (!teamManager) {
        statusMessage = "Team is not initialized";
        return;
    }

    GameObject* object = EntityFactory::CreateEntity(
        spawnType,
        worldMousePosition,
        { -1, -1 },
        teamManager,
        levelManager.GetLevelAccessBundle()
    );
    if (!object) {
        statusMessage = "Selected enemy type could not be created";
        return;
    }

    Enemy* enemy = static_cast<Enemy*>(object);
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

    if (!levelManager.IsValidSpawnLocation(object)) {
        delete object;
        statusMessage = "Spawn blocked by level collision";
        return;
    }

    levelManager.AddEntity(object);
    statusMessage = std::string("Spawned configured ") +
        EnemyTypeName(spawnType);
}

void AdminPanel::DeleteAllEnemies(LevelManager& levelManager) {
    int enemyCount = 0;
    for (GameObject* entity : levelManager.GetEntities()) {
        if (entity && entity->GetObjectType() == GameObjectType::Enemy) {
            levelManager.QueueRemoval(entity);
            ++enemyCount;
        }
    }

    statusMessage = enemyCount > 0
        ? std::string("Removing ") + std::to_string(enemyCount) +
            " enemies"
        : "No enemies to delete";
}

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
        levelManager.GetEnemyPathProfilingStats();
    pathSearchesPerSecond = pathStats.searchesLastSecond;
    pathAverageMilliseconds = pathStats.averageSearchMilliseconds;
    pathMaximumMilliseconds = pathStats.maximumSearchMilliseconds;

    Vector2 mousePosition = GetMousePosition();
    Rectangle panel = GetPanelBounds();
    float contentX = panel.x + PANEL_PADDING;
    float contentWidth = panel.width - PANEL_PADDING * 2.0f;

    Rectangle collisionToggle = {
        contentX, panel.y + 52.0f, contentWidth, TOGGLE_HEIGHT
    };
    Rectangle pathToggle = {
        contentX, panel.y + 88.0f, contentWidth, TOGGLE_HEIGHT
    };
    Rectangle immunityToggle = {
        contentX, panel.y + 124.0f, contentWidth, TOGGLE_HEIGHT
    };
    if (WasButtonPressed(collisionToggle, mousePosition)) {
        Constants::DEBUG_DRAW_ENTITY_COLLISION_BOXES =
            !Constants::DEBUG_DRAW_ENTITY_COLLISION_BOXES;
    }
    if (WasButtonPressed(pathToggle, mousePosition)) {
        Constants::DEBUG_DRAW_ENEMY_PATHS =
            !Constants::DEBUG_DRAW_ENEMY_PATHS;
    }
    if (WasButtonPressed(immunityToggle, mousePosition)) {
        Constants::DEBUG_PLAYER_IMMUNITY =
            !Constants::DEBUG_PLAYER_IMMUNITY;
    }

    constexpr std::array<MapObjectId, 4> TYPES = {
        MapObjectId::Chaser,
        MapObjectId::Range,
        MapObjectId::Diver,
        MapObjectId::Boss
    };
    float buttonWidth = (contentWidth - 8.0f) * 0.5f;
    for (std::size_t index = 0; index < TYPES.size(); ++index) {
        Rectangle button = {
            contentX + (index % 2) * (buttonWidth + 8.0f),
            panel.y + 195.0f + (index / 2) * (BUTTON_HEIGHT + 6.0f),
            buttonWidth,
            BUTTON_HEIGHT
        };
        if (WasButtonPressed(button, mousePosition)) {
            spawnType = TYPES[index];
            placementArmed = true;
            propertyScroll = 0.0f;
            statusMessage = std::string("Editing next ") +
                EnemyTypeName(spawnType) + " spawn";
        }
    }

    float actionButtonWidth = (contentWidth - 8.0f) * 0.5f;
    Rectangle cancelButton = {
        contentX,
        panel.y + 285.0f,
        actionButtonWidth,
        BUTTON_HEIGHT
    };
    Rectangle deleteAllButton = {
        contentX + actionButtonWidth + 8.0f,
        panel.y + 285.0f,
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

    UpdatePropertyEditor(mousePosition);

    bool worldState = gameState == GameState::HUB ||
        gameState == GameState::GAMEPLAY;
    if (!worldState || IsPointInside(panel, mousePosition)) return;

    if (placementArmed && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        SpawnSelectedType(worldMousePosition, levelManager, teamManager);
    }
}

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
        DrawText(
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
        int valueWidth = MeasureText(valueText, TEXT_SIZE);
        DrawText(
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

void AdminPanel::Draw() const {
    if (!Constants::ENABLE_ADMIN_GUI || !open) return;

    Rectangle panel = GetPanelBounds();
    Vector2 mousePosition = GetMousePosition();
    float contentX = panel.x + PANEL_PADDING;
    float contentWidth = panel.width - PANEL_PADDING * 2.0f;

    DrawRectangleRec(panel, PANEL_COLOR);
    DrawRectangleLinesEx(panel, 1.0f, PANEL_BORDER_COLOR);
    DrawText(
        "ADMIN PANEL",
        (int)contentX,
        (int)panel.y + 10,
        TITLE_TEXT_SIZE,
        GOLD
    );
    DrawText(
        "F1: hide",
        (int)(panel.x + panel.width - 78.0f),
        (int)panel.y + 17,
        SMALL_TEXT_SIZE,
        GRAY
    );
    DrawText(
        TextFormat("FPS: %i", GetFPS()),
        (int)(panel.x + panel.width - 158.0f),
        (int)panel.y + 17,
        SMALL_TEXT_SIZE,
        LIME
    );
    if (Constants::DEBUG_SHOW_PATHFINDING_PROFILING) {
        DrawText(
            TextFormat(
                "Path: %i/s  avg %.2f ms  max %.2f ms",
                pathSearchesPerSecond,
                pathAverageMilliseconds,
                pathMaximumMilliseconds
            ),
            (int)contentX,
            (int)panel.y + 36,
            13,
            SKYBLUE
        );
    }

    DrawToggleRow(
        { contentX, panel.y + 52.0f, contentWidth, TOGGLE_HEIGHT },
        "Hitbox / collision boxes",
        Constants::DEBUG_DRAW_ENTITY_COLLISION_BOXES,
        mousePosition
    );
    DrawToggleRow(
        { contentX, panel.y + 88.0f, contentWidth, TOGGLE_HEIGHT },
        "Enemy paths and target points",
        Constants::DEBUG_DRAW_ENEMY_PATHS,
        mousePosition
    );
    DrawToggleRow(
        { contentX, panel.y + 124.0f, contentWidth, TOGGLE_HEIGHT },
        "Player immunity",
        Constants::DEBUG_PLAYER_IMMUNITY,
        mousePosition
    );

    DrawText(
        "SPAWN ENEMY",
        (int)contentX,
        (int)panel.y + 168,
        TEXT_SIZE,
        GOLD
    );
    constexpr std::array<MapObjectId, 4> TYPES = {
        MapObjectId::Chaser,
        MapObjectId::Range,
        MapObjectId::Diver,
        MapObjectId::Boss
    };
    float buttonWidth = (contentWidth - 8.0f) * 0.5f;
    for (std::size_t index = 0; index < TYPES.size(); ++index) {
        Rectangle button = {
            contentX + (index % 2) * (buttonWidth + 8.0f),
            panel.y + 195.0f + (index / 2) * (BUTTON_HEIGHT + 6.0f),
            buttonWidth,
            BUTTON_HEIGHT
        };
        DrawButton(
            button,
            EnemyTypeName(TYPES[index]),
            mousePosition,
            spawnType == TYPES[index]
        );
    }

    float actionButtonWidth = (contentWidth - 8.0f) * 0.5f;
    Rectangle cancelButton = {
        contentX,
        panel.y + 285.0f,
        actionButtonWidth,
        BUTTON_HEIGHT
    };
    Rectangle deleteAllButton = {
        contentX + actionButtonWidth + 8.0f,
        panel.y + 285.0f,
        actionButtonWidth,
        BUTTON_HEIGHT
    };
    DrawButton(cancelButton, "Cancel placement", mousePosition);
    DrawButton(deleteAllButton, "Delete all enemies", mousePosition);

    DrawText(
        statusMessage.c_str(),
        (int)contentX,
        (int)panel.y + 331,
        SMALL_TEXT_SIZE,
        placementArmed ? YELLOW : LIGHTGRAY
    );

    std::string heading = std::string("NEXT ") +
        EnemyTypeName(spawnType) + " SPAWN VALUES";
    DrawText(
        heading.c_str(),
        (int)contentX,
        (int)panel.y + 353,
        TEXT_SIZE,
        GOLD
    );

    DrawPropertyEditor();
}
