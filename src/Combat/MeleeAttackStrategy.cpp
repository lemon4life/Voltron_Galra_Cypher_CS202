#include "Combat/MeleeAttackStrategy.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/AudioManager.h"
#include "Entities/Enemy.h"
#include "Entities/Player/Paladin.h"
#include "Core/Manager/ParticleManager.h"
#include "Core/Utils/LineOfSightGeometry.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include "Core/Constants.h"

namespace {
    constexpr float MELEE_KNOCKBACK_FORCE = 350.0f;
    constexpr float MELEE_ATTACK_RANGE = 48.0f;
    constexpr float MAX_BLADE_SAMPLE_ANGLE = 12.0f;

    struct BladeCapsule {
        Vector2 start;
        Vector2 end;
        float radius;
    };

    /// Builds blade sweep.
    std::vector<BladeCapsule> BuildBladeSweep(
        Vector2 origin,
        float startAngleDegrees,
        float endAngleDegrees,
        float bladeLength,
        float bladeRadius
    ) {
        float angleDifference = endAngleDegrees - startAngleDegrees;
        int sampleCount = std::max(
            1,
            (int)std::ceil(
                std::abs(angleDifference) / MAX_BLADE_SAMPLE_ANGLE
            )
        );
        std::vector<BladeCapsule> samples;
        samples.reserve((std::size_t)sampleCount + 1);

        for (int index = 0; index <= sampleCount; ++index) {
            float amount = (float)index / (float)sampleCount;
            float angle = startAngleDegrees + angleDifference * amount;
            float radians = angle * DEG2RAD;
            Vector2 direction = { std::cos(radians), std::sin(radians) };
            samples.push_back({
                origin,
                {
                    origin.x + direction.x * bladeLength,
                    origin.y + direction.y * bladeLength
                },
                bladeRadius
            });
        }
        return samples;
    }

    /// Returns the current blade sweep bounds.
    Rectangle GetBladeSweepBounds(
        const std::vector<BladeCapsule>& samples
    ) {
        float minimumX = std::numeric_limits<float>::max();
        float minimumY = std::numeric_limits<float>::max();
        float maximumX = std::numeric_limits<float>::lowest();
        float maximumY = std::numeric_limits<float>::lowest();

        for (const BladeCapsule& sample : samples) {
            minimumX = std::min(
                minimumX,
                std::min(sample.start.x, sample.end.x) - sample.radius
            );
            minimumY = std::min(
                minimumY,
                std::min(sample.start.y, sample.end.y) - sample.radius
            );
            maximumX = std::max(
                maximumX,
                std::max(sample.start.x, sample.end.x) + sample.radius
            );
            maximumY = std::max(
                maximumY,
                std::max(sample.start.y, sample.end.y) + sample.radius
            );
        }

        return {
            minimumX,
            minimumY,
            maximumX - minimumX,
            maximumY - minimumY
        };
    }

    /// Implements the blade sweep intersects behavior for this component.
    bool BladeSweepIntersects(
        const std::vector<BladeCapsule>& samples,
        Rectangle bounds
    ) {
        return std::any_of(
            samples.begin(),
            samples.end(),
            [bounds](const BladeCapsule& sample) {
                return LineOfSightGeometry::CapsuleIntersectsRectangle(
                    sample.start,
                    sample.end,
                    sample.radius,
                    bounds
                );
            }
        );
    }
}

/// Creates a MeleeAttackStrategy instance from the supplied configuration.
MeleeAttackStrategy::MeleeAttackStrategy(
    Texture2D weapon,
    Texture2D att1,
    Texture2D att2,
    int lightDamage,
    int heavyDamage
)
    : weaponTex(weapon),
      attack1Tex(att1),
      attack2Tex(att2),
      comboStep(0),
      nextComboStep(1),
      frameTimer(0.0f),
      currentFrame(0),
      inputBuffered(false),
      kinematics(WeaponKinematicsType::Melee),
      lightDamage(lightDamage),
      heavyDamage(heavyDamage),
      lastCollisionAngleOffset(0.0f),
      lastFacingLeft(false)
{
    aimDir = {1.0f, 0.0f};
    aimAngle = 0.0f;
    timePerFrame = 0.05f; // Fast melee swing (4 frames = 0.2s total)
}

