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

class Projectile; // Forward declaration
class LevelManager; // Forward declaration
class GameObject; // Forward declaration

class GameManager {
private:
    GameState currentState;
    std::vector<GameObject*> levelEntities;
    std::vector<Projectile*> activeProjectiles;

    int targetFPS;
    
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
    void DrawProjectiles();
    void ClearProjectiles();
};
