#include "UI/AbilityDemoModal.h"
#include "UI/UIUtils.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Manager/AudioManager.h"
#include "Entities/Player/Paladin.h"
#include "raymath.h"
#include <cmath>
#include <algorithm>
#include <sstream>

namespace {
constexpr Rectangle CONTAINER = { 8.0f, 8.0f, 667.0f, 496.0f };
constexpr Rectangle STAGE_PANEL = { 20.0f, 46.0f, 432.0f, 408.0f };
constexpr Rectangle CARDS_PANEL = { 460.0f, 46.0f, 207.0f, 408.0f };
constexpr Rectangle BACK_BUTTON = { 575.0f, 462.0f, 88.0f, 28.0f };

constexpr float CARD_HEIGHT = 126.0f;
constexpr float CARD_GAP = 12.0f;

Rectangle GetCardBounds(int index) {
    return {
        CARDS_PANEL.x,
        CARDS_PANEL.y + index * (CARD_HEIGHT + CARD_GAP),
        CARDS_PANEL.width,
        CARD_HEIGHT
    };
}

void DrawWrappedDemoText(
    const std::string& text,
    Rectangle bounds,
    int fontSize,
    Color color
) {
    std::istringstream stream(text);
    std::string word;
    std::string line;
    float y = bounds.y;
    while (stream >> word && y + fontSize <= bounds.y + bounds.height) {
        std::string candidate = line.empty() ? word : line + " " + word;
        if (!line.empty() && UIUtils::MeasureText("PixeloidSans", candidate, static_cast<UIUtils::FontSize>(fontSize)).x > bounds.width) {
            UIUtils::DrawText("PixeloidSans", line, { bounds.x, y }, static_cast<UIUtils::FontSize>(fontSize), color);
            line = word;
            y += fontSize + 2.0f;
        } else {
            line = candidate;
        }
    }
    if (!line.empty() && y + fontSize <= bounds.y + bounds.height) {
        UIUtils::DrawText("PixeloidSans", line, { bounds.x, y }, static_cast<UIUtils::FontSize>(fontSize), color);
    }
}
}

AbilityDemoModal::AbilityDemoModal()
    : open(false),
      currentPaladin(PaladinId::Lance),
      currentMode(DemoPreviewMode::BasicAttack),
      loopTimer(0.0f),
      hitFlashTimer(0.0f),
      paladinFrame(0),
      paladinFrameTimer(0.0f),
      paladinOffsetX(0.0f),
      weaponRecoilX(0.0f),
      dummyShakeX(0.0f),
      keithSwingAngle(0.0f),
      attack1Triggered(false),
      attack2Triggered(false),
      impactVFXTriggered(false) {
}

AbilityDemoModal::~AbilityDemoModal() {
}

void AbilityDemoModal::Open(PaladinId paladinId) {
    open = true;
    currentPaladin = paladinId;
    currentMode = DemoPreviewMode::BasicAttack;
    ResetSimulation();
}

void AbilityDemoModal::Close() {
    open = false;
    ResetSimulation();
}

void AbilityDemoModal::ResetSimulation() {
    loopTimer = 0.0f;
    hitFlashTimer = 0.0f;
    paladinFrame = 0;
    paladinFrameTimer = 0.0f;
    paladinOffsetX = 0.0f;
    weaponRecoilX = 0.0f;
    dummyShakeX = 0.0f;
    keithSwingAngle = 0.0f;
    attack1Triggered = false;
    attack2Triggered = false;
    impactVFXTriggered = false;

    damagePopups.clear();
    particles.clear();
    projectiles.clear();
}

Vector2 AbilityDemoModal::GetPaladinPosition(Rectangle stageBounds) const {
    float baseX = (currentPaladin == PaladinId::Keith) ? (stageBounds.x + 115.0f) : (stageBounds.x + 85.0f);
    return { baseX + paladinOffsetX, stageBounds.y + 225.0f };
}

Vector2 AbilityDemoModal::GetDummyPosition(Rectangle stageBounds) const {
    float baseX = (currentPaladin == PaladinId::Keith) ? (stageBounds.x + 195.0f) : (stageBounds.x + 325.0f);
    float shake = (dummyShakeX > 0.0f) ? (float)GetRandomValue(-2, 2) : 0.0f;
    return { baseX + shake, stageBounds.y + 225.0f };
}

Vector2 AbilityDemoModal::GetWeaponPivot(Vector2 paladinPos) const {
    // Matches Paladin::GetWeaponPivot() for right-facing character in runtime code
    if (currentPaladin == PaladinId::Keith) {
        return { paladinPos.x - 4.0f, paladinPos.y + 6.0f };
    } else if (currentPaladin == PaladinId::Lance) {
        return { paladinPos.x - 4.0f + weaponRecoilX, paladinPos.y + 6.0f };
    } else if (currentPaladin == PaladinId::Hunk) {
        return { paladinPos.x - 4.0f + weaponRecoilX, paladinPos.y + 6.0f };
    } else if (currentPaladin == PaladinId::Pidge) {
        return { paladinPos.x - 4.0f, paladinPos.y + 6.0f };
    }
    return paladinPos;
}

