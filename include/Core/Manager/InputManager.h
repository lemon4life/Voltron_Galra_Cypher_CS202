#pragma once
#include "raylib.h"

enum class InputMode {
    MOUSE_AND_KEYBOARD,
    KEYBOARD_ONLY
};

class InputManager {
public:
    /// Advances this component's state for the current frame.
    static void Update();
    /// Returns the current mode.
    static InputMode GetMode();

    // Movement
    /// Returns the current movement vector.
    static Vector2 GetMovementVector();

    // Actions
    /// Reports whether the attack pressed condition is satisfied.
    static bool IsAttackPressed();
    /// Reports whether the attack down condition is satisfied.
    static bool IsAttackDown();
    /// Reports whether the dash pressed condition is satisfied.
    static bool IsDashPressed();
    /// Reports whether the parry pressed condition is satisfied.
    static bool IsParryPressed();
    /// Reports whether the parry down condition is satisfied.
    static bool IsParryDown();
    /// Reports whether the skill pressed condition is satisfied.
    static bool IsSkillPressed();
    /// Reports whether the ultimate pressed condition is satisfied.
    static bool IsUltimatePressed();
    
    // UI/Utility
    /// Reports whether the switch character pressed condition is satisfied.
    static bool IsSwitchCharacterPressed();
    /// Reports whether the interact pressed condition is satisfied.
    static bool IsInteractPressed();
    /// Reports whether the toggle auto aim pressed condition is satisfied.
    static bool IsToggleAutoAimPressed();
    /// Reports whether the pause pressed condition is satisfied.
    static bool IsPausePressed();

private:
    static InputMode currentMode;
    static Vector2 lastMousePos;
};
