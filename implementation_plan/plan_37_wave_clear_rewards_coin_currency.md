# Wave-Clear Rewards, Coin Currency Pickups, and HUD Coin Counter

This feature implements the complete gameplay reward loop for room wave clearing in "Voltron Mission - Galra Cypher":
1. When all waves in a combat room are cleared, a reward chest teleports directly to the player's position accompanied by character spawn VFX (`AppearLight` and `AppearSmoke`) and SFX.
2. Opening the reward chest bursts animated `Coin` pickups outward in a radial arc.
3. Burst coins dynamically magnetize and home toward the active Paladin when within proximity, adding to the team's total currency upon collection.
4. A dedicated HUD coin counter is rendered using the rounded minimap panel aesthetic.

---

## User Review Required

> [!NOTE]
> * The coin world sprite is 4 frames ($8 \times 9$ pixels) from `assets/Objects/coin.png`.
> * The coin icon for HUD is loaded from `assets/UI/coin.png`.
> * Audio effect `fx_coin` is loaded from `assets/audio/SFX/Item/fx_coin.wav`.

---

## Proposed Changes

### 1. Asset & Audio Management

#### [MODIFY] [`AssetManager.cpp`](file:///Users/lemon4life/Library/CloudStorage/OneDrive-Personal/Documents/1_HCMUS/3rd/CS202/Voltron%20Mission%20Galra%20Cypher/src/Core/Manager/AssetManager.cpp)
* Load `coin_world` from `assets/Objects/coin.png`.
* Load `coin_icon` from `assets/UI/coin.png`.

#### [MODIFY] [`AudioManager.cpp`](file:///Users/lemon4life/Library/CloudStorage/OneDrive-Personal/Documents/1_HCMUS/3rd/CS202/Voltron%20Mission%20Galra%20Cypher/src/Core/Manager/AudioManager.cpp)
* Load `fx_coin` from `assets/audio/SFX/Item/fx_coin.wav`.

---

### 2. Team Currency Tracking

#### [MODIFY] [`TeamManager.h`](file:///Users/lemon4life/Library/CloudStorage/OneDrive-Personal/Documents/1_HCMUS/3rd/CS202/Voltron%20Mission%20Galra%20Cypher/include/Core/Manager/TeamManager.h) & [`TeamManager.cpp`](file:///Users/lemon4life/Library/CloudStorage/OneDrive-Personal/Documents/1_HCMUS/3rd/CS202/Voltron%20Mission%20Galra%20Cypher/src/Core/Manager/TeamManager.cpp)
* Add `int coins = 0;` member to `TeamManager`.
* Add `int GetCoins() const`, `void AddCoins(int amount)`, and `bool ConsumeCoins(int amount)`.
* Reset coins to 0 in `ResetForNewGame()`.

---

### 3. Coin Pickup Entity & Magnet Physics

#### [NEW] [`Coin.h`](file:///Users/lemon4life/Library/CloudStorage/OneDrive-Personal/Documents/1_HCMUS/3rd/CS202/Voltron%20Mission%20Galra%20Cypher/include/Entities/Items/Coin.h)
* Define `Coin` inheriting from `GameObject`.
* State variables: `velocity`, `currentFrame`, `frameTimer`, `burstTimer`, `isCollected`.
* Bounding box: $8 \times 9\text{px}$.

#### [NEW] [`Coin.cpp`](file:///Users/lemon4life/Library/CloudStorage/OneDrive-Personal/Documents/1_HCMUS/3rd/CS202/Voltron%20Mission%20Galra%20Cypher/src/Entities/Items/Coin.cpp)
* **Animation**: Cycle through 4 horizontal frames ($8 \times 9\text{px}$) at ~8 FPS.
* **Initial Burst Arc**: For the first 0.25s, drift with initial velocity + upward momentum and friction.
* **Magnet Physics**: If distance to active Paladin $< 120.0\text{f}$, smoothly interpolate velocity toward the Paladin at high speed ($320\text{px/s}$).
* **Collection**: If distance $< 16.0\text{f}$, call `TeamManager::AddCoins(1)`, play `fx_coin`, and queue removal.

---

### 4. Reward Chest Teleportation VFX & Coin Burst

#### [MODIFY] [`WaveManager.cpp`](file:///Users/lemon4life/Library/CloudStorage/OneDrive-Personal/Documents/1_HCMUS/3rd/CS202/Voltron%20Mission%20Galra%20Cypher/src/Core/Manager/WaveManager.cpp)
* When all waves in a room are completed (`dungeonCurrentWave >= dungeonTotalWaves`):
  * Capture `spawnPos = activePaladin->GetPosition()`.
  * Trigger `AppearSmoke` and `AppearLight` particle effects at `spawnPos`.
  * Play `fx_show_up` audio.
  * Instantiate and register a `Chest` entity at `spawnPos`.

#### [MODIFY] [`Chest.cpp`](file:///Users/lemon4life/Library/CloudStorage/OneDrive-Personal/Documents/1_HCMUS/3rd/CS202/Voltron%20Mission%20Galra%20Cypher/src/Entities/Props/Chest.cpp)
* When `potScaleProgress >= 1.0f`:
  * Spawn the consumable pot.
  * Spawn 5 to 10 `Coin` entities with randomized radial scatter velocities and initial upward arcs.

---

### 5. Dedicated HUD Coin Counter

#### [MODIFY] [`UIManager.h`](file:///Users/lemon4life/Library/CloudStorage/OneDrive-Personal/Documents/1_HCMUS/3rd/CS202/Voltron%20Mission%20Galra%20Cypher/include/UI/UIManager.h) & [`UIManager.cpp`](file:///Users/lemon4life/Library/CloudStorage/OneDrive-Personal/Documents/1_HCMUS/3rd/CS202/Voltron%20Mission%20Galra%20Cypher/src/UI/UIManager.cpp)
* Implement `DrawCoinHUD(Rectangle bounds, int coins)`:
  * Draw rounded panel with `ColorAlpha(Color{ 10, 10, 15, 255 }, 0.8f)` and rounded border `ColorAlpha(GRAY, 0.4f)` matching the minimap.
  * Draw `coin_icon` on the left.
  * Draw total coins on the right with formatted text.
* Position below the minimap in `DrawTeamHUD`.

---

## Verification Plan

### Automated Build Verification
* Build project: `cmake --build build --config Release`.

### Manual Gameplay Verification
1. Enter combat room and defeat all enemy waves.
2. Verify reward chest teleports to player position with `AppearLight` and `AppearSmoke` particles.
3. Approach and open chest; verify 5-10 coins burst outwards.
4. Move active Paladin near coins; verify they smoothly magnetize and collect into the HUD counter.
5. Verify HUD coin counter renders cleanly below the minimap without overlapping other HUD components.