Vector2 AbilityDemoModal::GetMuzzlePosition(Vector2 paladinPos) const {
    Vector2 pivot = GetWeaponPivot(paladinPos);
    const PaladinDefinition& def = PaladinCatalog::Get(currentPaladin);
    Texture2D weaponTex = AssetManager::GetInstance().GetTexture(def.weapon.textureKey);
    float barrelLength = (weaponTex.id != 0) ? ((float)weaponTex.width * 2.0f) : 24.0f;
    return { pivot.x + barrelLength, pivot.y };
}

void AbilityDemoModal::SpawnDamagePopup(Vector2 pos, const std::string& text, Color color, float lifetime) {
    DemoDamagePopup popup;
    popup.text = text;
    popup.position = { pos.x + (float)GetRandomValue(-6, 6), pos.y - 30.0f };
    popup.velocity = { (float)GetRandomValue(-12, 12), -48.0f };
    popup.alpha = 1.0f;
    popup.lifetime = lifetime;
    popup.maxLifetime = lifetime;
    popup.color = color;
    damagePopups.push_back(popup);
}

void AbilityDemoModal::SpawnHitParticles(Vector2 pos, Color color, int count) {
    for (int i = 0; i < count; ++i) {
        DemoParticle p;
        p.position = pos;
        float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
        float speed = (float)GetRandomValue(40, 140);
        p.velocity = { cosf(angle) * speed, sinf(angle) * speed };
        p.size = (float)GetRandomValue(2, 5);
        p.lifetime = 0.35f + (float)GetRandomValue(0, 20) * 0.01f;
        p.maxLifetime = p.lifetime;
        p.alpha = 1.0f;
        p.color = color;
        particles.push_back(p);
    }
}

void AbilityDemoModal::Update(float deltaTime, Vector2 mousePosition) {
    if (!open) return;

    if (IsKeyPressed(KEY_ESCAPE)) {
        AudioManager::GetInstance().PlayRandomClick();
        Close();
        return;
    }

    // Handle Card Selection
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (CheckCollisionPointRec(mousePosition, BACK_BUTTON)) {
            AudioManager::GetInstance().PlayRandomClick();
            Close();
            return;
        }

        for (int i = 0; i < 3; ++i) {
            Rectangle cardBounds = GetCardBounds(i);
            if (CheckCollisionPointRec(mousePosition, cardBounds)) {
                DemoPreviewMode selected = static_cast<DemoPreviewMode>(i);
                if (currentMode != selected) {
                    AudioManager::GetInstance().PlayRandomClick();
                    currentMode = selected;
                    ResetSimulation();
                }
                return;
            }
        }
    }

    // Update the active preview mode
    if (currentMode == DemoPreviewMode::BasicAttack) {
        UpdateBasicAttackDemo(deltaTime);
    } else {
        loopTimer += deltaTime;
        if (loopTimer >= 2.0f) {
            ResetSimulation();
        }
    }

    UpdateParticles(deltaTime);
    UpdateDamagePopups(deltaTime);
}

