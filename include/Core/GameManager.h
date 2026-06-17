#pragma once

enum class GameState {
    MENU,
    PLAYING,
    PAUSED,
    GAMEOVER
};

class GameManager {
private:
    GameState currentState;

    GameManager(); // Private constructor
    ~GameManager() = default;

public:
    static GameManager& GetInstance();

    // Delete copy and assignment operators to enforce singleton behavior
    GameManager(const GameManager&) = delete;
    GameManager& operator=(const GameManager&) = delete;
    GameManager(GameManager&&) = delete;
    GameManager& operator=(GameManager&&) = delete;

    GameState GetState() const { return currentState; }
    void SetState(GameState state) { currentState = state; }
};
