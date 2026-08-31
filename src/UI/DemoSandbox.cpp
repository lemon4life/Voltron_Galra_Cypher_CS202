#include "UI/DemoSandbox.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Manager/AudioManager.h"
#include "UI/UIUtils.h"
#include "raymath.h"
#include <cmath>
#include <algorithm>

namespace {
constexpr float ENTITY_SCALE = 1.70f;
constexpr float WEAPON_SCALE = 1.55f;
}

// =========================================================================
// 1. TrainingDummy Implementation
// =========================================================================

TrainingDummy::TrainingDummy(Vector2 pos)
    : position(pos),
      hitFlashTimer(0.0f),
      shakeX(0.0f),
      isPoisoned(false),
      isFrozen(false),
      isDizzy(false),
      statusTimer(0.0f) {
    Reset(pos);
}

void TrainingDummy::Reset(Vector2 pos) {
    position = pos;
    boundingBox = { pos.x - 12.0f, pos.y - 24.0f, 24.0f, 48.0f };
    hitFlashTimer = 0.0f;
    shakeX = 0.0f;
    isPoisoned = false;
    isFrozen = false;
    isDizzy = false;
    statusTimer = 0.0f;
}

void TrainingDummy::SetPosition(Vector2 pos) {
    position = pos;
    boundingBox = { pos.x - 12.0f, pos.y - 24.0f, 24.0f, 48.0f };
}

void TrainingDummy::Update(float deltaTime) {
    if (hitFlashTimer > 0.0f) {
        hitFlashTimer -= deltaTime;
        if (hitFlashTimer < 0.0f) hitFlashTimer = 0.0f;
    }

    if (shakeX > 0.0f) {
        shakeX = std::max(0.0f, shakeX - deltaTime * 25.0f);
    }

    if (statusTimer > 0.0f) {
        statusTimer -= deltaTime;
        if (statusTimer <= 0.0f) {
            isPoisoned = false;
            isFrozen = false;
            isDizzy = false;
        }
    }
}

void TrainingDummy::TakeDamage(int damage, Color color) {
    hitFlashTimer = 0.22f;
    shakeX = 5.0f;
}

void TrainingDummy::ApplyStatus(const std::string& statusName, float duration) {
    if (statusName.empty() || duration <= 0.0f) {
        isPoisoned = false;
        isFrozen = false;
        isDizzy = false;
        statusTimer = 0.0f;
        return;
    }
    statusTimer = duration;
    if (statusName == "BURN") {
        hitFlashTimer = 0.25f;
    } else if (statusName == "FREEZE") {
        isFrozen = true;
    } else if (statusName == "DIZZY") {
        isDizzy = true;
        shakeX = 10.0f;
    } else if (statusName == "POISON") {
        isPoisoned = true;
    }
}

void TrainingDummy::Draw() const {
    Texture2D dummyTex = AssetManager::GetInstance().GetTexture("Knight_Idle");
    float shake = (shakeX > 0.0f) ? (float)GetRandomValue(-2, 2) : 0.0f;
    Vector2 renderPos = { position.x + shake, position.y };

    Color dummyColor = WHITE;
    if (hitFlashTimer > 0.0f) {
        dummyColor = Color{ 255, 120, 120, 255 };
    } else if (isFrozen) {
        dummyColor = Color{ 140, 210, 255, 255 };
    } else if (isPoisoned) {
        dummyColor = Color{ 160, 255, 160, 255 };
    }

    if (dummyTex.id != 0) {
        float frameW = (float)dummyTex.width;
        float frameH = (float)dummyTex.height;

        Rectangle src = { 0.0f, 0.0f, -frameW, frameH };
        Rectangle dest = { renderPos.x, renderPos.y, frameW * ENTITY_SCALE, frameH * ENTITY_SCALE };
        Vector2 origin = { dest.width * 0.5f, dest.height * 0.5f };

        DrawTexturePro(dummyTex, src, dest, origin, 0.0f, dummyColor);
    } else {
        DrawRectangle((int)renderPos.x - 16, (int)renderPos.y - 28, 32, 56, dummyColor);
    }

    if (isFrozen) {
        Texture2D freezeTex = AssetManager::GetInstance().GetTexture("Freeze");
        if (freezeTex.id != 0) {
            Rectangle fSrc = { 0.0f, 0.0f, (float)freezeTex.width, (float)freezeTex.height };
            Rectangle fDest = { renderPos.x, renderPos.y, (float)freezeTex.width * 1.5f, (float)freezeTex.height * 1.5f };
            Vector2 fOrigin = { (float)freezeTex.width * 0.75f, (float)freezeTex.height * 0.75f };
            DrawTexturePro(freezeTex, fSrc, fDest, fOrigin, 0.0f, WHITE);
        }
    }

    if (isDizzy) {
        float angle = (float)GetTime() * 4.0f;
        for (int i = 0; i < 3; ++i) {
            float a = angle + i * (2.0f * PI / 3.0f);
            Vector2 starPos = { renderPos.x + cosf(a) * 12.0f, renderPos.y - 34.0f + sinf(a) * 4.0f };
            DrawCircle((int)starPos.x, (int)starPos.y, 2.5f, GOLD);
        }
    }

    UIUtils::DrawCenteredText("PixeloidSans", "TRAINING DUMMY", { renderPos.x, renderPos.y - 38.0f }, static_cast<UIUtils::FontSize>(9), Color{ 170, 185, 205, 220 });
}

// =========================================================================
// 2. Concrete Strategy: KeithDemoSequence
// =========================================================================
class KeithDemoSequence : public IDemoSequence {
private:
    DemoPreviewMode mode;
    float swingAngle;
    float auraAngle;
    float auraScale;
    int sequenceStep;

    // Cached Textures
    Texture2D weaponTex;
    Texture2D slashTex;
    Texture2D fireRangeTex;
    Texture2D ultiFireTex;
    Texture2D fireAnimTex;

public:
    KeithDemoSequence(DemoPreviewMode m)
        : mode(m),
          swingAngle(0.0f),
          auraAngle(0.0f),
          auraScale(0.0f),
          sequenceStep(0),
          weaponTex({ 0 }),
          slashTex({ 0 }),
          fireRangeTex({ 0 }),
          ultiFireTex({ 0 }),
          fireAnimTex({ 0 }) {}

    void Reset(DemoUpdateContext& ctx) override {
        swingAngle = 0.0f;
        auraAngle = 0.0f;
        auraScale = 0.0f;
        sequenceStep = 0;

        AssetManager& assets = AssetManager::GetInstance();
        weaponTex = assets.GetTexture("Keith_Weapon");
        slashTex = assets.GetTexture("Sword_Slash_Small");
        fireRangeTex = assets.GetTexture("fire_range");
        ultiFireTex = assets.GetTexture("ulti_fire");
        fireAnimTex = assets.GetTexture("fire_anim");
    }

    float GetDuration() const override {
        return (mode == DemoPreviewMode::BasicAttack) ? 2.0f : ((mode == DemoPreviewMode::Skill) ? 2.6f : 2.8f);
    }