void AbilityDemoModal::UpdateBasicAttackDemo(float deltaTime) {
    loopTimer += deltaTime;

    // Paladin animation frame cycling (10 FPS idle loop)
    paladinFrameTimer += deltaTime;
    if (paladinFrameTimer >= 0.12f) {
        paladinFrame = (paladinFrame + 1) % 4;
        paladinFrameTimer = 0.0f;
    }

    if (hitFlashTimer > 0.0f) {
        hitFlashTimer -= deltaTime;
        if (hitFlashTimer < 0.0f) hitFlashTimer = 0.0f;
    }

    if (dummyShakeX > 0.0f) {
        dummyShakeX = std::max(0.0f, dummyShakeX - deltaTime * 20.0f);
    }

    const PaladinDefinition& def = PaladinCatalog::Get(currentPaladin);
    Vector2 paladinBasePos = GetPaladinPosition(STAGE_PANEL);
    Vector2 dummyPos = GetDummyPosition(STAGE_PANEL);

    // --- Automated Loop Stages Verified Against Concrete Paladin Classes ---
    if (currentPaladin == PaladinId::Keith) {
        // Keith: MeleeAttackStrategy Combo 1 (downward arc) -> Combo 2 (upward arc)
        if (loopTimer >= 0.45f && loopTimer < 0.65f) {
            float t = (loopTimer - 0.45f) / 0.20f;
            float easedT = (1.0f - cosf(t * PI)) / 2.0f;
            keithSwingAngle = -60.0f + 120.0f * easedT;
            paladinOffsetX = sinf(t * PI) * 22.0f;

            if (t >= 0.50f && !attack1Triggered) {
                attack1Triggered = true;
                hitFlashTimer = 0.22f;
                dummyShakeX = 5.0f;
                AudioManager::GetInstance().PlayRandomSwordSlash();
                int dmg1 = static_cast<int>(BaseStats::Damage * def.weapon.minDamageScalar);
                SpawnDamagePopup(dummyPos, std::to_string(dmg1), Color{ 255, 75, 75, 255 });
                SpawnHitParticles(dummyPos, Color{ 255, 80, 80, 255 }, 8);
            }
        } else if (loopTimer >= 0.65f && loopTimer < 0.75f) {
            keithSwingAngle = 0.0f;
            paladinOffsetX = 0.0f;
        } else if (loopTimer >= 0.75f && loopTimer < 0.95f) {
            float t = (loopTimer - 0.75f) / 0.20f;
            float easedT = (1.0f - cosf(t * PI)) / 2.0f;
            keithSwingAngle = 60.0f - 120.0f * easedT;
            paladinOffsetX = sinf(t * PI) * 26.0f;

            if (t >= 0.50f && !attack2Triggered) {
                attack2Triggered = true;
                hitFlashTimer = 0.30f;
                dummyShakeX = 8.0f;
                AudioManager::GetInstance().PlayRandomSwordSlash();
                int dmg2 = static_cast<int>(BaseStats::Damage * def.weapon.maxDamageScalar);
                SpawnDamagePopup(dummyPos, "CRIT " + std::to_string(dmg2), Color{ 255, 215, 0, 255 }, 1.0f);
                SpawnHitParticles(dummyPos, Color{ 255, 180, 40, 255 }, 14);
            }
        } else if (loopTimer >= 0.95f) {
            keithSwingAngle = 0.0f;
            paladinOffsetX = 0.0f;
        }
    } else if (currentPaladin == PaladinId::Lance) {
        // Lance: RangedAttackStrategy logic with recoil kickback & linear projectile
        if (loopTimer >= 0.45f && !attack1Triggered) {
            attack1Triggered = true;
            weaponRecoilX = -15.0f; // matches Lance recoil = 15.0f
            AudioManager::GetInstance().PlayRandomLaserGun();

            Vector2 muzzlePos = GetMuzzlePosition(paladinBasePos);
            DemoProjectile proj;
            proj.startPos = muzzlePos;
            proj.targetPos = dummyPos;
            proj.position = proj.startPos;
            proj.progress = 0.0f;
            proj.speed = 5.0f; // rapid blaster bolt
            proj.active = true;
            proj.textureKey = "Lance_Bullet";
            proj.rotation = 0.0f;
            projectiles.push_back(proj);
        }

        // Exponential decay matching WeaponKinematics
        if (weaponRecoilX < 0.0f) {
            weaponRecoilX = std::min(0.0f, weaponRecoilX + deltaTime * 35.0f);
        }

        // Projectile trajectory & impact
        for (auto& proj : projectiles) {
            if (!proj.active) continue;
            proj.progress += deltaTime * proj.speed;
            proj.position = Vector2Lerp(proj.startPos, proj.targetPos, proj.progress);
            if (proj.progress >= 1.0f && !impactVFXTriggered) {
                proj.active = false;
                impactVFXTriggered = true;
                hitFlashTimer = 0.25f;
                dummyShakeX = 4.0f;
                int dmg = static_cast<int>(BaseStats::Damage * def.weapon.maxDamageScalar);
                SpawnDamagePopup(dummyPos, std::to_string(dmg), Color{ 80, 200, 255, 255 });
                SpawnHitParticles(dummyPos, Color{ 70, 190, 255, 255 }, 8);
            }
        }
    } else if (currentPaladin == PaladinId::Hunk) {
        // Hunk: LaserAttackStrategy continuous piercing beam logic (maxLaserTime = 0.15s)
        if (loopTimer >= 0.45f && !attack1Triggered) {
            attack1Triggered = true;
            weaponRecoilX = -30.0f; // matches Hunk recoil = 30.0f
            AudioManager::GetInstance().PlayRandomLaserGun();
            hitFlashTimer = 0.35f;
            dummyShakeX = 8.0f;
            impactVFXTriggered = true;
            int dmg = static_cast<int>(BaseStats::Damage * def.weapon.maxDamageScalar);
            SpawnDamagePopup(dummyPos, std::to_string(dmg), Color{ 255, 180, 40, 255 });
            SpawnHitParticles(dummyPos, Color{ 255, 160, 20, 255 }, 14);
        }

        if (weaponRecoilX < 0.0f) {
            weaponRecoilX = std::min(0.0f, weaponRecoilX + deltaTime * 28.0f);
        }
    } else if (currentPaladin == PaladinId::Pidge) {
        // Pidge: Grappling Katar throw, flying blade, and tether line (Pidge.cpp)
        if (loopTimer >= 0.45f && !attack1Triggered) {
            attack1Triggered = true;
            AudioManager::GetInstance().PlayRandomLaser();
        }

        // Impact triggered upon reaching full extension at dummy
        if (loopTimer >= 0.70f && !impactVFXTriggered) {
            impactVFXTriggered = true;
            hitFlashTimer = 0.25f;
            dummyShakeX = 5.0f;
            int dmg = static_cast<int>(BaseStats::Damage * def.weapon.maxDamageScalar);
            SpawnDamagePopup(dummyPos, std::to_string(dmg), Color{ 110, 255, 140, 255 });
            SpawnHitParticles(dummyPos, Color{ 100, 255, 130, 255 }, 8);
        }
    }

    if (loopTimer >= 2.0f) {
        ResetSimulation();
    }
}

