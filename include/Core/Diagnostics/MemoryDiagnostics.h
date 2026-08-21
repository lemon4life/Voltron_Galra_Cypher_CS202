#pragma once

#include <string>

class GameManager;

class MemoryDiagnostics {
public:
    static void ResetLog();
    static void Capture(const std::string& label, const GameManager& game);
    static void UpdatePeriodic(float deltaTime, const GameManager& game);
};