    void Update(float deltaTime, DemoUpdateContext& ctx) override {
        float t = ctx.loopTimer;
        const PaladinDefinition& def = ctx.def;

        if (mode == DemoPreviewMode::BasicAttack) {
            if (t >= 0.45f && t < 0.65f) {
                float s = (t - 0.45f) / 0.20f;
                float eased = (1.0f - cosf(s * PI)) / 2.0f;
                swingAngle = -60.0f + 120.0f * eased;

                if (s >= 0.50f && sequenceStep == 0) {
                    sequenceStep = 1;
                    ctx.dummy.TakeDamage(static_cast<int>(BaseStats::Damage * def.weapon.minDamageScalar));
                    AudioManager::GetInstance().PlayRandomSwordSlash();
                    ctx.sandbox.SpawnHitParticles(ctx.dummyPos, Color{ 255, 80, 80, 255 }, 8);
                }
            } else if (t >= 0.65f && t < 0.75f) {
                swingAngle = 0.0f;
            } else if (t >= 0.75f && t < 0.95f) {
                float s = (t - 0.75f) / 0.20f;
                float eased = (1.0f - cosf(s * PI)) / 2.0f;
                swingAngle = 60.0f - 120.0f * eased;

                if (s >= 0.50f && sequenceStep == 1) {
                    sequenceStep = 2;
                    ctx.dummy.TakeDamage(static_cast<int>(BaseStats::Damage * def.weapon.maxDamageScalar));
                    AudioManager::GetInstance().PlayRandomSwordSlash();
                    ctx.sandbox.SpawnEffectPopup(ctx.dummyPos, "CRIT!", Color{ 255, 215, 0, 255 }, 0.9f);
                    ctx.sandbox.SpawnHitParticles(ctx.dummyPos, Color{ 255, 180, 40, 255 }, 12);
                }
            } else if (t >= 0.95f) {
                swingAngle = 0.0f;
            }
        } else if (mode == DemoPreviewMode::Skill) {
            if (t >= 0.30f && t < 2.20f) {
                auraAngle += 60.0f * deltaTime;
                if (t < 0.50f) auraScale = (t - 0.30f) / 0.20f;
                else if (t > 2.00f) auraScale = std::max(0.0f, 1.0f - (t - 2.00f) / 0.20f);
                else auraScale = 1.0f;

                if (sequenceStep == 0) {
                    sequenceStep = 1;
                    AudioManager::GetInstance().PlaySoundEffect("fx_fire");
                    ctx.sandbox.SpawnEffectPopup(ctx.paladinPos, "FIRE CIRCLE", Color{ 255, 140, 40, 255 }, 0.9f);
                }
                if (t >= 0.80f && sequenceStep == 1) {
                    sequenceStep = 2;
                    ctx.dummy.ApplyStatus("BURN", 3.0f);
                    ctx.sandbox.SpawnEffectPopup(ctx.dummyPos, "BURN", Color{ 255, 110, 30, 255 }, 0.7f);
                    ctx.sandbox.SpawnHitParticles(ctx.dummyPos, Color{ 255, 120, 20, 255 }, 6);
                }
                if (t >= 1.10f && t < 1.30f) {
                    float s = (t - 1.10f) / 0.20f;
                    swingAngle = -60.0f + 120.0f * ((1.0f - cosf(s * PI)) / 2.0f);
                    if (s >= 0.50f && sequenceStep == 2) {
                        sequenceStep = 3;
                        ctx.dummy.TakeDamage(static_cast<int>(BaseStats::Damage * def.weapon.minDamageScalar));
                        AudioManager::GetInstance().PlayRandomSwordSlash();
                        ctx.sandbox.SpawnHitParticles(ctx.dummyPos, Color{ 255, 80, 80, 255 }, 8);
                    }
                } else if (t >= 1.30f) {
                    swingAngle = 0.0f;
                }
                if (t >= 1.60f && sequenceStep == 3) {
                    sequenceStep = 4;
                    ctx.dummy.ApplyStatus("BURN", 3.0f);
                    ctx.sandbox.SpawnEffectPopup(ctx.dummyPos, "BURN", Color{ 255, 110, 30, 255 }, 0.7f);
                    ctx.sandbox.SpawnHitParticles(ctx.dummyPos, Color{ 255, 120, 20, 255 }, 6);
                }
            } else if (t >= 2.20f) {
                auraScale = 0.0f;
                swingAngle = 0.0f;
            }
        } else if (mode == DemoPreviewMode::Ultimate) {
            if (t >= 0.35f && sequenceStep == 0) {
                sequenceStep = 1;
                AudioManager::GetInstance().PlaySoundEffect("fx_keith_ult");
                AudioManager::GetInstance().PlaySoundEffect("vl_keith_ult");
                ctx.sandbox.SpawnEffectPopup(ctx.paladinPos, "EXCALIBUR!", Color{ 255, 80, 40, 255 }, 1.0f);
            }
            if (t >= 0.70f && sequenceStep == 1) {
                sequenceStep = 2;
                ctx.dummy.TakeDamage(250);
                ctx.dummy.ApplyStatus("BURN", 3.0f);
                ctx.sandbox.SpawnEffectPopup(ctx.dummyPos, "CRIT!", Color{ 255, 215, 0, 255 }, 1.2f);
                ctx.sandbox.SpawnHitParticles(ctx.dummyPos, Color{ 255, 100, 30, 255 }, 16);
            }
            if (t >= 1.25f && sequenceStep == 2) {
                sequenceStep = 3;
                ctx.dummy.ApplyStatus("BURN", 3.0f);
                ctx.sandbox.SpawnEffectPopup(ctx.dummyPos, "BURN", Color{ 255, 120, 20, 255 }, 0.8f);
                ctx.sandbox.SpawnHitParticles(ctx.dummyPos, Color{ 255, 140, 30, 255 }, 8);
            }
        }
    }

    void DrawWeapon(const DemoDrawContext& ctx) const override {
        if (weaponTex.id == 0) return;
        Vector2 pivot = { ctx.paladinPos.x - 3.0f, ctx.paladinPos.y + 4.0f };

        Rectangle src = { 0.0f, 0.0f, (float)weaponTex.width, (float)weaponTex.height };
        Rectangle dest = { pivot.x, pivot.y, (float)weaponTex.width * WEAPON_SCALE, (float)weaponTex.height * WEAPON_SCALE };
        Vector2 origin = { (float)weaponTex.width * WEAPON_SCALE * 0.12f, (float)weaponTex.height * WEAPON_SCALE * 0.5f };
        DrawTexturePro(weaponTex, src, dest, origin, swingAngle, WHITE);
    }

    void DrawVFX(const DemoDrawContext& ctx) const override {
        float t = ctx.loopTimer;
        Vector2 pivot = { ctx.paladinPos.x - 3.0f, ctx.paladinPos.y + 4.0f };

        if (mode == DemoPreviewMode::BasicAttack) {
            if (slashTex.id != 0) {
                float frameW = (float)slashTex.width / 3.0f;
                float frameH = (float)slashTex.height;

                if (t >= 0.45f && t < 0.65f) {
                    float s = (t - 0.45f) / 0.20f;
                    if (s >= 0.15f && s <= 0.90f) {
                        int f = std::clamp((int)((s - 0.15f) / 0.75f * 3.0f), 0, 2);
                        float dist = (float)weaponTex.width * WEAPON_SCALE * 0.85f;
                        float rad = swingAngle * DEG2RAD;
                        Vector2 slashPos = { pivot.x + cosf(rad) * dist, pivot.y + sinf(rad) * dist };
                        BeginBlendMode(BLEND_ADDITIVE);
                        DrawTexturePro(slashTex, { f * frameW, 0.0f, frameW, frameH }, { slashPos.x, slashPos.y, frameW * 1.4f, frameH * 1.4f }, { frameW * 0.7f, frameH * 0.7f }, swingAngle, WHITE);
                        EndBlendMode();
                    }
                } else if (t >= 0.75f && t < 0.95f) {
                    float s = (t - 0.75f) / 0.20f;
                    if (s >= 0.15f && s <= 0.90f) {
                        int f = std::clamp((int)((s - 0.15f) / 0.75f * 3.0f), 0, 2);
                        float dist = (float)weaponTex.width * WEAPON_SCALE * 0.85f;
                        float rad = swingAngle * DEG2RAD;
                        Vector2 slashPos = { pivot.x + cosf(rad) * dist, pivot.y + sinf(rad) * dist };
                        BeginBlendMode(BLEND_ADDITIVE);
                        DrawTexturePro(slashTex, { f * frameW, 0.0f, frameW, -frameH }, { slashPos.x, slashPos.y, frameW * 1.5f, frameH * 1.5f }, { frameW * 0.75f, frameH * 0.75f }, swingAngle, Color{ 255, 210, 100, 255 });
                        EndBlendMode();
                    }
                }
            }
        } else if (mode == DemoPreviewMode::Skill) {
            if (auraScale > 0.01f && fireRangeTex.id != 0) {
                float diameter = 200.0f * ENTITY_SCALE * auraScale;
                BeginBlendMode(BLEND_ADDITIVE);
                DrawTexturePro(fireRangeTex, { 0, 0, (float)fireRangeTex.width, (float)fireRangeTex.height }, { ctx.paladinPos.x, ctx.paladinPos.y, diameter, diameter }, { diameter * 0.5f, diameter * 0.5f }, auraAngle, WHITE);
                EndBlendMode();
            }
        } else if (mode == DemoPreviewMode::Ultimate) {
            if (t >= 0.35f && t < 2.60f) {
                float waveFlightDuration = 0.55f; // Fast, piercing wave travel
                float waveProgress = std::clamp((t - 0.35f) / waveFlightDuration, 0.0f, 1.0f);
                float maxTrailDist = 420.0f; // Travels past the dummy across the arena
                float currentTrailLength = waveProgress * maxTrailDist;
                float startX = ctx.paladinPos.x + 10.0f;
                float waveX = startX + currentTrailLength;

                // Lingering alpha for the flame trail
                float trailAlpha = 1.0f;
                if (t > 2.10f) {
                    trailAlpha = std::clamp((2.60f - t) / 0.50f, 0.0f, 1.0f);
                }

                // 1. Draw glowing fiery ground trail matching Keith.cpp (width = 70.0f)
                if (currentTrailLength > 0.0f) {
                    float trailHeight = 70.0f * ENTITY_SCALE;
                    Rectangle trailRect = { startX, ctx.paladinPos.y, currentTrailLength, trailHeight };
                    Vector2 origin = { 0.0f, trailHeight * 0.5f };
                    DrawRectanglePro(trailRect, origin, 0.0f, ColorAlpha(RED, 0.35f * trailAlpha));
                    DrawRectanglePro(trailRect, origin, 0.0f, ColorAlpha(ORANGE, 0.18f * trailAlpha));
                }

                // 2. Draw animated 4-frame fire sprites along the trail matching KeithUltiProjectile.cpp
                if (fireAnimTex.id != 0 && currentTrailLength > 10.0f) {
                    float trailW = (float)fireAnimTex.width / 4.0f;
                    float trailH = (float)fireAnimTex.height;
                    for (float x = startX + 15.0f; x <= startX + currentTrailLength; x += 26.0f) {
                        int f = ((int)(t * 12.0f + (x - startX) * 0.05f)) % 4;
                        float yOffset = sinf((x - startX) * 0.2f + t * 4.0f) * 3.0f;
                        DrawTexturePro(fireAnimTex, { f * trailW, 0, trailW, trailH },
                            { x, ctx.paladinPos.y + 6.0f + yOffset, trailW * ENTITY_SCALE, trailH * ENTITY_SCALE },
                            { (trailW * ENTITY_SCALE) * 0.5f, (trailH * ENTITY_SCALE) * 0.5f }, 0.0f, ColorAlpha(WHITE, trailAlpha));
                    }
                }

                // 3. Draw piercing wave projectile while in flight
                if (waveProgress < 1.0f && ultiFireTex.id != 0) {
                    float waveW = (float)ultiFireTex.width * ENTITY_SCALE;
                    float waveH = (float)ultiFireTex.height * ENTITY_SCALE;
                    DrawTexturePro(ultiFireTex, { 0, 0, (float)ultiFireTex.width, (float)ultiFireTex.height },
                        { waveX, ctx.paladinPos.y, waveW, waveH },
                        { waveW * 0.5f, waveH * 0.5f }, 0.0f, WHITE);
                }
            }
        }
    }
};

