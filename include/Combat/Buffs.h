#pragma once
#include "Combat/IBuff.h"
#include "Entities/Player/Paladin.h"
#include "Core/Manager/GameManager.h"
#include "Entities/Enemy.h"

// Aegis Shield Buff (Hunk)
class AegisShieldBuff : public IBuff {
private:
    float timer;
public:
    AegisShieldBuff(float duration) : timer(duration) {}

    void OnApply(Paladin* target) override {
        if (target) target->SetInvulnerable(true);
    }

    void Update(float deltaTime, Paladin* activePaladin) override {
        timer -= deltaTime;
        if (activePaladin) {
            activePaladin->SetInvulnerable(true); // Ensure it's applied even after a swap
        }
    }

    void OnRemove(Paladin* target) override {
        if (target) target->SetInvulnerable(false);
    }

    bool IsFinished() const override {
        return timer <= 0.0f;
    }
};

// Fire Circle Buff (Keith)
class FireCircleBuff : public IBuff {
private:
    float timer;
    float tickTimer;
public:
    FireCircleBuff(float duration) : timer(duration), tickTimer(0.0f) {}

    void Update(float deltaTime, Paladin* activePaladin) override {
        if (!activePaladin) return;

        timer -= deltaTime;
        tickTimer += deltaTime;

        // Apply damage over time and status effect
        float skillRadius = 100.0f;
        const auto& enemies = GameManager::GetInstance()
            .GetObjectManager().GetEnemies();
        for (Enemy* enemy : enemies) {
            if (enemy && !enemy->IsDead() && enemy->IsEnabled()) {
                if (CheckCollisionCircles(activePaladin->GetPosition(), skillRadius, enemy->GetPosition(), 15.0f)) {
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

    bool IsFinished() const override {
        return timer <= 0.0f;
    }

    void Draw(Paladin* activePaladin) override {
        if (activePaladin) {
            DrawCircleV(activePaladin->GetPosition(), 100.0f, ColorAlpha(RED, 0.4f));
        }
    }
};

// Dual Wield Buff (Lance)
class DualWieldBuff : public IBuff {
private:
    float timer;
public:
    DualWieldBuff(float duration) : timer(duration) {}

    void Update(float deltaTime, Paladin* activePaladin) override {
        timer -= deltaTime;
    }

    bool IsFinished() const override {
        return timer <= 0.0f;
    }
};
