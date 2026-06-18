#pragma once
#include <vector>

enum class GameState {
    MENU,
    PLAYING,
    PAUSED,
    GAMEOVER
};

class Projectile; // Forward declaration
class LevelManager; // Forward declaration
class GameObject; // Forward declaration

class GameManager {
private:
    GameState currentState;
    std::vector<Projectile*> activeProjectiles;
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

    GameState GetState() const { return currentState; }
    void SetState(GameState state) { currentState = state; }

    void SetLevelManager(LevelManager* lm) { levelManager = lm; }
    const std::vector<GameObject*>& GetLevelEntities() const;

    void AddProjectile(Projectile* p);
    void UpdateProjectiles(float deltaTime);
    void DrawProjectiles();
};