// =========================================================================
// 3. Concrete Strategy: LanceDemoSequence
// =========================================================================
class LanceDemoSequence : public IDemoSequence {
private:
    DemoPreviewMode mode;
    float recoilX;
    bool isDual;
    int sequenceStep;

    Texture2D weaponTex;
    Texture2D muzzleTex;
    Texture2D bulletTex;
    Texture2D impactTex;
    Texture2D explodeTex;

public:
    LanceDemoSequence(DemoPreviewMode m)
        : mode(m),
          recoilX(0.0f),
          isDual(false),
          sequenceStep(0),
          weaponTex({ 0 }),
          muzzleTex({ 0 }),
          bulletTex({ 0 }),
          impactTex({ 0 }),
          explodeTex({ 0 }) {}

    void Reset(DemoUpdateContext& ctx) override {
        recoilX = 0.0f;
        isDual = false;
        sequenceStep = 0;

        AssetManager& assets = AssetManager::GetInstance();
        weaponTex = assets.GetTexture("Lance_Weapon");
        muzzleTex = assets.GetTexture("Lance_Muzzle");
        bulletTex = assets.GetTexture("Lance_Bullet");
        impactTex = assets.GetTexture("Lance_Impact");
        explodeTex = assets.GetTexture("Ulti_explode");
    }

    float GetDuration() const override {
        return (mode == DemoPreviewMode::BasicAttack) ? 2.0f : ((mode == DemoPreviewMode::Skill) ? 2.6f : 2.6f);
    }

    void Update(float deltaTime, DemoUpdateContext& ctx) override {
        float t = ctx.loopTimer;
        const PaladinDefinition& def = ctx.def;

        if (recoilX < 0.0f) {
            recoilX = std::min(0.0f, recoilX + deltaTime * 35.0f);
        }

        if (mode == DemoPreviewMode::BasicAttack) {
            if (t >= 0.45f && sequenceStep == 0) {
                sequenceStep = 1;
                recoilX = -12.0f;
                AudioManager::GetInstance().PlayRandomLaserGun();

                DemoProjectile proj;
                proj.startPos = { ctx.paladinPos.x + 18.0f, ctx.paladinPos.y + 4.0f };
                proj.targetPos = ctx.dummyPos;
                proj.position = proj.startPos;
                proj.progress = 0.0f;
                proj.speed = 5.0f;
                proj.active = true;
                proj.textureKey = "Lance_Bullet";
                proj.rotation = 0.0f;
                proj.damage = static_cast<int>(BaseStats::Damage * def.weapon.maxDamageScalar);
                proj.trailColor = Color{ 80, 200, 255, 255 };
                ctx.sandbox.SpawnProjectile(proj);
            }
        } else if (mode == DemoPreviewMode::Skill) {
            if (t >= 0.30f && t < 2.30f) {
                isDual = true;
                if (sequenceStep == 0) {
                    sequenceStep = 1;
                    AudioManager::GetInstance().PlaySoundEffect("fx_lance_skill");
                    AudioManager::GetInstance().PlaySoundEffect("vl_lance_skill");
                    ctx.sandbox.SpawnEffectPopup(ctx.paladinPos, "DUAL WIELD", Color{ 80, 200, 255, 255 }, 0.9f);
                }

                auto FireDual = [&](int dmg) {
                    recoilX = -10.0f;
                    AudioManager::GetInstance().PlayRandomLaserGun();
                    Vector2 pivot = { ctx.paladinPos.x - 3.0f, ctx.paladinPos.y + 4.0f };
                    DemoProjectile p1;
                    p1.startPos = { pivot.x + 20.0f, pivot.y - 5.0f };
                    p1.targetPos = { ctx.dummyPos.x, ctx.dummyPos.y - 6.0f };
                    p1.position = p1.startPos;
                    p1.progress = 0.0f;
                    p1.speed = 5.5f;
                    p1.active = true;
                    p1.textureKey = "Lance_Bullet";
                    p1.rotation = 0.0f;
                    p1.damage = dmg;
                    p1.trailColor = Color{ 80, 200, 255, 255 };
                    ctx.sandbox.SpawnProjectile(p1);

                    DemoProjectile p2 = p1;
                    p2.startPos = { pivot.x + 20.0f, pivot.y + 5.0f };
                    p2.targetPos = { ctx.dummyPos.x, ctx.dummyPos.y + 6.0f };
                    p2.position = p2.startPos;
                    ctx.sandbox.SpawnProjectile(p2);
                };

                if (t >= 0.55f && sequenceStep == 1) { sequenceStep = 2; FireDual(static_cast<int>(BaseStats::Damage * def.weapon.maxDamageScalar)); }
                if (t >= 1.15f && sequenceStep == 2) { sequenceStep = 3; FireDual(static_cast<int>(BaseStats::Damage * def.weapon.maxDamageScalar)); }
                if (t >= 1.75f && sequenceStep == 3) { sequenceStep = 4; FireDual(static_cast<int>(BaseStats::Damage * def.weapon.maxDamageScalar)); }
            } else if (t >= 2.30f) {
                isDual = false;
            }
        } else if (mode == DemoPreviewMode::Ultimate) {
            if (t >= 0.40f && sequenceStep == 0) {
                sequenceStep = 1;
                AudioManager::GetInstance().PlaySoundEffect("vl_lance_ult");
                AudioManager::GetInstance().PlaySoundEffect("fx_lance_ult");
                AudioManager::GetInstance().PlaySoundEffect("fx_ice_explode");
                ctx.sandbox.SpawnEffectPopup(ctx.paladinPos, "GLACIER PIERCE!", Color{ 100, 210, 255, 255 }, 1.0f);
            }
            if (t >= 0.60f && sequenceStep == 1) {
                sequenceStep = 2;
                ctx.dummy.TakeDamage(180);
                ctx.dummy.ApplyStatus("FREEZE", 4.0f);
                ctx.sandbox.SpawnEffectPopup(ctx.dummyPos, "FROZEN!", Color{ 140, 230, 255, 255 }, 1.3f);
                ctx.sandbox.SpawnHitParticles(ctx.dummyPos, Color{ 120, 220, 255, 255 }, 16);
            }
        }

        for (auto& proj : ctx.sandbox.GetProjectiles()) {
            if (!proj.active) continue;
            proj.progress += deltaTime * proj.speed;
            proj.position = Vector2Lerp(proj.startPos, proj.targetPos, proj.progress);
            if (proj.progress >= 1.0f) {
                proj.active = false;
                ctx.dummy.TakeDamage(proj.damage);
                ctx.sandbox.SpawnHitParticles(proj.targetPos, proj.trailColor, 6);
            }
        }
    }