/// Starts this attack behavior when its current conditions allow it.
void MeleeAttackStrategy::Attack(Vector2 playerPos) {
    lastPlayerPos = playerPos;
    if (comboStep == 0) {
        // Start combo
        comboStep = (rand() % 2) + 1;
        currentFrame = 0;
        frameTimer = 0.0f;
        inputBuffered = false;
        objectsHit.clear();
        mapObjectsHit.clear();
        AudioManager::GetInstance().PlayRandomSwordSlash();
        kinematics.ApplySwing(0.2f, 120.0f, (comboStep == 2));
        lastCollisionAngleOffset = GetSignedSwingOffset();
    } else if (comboStep == 1 || comboStep == 2) {
        // Buffer the next hit if clicked during the active swing
        if (currentFrame >= 1) {
            inputBuffered = true;
        }
    }
}

/// Advances this component's state for the current frame.
void MeleeAttackStrategy::Update(float deltaTime) {
    kinematics.Update(deltaTime);
    if (comboStep == 0) return;

    frameTimer += deltaTime;
    if (frameTimer >= timePerFrame) {
        frameTimer -= timePerFrame;
        currentFrame++;

        // Process Hitbox Logic ON Impact Frames (frames 1 & 2, since 0-indexed)
        if (currentFrame == 1 || currentFrame == 2) {
            float currentOffset = GetSignedSwingOffset();
            ProcessBladeCollision(
                aimAngle + lastCollisionAngleOffset,
                aimAngle + currentOffset
            );
            lastCollisionAngleOffset = currentOffset;
        }

        // Handle animation end & combo transition
        if (currentFrame >= 4) {
            if (inputBuffered) {
                // Chain to a random attack
                comboStep = (rand() % 2) + 1;
                currentFrame = 0;
                inputBuffered = false;
                objectsHit.clear();
                mapObjectsHit.clear();
                AudioManager::GetInstance().PlayRandomSwordSlash();
                kinematics.ApplySwing(0.2f, 120.0f, (comboStep == 2));
                lastCollisionAngleOffset = GetSignedSwingOffset();
            } else {
                // End combo
                comboStep = 0;
                currentFrame = 0;
                inputBuffered = false;
            }
        }
    }
}

/// Returns the current signed swing offset.
float MeleeAttackStrategy::GetSignedSwingOffset() const {
    bool facingLeft = aimDir.x < -0.001f
        ? true
        : (aimDir.x > 0.001f ? false : lastFacingLeft);
    return facingLeft
        ? -kinematics.GetAngleOffset()
        : kinematics.GetAngleOffset();
}

