#pragma once

#include <string>

class GameManager;

class MemoryDiagnostics {
public:
    /// Resets log.
    static void ResetLog();
    /// Records a timestamped runtime memory snapshot in the diagnostics log.
    static void Capture(const std::string& label, const GameManager& game);
    /// Updates periodic.
    static void UpdatePeriodic(float deltaTime, const GameManager& game);
};