    void DrawWeapon(const DemoDrawContext& ctx) const override {
        if (weaponTex.id == 0) return;
        Vector2 pivot = { ctx.paladinPos.x - 3.0f + recoilX, ctx.paladinPos.y + 4.0f };

        if (isDual) {
            Vector2 leftPivot = { pivot.x, pivot.y - 5.0f };
            Vector2 rightPivot = { pivot.x, pivot.y + 5.0f };
            Vector2 origin = { 0.0f, (float)weaponTex.height * WEAPON_SCALE * 0.5f };
            DrawTexturePro(weaponTex, { 0, 0, (float)weaponTex.width, (float)weaponTex.height }, { leftPivot.x, leftPivot.y, (float)weaponTex.width * WEAPON_SCALE, (float)weaponTex.height * WEAPON_SCALE }, origin, 0.0f, WHITE);
            DrawTexturePro(weaponTex, { 0, 0, (float)weaponTex.width, (float)weaponTex.height }, { rightPivot.x, rightPivot.y, (float)weaponTex.width * WEAPON_SCALE, (float)weaponTex.height * WEAPON_SCALE }, origin, 0.0f, WHITE);
        } else {
            Vector2 origin = { 0.0f, (float)weaponTex.height * WEAPON_SCALE * 0.5f };
            DrawTexturePro(weaponTex, { 0, 0, (float)weaponTex.width, (float)weaponTex.height }, { pivot.x, pivot.y, (float)weaponTex.width * WEAPON_SCALE, (float)weaponTex.height * WEAPON_SCALE }, origin, 0.0f, WHITE);
        }
    }

    void DrawVFX(const DemoDrawContext& ctx) const override {
        float t = ctx.loopTimer;
        Vector2 muzzlePos = { ctx.paladinPos.x + 18.0f, ctx.paladinPos.y + 4.0f };

        if (mode == DemoPreviewMode::BasicAttack) {
            if (t >= 0.45f && t < 0.51f && muzzleTex.id != 0) {
                DrawTexturePro(muzzleTex, { 0, 0, (float)muzzleTex.width, (float)muzzleTex.height }, { muzzlePos.x, muzzlePos.y, (float)muzzleTex.width * 1.4f, (float)muzzleTex.height * 1.4f }, { (float)muzzleTex.width * 0.7f, (float)muzzleTex.height * 0.7f }, 0.0f, WHITE);
            }
        } else if (mode == DemoPreviewMode::Skill) {
            if (isDual && recoilX < -5.0f && muzzleTex.id != 0) {
                DrawTexturePro(muzzleTex, { 0, 0, (float)muzzleTex.width, (float)muzzleTex.height }, { muzzlePos.x, muzzlePos.y - 5.0f, (float)muzzleTex.width * 1.3f, (float)muzzleTex.height * 1.3f }, { (float)muzzleTex.width * 0.65f, (float)muzzleTex.height * 0.65f }, 0.0f, WHITE);
                DrawTexturePro(muzzleTex, { 0, 0, (float)muzzleTex.width, (float)muzzleTex.height }, { muzzlePos.x, muzzlePos.y + 5.0f, (float)muzzleTex.width * 1.3f, (float)muzzleTex.height * 1.3f }, { (float)muzzleTex.width * 0.65f, (float)muzzleTex.height * 0.65f }, 0.0f, WHITE);
            }
        } else if (mode == DemoPreviewMode::Ultimate) {
            if (t >= 0.40f && t < 1.00f && explodeTex.id != 0) {
                float frameW = (float)explodeTex.width / 8.0f;
                int f = std::clamp((int)(((t - 0.40f) / 0.48f) * 8.0f), 0, 7);
                DrawTexturePro(explodeTex, { f * frameW, 0.0f, frameW, (float)explodeTex.height }, { ctx.dummyPos.x, ctx.dummyPos.y, frameW * 1.6f, (float)explodeTex.height * 1.6f }, { frameW * 0.8f, (float)explodeTex.height * 0.8f }, 0.0f, WHITE);
            }
        }
    }
};

// =========================================================================
// 4. Concrete Strategy: HunkDemoSequence
// =========================================================================
class HunkDemoSequence : public IDemoSequence {
private:
    DemoPreviewMode mode;
    float recoilX;
    float offsetY;
    float shockRadius;
    float shieldAngle;
    float shieldScale;
    int sequenceStep;

    Texture2D weaponTex;
    Texture2D muzzleTex;
    Texture2D beamTex;
    Texture2D impactTex;
    Texture2D shieldTex;

public:
    HunkDemoSequence(DemoPreviewMode m)
        : mode(m),
          recoilX(0.0f),
          offsetY(0.0f),
          shockRadius(0.0f),
          shieldAngle(0.0f),
          shieldScale(0.0f),
          sequenceStep(0),
          weaponTex({ 0 }),
          muzzleTex({ 0 }),
          beamTex({ 0 }),
          impactTex({ 0 }),
          shieldTex({ 0 }) {}

    void Reset(DemoUpdateContext& ctx) override {
        recoilX = offsetY = shockRadius = shieldAngle = shieldScale = 0.0f;
        sequenceStep = 0;

        AssetManager& assets = AssetManager::GetInstance();
        weaponTex = assets.GetTexture("Hunk_Weapon");
        muzzleTex = assets.GetTexture("Hunk_Muzzle");
        beamTex = assets.GetTexture("Hunk_Bullet");
        impactTex = assets.GetTexture("Hunk_Impact");
        shieldTex = assets.GetTexture("shield");
    }

    float GetDuration() const override {
        return (mode == DemoPreviewMode::BasicAttack) ? 2.0f : ((mode == DemoPreviewMode::Skill) ? 2.6f : 2.8f);
    }