void AbilityDemoModal::UpdateParticles(float deltaTime) {
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
}

void AbilityDemoModal::UpdateDamagePopups(float deltaTime) {
    for (auto& d : damagePopups) {
        d.position.x += d.velocity.x * deltaTime;
        d.position.y += d.velocity.y * deltaTime;
        d.velocity.y += 35.0f * deltaTime;
        d.lifetime -= deltaTime;
        d.alpha = std::clamp(d.lifetime / d.maxLifetime, 0.0f, 1.0f);
    }
    damagePopups.erase(
        std::remove_if(damagePopups.begin(), damagePopups.end(), [](const DemoDamagePopup& d) { return d.lifetime <= 0.0f; }),
        damagePopups.end()
    );
}

void AbilityDemoModal::Draw(Vector2 mousePosition) const {
    if (!open) return;

    // 1. Semi-transparent dark backdrop overlay
    DrawRectangle(-1000, -1000, 3000, 3000, ColorAlpha(BLACK, 0.75f));

    // 2. Main modal container
    UIUtils::DrawPanel(CONTAINER, Color{ 14, 18, 26, 252 });
    
    // Header Title
    const PaladinDefinition& def = PaladinCatalog::Get(currentPaladin);
    std::string title = "ABILITY DEMO — " + def.name;
    UIUtils::DrawCenteredText("PixeloidBold", title, { CONTAINER.x + CONTAINER.width * 0.5f, CONTAINER.y + 16.0f }, UIUtils::FontSize::SMALL, GOLD);

    // 3. Left Showcase Arena
    DrawArenaStage(STAGE_PANEL);

    // 4. Right Action Cards
    const_cast<AbilityDemoModal*>(this)->DrawRightActionCards(CARDS_PANEL, mousePosition);

    // 5. Back Button
    bool hovered = CheckCollisionPointRec(mousePosition, BACK_BUTTON);
    Color btnColor = hovered ? Color{ 70, 92, 122, 255 } : Color{ 44, 58, 78, 255 };
    UIUtils::DrawPanel(BACK_BUTTON, btnColor);
    UIUtils::DrawCenteredText("PixeloidSans", "BACK", { BACK_BUTTON.x + BACK_BUTTON.width * 0.5f, BACK_BUTTON.y + BACK_BUTTON.height * 0.5f }, static_cast<UIUtils::FontSize>(13), RAYWHITE);
}

void AbilityDemoModal::DrawArenaStage(Rectangle bounds) const {
    // Stage background & sci-fi training grid
    DrawRectangleRec(bounds, Color{ 18, 24, 34, 255 });
    DrawRectangleLinesEx(bounds, 1.5f, Color{ 45, 62, 85, 255 });

    // Grid pattern
    constexpr float GRID_STEP = 24.0f;
    for (float x = bounds.x + GRID_STEP; x < bounds.x + bounds.width; x += GRID_STEP) {
        DrawLineV({ x, bounds.y }, { x, bounds.y + bounds.height }, Color{ 26, 36, 50, 160 });
    }
    for (float y = bounds.y + GRID_STEP; y < bounds.y + bounds.height; y += GRID_STEP) {
        DrawLineV({ bounds.x, y }, { bounds.x + bounds.width, y }, Color{ 26, 36, 50, 160 });
    }

    // Dynamic Positions
    Vector2 paladinPos = GetPaladinPosition(bounds);
    Vector2 dummyPos = GetDummyPosition(bounds);

    // Floor Target Platters
    DrawEllipse((int)paladinPos.x, (int)(paladinPos.y + 36.0f), 32.0f, 12.0f, Color{ 10, 14, 20, 160 });
    DrawEllipse((int)dummyPos.x, (int)(dummyPos.y + 36.0f), 32.0f, 12.0f, Color{ 10, 14, 20, 160 });
    DrawCircleLines((int)dummyPos.x, (int)(dummyPos.y + 36.0f), 28.0f, Color{ 80, 120, 160, 100 });

    // 1. Draw Paladin Sprite
    DrawPaladinSprite(paladinPos);

    // 2. Draw Paladin Weapon
    DrawPaladinWeapon(paladinPos);

    // 3. Draw Training Dummy
    DrawTrainingDummy(dummyPos);

    // 4. Draw Visual FX, Projectiles, Slash Arcs, and Laser Beams
    DrawBasicAttackVFX(paladinPos, dummyPos);

    // 5. Draw Particles
    for (const auto& p : particles) {
        DrawRectangle((int)p.position.x, (int)p.position.y, (int)p.size, (int)p.size, ColorAlpha(p.color, p.alpha));
    }

    // 6. Draw Floating Damage Numbers on TOP LAYER
    for (const auto& d : damagePopups) {
        UIUtils::DrawCenteredText("PixeloidBold", d.text, { d.position.x + 1.0f, d.position.y + 1.0f }, UIUtils::FontSize::SMALL, ColorAlpha(BLACK, d.alpha));
        UIUtils::DrawCenteredText("PixeloidBold", d.text, d.position, UIUtils::FontSize::SMALL, ColorAlpha(d.color, d.alpha));
    }

    // Showcase mode badge on stage
    std::string badgeText = currentMode == DemoPreviewMode::BasicAttack ? "PREVIEW: BASIC ATTACK"
                           : (currentMode == DemoPreviewMode::Skill ? "PREVIEW: SKILL" : "PREVIEW: ULTIMATE");
    UIUtils::DrawText("PixeloidBold", badgeText, { bounds.x + 12.0f, bounds.y + 12.0f }, static_cast<UIUtils::FontSize>(12), Color{ 120, 190, 255, 220 });
}

