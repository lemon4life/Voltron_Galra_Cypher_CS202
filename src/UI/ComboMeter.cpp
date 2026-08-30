#include "UI/ComboMeter.h"
#include "UI/UIUtils.h"
#include "raymath.h"
#include <cmath>
#include <string>

/// Adds damage.
void ComboMeter::AddDamage(int amount) {
    accumulatedDamage += amount;
    comboTimer = 2.0f;
    state = State::ACTIVE;
    slideOffsetX = 0.0f;
    popScale = 1.0f;
}

/// Restores this component to its initial runtime state.
void ComboMeter::Reset() {
    accumulatedDamage = 0;
    comboTimer = 0.0f;
    lingerTimer = 0.0f;
    state = State::ACTIVE;
    slideOffsetX = -300.0f;
    popScale = 1.0f;
}

/// Advances this component's state for the current frame.
void ComboMeter::Update(float deltaTime) {
    if (accumulatedDamage > 0) {
        if (state == State::ACTIVE) {
            comboTimer -= deltaTime;
            if (comboTimer <= 0.0f) {
                state = State::LINGERING;
                lingerTimer = 0.6f; // Linger for 1 second
            }
        } else if (state == State::LINGERING) {
            // Zoom slightly bigger
            popScale = Lerp(popScale, 1.8f, deltaTime * 8.0f);
            lingerTimer -= deltaTime;
            if (lingerTimer <= 0.0f) {
                state = State::SLIDING;
            }
        } else if (state == State::SLIDING) {
            // Slide away
            slideOffsetX = Lerp(slideOffsetX, -300.0f, deltaTime * 5.0f);
            
            if (slideOffsetX <= -290.0f) {
                Reset();
            }
        }
    }
}

/// Renders this component using its current state and visual resources.
void ComboMeter::Draw(Vector2 basePosition) {
    if (accumulatedDamage <= 0) return;
    
    float pulse = 1.0f;
    if (accumulatedDamage >= 100) {
        pulse = 1.0f + (std::sin(GetTime() * 5.0f) * 0.05f); // less extreme, slower pulse
    }
    
    Color textColor;
    float finalScale = 1.0f;
    bool drawOutline = true;
    
    if (accumulatedDamage < 200) {
        textColor = RAYWHITE;
        finalScale = popScale * pulse;
    } else if (accumulatedDamage < 500) {
        textColor = UIUtils::HP_GRADIENT_RIGHT;
        finalScale = popScale * pulse;
    } else if (accumulatedDamage < 4000) {
        textColor = UIUtils::EX_GRADIENT_RIGHT;
        finalScale = (popScale * 1.1f) * pulse;
    } else {
        textColor = UIUtils::QUINT_GRADIENT_RIGHT;
        finalScale = (popScale * 1.25f) * pulse;
    }
    
    std::string text = std::to_string(accumulatedDamage) + " COMBO!";
    
    // Convert to FontSize, adjust by scale (using smaller base size)
    float baseFontSize = static_cast<float>(UIUtils::FontSize::SMALL) * finalScale;
    
    Vector2 drawPos = { basePosition.x + slideOffsetX, basePosition.y - 55.0f };
    
    if (drawOutline) {
        UIUtils::FontSize outlineSize = static_cast<UIUtils::FontSize>(baseFontSize);
        UIUtils::DrawText("PixeloidBold", text, { drawPos.x - 2, drawPos.y }, outlineSize, BLACK);
        UIUtils::DrawText("PixeloidBold", text, { drawPos.x + 2, drawPos.y }, outlineSize, BLACK);
        UIUtils::DrawText("PixeloidBold", text, { drawPos.x, drawPos.y - 2 }, outlineSize, BLACK);
        UIUtils::DrawText("PixeloidBold", text, { drawPos.x, drawPos.y + 2 }, outlineSize, BLACK);
    }
    
    UIUtils::DrawText("PixeloidBold", text, drawPos, static_cast<UIUtils::FontSize>(baseFontSize), textColor);
}
