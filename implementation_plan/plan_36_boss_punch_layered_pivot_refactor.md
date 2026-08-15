# Plan 17: Boss Punch Layered Pivot Refactor

## Objective

Replace the obsolete Boss punch implementation with a frame-accurate layered
animation that:

- plays the updated ready animation exactly once on Punch-state entry;
- renders the punch body and detached hand as separate synchronized layers;
- attaches the hand pivot at pixel (8, 7) to the body attachment point at
  pixel (13, 43);
- flips both body and hand consistently according to the active player's
  horizontal position;
- rotates the hand so it continuously points at the active player; and
- completes exactly ten four-frame punch cycles before returning to idle.

Punch remains animation-only. It will not create damage, attack hitboxes,
knockback, or collision behavior.

## Confirmed Updated Asset Contract

| Asset | Texture size | Frame size | Frame count | Use |
|---|---:|---:|---:|---|
| Boss-punch-ready.png | 640x72 | 64x72 | 10 | Combined preparation animation |
| Boss-punch-body.png | 256x72 | 64x72 | 4 | Body layer while punching |
| Hand.png | 216x14 | 54x14 | 4 | Detached rotating hand layer |

The old assumptions are no longer valid:

- ready is no longer three frames;
- punch is no longer six frames;
- the hand is no longer a 64x72 overlay;
- Boss-punch-play.png and Boss-punch-hand.png no longer exist; and
- five punch repetitions must become ten.

## Refactor Direction

### 1. Replace obsolete asset keys and geometry

In AssetManager::QueueCharacterAssets():

- retain Boss_Punch_Ready, pointing to Boss-punch-ready.png;
- replace Boss_Punch_Play with Boss_Punch_Body, pointing to
  Boss-punch-body.png;
- replace the old hand path with Hand.png;
- validate the hand against its own 54x14 frame size instead of the general
  Boss 64x72 frame size.

In Boss:

- rename punchPlayTexture to punchBodyTexture;
- retain a separate punchHandTexture;
- remove hand rendering that assumes the hand occupies a complete Boss frame.

### 2. Make BossPunchState own punch sequencing

Remove the redundant punchReadyAnimation flag and
BeginPunchReadyAnimation()/BeginPunchPlayAnimation() methods from Boss.

Give BossPunchState explicit animation state:

~~~cpp
enum class Phase {
    Ready,
    Punch
};

Phase phase = Phase::Ready;
float frameTimer = 0.0f;
int frameIndex = 0;
int completedPunches = 0;
~~~

Expose read-only accessors needed by Boss::Draw():

- GetPhase();
- GetFrameIndex();
- GetCompletedPunches() if useful for debugging.

This prevents the generic idle/run/spell counter from controlling Punch and
guarantees that frame skips or state transitions cannot add or remove a punch.

### 3. Punch-state timeline

On BossPunchState::Enter():

1. stop pathfinding;
2. zero Boss velocity;
3. set phase to Ready;
4. reset frame timer and frame index;
5. reset completed punches to zero.

Use a while loop during update so timing remains correct during a slow frame:

~~~text
frameTimer += max(deltaTime, 0)

while frameTimer >= frameDuration:
    frameTimer -= frameDuration

    if phase == Ready:
        advance ready frame
        if all 10 ready frames completed:
            phase = Punch
            frameIndex = 0

    else:
        advance punch frame
        if all 4 punch frames completed:
            frameIndex = 0
            completedPunches += 1

            if completedPunches == 10:
                change to IdlingState
                return
~~~

At the current 0.11-second frame duration:

- ready duration = 10 * 0.11 = 1.10 seconds;
- one punch duration = 4 * 0.11 = 0.44 seconds;
- ten punches = 10 * 0.44 = 4.40 seconds;
- total Punch-state duration = approximately 5.50 seconds.

Ready is not repeated between punches.

## Layered Rendering Calculation

### Shared body rendering

Use the same rounded Boss draw position and body origin for the ready and
punch-body sheets:

~~~text
bodyDrawPosition = round(Boss.position)
bodyDrawOrigin   = (31.5, 37.5)
bodyFrameSize    = (64, 72)
~~~

During Ready:

- draw only Boss-punch-ready.png;
- select the state-provided ready frame;
- apply the existing Boss horizontal flip;
- do not draw the detached hand because the ready sheet contains the complete
  authored pose.

During Punch:

- draw Boss-punch-body.png using the state-provided punch frame;
- draw the matching frame from Hand.png afterward.

### Body attachment point

The hand attaches to pixel (13, 43) in every 64x72 body frame.

For an unflipped body:

~~~text
bodyAnchorLocal = (13, 43)
~~~

For a horizontally flipped body, mirror the pixel coordinate using pixel-center
coordinates:

~~~text
mirroredBodyAnchorX = (64 - 1) - 13 = 50
bodyAnchorLocal     = (50, 43)
~~~