void AbilityDemoModal::DrawPaladinSprite(Vector2 centerPos) const {
    const PaladinDefinition& def = PaladinCatalog::Get(currentPaladin);
    Texture2D idleTex = AssetManager::GetInstance().GetTexture(def.idleTextureKey);

    if (idleTex.id != 0) {
        float frameW = (float)idleTex.width / 4.0f;
        float frameH = (float)idleTex.height;
        float scale = 2.4f;

        Rectangle src = { paladinFrame * frameW, 0.0f, frameW, frameH };
        Rectangle dest = { centerPos.x, centerPos.y, frameW * scale, frameH * scale };
        Vector2 origin = { dest.width * 0.5f, dest.height * 0.5f };

        DrawTexturePro(idleTex, src, dest, origin, 0.0f, WHITE);
    }
}

void AbilityDemoModal::DrawPaladinWeapon(Vector2 paladinPos) const {
    const PaladinDefinition& def = PaladinCatalog::Get(currentPaladin);
    Texture2D weaponTex = AssetManager::GetInstance().GetTexture(def.weapon.textureKey);
    if (weaponTex.id == 0) return;

    Vector2 pivot = GetWeaponPivot(paladinPos);
    float scale = 2.0f;

    if (currentPaladin == PaladinId::Keith) {
        // Red Bayard sword: pivot at hilt, rotates with keithSwingAngle (MeleeAttackStrategy)
        Rectangle src = { 0.0f, 0.0f, (float)weaponTex.width, (float)weaponTex.height };
        Rectangle dest = { pivot.x, pivot.y, (float)weaponTex.width * scale, (float)weaponTex.height * scale };
        Vector2 origin = { 0.0f, (float)weaponTex.height * scale * 0.5f }; // hilt pivot
        DrawTexturePro(weaponTex, src, dest, origin, keithSwingAngle, WHITE);
    } else if (currentPaladin == PaladinId::Lance || currentPaladin == PaladinId::Hunk) {
        // Blaster / Cannon held pointing right with recoil offset
        Rectangle src = { 0.0f, 0.0f, (float)weaponTex.width, (float)weaponTex.height };
        Rectangle dest = { pivot.x, pivot.y, (float)weaponTex.width * scale, (float)weaponTex.height * scale };
        Vector2 origin = { 0.0f, (float)weaponTex.height * scale * 0.5f };
        DrawTexturePro(weaponTex, src, dest, origin, 0.0f, WHITE);
    } else if (currentPaladin == PaladinId::Pidge) {
        // Katar held in hand only when NOT in flight (Pidge.cpp)
        if (loopTimer < 0.45f || loopTimer >= 0.95f) {
            Rectangle src = { 0.0f, 0.0f, (float)weaponTex.width, (float)weaponTex.height };
            Rectangle dest = { pivot.x, pivot.y, (float)weaponTex.width * scale, (float)weaponTex.height * scale };
            Vector2 origin = { 0.0f, (float)weaponTex.height * scale * 0.5f };
            DrawTexturePro(weaponTex, src, dest, origin, 0.0f, WHITE);
        }
    }
}

