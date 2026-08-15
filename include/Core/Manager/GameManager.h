#pragma once
#include "raylib.h"
#include <vector>
#include <memory>
#include "Core/DepthRenderItem.h"

#include<memory>

enum class GameState {
    MAIN_MENU,
    HUB,
    GAMEPLAY,
    PAUSE,
    SETTINGS,
    ROOM_EDITOR,
    GAME_OVER,
    VICTORY
};

struct ImpactEffect {
    Vector2 position;
    float lifetime;
    float maxLifetime;
    int currentFrame;
    int numFrames;
    Texture2D texture;
    bool drawBehind;
};

class Projectile; // Forward declaration
class LevelManager; // Forward declaration
class GameObject; // Forward declaration
class Rover; // Forward declaration
class IGameState; // Forward declaration

class GameManager {
private:
    GameState currentState;
    GameState previousGameState;
    std::vector<GameObject*> levelEntities;
    std::vector<Projectile*> activeProjectiles;
    std::vector<ImpactEffect> activeEffects;
    std::vector<std::unique_ptr<Rover>> activeRovers;
    Texture2D bulletImpactTex;
    std::unique_ptr<IGameState> currentStateObj;

    int targetFPS;
    float hitstopTimer;
    
    int currentFloor;
    
    float levelWidth = 0.0f;
    float levelHeight = 0.0f;

    LevelManager* levelManager;

    GameManager(); // Private constructor
    ~GameManager();

public:
    static GameManager& GetInstance();

    // Delete copy and assignment operators to enforce singleton behavior
    static constexpr int MAX_FLOORS = 3;

    GameManager(const GameManager&) = delete;
    GameManager& operator=(const GameManager&) = delete;
    GameManager(GameManager&&) = delete;
    GameManager& operator=(GameManager&&) = delete;

    // --- Accessors ---
    void TriggerHitstop(float duration) { hitstopTimer = duration; }
    float GetHitstopTimer() const { return hitstopTimer; }
    void ClearHitstop() { hitstopTimer = 0.0f; }

    int GetCurrentFloor() const { return currentFloor; }
    void AdvanceFloorCount() { currentFloor++; }
    void ResetFloorCount() { currentFloor = 1; }

    void UpdateHitstop(float dt) { if (hitstopTimer > 0.0f) hitstopTimer -= dt; }

    void SetState(GameState newState);
    GameState GetState() const;
    
    void SetCurrentStateObj(std::unique_ptr<IGameState> state);
    IGameState* GetCurrentStateObj() const;
    std::unique_ptr<IGameState> TakeCurrentStateObj();
    bool PauseGame();
    bool ResumeGame();
    bool IsPaused() const;
    GameState GetPreviousGameState() const;
    GameState GetRenderState() const;

    void SetLevelBounds(float w, float h) { levelWidth = w; levelHeight = h; }
    float GetLevelWidth() const { return levelWidth; }
    float GetLevelHeight() const { return levelHeight; }

    const std::vector<GameObject*>& GetLevelEntities() const;
    void SetLevelEntities(const std::vector<GameObject*>& entities) { levelEntities = entities; }

    void SetLevelManager(LevelManager* lm) { levelManager = lm; }
    LevelManager* GetLevelManager() const { return levelManager; }
    
    void UpdateTargetFPS(int fps) { targetFPS = fps; SetTargetFPS(fps); }
    int GetTargetFPS() const { return targetFPS; }

    void AddProjectile(Projectile* p);
    void UpdateProjectiles(float deltaTime, class TeamManager* teamManager = nullptr);
    void UpdateEffects(float deltaTime);
    void DrawEffects(bool background);
    void SetBulletImpactTexture(Texture2D tex) { bulletImpactTex = tex; }
    void AddEffect(Vector2 pos, Texture2D tex, int frames, float lifetime, bool drawBehind = false);
    void AddImpactEffect(Vector2 pos);
    void DrawProjectiles();
    void AddDepthRenderItems(std::vector<DepthRenderItem>& items);
    void DrawDebugOverlays(class TeamManager* teamManager) const;
    void ClearProjectiles();
    void ResetTransientState();
    
    // Pidge skills
    void AddRover(std::unique_ptr<Rover> rover);
    void UpdateAssists(float deltaTime, class TeamManager* teamManager = nullptr);
};
