#include "Entities/BossFirePunchProjectile.h"

#include "Core/Manager/TeamManager.h"
#include "Entities/Player/Paladin.h"

#include <algorithm>
#include <cmath>

namespace {
    constexpr int FIRE_PUNCH_FRAME_COUNT = 4;
    constexpr float FIRE_PUNCH_FRAME_SIZE = 64.0f;
    constexpr float FIRE_PUNCH_FRAME_DURATION = 0.10f;
    constexpr float FIRE_PUNCH_MAX_LIFETIME = 3.0f;
    constexpr Vector2 FIRE_PUNCH_DRAW_ORIGIN = { 26.0f, 29.0f };
    constexpr float FIRE_PUNCH_COLLISION_LEFT = 17.0f;
    constexpr float FIRE_PUNCH_COLLISION_TOP = 21.0f;
    constexpr float FIRE_PUNCH_COLLISION_RIGHT = 48.0f;
    constexpr float FIRE_PUNCH_COLLISION_BOTTOM = 39.0f;
    constexpr float POSITION_EPSILON = 0.0001f;

    float NormalizeSignedAngle(float angleDegrees) {
        float normalized = std::fmod(angleDegrees + 180.0f, 360.0f);
        if (normalized < 0.0f) {
            normalized += 360.0f;
        }
        return normalized - 180.0f;
    }

    Vector2 DirectionFromAngle(float angleDegrees) {
        float radians = angleDegrees * DEG2RAD;
        return { std::cos(radians), std::sin(radians) };
    }

    Vector2 RotateOffset(Vector2 offset, float angleDegrees) {
        float radians = angleDegrees * DEG2RAD;
        float cosine = std::cos(radians);
        float sine = std::sin(radians);
        return {
            offset.x * cosine - offset.y * sine,
            offset.x * sine + offset.y * cosine
        };
    }

    bool IsFiniteRectangle(Rectangle bounds) {
        return std::isfinite(bounds.x) &&
            std::isfinite(bounds.y) &&
            std::isfinite(bounds.width) &&
            std::isfinite(bounds.height);
    }

    bool HasUsableBounds(Rectangle bounds) {
        return IsFiniteRectangle(bounds) &&
            bounds.width > 0.0f &&
            bounds.height > 0.0f;
    }
}

BossFirePunchProjectile::BossFirePunchProjectile(
    Vector2 spawnPosition,
    Vector2 initialTargetPosition,
    TeamManager* target,
    float speed,
    float maximumTurnRate,
    int damage,
    Texture2D texture,
    Rectangle initialRoomBounds,
    Rectangle initialMapBounds
)
    : Projectile(
          spawnPosition,
          { 0.0f, 0.0f },
          FIRE_PUNCH_MAX_LIFETIME,
          damage,
          true
      ),
      targetTeam(target),
      animationTexture(texture),
      roomBounds(initialRoomBounds),
      mapBounds(initialMapBounds),
      movementSpeed(std::max(0.0f, speed)),
      maximumTurnRateDegrees(std::max(0.0f, maximumTurnRate)),
      currentAngleDegrees(0.0f) {
    Vector2 initialDirection = {
        initialTargetPosition.x - spawnPosition.x,
        initialTargetPosition.y - spawnPosition.y
    };
    if (initialDirection.x * initialDirection.x +
            initialDirection.y * initialDirection.y > POSITION_EPSILON) {
        currentAngleDegrees = std::atan2(
            initialDirection.y,
            initialDirection.x
        ) * RAD2DEG;
    }

    Vector2 direction = DirectionFromAngle(currentAngleDegrees);
    SetVelocity({
        direction.x * movementSpeed,
        direction.y * movementSpeed
    });
    UpdateCollisionBox();
}

void BossFirePunchProjectile::Update(float deltaTime) {
    if (!IsActive()) return;

    float safeDeltaTime = std::max(0.0f, deltaTime);
    lifetime -= safeDeltaTime;
    if (lifetime <= 0.0f) {
        Destroy();
        return;
    }

    // Steering is intentionally evaluated on every projectile update. The
    // per-second limit is converted to this frame's permitted angle change.
    Paladin* target = targetTeam ? targetTeam->GetActivePaladin() : nullptr;
    if (target) {
        Vector2 targetDirection = {
            target->GetPosition().x - position.x,
            target->GetPosition().y - position.y
        };
        float targetDistanceSquared =
            targetDirection.x * targetDirection.x +
            targetDirection.y * targetDirection.y;
        if (targetDistanceSquared > POSITION_EPSILON) {
            float desiredAngle = std::atan2(
                targetDirection.y,
                targetDirection.x
            ) * RAD2DEG;
            float angleDifference = NormalizeSignedAngle(
                desiredAngle - currentAngleDegrees
            );
            float maximumStep = maximumTurnRateDegrees * safeDeltaTime;
            currentAngleDegrees += std::clamp(
                angleDifference,
                -maximumStep,
                maximumStep
            );
        }
    }

    Vector2 direction = DirectionFromAngle(currentAngleDegrees);
    SetVelocity({
        direction.x * movementSpeed,
        direction.y * movementSpeed
    });
    position.x += GetVelocity().x * safeDeltaTime;
    position.y += GetVelocity().y * safeDeltaTime;

    animationTimer += safeDeltaTime;
    while (animationTimer >= FIRE_PUNCH_FRAME_DURATION) {
        animationTimer -= FIRE_PUNCH_FRAME_DURATION;
        animationFrame = (animationFrame + 1) % FIRE_PUNCH_FRAME_COUNT;
    }

    UpdateCollisionBox();
    if (IsOutsideTravelBounds()) {
        Destroy();
    }
}