void AbilityDemoModal::DrawTrainingDummy(Vector2 centerPos) const {
    Texture2D dummyTex = AssetManager::GetInstance().GetTexture("Knight_Idle");
    Color dummyColor = hitFlashTimer > 0.0f ? Color{ 255, 120, 120, 255 } : WHITE;

    if (dummyTex.id != 0) {
        float frameW = 32.0f;
        float frameH = (float)dummyTex.height;
        float scale = 2.4f;

        // Facing left towards Paladin
        Rectangle src = { 0.0f, 0.0f, -frameW, frameH };
        Rectangle dest = { centerPos.x, centerPos.y, frameW * scale, frameH * scale };
        Vector2 origin = { dest.width * 0.5f, dest.height * 0.5f };

        DrawTexturePro(dummyTex, src, dest, origin, 0.0f, dummyColor);
    } else {
        DrawRectangle((int)centerPos.x - 16, (int)centerPos.y - 32, 32, 64, dummyColor);
    }

    // Dummy "TRAINING DUMMY" Tag
    UIUtils::DrawCenteredText("PixeloidSans", "TRAINING DUMMY", { centerPos.x, centerPos.y - 50.0f }, static_cast<UIUtils::FontSize>(10), Color{ 170, 185, 205, 220 });
}

void AbilityDemoModal::DrawBasicAttackVFX(Vector2 paladinPos, Vector2 dummyPos) const {
    Vector2 pivot = GetWeaponPivot(paladinPos);
    Vector2 muzzlePos = GetMuzzlePosition(paladinPos);

    if (currentPaladin == PaladinId::Keith) {
        // Keith: MeleeAttackStrategy 3-frame animated blade slash VFX
        Texture2D swordTex = AssetManager::GetInstance().GetTexture("Keith_Weapon");
        Texture2D slashTex = AssetManager::GetInstance().GetTexture("Sword_Slash_Small");
        float scale = 2.0f;

        if (slashTex.id != 0) {
            float frameWidth = (float)slashTex.width / 3.0f; // 3-frame animation
            float frameHeight = (float)slashTex.height;

            if (loopTimer >= 0.45f && loopTimer < 0.65f) {
                // Combo 1: Downward slash arc at the tip of the blade
                float t = (loopTimer - 0.45f) / 0.20f;
                if (t >= 0.15f && t <= 0.90f) {
                    int slashFrame = std::clamp((int)((t - 0.15f) / 0.75f * 3.0f), 0, 2);
                    Rectangle src = { slashFrame * frameWidth, 0.0f, frameWidth, frameHeight };
                    float distanceOut = (float)swordTex.width * scale * 0.85f;
                    float rad = keithSwingAngle * DEG2RAD;
                    Vector2 slashPos = { pivot.x + cosf(rad) * distanceOut, pivot.y + sinf(rad) * distanceOut };
                    Rectangle dest = { slashPos.x, slashPos.y, frameWidth * scale * 1.2f, frameHeight * scale * 1.2f };
                    Vector2 origin = { (dest.width) * 0.5f, (dest.height) * 0.5f };

                    BeginBlendMode(BLEND_ADDITIVE);
                    DrawTexturePro(slashTex, src, dest, origin, keithSwingAngle, WHITE);
                    EndBlendMode();
                }
            } else if (loopTimer >= 0.75f && loopTimer < 0.95f) {
                // Combo 2: Upward slash arc (inverted source height matching MeleeAttackStrategy.cpp)
                float t = (loopTimer - 0.75f) / 0.20f;
                if (t >= 0.15f && t <= 0.90f) {
                    int slashFrame = std::clamp((int)((t - 0.15f) / 0.75f * 3.0f), 0, 2);
                    Rectangle src = { slashFrame * frameWidth, 0.0f, frameWidth, -frameHeight };
                    float distanceOut = (float)swordTex.width * scale * 0.85f;
                    float rad = keithSwingAngle * DEG2RAD;
                    Vector2 slashPos = { pivot.x + cosf(rad) * distanceOut, pivot.y + sinf(rad) * distanceOut };
                    Rectangle dest = { slashPos.x, slashPos.y, frameWidth * scale * 1.3f, frameHeight * scale * 1.3f };
                    Vector2 origin = { (dest.width) * 0.5f, (dest.height) * 0.5f };

                    BeginBlendMode(BLEND_ADDITIVE);
                    DrawTexturePro(slashTex, src, dest, origin, keithSwingAngle, Color{ 255, 210, 100, 255 });
                    EndBlendMode();
                }
            }
        }
    } else if (currentPaladin == PaladinId::Lance) {
        // Lance: Muzzle flash at barrel tip (RangedAttackStrategy.cpp)
        if (loopTimer >= 0.45f && loopTimer < 0.51f) {
            Texture2D muzzle = AssetManager::GetInstance().GetTexture("Lance_Muzzle");
            if (muzzle.id != 0) {
                Rectangle dest = { muzzlePos.x, muzzlePos.y, (float)muzzle.width * 2.0f, (float)muzzle.height * 2.0f };
                Vector2 origin = { (float)muzzle.width, (float)muzzle.height };
                DrawTexturePro(muzzle, { 0.0f, 0.0f, (float)muzzle.width, (float)muzzle.height }, dest, origin, 0.0f, WHITE);
            }
        }
        // Flying Projectile
        for (const auto& proj : projectiles) {
            if (!proj.active) continue;
            Texture2D bullet = AssetManager::GetInstance().GetTexture(proj.textureKey);
            if (bullet.id != 0) {
                Rectangle dest = { proj.position.x, proj.position.y, (float)bullet.width * 2.0f, (float)bullet.height * 2.0f };
                Vector2 origin = { (float)bullet.width, (float)bullet.height };
                DrawTexturePro(bullet, { 0.0f, 0.0f, (float)bullet.width, (float)bullet.height }, dest, origin, 0.0f, WHITE);
            }
        }
        // Impact burst on dummy
        if (impactVFXTriggered && loopTimer < 0.70f) {
            Texture2D impact = AssetManager::GetInstance().GetTexture("Lance_Impact");
            if (impact.id != 0) {
                Rectangle dest = { dummyPos.x, dummyPos.y, (float)impact.width * 2.0f, (float)impact.height * 2.0f };
                Vector2 origin = { (float)impact.width, (float)impact.height };
                DrawTexturePro(impact, { 0.0f, 0.0f, (float)impact.width, (float)impact.height }, dest, origin, 0.0f, WHITE);
            }
        }
    } else if (currentPaladin == PaladinId::Hunk) {
        // Hunk: LaserAttackStrategy 2-frame continuous piercing laser beam (maxLaserTime = 0.15s)
        if (loopTimer >= 0.45f && loopTimer < 0.60f) {
            float progress = (loopTimer - 0.45f) / 0.15f;
            int frame = std::clamp((int)(progress * 2.0f), 0, 1);

            // Muzzle flash at dynamic cannon tip (2 frames)
            Texture2D muzzle = AssetManager::GetInstance().GetTexture("Hunk_Muzzle");
            if (muzzle.id != 0) {
                float frameW = (float)muzzle.width / 2.0f;
                Rectangle mzSource = { frame * frameW, 0.0f, frameW, (float)muzzle.height };
                Rectangle mzDest = { muzzlePos.x, muzzlePos.y, frameW * 2.0f, (float)muzzle.height * 2.0f };
                Vector2 mzOrigin = { frameW, (float)muzzle.height };
                DrawTexturePro(muzzle, mzSource, mzDest, mzOrigin, 0.0f, WHITE);
            }

            // Continuous beam texture stretched horizontally across dummy (2 frames)
            Texture2D beam = AssetManager::GetInstance().GetTexture("Hunk_Bullet");
            if (beam.id != 0) {
                float frameW = (float)beam.width / 2.0f;
                float beamDist = (dummyPos.x + 50.0f) - muzzlePos.x;
                Rectangle bSrc = { frame * frameW, 0.0f, frameW, (float)beam.height };
                Rectangle bDest = { muzzlePos.x, muzzlePos.y, beamDist, (float)beam.height * 2.0f };
                Vector2 bOrigin = { 0.0f, (float)beam.height };
                DrawTexturePro(beam, bSrc, bDest, bOrigin, 0.0f, WHITE);
            }

            // Heavy impact explosion on dummy (2 frames)
            Texture2D impact = AssetManager::GetInstance().GetTexture("Hunk_Impact");
            if (impact.id != 0) {
                float frameW = (float)impact.width / 2.0f;
                Rectangle imSource = { frame * frameW, 0.0f, frameW, (float)impact.height };
                Rectangle imDest = { dummyPos.x, dummyPos.y, frameW * 2.0f, (float)impact.height * 2.0f };
                Vector2 imOrigin = { frameW, (float)impact.height };
                DrawTexturePro(impact, imSource, imDest, imOrigin, 0.0f, WHITE);
            }
        }
    } else if (currentPaladin == PaladinId::Pidge) {
        // Pidge: Tethered Grappling Katar calculation (Pidge.cpp)
        Vector2 katarOrigin = pivot;
        Vector2 katarPos = katarOrigin;

        if (loopTimer >= 0.45f && loopTimer < 0.70f) {
            // Extension to dummy
            float t = (loopTimer - 0.45f) / 0.25f;
            katarPos = Vector2Lerp(katarOrigin, dummyPos, std::clamp(t, 0.0f, 1.0f));
        } else if (loopTimer >= 0.70f && loopTimer < 0.95f) {
            // Retraction to hand
            float t = (loopTimer - 0.70f) / 0.25f;
            katarPos = Vector2Lerp(dummyPos, katarOrigin, std::clamp(t, 0.0f, 1.0f));
        }

        if (loopTimer >= 0.45f && loopTimer < 0.95f) {
            // Electric cable tether line (matching Pidge.cpp line 242)
            DrawLineEx(katarOrigin, katarPos, 2.0f, GREEN);

            // Flying Katar Blade Head without rotation (pointing forward)
            Texture2D katar = AssetManager::GetInstance().GetTexture("Paladin_Pidge_Weapon");
            if (katar.id != 0) {
                Rectangle src = { 0.0f, 0.0f, (float)katar.width, (float)katar.height };
                Rectangle dest = { katarPos.x, katarPos.y, (float)katar.width * 2.0f, (float)katar.height * 2.0f };
                Vector2 origin = { (float)katar.width, (float)katar.height };
                DrawTexturePro(katar, src, dest, origin, 0.0f, WHITE);
            }
        }
    }
}