    void Update(float deltaTime, DemoUpdateContext& ctx) override {
        float t = ctx.loopTimer;
        const PaladinDefinition& def = ctx.def;

        if (recoilX < 0.0f) {
            recoilX = std::min(0.0f, recoilX + deltaTime * 28.0f);
        }

        if (mode == DemoPreviewMode::BasicAttack) {
            if (t >= 0.45f && sequenceStep == 0) {
                sequenceStep = 1;
                recoilX = -24.0f;
                AudioManager::GetInstance().PlayRandomLaserGun();
                ctx.dummy.TakeDamage(static_cast<int>(BaseStats::Damage * def.weapon.maxDamageScalar));
                ctx.sandbox.SpawnHitParticles(ctx.dummyPos, Color{ 255, 160, 20, 255 }, 12);
            }
        } else if (mode == DemoPreviewMode::Skill) {
            if (t >= 0.25f && t < 0.45f) {
                offsetY = -sinf((t - 0.25f) / 0.20f * PI) * 12.0f;
            } else if (t >= 0.45f) {
                offsetY = 0.0f;
                if (sequenceStep == 0) {
                    sequenceStep = 1;
                    AudioManager::GetInstance().PlaySoundEffect("fx_hunk_skill");
                    shockRadius = 10.0f;
                    ctx.sandbox.SpawnHitParticles({ ctx.paladinPos.x + 8.0f, ctx.paladinPos.y + 26.0f }, Color{ 160, 120, 70, 255 }, 14);
                }
                if (shockRadius > 0.0f && shockRadius < 180.0f) {
                    shockRadius += deltaTime * 420.0f;
                }
                if (t >= 0.65f && sequenceStep == 1) {
                    sequenceStep = 2;
                    ctx.dummy.ApplyStatus("DIZZY", 2.0f);
                    ctx.sandbox.SpawnEffectPopup(ctx.dummyPos, "STUNNED!", Color{ 255, 215, 0, 255 }, 1.3f);
                    ctx.sandbox.SpawnHitParticles(ctx.dummyPos, Color{ 255, 200, 50, 255 }, 12);
                }
                if (t >= 1.15f && sequenceStep == 2) {
                    sequenceStep = 3;
                    recoilX = -24.0f;
                    AudioManager::GetInstance().PlayRandomLaserGun();
                    ctx.dummy.TakeDamage(static_cast<int>(BaseStats::Damage * def.weapon.maxDamageScalar));
                    ctx.sandbox.SpawnHitParticles(ctx.dummyPos, Color{ 255, 160, 20, 255 }, 12);
                }
            }
        } else if (mode == DemoPreviewMode::Ultimate) {
            if (t >= 0.35f && sequenceStep == 0) {
                sequenceStep = 1;
                AudioManager::GetInstance().PlaySoundEffect("vl_hunk_ult");
                AudioManager::GetInstance().PlaySoundEffect("fx_get_buff");
                ctx.sandbox.SpawnEffectPopup(ctx.paladinPos, "AEGIS SHIELD!", Color{ 255, 215, 0, 255 }, 1.0f);
            }
            if (t >= 0.35f && t < 2.50f) {
                shieldAngle += 90.0f * deltaTime;
                if (t < 0.55f) shieldScale = (t - 0.35f) / 0.20f;
                else if (t > 2.30f) shieldScale = std::max(0.0f, 1.0f - (t - 2.30f) / 0.20f);
                else shieldScale = 1.0f;
            }

            auto EnemyShootAtHunk = [&]() {
                AudioManager::GetInstance().PlaySoundEffect("fx_laser_bullet");
                DemoProjectile p;
                p.startPos = ctx.dummyPos;
                p.targetPos = { ctx.paladinPos.x + 12.0f, ctx.paladinPos.y };
                p.position = p.startPos;
                p.progress = 0.0f;
                p.speed = 2.8f;
                p.active = true;
                p.textureKey = "Knight_Gun_Bullet";
                p.rotation = 180.0f;
                p.damage = 0; // Prevented by Aegis Shield!
                p.trailColor = Color{ 255, 220, 80, 255 };
                ctx.sandbox.SpawnProjectile(p);
            };

            // Dummy shoots bullet 1 at Hunk while shield is active
            if (t >= 0.75f && sequenceStep == 1) {
                sequenceStep = 2;
                EnemyShootAtHunk();
            }

            // Dummy shoots bullet 2 at Hunk
            if (t >= 1.45f && sequenceStep == 2) {
                sequenceStep = 3;
                EnemyShootAtHunk();
            }
        }

        for (auto& proj : ctx.sandbox.GetProjectiles()) {
            if (!proj.active) continue;
            proj.progress += deltaTime * proj.speed;
            proj.position = Vector2Lerp(proj.startPos, proj.targetPos, proj.progress);
            if (proj.progress >= 1.0f) {
                proj.active = false;
                if (mode == DemoPreviewMode::Ultimate) {
                    // Shield deflects the incoming attack!
                    AudioManager::GetInstance().PlaySoundEffect("fx_shield_hit");
                    ctx.sandbox.SpawnEffectPopup(ctx.paladinPos, "BLOCKED!", GOLD, 0.8f);
                    ctx.sandbox.SpawnHitParticles(proj.targetPos, Color{ 255, 220, 80, 255 }, 12);
                } else {
                    ctx.dummy.TakeDamage(proj.damage);
                    ctx.sandbox.SpawnHitParticles(proj.targetPos, proj.trailColor, 6);
                }
            }
        }
    }

    void DrawWeapon(const DemoDrawContext& ctx) const override {
        if (weaponTex.id == 0) return;
        Vector2 pivot = { ctx.paladinPos.x - 3.0f + recoilX, ctx.paladinPos.y + 4.0f + offsetY };
        Vector2 origin = { 0.0f, (float)weaponTex.height * WEAPON_SCALE * 0.5f };
        DrawTexturePro(weaponTex, { 0, 0, (float)weaponTex.width, (float)weaponTex.height }, { pivot.x, pivot.y, (float)weaponTex.width * WEAPON_SCALE, (float)weaponTex.height * WEAPON_SCALE }, origin, 0.0f, WHITE);
    }

    void DrawVFX(const DemoDrawContext& ctx) const override {
        float t = ctx.loopTimer;
        Vector2 muzzlePos = { ctx.paladinPos.x + 22.0f + recoilX, ctx.paladinPos.y + 4.0f + offsetY };

        if (mode == DemoPreviewMode::BasicAttack || (mode == DemoPreviewMode::Skill && t >= 1.15f && t < 1.30f)) {
            float startT = (mode == DemoPreviewMode::BasicAttack) ? 0.45f : 1.15f;
            if (t >= startT && t < startT + 0.15f) {
                float s = (t - startT) / 0.15f;
                int f = std::clamp((int)(s * 2.0f), 0, 1);
                if (muzzleTex.id != 0) {
                    float fw = (float)muzzleTex.width / 2.0f;
                    DrawTexturePro(muzzleTex, { f * fw, 0, fw, (float)muzzleTex.height }, { muzzlePos.x, muzzlePos.y, fw * 1.4f, (float)muzzleTex.height * 1.4f }, { fw * 0.7f, (float)muzzleTex.height * 0.7f }, 0.0f, WHITE);
                }
                if (beamTex.id != 0) {
                    float fw = (float)beamTex.width / 2.0f;
                    float dist = (ctx.dummyPos.x + 35.0f) - muzzlePos.x;
                    DrawTexturePro(beamTex, { f * fw, 0, fw, (float)beamTex.height }, { muzzlePos.x, muzzlePos.y, dist, (float)beamTex.height * 1.4f }, { 0.0f, (float)beamTex.height * 0.7f }, 0.0f, WHITE);
                }
                if (impactTex.id != 0) {
                    float fw = (float)impactTex.width / 2.0f;
                    DrawTexturePro(impactTex, { f * fw, 0, fw, (float)impactTex.height }, { ctx.dummyPos.x, ctx.dummyPos.y, fw * 1.4f, (float)impactTex.height * 1.4f }, { fw * 0.7f, (float)impactTex.height * 0.7f }, 0.0f, WHITE);
                }
            }
        } else if (mode == DemoPreviewMode::Skill) {
            if (shockRadius > 0.0f && shockRadius < 180.0f) {
                float alpha = std::clamp(1.0f - (shockRadius / 180.0f), 0.0f, 1.0f);
                DrawCircleLines((int)ctx.paladinPos.x, (int)(ctx.paladinPos.y + 18.0f), shockRadius, ColorAlpha(Color{ 180, 120, 60, 255 }, alpha * 0.9f));
                DrawCircleLines((int)ctx.paladinPos.x, (int)(ctx.paladinPos.y + 18.0f), shockRadius - 4.0f, ColorAlpha(GOLD, alpha * 0.7f));
            }
            if (ctx.dummy.IsDizzy()) {
                float starAngle = t * 360.0f * DEG2RAD;
                for (int i = 0; i < 3; ++i) {
                    float a = starAngle + i * (2.0f * PI / 3.0f);
                    DrawRectangle((int)(ctx.dummyPos.x + cosf(a) * 14.0f) - 2, (int)(ctx.dummyPos.y - 36.0f + sinf(a) * 4.0f) - 2, 4, 4, GOLD);
                }
            }
        } else if (mode == DemoPreviewMode::Ultimate) {
            if (shieldScale > 0.01f && shieldTex.id != 0) {
                float w = (float)shieldTex.width * ENTITY_SCALE * shieldScale;
                float h = (float)shieldTex.height * ENTITY_SCALE * shieldScale;
                DrawTexturePro(shieldTex, { 0, 0, (float)shieldTex.width, (float)shieldTex.height }, { ctx.paladinPos.x, ctx.paladinPos.y, w, h }, { w * 0.5f, h * 0.5f }, shieldAngle, WHITE);
            }
        }
    }
};

// =========================================================================
// 5. Concrete Strategy: PidgeDemoSequence
// =========================================================================
class PidgeDemoSequence : public IDemoSequence {
private:
    struct ToxicDemoParticle {
        Vector2 position;
        Vector2 velocity;
        float life;
        float maxLife;
        float frameTimer;
        int currentFrame;
        float scale;
        float alpha;
    };

