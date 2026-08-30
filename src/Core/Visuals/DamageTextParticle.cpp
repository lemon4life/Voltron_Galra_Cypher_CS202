#include "Core/Visuals/DamageTextParticle.h"
#include "UI/UIUtils.h"
#include <algorithm>

/// Creates a DamageTextParticle instance from the supplied configuration.
DamageTextParticle::DamageTextParticle(Vector2 pos, Vector2 vel, int dmg, float life)
    : position(pos), velocity(vel), damage(dmg), lifetime(life), maxLifetime(life) {
    text = std::to_string(damage);
}

/// Advances this component's state for the current frame.
void DamageTextParticle::Update(float deltaTime) {
    // Apply gentle gravity
    velocity.y += 150.0f * deltaTime;
    
    // Apply velocity
    position.x += velocity.x * deltaTime;
    position.y += velocity.y * deltaTime;
    
    lifetime -= deltaTime;
}

/// Renders this component using its current state and visual resources.
void DamageTextParticle::Draw() const {
    if (lifetime <= 0.0f) return;
    
    // Fade out in the last 0.2 seconds
    float alpha = 1.0f;
    if (lifetime < 0.2f) {
        alpha = lifetime / 0.2f;
    }
    
    Color textColor = Fade(RED, alpha);
    Color outlineColor = Fade(BLACK, alpha);
    
    // Use TINY font size or a custom small size
    float fontSizeRaw = static_cast<float>(UIUtils::FontSize::SMALL) * 0.7f;
    UIUtils::FontSize fontSize = static_cast<UIUtils::FontSize>(fontSizeRaw);
    
    // Draw outline (4 directions)
    UIUtils::DrawCenteredText("PixeloidBold", text, { position.x - 1, position.y }, fontSize, outlineColor);
    UIUtils::DrawCenteredText("PixeloidBold", text, { position.x + 1, position.y }, fontSize, outlineColor);
    UIUtils::DrawCenteredText("PixeloidBold", text, { position.x, position.y - 1 }, fontSize, outlineColor);
    UIUtils::DrawCenteredText("PixeloidBold", text, { position.x, position.y + 1 }, fontSize, outlineColor);
    
    // Draw main text
    UIUtils::DrawCenteredText("PixeloidBold", text, position, fontSize, textColor);
}

/// Reports whether the dead condition is satisfied.
bool DamageTextParticle::IsDead() const {
    return lifetime <= 0.0f;
}
