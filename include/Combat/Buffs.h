#pragma once
#include "Combat/IBuff.h"
#include "Entities/Player/Paladin.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/AssetManager.h"
#include "Entities/Enemy.h"

// Aegis Shield Buff (Hunk)
class AegisShieldBuff : public IBuff {
private:
    float timer;
    float maxDuration;
    float rotationAngle = 0.0f;
    static constexpr float ROTATE_SPEED = 90.0f; // degrees per second
    static constexpr float TRANSITION_DURATION = 0.2f;

public:
    /// Creates a AegisShieldBuff instance from the supplied configuration.
    AegisShieldBuff(float duration) 
        : timer(duration), maxDuration(duration), rotationAngle(0.0f) {}

    /// Handles the apply event.
    void OnApply(Paladin* target) override {
        if (target) target->SetInvulnerable(true);
    }

    /// Advances this component's state for the current frame.
    void Update(float deltaTime, Paladin* activePaladin) override {
        timer -= deltaTime;
        rotationAngle += ROTATE_SPEED * deltaTime;
        if (rotationAngle >= 360.0f) rotationAngle -= 360.0f;

        if (activePaladin) {
            activePaladin->SetInvulnerable(true); // Ensure it's applied even after a swap
        }
    }

    /// Handles the remove event.
    void OnRemove(Paladin* target) override {
        if (target) target->SetInvulnerable(false);
    }

    /// Reports whether the finished condition is satisfied.
    bool IsFinished() const override {
        return timer <= 0.0f;
    }

    /// Renders this component using its current state and visual resources.
    void Draw(Paladin* activePaladin) override {
        if (!activePaladin) return;

        Texture2D shieldTex = AssetManager::GetInstance().GetTexture("shield");
        if (shieldTex.id == 0) return;

        // Smooth scale transition: zoom-out entrance (0.0 -> 1.0) and zoom-in exit (1.0 -> 0.0)
        float scale = 1.0f;
        float elapsed = maxDuration - timer;
        if (elapsed < TRANSITION_DURATION) {
            scale = elapsed / TRANSITION_DURATION;
        } else if (timer < TRANSITION_DURATION) {
            scale = timer / TRANSITION_DURATION;
        }
        if (scale < 0.0f) scale = 0.0f;
        if (scale > 1.0f) scale = 1.0f;

        Vector2 pos = activePaladin->GetPosition();
        float width = (float)shieldTex.width * scale;
        float height = (float)shieldTex.height * scale;

        Rectangle src = { 0.0f, 0.0f, (float)shieldTex.width, (float)shieldTex.height };
        Rectangle dest = { pos.x, pos.y, width, height };
        Vector2 origin = { width * 0.5f, height * 0.5f };

        DrawTexturePro(shieldTex, src, dest, origin, rotationAngle, WHITE);
    }
};

// Fire Circle Buff (Keith)
class FireCircleBuff : public IBuff {
private:
    float timer;
    float maxDuration;
    float tickTimer;
    float rotationAngle = 0.0f;
    static constexpr float ROTATE_SPEED = 45.0f; // degrees per second
    static constexpr float TRANSITION_DURATION = 0.2f;
    static constexpr float SKILL_RADIUS = 100.0f;

public:
    /// Creates a FireCircleBuff instance from the supplied configuration.
    FireCircleBuff(float duration) 
        : timer(duration), maxDuration(duration), tickTimer(0.0f), rotationAngle(0.0f) {}

    /// Advances this component's state for the current frame.
    void Update(float deltaTime, Paladin* activePaladin) override {
        if (!activePaladin) return;

        timer -= deltaTime;
        tickTimer += deltaTime;
        rotationAngle += ROTATE_SPEED * deltaTime;
        if (rotationAngle >= 360.0f) rotationAngle -= 360.0f;

        // Apply damage over time and status effect
        const auto& enemies = GameManager::GetInstance()
            .GetObjectManager().GetEnemies();
        for (Enemy* enemy : enemies) {
            if (enemy && !enemy->IsDead() && enemy->IsEnabled()) {
                if (CheckCollisionCircles(activePaladin->GetPosition(), SKILL_RADIUS, enemy->GetPosition(), 15.0f)) {
                    enemy->GetStatusComponent().AddEffect(EffectType::BURN, 5.0f, 5.0f);
                    
                    if (tickTimer >= 0.5f) { // Generate EX roughly twice a second
                        activePaladin->OnHitEnemy(5); 
                    }
                }
            }
        }
        
        if (tickTimer >= 0.5f) {
            tickTimer = 0.0f;
        }
    }

    /// Reports whether the finished condition is satisfied.
    bool IsFinished() const override {
        return timer <= 0.0f;
    }

    /// Renders this component using its current state and visual resources.
    void Draw(Paladin* activePaladin) override {
        if (!activePaladin) return;

        Texture2D fireRangeTex = AssetManager::GetInstance().GetTexture("fire_range");
        if (fireRangeTex.id == 0) return;

        // Smooth scale transition: zoom-out entrance (0.0 -> 1.0) and zoom-in exit (1.0 -> 0.0)
        float scale = 1.0f;
        float elapsed = maxDuration - timer;
        if (elapsed < TRANSITION_DURATION) {
            scale = elapsed / TRANSITION_DURATION;
        } else if (timer < TRANSITION_DURATION) {
            scale = timer / TRANSITION_DURATION;
        }
        if (scale < 0.0f) scale = 0.0f;
        if (scale > 1.0f) scale = 1.0f;

        // Diameter = 2.0f * SKILL_RADIUS
        float diameter = 2.0f * SKILL_RADIUS * scale;
        Vector2 pos = activePaladin->GetPosition();

        Rectangle src = { 0.0f, 0.0f, (float)fireRangeTex.width, (float)fireRangeTex.height };
        Rectangle dest = { pos.x, pos.y, diameter, diameter };
        Vector2 origin = { diameter * 0.5f, diameter * 0.5f };

        DrawTexturePro(fireRangeTex, src, dest, origin, rotationAngle, WHITE);
    }
};

// Dual Wield Buff (Lance)
class DualWieldBuff : public IBuff {
private:
    float timer;
public:
    /// Creates a DualWieldBuff instance from the supplied configuration.
    DualWieldBuff(float duration) : timer(duration) {}

    /// Advances this component's state for the current frame.
    void Update(float deltaTime, Paladin* activePaladin) override {
        timer -= deltaTime;
    }

    /// Reports whether the finished condition is satisfied.
    bool IsFinished() const override {
        return timer <= 0.0f;
    }
};