    DemoPreviewMode mode;
    bool isVenom;
    Vector2 venomPos;
    bool roverActive;
    Vector2 roverPos;
    int sequenceStep;

    // Cached Textures
    Texture2D weaponTex;
    Texture2D explodeTex;
    Texture2D roverTex;
    Texture2D roverBulletTex;
    Texture2D toxicTex;

    std::vector<ToxicDemoParticle> toxicParticles;
    float toxicSpawnTimer;

public:
    PidgeDemoSequence(DemoPreviewMode m)
        : mode(m),
          isVenom(false),
          venomPos({ 0, 0 }),
          roverActive(false),
          roverPos({ 0, 0 }),
          sequenceStep(0),
          weaponTex({ 0 }),
          explodeTex({ 0 }),
          roverTex({ 0 }),
          roverBulletTex({ 0 }),
          toxicTex({ 0 }),
          toxicSpawnTimer(0.0f) {}

    void Reset(DemoUpdateContext& ctx) override {
        isVenom = roverActive = false;
        venomPos = roverPos = { 0, 0 };
        sequenceStep = 0;
        toxicParticles.clear();
        toxicSpawnTimer = 0.0f;

        AssetManager& assets = AssetManager::GetInstance();
        weaponTex = assets.GetTexture("Paladin_Pidge_Weapon");
        explodeTex = assets.GetTexture("skill_explode");
        roverTex = assets.GetTexture("Rover");
        roverBulletTex = assets.GetTexture("Rover_bullet");
        toxicTex = assets.GetTexture("toxic");
    }

    float GetDuration() const override {
        return (mode == DemoPreviewMode::BasicAttack) ? 2.0f : ((mode == DemoPreviewMode::Skill) ? 2.6f : 2.8f);
    }

    void Update(float deltaTime, DemoUpdateContext& ctx) override {
        float t = ctx.loopTimer;
        const PaladinDefinition& def = ctx.def;

        if (mode == DemoPreviewMode::BasicAttack) {
            float startT = 0.45f;
            float hitT = 0.70f;

            if (t >= startT && sequenceStep == 0) {
                sequenceStep = 1;
                AudioManager::GetInstance().PlayRandomLaser();
            }

            // Hit trigger strictly synchronized to when katar reaches target
            if (t >= hitT && sequenceStep == 1) {
                sequenceStep = 2;
                ctx.dummy.TakeDamage(static_cast<int>(BaseStats::Damage * def.weapon.maxDamageScalar));
                ctx.sandbox.SpawnHitParticles(ctx.dummyPos, Color{ 100, 255, 130, 255 }, 8);
            }
        } else if (mode == DemoPreviewMode::Skill) {
            if (t >= 0.35f && t < 2.25f) {
                isVenom = true;
                venomPos = { ctx.dummyPos.x, ctx.dummyPos.y };
                if (sequenceStep == 0) {
                    sequenceStep = 1;
                    AudioManager::GetInstance().PlaySoundEffect("fx_flash_lighting");
                    ctx.sandbox.SpawnEffectPopup(venomPos, "VENOM ZONE", Color{ 120, 255, 120, 255 }, 0.9f);
                }
                if (t >= 0.85f && sequenceStep == 1) {
                    sequenceStep = 2;
                    ctx.dummy.ApplyStatus("POISON", 3.0f);
                    ctx.sandbox.SpawnEffectPopup(ctx.dummyPos, "POISON", Color{ 110, 255, 130, 255 }, 0.7f);
                    ctx.sandbox.SpawnHitParticles(ctx.dummyPos, Color{ 90, 255, 110, 255 }, 6);
                }

                // Katar strike synchronized strictly to hit extension (t >= 1.40f)
                if (t >= 1.40f && sequenceStep == 2) {
                    sequenceStep = 3;
                    ctx.dummy.TakeDamage(static_cast<int>(BaseStats::Damage * def.weapon.maxDamageScalar));
                    ctx.sandbox.SpawnHitParticles(ctx.dummyPos, Color{ 110, 255, 140, 255 }, 8);
                }
                if (t >= 1.95f && sequenceStep == 3) {
                    sequenceStep = 4;
                    ctx.dummy.ApplyStatus("POISON", 3.0f);
                    ctx.sandbox.SpawnEffectPopup(ctx.dummyPos, "POISON", Color{ 110, 255, 130, 255 }, 0.7f);
                    ctx.sandbox.SpawnHitParticles(ctx.dummyPos, Color{ 90, 255, 110, 255 }, 6);
                }

                // Spawn and update toxic particles matching in-combat logic
                float zoneRadius = 150.0f;
                toxicSpawnTimer += deltaTime;
                if (toxicSpawnTimer >= 0.08f && toxicParticles.size() < 24) {
                    toxicSpawnTimer = 0.0f;
                    float r = ((float)GetRandomValue(0, 1000) / 1000.0f) * zoneRadius * 0.85f;
                    float theta = ((float)GetRandomValue(0, 6283) / 1000.0f);
                    Vector2 spawnPos = { venomPos.x + r * cosf(theta), venomPos.y + r * sinf(theta) };
                    Vector2 vel = { (float)GetRandomValue(-10, 10), (float)GetRandomValue(-24, -12) };
                    float life = 0.8f + (float)GetRandomValue(0, 60) * 0.01f;
                    float pScale = (0.8f + (float)GetRandomValue(0, 40) * 0.01f) * ENTITY_SCALE;
                    toxicParticles.push_back({ spawnPos, vel, life, life, 0.0f, 0, pScale, 1.0f });
                }

                for (auto it = toxicParticles.begin(); it != toxicParticles.end();) {
                    it->life -= deltaTime;
                    if (it->life <= 0.0f) {
                        it = toxicParticles.erase(it);
                    } else {
                        it->position.x += it->velocity.x * deltaTime;
                        it->position.y += it->velocity.y * deltaTime;
                        it->frameTimer += deltaTime;
                        if (it->frameTimer >= 0.12f) {
                            it->frameTimer -= 0.12f;
                            it->currentFrame = (it->currentFrame + 1) % 3;
                        }
                        float progress = it->life / it->maxLife;
                        it->alpha = (progress < 0.3f) ? (progress / 0.3f) : 1.0f;
                        ++it;
                    }
                }
            } else if (t >= 2.25f) {
                isVenom = false;
                toxicParticles.clear();
            }
        } else if (mode == DemoPreviewMode::Ultimate) {
            if (t >= 0.40f && sequenceStep == 0) {
                sequenceStep = 1;
                roverActive = true;
                roverPos = { ctx.paladinPos.x + 36.0f, ctx.paladinPos.y - 36.0f };
                AudioManager::GetInstance().PlaySoundEffect("vl_pidge_ult");
                AudioManager::GetInstance().PlaySoundEffect("fx_pidge_ult");
                ctx.sandbox.SpawnEffectPopup(ctx.paladinPos, "ROVER OVERRIDE!", Color{ 100, 255, 140, 255 }, 1.0f);
            }
            if (roverActive) {
                roverPos.y = ctx.paladinPos.y - 36.0f + sinf(t * 6.0f) * 3.0f;
                auto RoverFire = [&](int dmg) {
                    AudioManager::GetInstance().PlayRandomLaser();
                    DemoProjectile p;
                    p.startPos = roverPos;
                    p.targetPos = ctx.dummyPos;
                    p.position = p.startPos;
                    p.progress = 0.0f;
                    p.speed = 6.0f;
                    p.active = true;
                    p.textureKey = "Rover_bullet";
                    p.rotation = 0.0f;
                    p.damage = dmg;
                    p.trailColor = Color{ 100, 255, 140, 255 };
                    ctx.sandbox.SpawnProjectile(p);
                };
                if (t >= 0.70f && sequenceStep == 1) { sequenceStep = 2; RoverFire(45); }
                if (t >= 1.20f && sequenceStep == 2) { sequenceStep = 3; RoverFire(45); }
                if (t >= 1.70f && sequenceStep == 3) { sequenceStep = 4; RoverFire(45); }
            }
        }

        for (auto& proj : ctx.sandbox.GetProjectiles()) {
            if (!proj.active) continue;
            proj.progress += deltaTime * proj.speed;
            proj.position = Vector2Lerp(proj.startPos, proj.targetPos, proj.progress);
            if (proj.progress >= 1.0f) {
                proj.active = false;
                ctx.dummy.TakeDamage(proj.damage);
                ctx.sandbox.SpawnHitParticles(proj.targetPos, proj.trailColor, 6);
            }
        }
    }

