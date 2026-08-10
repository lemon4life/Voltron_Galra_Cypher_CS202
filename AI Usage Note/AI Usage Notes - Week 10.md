## Prompt:
Context: We are finalizing the character-specific skills and ultimate abilities for the Paladins.

Task:
Implement Keith's and Lance's skill and ultimate abilities, and fix bugs regarding EX generation and aiming mechanics.

Requirements:
1. Keith Skill & Ultimate:
- Make Keith's skill circle follow the active paladin even after character switching.
- Keith's ultimate should have a two-phase mechanic: first a 50% transparent ghost hitbox that follows the mouse/aim vector while holding the ultimate, and then deals damage in that hitbox when triggered.
- Ensure Keith can run and dash during his ultimate aiming phase, but disable attack and parry.
- Ensure the ultimate rotates around a small circle at Keith's feet (`position.y + 17.0f`).

2. Lance Implementation:
- Create Lance subclass.
- Twin Blasters skill: halves attack cooldown and spawns two parallel projectiles offset to the left and right of his aim vector.
- Absolute Zero ultimate: instantly freezes all active enemies in the room for 5.0 seconds and triggers a screen flash.

3. General Bug Fixes & Refinements:
- Fix EX generation: skills and ultimates should NOT generate EX. Cap standard EX generation.
- Fix auto-aim mode overriding manual aim during ultimates.
- Update `Lance.cpp` visual math to ensure distinct weapon sockets when dual-wielding, avoiding awkward vertical orbiting when aiming left or right by dampening the perpendicular normal.
- Implement a `debugSpamMode` for Lance to test abilities without EX cost or cooldowns.

## Purpose:
To fully implement the intricate aiming, rendering, and logic requirements for Keith and Lance's abilities, while balancing the EX meter and fixing edge cases related to character swapping and hit detection offsets.

## Content Generated:
- Modified `Keith.cpp` and `Paladin.cpp` to separate the ultimate into aiming and triggering phases, and updated rendering pivots to correctly match the floor shadow.
- Modified `PlayerState` handling to allow movement and dashing during ultimates while blocking basic attacks and parries.
- Modified `Paladin.h` to make `Attack()` virtual, allowing `Lance.cpp` to intercept firing commands.
- Implemented `Lance`'s dual-wielding skill by overriding `Draw` and `Attack` to spawn parallel projectiles from distinct shoulder sockets using a squashed perpendicular normal.
- Implemented `Lance`'s Absolute Zero ultimate by globally iterating over `GameManager` enemies and applying the `FREEZE` status.
- Added `debugSpamMode` toggle in `Lance.h` to bypass ability constraints.

## Prompt:
Context: We have an established `Paladin` base class and a global enemy list. We are implementing the `Hunk` subclass (The Anchor), focused on defensive mechanics and crowd control.

Task: Implement the `Hunk` subclass derived from `Paladin`. Implement his base stats, a radial knockback skill ("Earthshatter"), a team-wide invulnerability ultimate ("Aegis Shield"), and a debug harness for instant cooldowns and EX bypass.

Requirements:
1. Hunk Base Setup (Task 4.1): Inherit Paladin. Initialize 1.5x HP and slower speed. Override ApplyKnockback to ignore collisions.
2. Earthshatter Skill (Task 4.2): Radial knockback pushing enemies away and drawing an expanding brown circle.
3. Aegis Shield Ultimate (Task 4.3): Team-wide 5.0s invulnerability buff added to base Paladin class with a yellow aura.
4. Debug Testing Harness: `debugSpamMode` to bypass EX and cooldowns for instant casting.

## Purpose:
To implement the final Paladin subclass, Hunk, along with his defensive radial abilities and global immunity buffs that persist through character swapping.

## Content Generated:
- `Paladin.h`/`.cpp`: Added `isInvulnerable` and `invulnerabilityTimer`, modified `TakeDamage()` to prevent damage, made `ApplyKnockback()` virtual, and drew the yellow aura in `Draw()$.
- `Hunk.h`/`.cpp`: Increased `maxHealth`, overrode `ApplyKnockback()` to do nothing (knockback resistance), implemented Earthshatter radial knockback math against all enemies, and tied everything to `debugSpamMode`.


## Fixes & Enhancements: Pidge's Rover & Venom Zone (Aug 10)
**Prompts Used:**
- "double check why Rover still isn't moving or shooting at all?"
- "Context: We are refactoring the Rover entity. Currently, Rover's update loop is improperly nested inside Pidge's skill logic, its movement ignores environment walls (causing it to get stuck), and it lacks a catch-up mechanic... Decouple Rover's update loop from Pidge, implement standard AI environment pathfinding/collision for its movement, and add a teleportation failsafe."
- "find a way to smoothen Rover's movement"
- "Context: We are finalizing the Pidge subclass. The agent previously missed the implementation for her active skill, the "Venom Zone"... Implement Pidge's active skill: a stationary, 7-second area-of-effect hazard that inflicts Poison and Slow status effects on enemies that step inside it."
- "allow Pidge to spam skills for mock-up"

**Purpose:**
To finalize Pidge's active abilities (Venom Zone) and ultimate ability (Rover companion) by fixing update loops, collision logic, and ability mechanics.

**Content Generated:**
- Decoupled `Rover` updates from Pidge's skill loop, converting `UpdateWithTeam()` into a standard `GameObject::Update()` override called globally in `main.cpp`.
- Implemented physics-based movement smoothing (velocity interpolation) and wall-sliding collision resolution for Rover.
- Added a teleport failsafe (leash mechanic) if Rover falls more than `400.0f` distance behind the player.
- Migrated `VenomZone` AoE hazard tracking directly into `Pidge` subclass with strict 7-second durations and collision-circle evaluations.
- Updated `Enemy::GetSpeed()` to calculate a 50% velocity reduction when the `SLOW` status effect is active from the Venom Zone.
- Added a `debugSpamMode` bypass to Pidge's `UseSkill()` for testing without energy constraints.
