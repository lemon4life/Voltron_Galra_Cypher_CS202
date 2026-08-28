# Implementation Plan: Complete the Observer Pattern (TeamManager -> UIManager)

Complete the implementation of the **Observer Pattern** across `TeamManager` (Subject) and `UIManager` (Observer). Currently, `UIManager::OnPlayerStatsChanged` is an empty stub (`{}`) marked "Obsolete". This plan establishes a genuine, event-driven decoupled pipeline where `TeamManager` publishes stat snapshots and `UIManager` renders the player and team HUDs from its cached observer data.

## User Review Required

> [!IMPORTANT]
> This refactor changes `IObserver.h` to pass structured stat snapshots (`PlayerStatsSnapshot` and `TeamStatsSnapshot`) rather than the obsolete individual primitive arguments. `UIManager` will store these snapshots and use them for all HUD rendering (health bars, ghost bars, EX energy, and Quintessence gauges), ensuring the Observer Pattern is fully functional and defensible for grading.

---

## Proposed Changes

### Core Interface & Subject Layer

#### [MODIFY] [IObserver.h](file:///Users/lemon4life/Library/CloudStorage/OneDrive-Personal/Documents/1_HCMUS/3rd/CS202/Voltron%20Mission%20Galra%20Cypher/include/Core/IObserver.h)
- Define `PlayerStatsSnapshot` struct containing:
  - `int slotIndex`
  - `int health`, `int maxHealth`
  - `float displayedHp`, `float ghostHp`
  - `float exEnergy`, `float displayedEx`, `float maxEx`
  - `float skillCost`
  - `bool isDowned`
- Define `TeamStatsSnapshot` struct containing:
  - `int activeIndex`
  - `int sharedArmor`, `int maxSharedArmor`
  - `float currentQuintessence`, `float displayedQuintessence`, `float maxQuintessence`
- Update `IObserver` interface:
  ```cpp
  class IObserver {
  public:
      virtual ~IObserver() = default;
      virtual void OnPlayerStatsChanged(const PlayerStatsSnapshot& stats, int slotIndex) = 0;
      virtual void OnTeamStatsChanged(const TeamStatsSnapshot& stats) = 0;
  };
  ```

#### [MODIFY] [TeamManager.h](file:///Users/lemon4life/Library/CloudStorage/OneDrive-Personal/Documents/1_HCMUS/3rd/CS202/Voltron%20Mission%20Galra%20Cypher/include/Core/Manager/TeamManager.h) & [TeamManager.cpp](file:///Users/lemon4life/Library/CloudStorage/OneDrive-Personal/Documents/1_HCMUS/3rd/CS202/Voltron%20Mission%20Galra%20Cypher/src/Core/Manager/TeamManager.cpp)
- In `TeamManager::NotifyObservers()`:
  - Build `PlayerStatsSnapshot` for each paladin in `team` and dispatch `OnPlayerStatsChanged(snapshot, i)` to all registered observers.
  - Build `TeamStatsSnapshot` and dispatch `OnTeamStatsChanged(teamSnapshot)` to all registered observers.
- Trigger `NotifyObservers()` on key lifecycle events:
  - At the end of `TeamManager::Update()` (when lerping `displayedQuintessence` and `Paladin` displayed HP/EX).
  - In `SwapCharacter()`, `SwapCharacterToIndex()`, and `SwapDueToDeath()`.
  - In `AddQuintessence()` and `ConsumeQuintessence()`.
  - In `TakeArmorDamage()` and `ResetForNewGame()`.

---

### Player Entity Layer

#### [MODIFY] [Paladin.cpp](file:///Users/lemon4life/Library/CloudStorage/OneDrive-Personal/Documents/1_HCMUS/3rd/CS202/Voltron%20Mission%20Galra%20Cypher/src/Entities/Player/Paladin.cpp)
- Ensure `Paladin::TakeDamage()`, `Heal()`, `ResetStats()`, `AddExEnergy()`, and `UseSkill()` notify observers through `teamManager->NotifyObservers()` if `teamManager` is non-null.

---

### UI & Observer Layer

#### [MODIFY] [UIManager.h](file:///Users/lemon4life/Library/CloudStorage/OneDrive-Personal/Documents/1_HCMUS/3rd/CS202/Voltron%20Mission%20Galra%20Cypher/include/UI/UIManager.h) & [UIManager.cpp](file:///Users/lemon4life/Library/CloudStorage/OneDrive-Personal/Documents/1_HCMUS/3rd/CS202/Voltron%20Mission%20Galra%20Cypher/src/UI/UIManager.cpp)
- Add cached members to `UIManager`:
  - `std::vector<PlayerStatsSnapshot> cachedPlayerStats;`
  - `TeamStatsSnapshot cachedTeamStats;`
  - `bool hasReceivedStats = false;`
- Implement `OnPlayerStatsChanged(const PlayerStatsSnapshot& stats, int slotIndex) override`:
  - Ensure `cachedPlayerStats` is resized appropriately and store `cachedPlayerStats[slotIndex] = stats;`.
  - Set `hasReceivedStats = true;`.
- Implement `OnTeamStatsChanged(const TeamStatsSnapshot& stats) override`:
  - Store `cachedTeamStats = stats;`.
  - Set `hasReceivedStats = true;`.
- Refactor `UIManager::DrawTeamHUD`:
  - When `hasReceivedStats` is true, read active/inactive HP, ghost HP, real HP, EX energy, EX checkpoints, HP text (`"%d/%d"`), and Quintessence directly from `cachedPlayerStats` and `cachedTeamStats` instead of querying the `Paladin` pointers directly.

---

### Wiring / Application Layer

#### [MODIFY] [GameApplication.cpp](file:///Users/lemon4life/Library/CloudStorage/OneDrive-Personal/Documents/1_HCMUS/3rd/CS202/Voltron%20Mission%20Galra%20Cypher/src/Core/GameApplication.cpp)
- In `GameApplication::RunLoop()`, after `uiManager.SetTeamManager(teamManager);`:
  - Call `teamManager->AddObserver(&uiManager);`
  - Call `teamManager->NotifyObservers();` to seed initial stat snapshots immediately.

---

## Verification Plan

### Automated Build Verification
1. Build the project using CMake:
   ```bash
   cmake --build build --config Release
   ```
2. Verify zero compilation or linkage warnings/errors.

### Manual Verification
1. Launch `build/VoltronMissionGalraCypher` (or run executable).
2. Verify in **Hub**:
   - Initial active paladin portrait, health bar, and EX bar render properly.
   - Character selection / stands update the portraits and HUD.
3. Verify in **Gameplay Combat**:
   - Taking damage immediately updates the HUD health bar and ghost HP bar through observer notifications.
   - Using skills consumes EX energy and reflects on the EX bar.
   - Collecting Quintessence orbs increments the purple Quintessence gauge and updates checkpoints.
   - Swapping characters (`Tab`) smoothly switches the active portrait and updates stats.
   - Downed paladin state transitions properly and swaps characters.