    void DrawWeapon(const DemoDrawContext& ctx) const override {
        float t = ctx.loopTimer;
        bool inFlight = (mode == DemoPreviewMode::BasicAttack && t >= 0.45f && t < 0.95f) ||
                        (mode == DemoPreviewMode::Skill && t >= 1.15f && t < 1.65f);
        if (!inFlight && weaponTex.id != 0) {
            Vector2 pivot = { ctx.paladinPos.x - 3.0f, ctx.paladinPos.y + 4.0f };
            Vector2 origin = { 0.0f, (float)weaponTex.height * WEAPON_SCALE * 0.5f };
            DrawTexturePro(weaponTex, { 0, 0, (float)weaponTex.width, (float)weaponTex.height }, { pivot.x, pivot.y, (float)weaponTex.width * WEAPON_SCALE, (float)weaponTex.height * WEAPON_SCALE }, origin, 0.0f, WHITE);
        }
    }

    void DrawVFX(const DemoDrawContext& ctx) const override {
        float t = ctx.loopTimer;
        Vector2 pivot = { ctx.paladinPos.x - 3.0f, ctx.paladinPos.y + 4.0f };

        if (mode == DemoPreviewMode::BasicAttack || mode == DemoPreviewMode::Skill) {
            float startT = (mode == DemoPreviewMode::BasicAttack) ? 0.45f : 1.15f;
            float hitT = (mode == DemoPreviewMode::BasicAttack) ? 0.70f : 1.40f;
            float endT = (mode == DemoPreviewMode::BasicAttack) ? 0.95f : 1.65f;

            if (t >= startT && t < endT) {
                Vector2 katarPos = (t < hitT) ? Vector2Lerp(pivot, ctx.dummyPos, (t - startT) / (hitT - startT))
                                              : Vector2Lerp(ctx.dummyPos, pivot, (t - hitT) / (endT - hitT));
                DrawLineEx(pivot, katarPos, 2.0f, GREEN);
                if (weaponTex.id != 0) {
                    DrawTexturePro(weaponTex, { 0, 0, (float)weaponTex.width, (float)weaponTex.height }, { katarPos.x, katarPos.y, (float)weaponTex.width * WEAPON_SCALE, (float)weaponTex.height * WEAPON_SCALE }, { (float)weaponTex.width * WEAPON_SCALE * 0.5f, (float)weaponTex.height * WEAPON_SCALE * 0.5f }, 0.0f, WHITE);
                }
            }

            if (mode == DemoPreviewMode::Skill) {
                if (t >= 0.35f && t < 0.85f && explodeTex.id != 0) {
                    float fw = (float)explodeTex.width / 8.0f;
                    int f = std::clamp((int)(((t - 0.35f) / 0.50f) * 8.0f), 0, 7);
                    DrawTexturePro(explodeTex, { f * fw, 0.0f, fw, (float)explodeTex.height }, { venomPos.x, venomPos.y, fw * 1.8f * ENTITY_SCALE, (float)explodeTex.height * 1.8f * ENTITY_SCALE }, { fw * 0.9f * ENTITY_SCALE, (float)explodeTex.height * 0.9f * ENTITY_SCALE }, 0.0f, WHITE);
                }
                if (isVenom) {
                    float zoneRadius = 150.0f;
                    // Translucent dark green poison zone fill (matches combat)
                    DrawCircleV(venomPos, zoneRadius, ColorAlpha(DARKGREEN, 0.25f));

                    // Signature RED circular outline rings (matches combat)
                    DrawCircleLines(venomPos.x, venomPos.y, zoneRadius, RED);
                    DrawCircleLines(venomPos.x, venomPos.y, zoneRadius - 1.0f, ColorAlpha(RED, 0.7f));
                    DrawCircleLines(venomPos.x, venomPos.y, zoneRadius + 1.0f, ColorAlpha(RED, 0.7f));

                    // Draw animated toxic particles (3-frame "toxic" sprites)
                    if (toxicTex.id != 0) {
                        float frameWidth = (float)toxicTex.width / 3.0f;
                        float frameHeight = (float)toxicTex.height;
                        for (const auto& p : toxicParticles) {
                            Rectangle src = { (float)p.currentFrame * frameWidth, 0.0f, frameWidth, frameHeight };
                            Rectangle dest = { p.position.x, p.position.y, frameWidth * p.scale, frameHeight * p.scale };
                            Vector2 origin = { frameWidth * p.scale * 0.5f, frameHeight * p.scale * 0.5f };
                            DrawTexturePro(toxicTex, src, dest, origin, 0.0f, ColorAlpha(WHITE, p.alpha));
                        }
                    }
                }
            }
        } else if (mode == DemoPreviewMode::Ultimate) {
            if (roverActive && roverTex.id != 0) {
                DrawTexturePro(roverTex, { 0, 0, (float)roverTex.width, (float)roverTex.height }, { roverPos.x, roverPos.y, (float)roverTex.width * 1.4f, (float)roverTex.height * 1.4f }, { (float)roverTex.width * 0.7f, (float)roverTex.height * 0.7f }, 0.0f, WHITE);
            }
        }
    }
};

// =========================================================================
// 6. Factory Implementation: DemoSequenceFactory
// =========================================================================
std::unique_ptr<IDemoSequence> DemoSequenceFactory::Create(PaladinId id, DemoPreviewMode mode) {
    switch (id) {
        case PaladinId::Keith: return std::make_unique<KeithDemoSequence>(mode);
        case PaladinId::Lance: return std::make_unique<LanceDemoSequence>(mode);
        case PaladinId::Hunk:  return std::make_unique<HunkDemoSequence>(mode);
        case PaladinId::Pidge: return std::make_unique<PidgeDemoSequence>(mode);
    }
    return std::make_unique<LanceDemoSequence>(mode);
}

// =========================================================================
// 7. Sandbox Orchestrator Implementation: DemoSandbox
// =========================================================================

Vector2 DemoSandbox::GetPaladinPos() const {
    if (currentPaladinId == PaladinId::Keith && currentMode == DemoPreviewMode::Skill) {
        return { 16.0f + 230.0f, 38.0f + 212.0f }; // Center of the stage
    }
    if (currentPaladinId == PaladinId::Pidge && currentMode == DemoPreviewMode::Skill) {
        return { 16.0f + 110.0f, 38.0f + 220.0f };
    }
    bool isKeithMelee = (currentPaladinId == PaladinId::Keith && currentMode == DemoPreviewMode::BasicAttack);
    return { 16.0f + (isKeithMelee ? 130.0f : 80.0f), 38.0f + 220.0f };
}

Vector2 DemoSandbox::GetDummyPos() const {
    if (currentPaladinId == PaladinId::Keith && currentMode == DemoPreviewMode::Skill) {
        return { 16.0f + 300.0f, 38.0f + 212.0f }; // Inside Keith's 100px fire circle
    }
    if (currentPaladinId == PaladinId::Pidge && currentMode == DemoPreviewMode::Skill) {
        return { 16.0f + 250.0f, 38.0f + 220.0f }; // Closer so venom circle is well inside stage
    }
    bool isKeithMelee = (currentPaladinId == PaladinId::Keith && currentMode == DemoPreviewMode::BasicAttack);
    return { 16.0f + (isKeithMelee ? 210.0f : 360.0f), 38.0f + 220.0f };
}

DemoSandbox::DemoSandbox()
    : currentPaladinId(PaladinId::Lance),
      currentMode(DemoPreviewMode::BasicAttack),
      cachedDef(&PaladinCatalog::Get(PaladinId::Lance)),
      loopTimer(0.0f) {
    dummy = std::make_unique<TrainingDummy>(GetDummyPos());
    activeSequence = DemoSequenceFactory::Create(currentPaladinId, currentMode);
}

DemoSandbox::~DemoSandbox() = default;

void DemoSandbox::Init(PaladinId paladinId) {
    currentPaladinId = paladinId;
    currentMode = DemoPreviewMode::BasicAttack;
    cachedDef = &PaladinCatalog::Get(paladinId);
    activeSequence = DemoSequenceFactory::Create(currentPaladinId, currentMode);
    Reset();
}