/// Processes blade collision.
void MeleeAttackStrategy::ProcessBladeCollision(
    float startAngleDegrees,
    float endAngleDegrees
) {
    float bladeLength = MELEE_ATTACK_RANGE;
    float bladeRadius = weaponTex.id != 0
        ? std::max(3.0f, (float)weaponTex.height * 0.25f)
        : 5.0f;
    std::vector<BladeCapsule> bladeSweep = BuildBladeSweep(
        lastPlayerPos,
        startAngleDegrees,
        endAngleDegrees,
        bladeLength,
        bladeRadius
    );
    Rectangle broadPhase = GetBladeSweepBounds(bladeSweep);
    int damage = comboStep == 1 ? lightDamage : heavyDamage;

    GameManager& gameManager = GameManager::GetInstance();
    const auto& enemies = gameManager.GetObjectManager().GetEnemies();
    for (Enemy* enemy : enemies) {
        if (!enemy || !enemy->IsEnabled() ||
            objectsHit.find(enemy->GetObjectId()) != objectsHit.end() ||
            !CheckCollisionRecs(broadPhase, enemy->GetBoundingBox()) ||
            !BladeSweepIntersects(bladeSweep, enemy->GetBoundingBox())) {
            continue;
        }

        enemy->TakeDamage(damage);
        enemy->ApplyKnockback(aimDir, MELEE_KNOCKBACK_FORCE);
        if (owner) owner->OnHitEnemy(damage);
        objectsHit.insert(enemy->GetObjectId());
        gameManager.AddImpactEffect(enemy->GetPosition());
    }

    LevelManager* levelManager = gameManager.GetLevelManager();
    if (!levelManager) return;
    for (MapObject* mapObject :
         levelManager->FindSolidMapObjectCollisions(broadPhase)) {
        if (!mapObject ||
            mapObject->GetMapObjectType() != MapObjectId::DestructibleBox ||
            mapObjectsHit.find(mapObject->GetHandle()) != mapObjectsHit.end() ||
            !BladeSweepIntersects(
                bladeSweep,
                mapObject->GetCollisionBox()
            )) {
            continue;
        }

        mapObject->TakeDamage(damage);
        mapObjectsHit.insert(mapObject->GetHandle());
        gameManager.AddImpactEffect(mapObject->GetPosition());
    }
}

/// Renders this component using its current state and visual resources.
void MeleeAttackStrategy::Draw(Vector2 playerPos, bool facingLeft) {
    lastPlayerPos = playerPos;
    lastFacingLeft = facingLeft;
    float currentAngle = aimAngle + (facingLeft ? -kinematics.GetAngleOffset() : kinematics.GetAngleOffset());

    // 1. Draw Weapon
    if (weaponTex.id != 0) {
        Rectangle source = { 0.0f, 0.0f, (float)weaponTex.width, (float)weaponTex.height };
        if (facingLeft) {
            source.height = -source.height; 
        }
        Rectangle dest = { playerPos.x, playerPos.y, (float)weaponTex.width, (float)weaponTex.height };
        Vector2 origin = { 0.0f, (float)weaponTex.height / 2.0f }; // Pivot at hilt
        DrawTexturePro(weaponTex, source, dest, origin, currentAngle, WHITE);
    }

    // 2. Draw Effect ON TOP (triggered at +30 degrees approx -> frame 1)
    if (comboStep != 0 && currentFrame >= 1 && currentFrame <= 3) {
        Texture2D activeTex = (comboStep == 1) ? attack1Tex : attack2Tex;
        if (activeTex.id != 0) {
            int slashFrame = currentFrame - 1;
            float frameWidth = (float)activeTex.width / 3.0f; // Sword_slash_small is 3 frames
            float sourceHeight = (float)activeTex.height;
            
            if (facingLeft) sourceHeight = -sourceHeight;
            if (comboStep == 2) sourceHeight = -sourceHeight; // Reverse visual direction for upward swing

            Rectangle source = { slashFrame * frameWidth, 0.0f, frameWidth, sourceHeight };
            
            float distanceOut = weaponTex.id != 0 ? (float)weaponTex.width : 32.0f; // Calculate offset distance
            float rad = currentAngle * PI / 180.0f;
            Vector2 drawPos = { playerPos.x + std::cos(rad) * distanceOut, playerPos.y + std::sin(rad) * distanceOut };
            
            float scale = Constants::GLOBAL_SCALE; // Scale up the slash effect so it's proportional to the sword
            Rectangle dest = { drawPos.x, drawPos.y, frameWidth * scale, (float)activeTex.height * scale };
            Vector2 origin = { (frameWidth * scale) / 2.0f, ((float)activeTex.height * scale) / 2.0f };

            BeginBlendMode(BLEND_ADDITIVE);
            DrawTexturePro(activeTex, source, dest, origin, currentAngle, WHITE); // Align rotation exactly to sword
            EndBlendMode();
        }
    }
}