Convert the body-local point to world space using the same destination and
origin as the body:

~~~text
bodyTopLeftWorld = bodyDrawPosition - bodyDrawOrigin
handAnchorWorld  = bodyTopLeftWorld + bodyAnchorLocal
~~~

Therefore:

~~~text
unflipped handAnchorWorld =
    Boss.position + (13 - 31.5, 43 - 37.5)
  = Boss.position + (-18.5, 5.5)

flipped handAnchorWorld =
    Boss.position + (50 - 31.5, 43 - 37.5)
  = Boss.position + (18.5, 5.5)
~~~

Use the rounded body draw position in the real calculation so body and hand do
not separate by a sub-pixel.

### Hand pivot

Each hand frame is 54x14 and its authored rotation root is pixel (8, 7).

For an unflipped hand:

~~~text
handOrigin = (8, 7)
~~~

For a horizontally flipped hand, mirror the root with the same pixel-center
rule:

~~~text
mirroredHandRootX = (54 - 1) - 8 = 45
handOrigin        = (45, 7)
~~~

Draw the hand with:

~~~text
source      = matching 54x14 hand frame
destination = (handAnchorWorld.x, handAnchorWorld.y, 54, 14)
origin      = handOrigin
rotation    = calculated player-facing angle
~~~

This makes handAnchorWorld the fixed rotation point. Rotation changes the hand
pixels around that point without detaching its root from the body.

### Player-facing angle and flip compensation

Use the attachment point, not the Boss center, as the start of the aim vector:

~~~text
aimVector = activePlayer.position - handAnchorWorld
aimAngle  = atan2(aimVector.y, aimVector.x) * RAD2DEG
~~~

Use the same facing decision as the Boss body:

~~~text
facingLeft = activePlayer.position.x < Boss.position.x
~~~

Mirror the hand source when facingLeft. Horizontal mirroring reverses the
authored forward axis, so compensate its rotation:

~~~text
handRotation = aimAngle + (facingLeft ? 180 degrees : 0 degrees)
~~~

If visual inspection shows that the revised hand's zero-degree forward
direction differs, keep the correction in one named constant such as
HAND_AUTHORED_ANGLE_OFFSET. Do not scatter additional 180-degree adjustments
through the draw code.

If no active player exists, retain the current Boss facing and use a neutral
hand angle instead of dereferencing a missing target.

## Offense Selection During Punch Debugging

Preserve the current temporary offense values exactly as requested:

- Chase: 0%;
- Spell: 0%;
- Punch: 100%.

These values intentionally force Punch so the revised animation can be tested.
Do not restore the production distribution as part of this refactor.

## Files to Modify When Applying This Plan

- src/Core/Manager/AssetManager.cpp
  - update punch body and hand paths/keys.
- include/Entities/EnemyEntities/Boss.h
  - rename textures and remove obsolete phase-control fields/methods.
- include/AI/EnemyState.h
  - replace elapsed-duration tracking with explicit phase/frame/cycle state and
    read-only accessors.
- src/AI/BossState.cpp
  - implement ready-once plus exactly ten punch cycles;
  - preserve the temporary 0/0/100 forced-Punch values.
- src/Entities/EnemyEntities/Boss.cpp
  - use the updated frame counts;
  - replace the old full-frame hand overlay calculation;
  - render the 54x14 hand from pivot (8,7) at body point (13,43);
  - synchronize hand/body frames and apply facing/rotation math.
- AI Usage Note/AI Usage Notes - Week 10.md
  - record the applied implementation result when the plan is executed.

## Validation

### Build validation

- Run git diff --check.
- Run cmake --build build --config Release.
- If linking returns exit code 116, close the running/locked game executable
  before retrying; do not force-delete build output.

### Animation smoke test

1. Trigger Punch state.
2. Confirm all ten ready frames play exactly once.
3. Confirm ready does not restart between punches.
4. Count exactly ten complete four-frame body cycles.
5. Confirm body and hand always display the same frame index.
6. Move the player around all four quadrants:
   - body flips only when the player crosses its horizontal position;
   - hand uses the same flip;
   - hand continues pointing at the player above, below, left, and right.
7. Confirm hand pivot (8,7) stays attached to body point (13,43) throughout
   rotation and never orbits around the Boss center.
8. Confirm no sub-pixel separation or hand jitter.
9. Confirm the state returns to idle after the tenth punch.
10. Confirm no damage, hitbox, knockback, or projectile is produced.

## Acceptance Criteria

- Updated assets are sliced as 10, 4, and 4 frames respectively.
- Ready plays once per Punch-state entry.
- Punch body and hand render as separate synchronized layers.
- Hand pivot (8,7) remains attached to body point (13,43).
- Body and hand flip consistently while the hand still aims at the player.
- Exactly ten punch cycles occur.
- Punch remains animation-only.
- Obsolete old-sheet assumptions and redundant phase flags are removed.


