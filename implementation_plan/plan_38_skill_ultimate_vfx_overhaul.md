# Implementation Plan: Skill & Ultimate VFX Overhaul

Overhaul the visual effects (VFX) and animation layers for character skills, ultimates, and projectile managers across all four Paladins (Keith, Lance, Hunk, Pidge).

## User Review Required

> [!NOTE]
> All requested VFX sprite sheets (`use_skill.png`, `toxic.png`, `shield.png`, `fire_range.png`, `ulti_fire.png`, `Ulti_explode.png`, `skill_explode.png`) are already present in the workspace asset directories and will be registered in `AssetManager`.

## Proposed Changes

### Core / Managers

#### [MODIFY] [AssetManager.cpp](file:///Users/lemon4life/Library/CloudStorage/OneDrive-Personal/Documents/1_HCMUS/3rd/CS202/Voltron%20Mission%20Galra%20Cypher/src/Core/Manager/AssetManager.cpp)
- Add asset loading entries in `QueueCharacterAssets` and `LoadCommonAssets`:
  - `"use_skill"` $\to$ `assets/sprites/Effects/use_skill.png`
  - `"toxic"` $\to$ `assets/sprites/Effects/toxic.png`
  - `"shield"` $\to$ `assets/sprites/Hunk/shield.png`
  - `"fire_range"` $\to$ `assets/sprites/Keith/fire_range.png`
  - `"ulti_fire"` $\to$ `assets/sprites/Keith/ulti_fire.png`
  - `"Ulti_explode"` $\to$ `assets/sprites/Lance/Ulti_explode.png`
  - `"skill_explode"` $\to$ `assets/sprites/Pidge/skill_explode.png`

---

### Entities & Combat

#### [MODIFY] [Paladin.cpp](file:///Users/lemon4life/Library/CloudStorage/OneDrive-Personal/Documents/1_HCMUS/3rd/CS202/Voltron%20Mission%20Galra%20Cypher/src/Entities/Player/Paladin.cpp)
- In `Paladin::ActivateSkill(float duration)`:
  - Instantiate a one-shot 8-frame `use_skill` attached animation on top of the Paladin via `AddAttachedEffect`.

#### [MODIFY] [Buffs.h](file:///Users/lemon4life/Library/CloudStorage/OneDrive-Personal/Documents/1_HCMUS/3rd/CS202/Voltron%20Mission%20Galra%20Cypher/include/Combat/Buffs.h)
- `AegisShieldBuff` (Hunk Ultimate):
  - Add continuous rotation (`rotationAngle += 90.0f * deltaTime`).
  - Add smooth scale transitions: zoom-out entrance (0.0 to 1.0 over 0.2s) and zoom-in exit (1.0 to 0.0 over final 0.2s).
  - Draw centered on `activePaladin` using `shield` texture.
- `FireCircleBuff` (Keith Skill):
  - Remove debug circle rendering.
  - Add continuous rotation (`rotationAngle += 45.0f * deltaTime`).
  - Add smooth scale transitions (0.0 to 1.0 entrance over 0.2s, 1.0 to 0.0 exit over 0.2s).
  - Draw centered on `activePaladin` using `fire_range` texture scaled to diameter ($2 \times \text{SKILL\_RADIUS}$).

#### [MODIFY] [Keith.h](file:///Users/lemon4life/Library/CloudStorage/OneDrive-Personal/Documents/1_HCMUS/3rd/CS202/Voltron%20Mission%20Galra%20Cypher/include/Entities/Player/Keith.h) & [Keith.cpp](file:///Users/lemon4life/Library/CloudStorage/OneDrive-Personal/Documents/1_HCMUS/3rd/CS202/Voltron%20Mission%20Galra%20Cypher/src/Entities/Player/Keith.cpp)
- Update `ExecuteUltimateAction()` to fire high-velocity linear projectile using `ulti_fire` texture along `currentAimAngle`.
- Remove obsolete orange debug rectangle drawing in `Draw()`.

#### [MODIFY] [Lance.h](file:///Users/lemon4life/Library/CloudStorage/OneDrive-Personal/Documents/1_HCMUS/3rd/CS202/Voltron%20Mission%20Galra%20Cypher/include/Entities/Player/Lance.h) & [Lance.cpp](file:///Users/lemon4life/Library/CloudStorage/OneDrive-Personal/Documents/1_HCMUS/3rd/CS202/Voltron%20Mission%20Galra%20Cypher/src/Entities/Player/Lance.cpp)
- In `ExecuteUltimateAction()`:
  - Spawn 8-frame `Ulti_explode` impact effect on each targeted/hit enemy via `GameManager::GetInstance().AddEffect(...)`.
  - Introduce pending freeze target queue with delay matching 3rd animation frame (`(0.48s / 8) * 2 = 0.12s`).
  - Apply `EffectType::FREEZE` when timer reaches zero, visually synchronizing status tint and ice ground sprite with the 3rd explosion frame.

#### [MODIFY] [Pidge.h](file:///Users/lemon4life/Library/CloudStorage/OneDrive-Personal/Documents/1_HCMUS/3rd/CS202/Voltron%20Mission%20Galra%20Cypher/include/Entities/Player/Pidge.h) & [Pidge.cpp](file:///Users/lemon4life/Library/CloudStorage/OneDrive-Personal/Documents/1_HCMUS/3rd/CS202/Voltron%20Mission%20Galra%20Cypher/src/Entities/Player/Pidge.cpp)
- In `UseSkill()`:
  - Trigger one-shot 8-frame `skill_explode` animation at the center of the poison zone.
- In `UpdateVenomZone()`:
  - Continuously spawn drifting animated 3-frame `toxic` particles within the circular radius ($R = 120.0f$).
  - Update positions, 3-frame animation cycling, and alpha fadeout of active toxic particles.
- In `Draw()` and `DrawInactive()`:
  - Render circular poison area with red outline styling and green translucent fill.
  - Draw active 3-frame `toxic` particle sprites.

---

## Verification Plan

### Automated Build Verification
- Execute `cmake --build build --config Release` to ensure 0 compilation and linking errors.

### Manual Verification
- **Generic Skill VFX**: Trigger skill on any Paladin and verify 8-frame `use_skill` plays attached to character position.
- **Hunk Ultimate**: Activate Hunk ultimate and observe smooth shield entrance scale (0.0 $\to$ 1.0), continuous rotation, and smooth exit scaling (1.0 $\to$ 0.0).
- **Keith Skill & Ultimate**:
  - Activate Keith skill to check rotating `fire_range` aura with smooth scaling matching damage radius.
  - Activate Keith ultimate to verify high-velocity `ulti_fire` projectile traveling across the room.
- **Lance Ultimate**: Activate Lance ultimate and verify `Ulti_explode` plays over enemies, with freeze ice status emerging precisely on frame 3.
- **Pidge Skill**: Activate Pidge skill to verify `skill_explode` burst, circular red outline zone, and continuous floating 3-frame `toxic` particles.
