#include "Core/Manager/InputManager.h"
#include <cstdio>
#include "raymath.h"
#include "Core/Constants.h"

InputMode InputManager::currentMode = InputMode::MOUSE_AND_KEYBOARD;
Vector2 InputManager::lastMousePos = {0.0f, 0.0f};

/// Advances this component's state for the current frame.
void InputManager::Update() {
    Vector2 currentMousePos = GetMousePosition();
    
    // Check for Mouse+Keyboard activity
    if (Vector2Distance(currentMousePos, lastMousePos) > 1.0f || 
        IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || 
        IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
        if (currentMode != InputMode::MOUSE_AND_KEYBOARD) {
            currentMode = InputMode::MOUSE_AND_KEYBOARD;
            ShowCursor();
        }
    }
    
    // Check for exclusive Keyboard-only activity
    if (IsKeyPressed(KEY_J) || IsKeyPressed(KEY_L)) {
        if (currentMode != InputMode::KEYBOARD_ONLY) {
            currentMode = InputMode::KEYBOARD_ONLY;
            HideCursor();
            Constants::isAutoAimEnabled = true; // Auto-aim ON when changing to KEYBOARD_ONLY
        }
    }
    
    lastMousePos = currentMousePos;
}

/// Returns the current mode.
InputMode InputManager::GetMode() {
    return currentMode;
}

/// Returns the current movement vector.
Vector2 InputManager::GetMovementVector() {
    Vector2 moveDir = {0.0f, 0.0f};
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) moveDir.x += 1.0f;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) moveDir.x -= 1.0f;
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) moveDir.y -= 1.0f;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) moveDir.y += 1.0f;
    
    if (Vector2Length(moveDir) > 0.0f) {
        moveDir = Vector2Normalize(moveDir);
    }
    return moveDir;
}

/// Reports whether the attack pressed condition is satisfied.
bool InputManager::IsAttackPressed() {
    return IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsKeyPressed(KEY_J);
}

/// Reports whether the attack down condition is satisfied.
bool InputManager::IsAttackDown() {
    return IsMouseButtonDown(MOUSE_LEFT_BUTTON) || IsKeyDown(KEY_J);
}

/// Reports whether the dash pressed condition is satisfied.
bool InputManager::IsDashPressed() {
    return IsKeyPressed(KEY_SPACE);
}

/// Reports whether the parry pressed condition is satisfied.
bool InputManager::IsParryPressed() {
    return IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) || IsKeyPressed(KEY_L);
}

/// Reports whether the parry down condition is satisfied.
bool InputManager::IsParryDown() {
    return IsMouseButtonDown(MOUSE_RIGHT_BUTTON) || IsKeyDown(KEY_L);
}

/// Reports whether the skill pressed condition is satisfied.
bool InputManager::IsSkillPressed() {
    return IsKeyPressed(KEY_E);
}

/// Reports whether the ultimate pressed condition is satisfied.
bool InputManager::IsUltimatePressed() {
    return IsKeyPressed(KEY_Q);
}

/// Reports whether the switch character pressed condition is satisfied.
bool InputManager::IsSwitchCharacterPressed() {
    return IsKeyPressed(KEY_TAB) || IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_TWO);
}

/// Reports whether the interact pressed condition is satisfied.
bool InputManager::IsInteractPressed() {
    return IsKeyPressed(KEY_F);
}

/// Reports whether the toggle auto aim pressed condition is satisfied.
bool InputManager::IsToggleAutoAimPressed() {
    return IsKeyPressed(KEY_T);
}

/// Reports whether the pause pressed condition is satisfied.
bool InputManager::IsPausePressed() {
    return IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE);
}