void BossFirePunchProjectile::Draw() {
    if (!IsActive()) return;

    if (animationTexture.id == 0 ||
        animationTexture.width <
            (int)(FIRE_PUNCH_FRAME_SIZE * FIRE_PUNCH_FRAME_COUNT) ||
        animationTexture.height < (int)FIRE_PUNCH_FRAME_SIZE) {
        DrawRectangleRec(boundingBox, ORANGE);
        return;
    }

    DrawTexturePro(
        animationTexture,
        {
            animationFrame * FIRE_PUNCH_FRAME_SIZE,
            0.0f,
            FIRE_PUNCH_FRAME_SIZE,
            FIRE_PUNCH_FRAME_SIZE
        },
        {
            position.x,
            position.y,
            FIRE_PUNCH_FRAME_SIZE,
            FIRE_PUNCH_FRAME_SIZE
        },
        FIRE_PUNCH_DRAW_ORIGIN,
        currentAngleDegrees,
        WHITE
    );
}

void BossFirePunchProjectile::UpdateCollisionBox() {
    const Vector2 collisionCorners[] = {
        { FIRE_PUNCH_COLLISION_LEFT, FIRE_PUNCH_COLLISION_TOP },
        { FIRE_PUNCH_COLLISION_RIGHT, FIRE_PUNCH_COLLISION_TOP },
        { FIRE_PUNCH_COLLISION_RIGHT, FIRE_PUNCH_COLLISION_BOTTOM },
        { FIRE_PUNCH_COLLISION_LEFT, FIRE_PUNCH_COLLISION_BOTTOM }
    };

    float minimumX = INFINITY;
    float minimumY = INFINITY;
    float maximumX = -INFINITY;
    float maximumY = -INFINITY;
    for (Vector2 corner : collisionCorners) {
        Vector2 localOffset = {
            corner.x - FIRE_PUNCH_DRAW_ORIGIN.x,
            corner.y - FIRE_PUNCH_DRAW_ORIGIN.y
        };
        Vector2 rotatedOffset = RotateOffset(
            localOffset,
            currentAngleDegrees
        );
        Vector2 worldCorner = {
            position.x + rotatedOffset.x,
            position.y + rotatedOffset.y
        };
        minimumX = std::min(minimumX, worldCorner.x);
        minimumY = std::min(minimumY, worldCorner.y);
        maximumX = std::max(maximumX, worldCorner.x);
        maximumY = std::max(maximumY, worldCorner.y);
    }

    boundingBox = {
        minimumX,
        minimumY,
        maximumX - minimumX,
        maximumY - minimumY
    };
}

bool BossFirePunchProjectile::IsOutsideTravelBounds() const {
    if (!std::isfinite(position.x) ||
        !std::isfinite(position.y) ||
        !IsFiniteRectangle(boundingBox)) {
        return true;
    }

    float collisionRight = boundingBox.x + boundingBox.width;
    float collisionBottom = boundingBox.y + boundingBox.height;
    if (HasUsableBounds(roomBounds)) {
        float roomRight = roomBounds.x + roomBounds.width;
        float roomBottom = roomBounds.y + roomBounds.height;
        if (boundingBox.x <= roomBounds.x ||
            collisionRight >= roomRight ||
            boundingBox.y <= roomBounds.y ||
            collisionBottom >= roomBottom) {
            return true;
        }
    }

    if (HasUsableBounds(mapBounds)) {
        float mapRight = mapBounds.x + mapBounds.width;
        float mapBottom = mapBounds.y + mapBounds.height;
        if (collisionRight < mapBounds.x ||
            boundingBox.x > mapRight ||
            collisionBottom < mapBounds.y ||
            boundingBox.y > mapBottom) {
            return true;
        }
    }

    return false;
}