void DemoSandbox::SetPaladin(PaladinId paladinId) {
    currentPaladinId = paladinId;
    cachedDef = &PaladinCatalog::Get(paladinId);
    activeSequence = DemoSequenceFactory::Create(currentPaladinId, currentMode);
    Reset();
}

void DemoSandbox::SetMode(DemoPreviewMode mode) {
    currentMode = mode;
    activeSequence = DemoSequenceFactory::Create(currentPaladinId, currentMode);
    Reset();
}

void DemoSandbox::Reset() {
    loopTimer = 0.0f;
    effectPopups.clear();
    particles.clear();
    projectiles.clear();

    if (dummy) {
        dummy->Reset(GetDummyPos());
    }

    if (activeSequence && dummy && cachedDef) {
        DemoUpdateContext ctx{ *this, *dummy, *cachedDef, currentPaladinId, currentMode, GetPaladinPos(), GetDummyPos(), 0.0f };
        activeSequence->Reset(ctx);
    }
}

void DemoSandbox::SpawnEffectPopup(Vector2 pos, const std::string& text, Color color, float lifetime) {
    DemoEffectPopup popup;
    popup.text = text;
    popup.position = { pos.x + (float)GetRandomValue(-4, 4), pos.y - 22.0f };
    popup.velocity = { (float)GetRandomValue(-8, 8), -36.0f };
    popup.alpha = 1.0f;
    popup.lifetime = lifetime;
    popup.maxLifetime = lifetime;
    popup.color = color;
    effectPopups.push_back(popup);
}

void DemoSandbox::SpawnHitParticles(Vector2 pos, Color color, int count) {
    for (int i = 0; i < count; ++i) {
        DemoParticle p;
        p.position = pos;
        float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
        float speed = (float)GetRandomValue(35, 120);
        p.velocity = { cosf(angle) * speed, sinf(angle) * speed };
        p.size = (float)GetRandomValue(2, 4);
        p.lifetime = 0.30f + (float)GetRandomValue(0, 15) * 0.01f;
        p.maxLifetime = p.lifetime;
        p.alpha = 1.0f;
        p.color = color;
        particles.push_back(p);
    }
}

void DemoSandbox::SpawnProjectile(const DemoProjectile& proj) {
    projectiles.push_back(proj);
}

void DemoSandbox::Update(float deltaTime) {
    loopTimer += deltaTime;
    if (dummy) {
        dummy->SetPosition(GetDummyPos());
        dummy->Update(deltaTime);
    }

    if (activeSequence && dummy && cachedDef) {
        DemoUpdateContext ctx{ *this, *dummy, *cachedDef, currentPaladinId, currentMode, GetPaladinPos(), GetDummyPos(), loopTimer };
        activeSequence->Update(deltaTime, ctx);

        if (loopTimer >= activeSequence->GetDuration()) {
            Reset();
        }
    }

    UpdateVFX(deltaTime);
}

void DemoSandbox::UpdateVFX(float deltaTime) {
    for (auto& p : particles) {
        p.position.x += p.velocity.x * deltaTime;
        p.position.y += p.velocity.y * deltaTime;
        p.velocity.x *= 0.92f;
        p.velocity.y *= 0.92f;
        p.lifetime -= deltaTime;
        p.alpha = std::clamp(p.lifetime / p.maxLifetime, 0.0f, 1.0f);
    }
    particles.erase(
        std::remove_if(particles.begin(), particles.end(), [](const DemoParticle& p) { return p.lifetime <= 0.0f; }),
        particles.end()
    );

    for (auto& e : effectPopups) {
        e.position.x += e.velocity.x * deltaTime;
        e.position.y += e.velocity.y * deltaTime;
        e.lifetime -= deltaTime;
        e.alpha = std::clamp(e.lifetime / e.maxLifetime, 0.0f, 1.0f);
    }
    effectPopups.erase(
        std::remove_if(effectPopups.begin(), effectPopups.end(), [](const DemoEffectPopup& e) { return e.lifetime <= 0.0f; }),
        effectPopups.end()
    );
}

void DemoSandbox::Draw(Rectangle stageBounds) const {
    DrawRectangleRec(stageBounds, Color{ 18, 24, 34, 255 });
    DrawRectangleLinesEx(stageBounds, 1.5f, Color{ 45, 62, 85, 255 });

    constexpr float GRID_STEP = 24.0f;
    for (float x = stageBounds.x + GRID_STEP; x < stageBounds.x + stageBounds.width; x += GRID_STEP) {
        DrawLineV({ x, stageBounds.y }, { x, stageBounds.y + stageBounds.height }, Color{ 26, 36, 50, 160 });
    }
    for (float y = stageBounds.y + GRID_STEP; y < stageBounds.y + stageBounds.height; y += GRID_STEP) {
        DrawLineV({ stageBounds.x, y }, { stageBounds.x + stageBounds.width, y }, Color{ 26, 36, 50, 160 });
    }

    Vector2 paladinPos = GetPaladinPos();
    Vector2 dummyPos = GetDummyPos();

    DrawEllipse((int)paladinPos.x, (int)(paladinPos.y + 24.0f), 22.0f, 8.0f, Color{ 10, 14, 20, 160 });
    DrawEllipse((int)dummyPos.x, (int)(dummyPos.y + 24.0f), 22.0f, 8.0f, Color{ 10, 14, 20, 160 });
    DrawCircleLines((int)dummyPos.x, (int)(dummyPos.y + 24.0f), 20.0f, Color{ 80, 120, 160, 100 });

    // Draw Paladin Sprite
    if (cachedDef) {
        Texture2D idleTex = AssetManager::GetInstance().GetTexture(cachedDef->idleTextureKey);
        if (idleTex.id != 0) {
            float frameW = (float)idleTex.width / 4.0f;
            float frameH = (float)idleTex.height;
            int frame = ((int)(loopTimer / 0.12f)) % 4;
            DrawTexturePro(idleTex, { frame * frameW, 0.0f, frameW, frameH }, { paladinPos.x, paladinPos.y, frameW * ENTITY_SCALE, frameH * ENTITY_SCALE }, { frameW * ENTITY_SCALE * 0.5f, frameH * ENTITY_SCALE * 0.5f }, 0.0f, WHITE);
        }
    }

    // Draw Training Dummy (Const-correct)
    if (dummy) {
        dummy->Draw();
    }

    // Delegate weapon & VFX rendering polymorphically (Const-correct, zero const_cast)
    if (activeSequence && dummy && cachedDef) {
        DemoDrawContext ctx{ *this, *dummy, *cachedDef, currentPaladinId, currentMode, paladinPos, dummyPos, loopTimer };
        activeSequence->DrawWeapon(ctx);
        activeSequence->DrawVFX(ctx);
    }

    // Draw Projectiles
    for (const auto& proj : projectiles) {
        if (!proj.active) continue;
        Texture2D bullet = AssetManager::GetInstance().GetTexture(proj.textureKey);
        if (bullet.id != 0) {
            DrawTexturePro(bullet, { 0, 0, (float)bullet.width, (float)bullet.height }, { proj.position.x, proj.position.y, (float)bullet.width * 1.4f, (float)bullet.height * 1.4f }, { (float)bullet.width * 0.7f, (float)bullet.height * 0.7f }, proj.rotation, WHITE);
        }
    }

    // Draw Particles
    for (const auto& p : particles) {
        DrawRectangle((int)p.position.x, (int)p.position.y, (int)p.size, (int)p.size, ColorAlpha(p.color, p.alpha));
    }

    // Draw Floating Combat Effect Badges (Top layer)
    for (const auto& e : effectPopups) {
        UIUtils::DrawCenteredText("PixeloidBold", e.text, { e.position.x + 1.0f, e.position.y + 1.0f }, UIUtils::FontSize::SMALL, ColorAlpha(BLACK, e.alpha));
        UIUtils::DrawCenteredText("PixeloidBold", e.text, e.position, UIUtils::FontSize::SMALL, ColorAlpha(e.color, e.alpha));
    }

    std::string badgeText = currentMode == DemoPreviewMode::BasicAttack ? "PREVIEW: BASIC ATTACK"
                           : (currentMode == DemoPreviewMode::Skill ? "PREVIEW: SKILL" : "PREVIEW: ULTIMATE");
    UIUtils::DrawText("PixeloidBold", badgeText, { stageBounds.x + 12.0f, stageBounds.y + 12.0f }, static_cast<UIUtils::FontSize>(11), Color{ 120, 190, 255, 220 });
}
