#include "Core/GameManager.h"

GameManager::GameManager() : currentState(GameState::PLAYING) {
    // Starts in PLAYING state by default
}

GameManager& GameManager::GetInstance() {
    // Thread-safe in C++11+ (Meyers' Singleton)
    static GameManager instance;
    return instance;
}
