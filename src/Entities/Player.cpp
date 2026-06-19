#include "Entities/Player.h"
#include "Combat/RangedAttackStrategy.h"
#include "Combat/MeleeAttackStrategy.h"
Player::Player(Vector2 pos, Texture2D tIdle, Texture2D tRun, Texture2D tGun, Texture2D tKeith, Texture2D tSword)
    : Character(pos, 150.0f, 150, tIdle), // Lance's stats: 150 speed, 150 HP
      texIdle(tIdle),
      texRun(tRun),
      texGun(tGun),
      texKeithRun(tKeith),
      texSword(tSword),
      currentWeapon(nullptr),
      currentFrame(0),
      frameTimer(0.0f),
      frameDuration(0.1f), // 10 fps animation speed
      facingLeft(false),
      numFrames(12),
      maxHealth(150),
      armor(50),
      maxArmor(50),
      timeSinceLastDamage(0.0f),
      armorRegenTimer(0.0f),
      isPlayingAsLance(true),
      dashCooldown(0.0f),
      dashTimer(0.0f),
      isInvincible(false),
      lastMoveDir{1.0f, 0.0f} // Initialize pointing right
{
    currentState = &idleState;
    currentState->Enter(this);
    NotifyObservers();
}

void Player::NotifyObservers() {
    for (auto* observer : observers) {
        observer->OnPlayerStatsChanged(health, maxHealth, armor, maxArmor, isPlayingAsLance);
    }
}

Player::~Player() {
    if (currentState) {
        currentState->Exit(this);
    }
    if (currentWeapon) {
        delete currentWeapon;
    }
}



Vector2 Player::GetWeaponPivot() const {
    Vector2 pivot = position;
    // Align to the back shoulder (left side when facing right, right side when facing left)
    if (facingLeft) {
        pivot.x += 12.0f;
    } else {
        pivot.x -= 12.0f;
    }
    return pivot;
}

void Player::Attack() {
    if (currentWeapon) {
        currentWeapon->Attack(GetWeaponPivot());
    }
}

void Player::ChangeState(IPlayerState* newState) {
    if (currentState != newState) {
        if (currentState) currentState->Exit(this);
        currentState = newState;
        if (currentState) currentState->Enter(this);
    }
}

void Player::TakeDamage(int amount) {
    if (isInvincible) return; // Ignore damage if invincible
    
    timeSinceLastDamage = 0.0f; // Reset armor regen timer

    if (armor > 0) {
        armor -= amount;
        if (armor < 0) {
            health += armor; // spill over damage to health
            armor = 0;
        }
    } else {
        health -= amount;
    }
    if (health < 0) health = 0;
    NotifyObservers();
}

void Player::Update(float deltaTime) {
    // 360-degree aiming math
    Vector2 mouseScreen = GetMousePosition();
    // Assuming 1024x1024 window and 512x512 internal render texture
    Vector2 mouseWorld = { mouseScreen.x / 2.0f, mouseScreen.y / 2.0f }; 
    Vector2 dir = Vector2Subtract(mouseWorld, position);
    float distance = Vector2Length(dir);
    if (distance > 0.0f) {
        dir = Vector2Normalize(dir);
    } else {
        dir = {1.0f, 0.0f};
    }
    float angle = atan2f(dir.y, dir.x) * (180.0f / PI);
    
    facingLeft = (mouseWorld.x < position.x);

    if (currentWeapon) {
        currentWeapon->SetAim(dir, angle);
    }

    // Decrement dash cooldown over time
    if (dashCooldown > 0.0f) {
        dashCooldown -= deltaTime;
        if (dashCooldown < 0.0f) {
            dashCooldown = 0.0f;
        }
    }

    // Handle Character Switching
    if (IsKeyPressed(KEY_TAB)) {
        ToggleCharacter();
    }

    // Handle Armor Regeneration
    timeSinceLastDamage += deltaTime;
    if (timeSinceLastDamage >= 3.0f && armor < maxArmor) {
        armorRegenTimer += deltaTime;
        // Regenerate 10 armor per second (1 point per 0.1s)
        if (armorRegenTimer >= 0.1f) {
            armor += 1;
            armorRegenTimer = 0.0f;
            if (armor > maxArmor) armor = maxArmor;
            NotifyObservers();
        }
    }

    if (currentState) {
        currentState->Update(this, deltaTime);
    }
    if (currentWeapon) {
        currentWeapon->Update(deltaTime);
    }
}

void Player::ToggleCharacter() {
    isPlayingAsLance = !isPlayingAsLance;
    float hpPercent = (float)health / maxHealth;

    if (currentWeapon) {
        delete currentWeapon;
    }

    if (isPlayingAsLance) {
        speed = 150.0f;
        maxHealth = 150;
        health = (int)(maxHealth * hpPercent);
        currentWeapon = new RangedAttackStrategy(texGun);
        texture = texIdle;
    } else {
        speed = 220.0f;
        maxHealth = 100;
        health = (int)(maxHealth * hpPercent);
        currentWeapon = new MeleeAttackStrategy(texSword);
        texture = texKeithRun;
    }
    NotifyObservers();
}

void Player::ResetStats() {
    isPlayingAsLance = true;
    maxHealth = 150;
    health = maxHealth;
    maxArmor = 50;
    armor = maxArmor;
    speed = 150.0f;
    timeSinceLastDamage = 0.0f;
    
    if (currentWeapon) {
        delete currentWeapon;
    }
    currentWeapon = new RangedAttackStrategy(texGun);
    texture = texIdle;
    
    NotifyObservers();
}

Rectangle Player::GetBoundingBox() const {
    // 24x36 bounding box centered on playerPos
    return { position.x - 12.0f, position.y - 18.0f, 24.0f, 36.0f };
}

bool Player::CheckCollision(const std::vector<GameObject*>& entities) const {
    Rectangle pBox = GetBoundingBox();
    for (auto* entity : entities) {
        if (CheckCollisionRecs(pBox, entity->GetBoundingBox())) {
            return true;
        }
    }
    return false;
}

void Player::UpdateAnimation(float deltaTime) {
    frameTimer += deltaTime;
    if (frameTimer >= frameDuration) {
        frameTimer -= frameDuration;
        currentFrame = (currentFrame + 1) % numFrames;
    }
}

void Player::Draw() {
    // Calculate the frame width dynamically based on the active texture
    // and the number of frames (both sprite sheets have 12 frames, but idle is 32px wide and run is 36px wide per frame)
    const float frameWidth = (float)texture.width / numFrames;
    const float frameHeight = 48.0f;

    // Source rectangle based on current frame and facing direction
    // A negative width in DrawTexturePro flips the texture horizontally natively in Raylib
    Rectangle sourceRec = {
        (float)currentFrame * frameWidth,
        0.0f,
        facingLeft ? -frameWidth : frameWidth,
        frameHeight
    };

    // Destination rectangle centered exactly on the player's position
    Rectangle destRec = {
        position.x,
        position.y,
        frameWidth,
        frameHeight
    };

    // Origin for rotation/scaling, set to center of the sprite
    Vector2 origin = { frameWidth / 2.0f, frameHeight / 2.0f };

    // Apply a gray tint to indicate invincibility during a dash
    Color tint = isInvincible ? GRAY : WHITE;

    DrawTexturePro(texture, sourceRec, destRec, origin, 0.0f, tint);
    
    if (currentWeapon) {
        currentWeapon->Draw(GetWeaponPivot(), facingLeft);
    }
}
