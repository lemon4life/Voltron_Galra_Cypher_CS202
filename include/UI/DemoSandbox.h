#pragma once

#include "Entities/Player/PaladinDefinition.h"
#include "Entities/Player/Paladin.h"
#include "raylib.h"
#include <string>
#include <vector>
#include <memory>

enum class DemoPreviewMode {
    BasicAttack,
    Skill,
    Ultimate
};

struct DemoEffectPopup {
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
    int damage;
    Color trailColor;
};

// ==========================================
// Target Dummy Entity
// ==========================================
class TrainingDummy {
private:
    Vector2 position;
    Rectangle boundingBox;
    float hitFlashTimer;
    float shakeX;
    bool isPoisoned;
    bool isFrozen;
    bool isDizzy;
    float statusTimer;

public:
    TrainingDummy(Vector2 pos);
    void Reset(Vector2 pos);
    void Update(float deltaTime);
    void Draw() const;
    void TakeDamage(int damage, Color color = WHITE);
    void ApplyStatus(const std::string& statusName, float duration);

    Vector2 GetPosition() const { return position; }
    Rectangle GetBoundingBox() const { return boundingBox; }
    void SetPosition(Vector2 pos);
    float GetHitFlashTimer() const { return hitFlashTimer; }
    float GetShakeX() const { return shakeX; }
    bool IsPoisoned() const { return isPoisoned; }
    bool IsFrozen() const { return isFrozen; }
    bool IsDizzy() const { return isDizzy; }
};

// Forward declaration
class DemoSandbox;

// ==========================================
// Strategy Pattern: Context Payloads
// ==========================================
struct DemoUpdateContext {
    DemoSandbox& sandbox;
    TrainingDummy& dummy;
    const PaladinDefinition& def;
    PaladinId paladinId;
    DemoPreviewMode mode;
    Vector2 paladinPos;
    Vector2 dummyPos;
    float loopTimer;
};

struct DemoDrawContext {
    const DemoSandbox& sandbox;
    const TrainingDummy& dummy;
    const PaladinDefinition& def;
    PaladinId paladinId;
    DemoPreviewMode mode;
    Vector2 paladinPos;
    Vector2 dummyPos;
    float loopTimer;
};

// ==========================================
// Strategy Pattern: IDemoSequence Interface
// ==========================================
class IDemoSequence {
public:
    virtual ~IDemoSequence() = default;
    virtual void Reset(DemoUpdateContext& ctx) = 0;
    virtual void Update(float deltaTime, DemoUpdateContext& ctx) = 0;
    virtual void DrawWeapon(const DemoDrawContext& ctx) const = 0;
    virtual void DrawVFX(const DemoDrawContext& ctx) const = 0;
    virtual float GetDuration() const = 0;
};

// ==========================================
// Factory Pattern: DemoSequenceFactory
// ==========================================
class DemoSequenceFactory {
public:
    static std::unique_ptr<IDemoSequence> Create(PaladinId id, DemoPreviewMode mode);
};

// ==========================================
// Subsystem / Sandbox Orchestrator
// ==========================================
class DemoSandbox {
private:
    PaladinId currentPaladinId;
    DemoPreviewMode currentMode;
    const PaladinDefinition* cachedDef;
    std::unique_ptr<TrainingDummy> dummy;
    std::unique_ptr<IDemoSequence> activeSequence;

    float loopTimer;
    std::vector<DemoEffectPopup> effectPopups;
    std::vector<DemoParticle> particles;
    std::vector<DemoProjectile> projectiles;

    void UpdateVFX(float deltaTime);

public:
    DemoSandbox();
    ~DemoSandbox();

    void Init(PaladinId paladinId);
    void SetPaladin(PaladinId paladinId);
    void SetMode(DemoPreviewMode mode);
    void Reset();

    void SpawnEffectPopup(Vector2 pos, const std::string& text, Color color, float lifetime = 0.8f);
    void SpawnHitParticles(Vector2 pos, Color color, int count = 8);
    void SpawnProjectile(const DemoProjectile& proj);
    std::vector<DemoProjectile>& GetProjectiles() { return projectiles; }
    const std::vector<DemoProjectile>& GetProjectiles() const { return projectiles; }

    void Update(float deltaTime);
    void Draw(Rectangle stageBounds) const;

    PaladinId GetPaladinId() const { return currentPaladinId; }
    DemoPreviewMode GetMode() const { return currentMode; }
    const PaladinDefinition& GetPaladinDefinition() const { return *cachedDef; }
    Vector2 GetPaladinPos() const;
    Vector2 GetDummyPos() const;
};
