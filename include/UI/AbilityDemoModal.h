#pragma once

#include "Entities/Player/PaladinDefinition.h"
#include "raylib.h"
#include <string>
#include <vector>

enum class DemoPreviewMode {
    BasicAttack,
    Skill,
    Ultimate
};

struct DemoDamagePopup {
    std::string text;
    Vector2 position;
    Vector2 velocity;
    float alpha;
    float lifetime;
    float maxLifetime;
    Color color;
};

struct DemoParticle {
    Vector2 position;
    Vector2 velocity;
    float size;
    float alpha;
    float lifetime;
    float maxLifetime;
    Color color;
};

struct DemoProjectile {
    Vector2 position;
    Vector2 startPos;
    Vector2 targetPos;
    float progress; // 0.0 to 1.0
    float speed;
    bool active;
    std::string textureKey;
    float rotation;
};

// Design Pattern - View Component / Modal:
// AbilityDemoModal coordinates the character ability preview showcase,
// running an automated simulation of the selected Paladin's actions against
// a stationary training dummy with isolated visual FX, projectiles, and damage popups.
class AbilityDemoModal {
private:
    bool open;
    PaladinId currentPaladin;
    DemoPreviewMode currentMode;

    // Simulation loop timing
    float loopTimer;
    float hitFlashTimer;
    int paladinFrame;
    float paladinFrameTimer;
    float paladinOffsetX;
    float weaponRecoilX;
    float dummyShakeX;
    float keithSwingAngle;

    // Trigger state flags for synchronized combat steps
    bool attack1Triggered;
    bool attack2Triggered;
    bool impactVFXTriggered;

    // Visual FX containers
    std::vector<DemoDamagePopup> damagePopups;
    std::vector<DemoParticle> particles;
    std::vector<DemoProjectile> projectiles;

    // Helper methods
    void ResetSimulation();
    Vector2 GetPaladinPosition(Rectangle stageBounds) const;
    Vector2 GetDummyPosition(Rectangle stageBounds) const;
    Vector2 GetWeaponPivot(Vector2 paladinPos) const;
    Vector2 GetMuzzlePosition(Vector2 paladinPos) const;
    void SpawnDamagePopup(Vector2 pos, const std::string& text, Color color, float lifetime = 0.8f);
    void SpawnHitParticles(Vector2 pos, Color color, int count = 8);
    void UpdateBasicAttackDemo(float deltaTime);
    void UpdateParticles(float deltaTime);
    void UpdateDamagePopups(float deltaTime);

    void DrawArenaStage(Rectangle bounds) const;
    void DrawRightActionCards(Rectangle bounds, Vector2 mousePosition);
    void DrawPaladinSprite(Vector2 centerPos) const;
    void DrawPaladinWeapon(Vector2 paladinPos) const;
    void DrawTrainingDummy(Vector2 centerPos) const;
    void DrawBasicAttackVFX(Vector2 paladinPos, Vector2 dummyPos) const;

public:
    AbilityDemoModal();
    ~AbilityDemoModal();

    /// Opens the ability demo modal for the inspected Paladin.
    void Open(PaladinId paladinId);
    /// Closes the modal and resets simulation state.
    void Close();
    /// Reports whether the demo modal is currently open.
    bool IsOpen() const { return open; }

    /// Advances the demo animation and handles card switching.
    void Update(float deltaTime, Vector2 mousePosition);
    /// Renders the modal backdrop, arena stage, and action cards.
    void Draw(Vector2 mousePosition) const;
};
