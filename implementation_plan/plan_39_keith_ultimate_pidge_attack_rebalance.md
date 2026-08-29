# Implementation Plan: Keith Ultimate Rework & Pidge Attack Rebalance

Rebalance and enhance combat visuals and mechanics for Keith's ultimate and Pidge's primary attack.

## User Review Required

> [!NOTE]
> All requested VFX assets (`fire_anim.png` with 4 horizontal frames and `ulti_fire.png`) are present in `assets/sprites/Effects/` and `assets/sprites/Keith/`.

## Proposed Changes

### Asset Loading

#### [MODIFY] [AssetManager.cpp](file:///Users/lemon4life/Library/CloudStorage/OneDrive-Personal/Documents/1_HCMUS/3rd/CS202/Voltron%20Mission%20Galra%20Cypher/src/Core/Manager/AssetManager.cpp)
- Register `fire_anim` (4 frames horizontal sheet) with key `"fire_anim"` pointing to `assets/sprites/Effects/fire_anim.png`.

---

### Keith Ultimate Rework

#### [NEW] [KeithUltiProjectile.h](file:///Users/lemon4life/Library/CloudStorage/OneDrive-Personal/Documents/1_HCMUS/3rd/CS202/Voltron%20Mission%20Galra%20Cypher/include/Entities/Projectiles/KeithUltiProjectile.h) & [KeithUltiProjectile.cpp](file:///Users/lemon4life/Library/CloudStorage/OneDrive-Personal/Documents/1_HCMUS/3rd/CS202/Voltron%20Mission%20Galra%20Cypher/src/Entities/Projectiles/KeithUltiProjectile.cpp)
- Subclass `Projectile` for Keith's ultimate:
  - Override `IgnoresWorldCollision() const override { return true; }` to pierce all walls, boundary obstacles, and map props without bouncing or premature destruction.
  - Implement dynamic **Burning Fire Trail Zone**:
    - As projectile advances during flight ($t < 0.65\text{s}$), extend trail length ($L \le 500\text{px}$) and scatter `FireTrailNode` instances across width ($W = 70\text{px}$).
    - Lingering trail duration of $3.5\text{s}$ after wave completion.
    - Continuous 4-frame animation cycling for all fire sprites along the trail.
    - Periodic burn damage / `EffectType::BURN` application to all enemies within the oriented trail rectangle.
    - Alpha fadeout on zone rectangle and fire sprites during final $0.5\text{s}$.
  - Heavy initial impact damage ($250$ base scaled by Paladin damage) + Burn status.

#### [MODIFY] [Keith.cpp](file:///Users/lemon4life/Library/CloudStorage/OneDrive-Personal/Documents/1_HCMUS/3rd/CS202/Voltron%20Mission%20Galra%20Cypher/src/Entities/Player/Keith.cpp)
- In `ExecuteUltimateAction()`:
  - Instantiate and spawn `KeithUltiProjectile`.
- In `Draw()`:
  - Rescale pre-attack orange telegraph indicator rectangle to match true hitbox ($500\text{px} \text{ length} \times 70\text{px} \text{ width}$).

---

### Pidge Basic Attack Speed Buff

#### [MODIFY] [PaladinDefinition.cpp](file:///Users/lemon4life/Library/CloudStorage/OneDrive-Personal/Documents/1_HCMUS/3rd/CS202/Voltron%20Mission%20Galra%20Cypher/src/Entities/Player/PaladinDefinition.cpp)
- Reduce Pidge's `attackCooldownScalar` from `0.8f` to `0.55f`.

#### [MODIFY] [Pidge.cpp](file:///Users/lemon4life/Library/CloudStorage/OneDrive-Personal/Documents/1_HCMUS/3rd/CS202/Voltron%20Mission%20Galra%20Cypher/src/Entities/Player/Pidge.cpp)
- Increase Pidge's Bayard projectile throw velocity from $800\text{px/s}$ to $1100\text{px/s}$ with max fly time $0.35\text{s}$.

#### [MODIFY] [Projectile.cpp](file:///Users/lemon4life/Library/CloudStorage/OneDrive-Personal/Documents/1_HCMUS/3rd/CS202/Voltron%20Mission%20Galra%20Cypher/src/Entities/Projectile.cpp)
- Boost return homing speed when returning to owner ($\ge 1200\text{px/s}$) for snappy catch cycles.

---

## Verification Plan

### Automated Build Verification
- Build project using `cmake --build build --config Release`.

### Gameplay Verification
- **Keith Ultimate Telegraph & Wall Penetration:**
  - Enter dungeon or combat room, charge Keith's ultimate, and verify the orange indicator is $500\text{px} \times 70\text{px}$.
  - Fire ultimate toward walls and enemies: verify wave cuts completely through all walls and obstacles without bouncing.
- **Burning Fire Trail:**
  - Verify translucent red zone appears along path.
  - Verify 4-frame `fire_anim` sprites flicker continuously along the trail.
  - Verify enemies stepping into trail receive Burn status and tick damage.
  - Verify trail disappears after $\sim 3.5\text{s}$ with smooth fadeout.
- **Pidge Basic Attack:**
  - Verify noticeably faster attack rate ($0.55$ cooldown scalar) and rapid projectile throw/return speed.
