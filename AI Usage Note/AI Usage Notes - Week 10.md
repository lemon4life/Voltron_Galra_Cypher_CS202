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