void AbilityDemoModal::DrawRightActionCards(Rectangle panelBounds, Vector2 mousePosition) {
    const PaladinDefinition& def = PaladinCatalog::Get(currentPaladin);

    struct CardInfo {
        std::string tag;
        std::string title;
        std::string desc;
        Color tagColor;
    };

    std::string basicDesc;
    std::string skillTitle, skillDesc;
    std::string ultTitle, ultDesc;

    if (currentPaladin == PaladinId::Keith) {
        basicDesc = "Slices enemies in a sweeping 2-hit melee combo with bonus critical strike damage on the second swing.";
        skillTitle = "Fire Circle";
        skillDesc = "Ignites a rotating fiery aura for 5s that continuously burns nearby enemies and generates bonus EX energy.";
        ultTitle = "Excalibur";
        ultDesc = "Charges and unleashes a massive flaming energy wave across 500px, leaving a lingering fire trail that incinerates foes.";
    } else if (currentPaladin == PaladinId::Lance) {
        basicDesc = "Fires rapid, high-velocity laser rifle bolts with steady recoil kickback and pinpoint forward accuracy.";
        skillTitle = "Dual Wield";
        skillDesc = "Equips dual Blue Bayards for 5s, doubling fire rate (halving cooldown) to unleash synchronized double volleys.";
        ultTitle = "Glacier Pierce";
        ultDesc = "Detonates cryogenic explosions under all enemies on the battlefield, freezing every hostile solid for 5s.";
    } else if (currentPaladin == PaladinId::Hunk) {
        basicDesc = "Fires a continuous piercing laser beam that tears through multiple lined-up targets with heavy recoil.";
        skillTitle = "Earthshatter";
        skillDesc = "Slams the ground to emit a seismic shockwave, knocking back surrounding enemies and stunning (dizzying) them for 2s.";
        ultTitle = "Aegis Shield";
        ultDesc = "Deploys an impenetrable rotating Aegis barrier around the team, granting complete invulnerability for 5s.";
    } else { // Pidge
        basicDesc = "Launches a grappling katar forward on an electric cable that pierces targets and snaps back to hand with zero recoil.";
        skillTitle = "Venom Zone";
        skillDesc = "Deploys a caustic chemical field for 7s that inflicts lingering poison damage (DoT) and severely slows enemy movement.";
        ultTitle = "Rover Override";
        ultDesc = "Deploys Rover, an autonomous flying combat drone companion that follows the team and provides heavy fire support.";
    }

    std::string dmgInfo = "Damage: " + (def.weapon.minDamageScalar == def.weapon.maxDamageScalar
        ? std::to_string(static_cast<int>(BaseStats::Damage * def.weapon.maxDamageScalar))
        : std::to_string(static_cast<int>(BaseStats::Damage * def.weapon.minDamageScalar)) + "-" +
          std::to_string(static_cast<int>(BaseStats::Damage * def.weapon.maxDamageScalar)));

    CardInfo cards[3] = {
        { "BASIC ATTACK", def.weapon.name, basicDesc + " (" + dmgInfo + ")", Color{ 80, 190, 255, 255 } },
        { "SKILL (E)", skillTitle, skillDesc, Color{ 255, 180, 50, 255 } },
        { "ULTIMATE (Q)", ultTitle, ultDesc, Color{ 255, 80, 80, 255 } }
    };

    for (int i = 0; i < 3; ++i) {
        Rectangle card = GetCardBounds(i);
        bool selected = (static_cast<int>(currentMode) == i);
        bool hovered = CheckCollisionPointRec(mousePosition, card);

        Color bg = selected ? Color{ 36, 48, 68, 255 }
                 : (hovered ? Color{ 28, 38, 54, 255 } : Color{ 18, 24, 34, 255 });
        Color border = selected ? GOLD : (hovered ? Color{ 90, 120, 160, 255 } : Color{ 40, 54, 76, 255 });

        UIUtils::DrawPanel(card, bg);
        DrawRectangleLinesEx(card, selected ? 2.0f : 1.0f, border);

        // Badge Tag
        UIUtils::DrawText("PixeloidBold", cards[i].tag, { card.x + 10.0f, card.y + 10.0f }, static_cast<UIUtils::FontSize>(10), cards[i].tagColor);
        // Card Title
        UIUtils::DrawText("PixeloidBold", cards[i].title, { card.x + 10.0f, card.y + 24.0f }, static_cast<UIUtils::FontSize>(14), RAYWHITE);
        // Card Description
        DrawWrappedDemoText(cards[i].desc, { card.x + 10.0f, card.y + 44.0f, card.width - 20.0f, 72.0f }, 10, Color{ 190, 205, 225, 255 });
    }
}
