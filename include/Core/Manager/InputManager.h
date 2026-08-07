#pragma once
#include "raylib.h"

enum class InputMode {
    MOUSE_AND_KEYBOARD,
    KEYBOARD_ONLY
};

class InputManager {
public:
    static void Update();
    static InputMode GetMode();

    // Movement
    static Vector2 GetMovementVector();

    // Actions
    static bool IsAttackPressed();
    static bool IsAttackDown();
    static bool IsDashPressed();
    static bool IsParryPressed();
    static bool IsParryDown();
    static bool IsSkillPressed();
    static bool IsUltimatePressed();
    
    // UI/Utility
    static bool IsSwitchCharacterPressed();
    static bool IsInteractPressed();
    static bool IsToggleAutoAimPressed();
    static bool IsPausePressed();

private:
    static InputMode currentMode;
    static Vector2 lastMousePos;
};
