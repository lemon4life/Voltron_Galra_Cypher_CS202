#pragma once
#include "raylib.h"
#include <vector>

enum class GameState {
    MENU,
    HUB,
    PLAYING,
    PAUSED,
    GAMEOVER,
    VICTORY
};

struct ImpactEffect {
    Vector2 position;
    float lifetime;
    float maxLifetime;
    int currentFrame;
    int numFrames;
};

class Projectile; // Forward declaration
class LevelManager; // Forward declaration
class GameObject; // Forward declaration

class GameManager {
private:
    GameState currentState;
    std::vector<GameObject*> levelEntities;
    std::vector<Projectile*> activeProjectiles;
    std::vector<ImpactEffect> activeEffects;
    Texture2D bulletImpactTex;

    int targetFPS;
    float hitstopTimer;
    
    float levelWidth = 0.0f;
    float levelHeight = 0.0f;

    LevelManager* levelManager;

    GameManager(); // Private constructor
    ~GameManager();

public:
    static GameManager& GetInstance();

    // Delete copy and assignment operators to enforce singleton behavior
    GameManager(const GameManager&) = delete;
    GameManager& operator=(const GameManager&) = delete;
    GameManager(GameManager&&) = delete;
    GameManager& operator=(GameManager&&) = delete;

    // --- Accessors ---
    void TriggerHitstop(float duration) { hitstopTimer = duration; }
    float GetHitstopTimer() const { return hitstopTimer; }
    void UpdateHitstop(float dt) { if (hitstopTimer > 0.0f) hitstopTimer -= dt; }

    GameState GetState() const { return currentState; }
    void SetState(GameState newState) { currentState = newState; }

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
    void UpdateAndDrawEffects(float deltaTime);
    void SetBulletImpactTexture(Texture2D tex) { bulletImpactTex = tex; }
    void AddImpactEffect(Vector2 pos);
    void DrawProjectiles();
    void ClearProjectiles();
};
