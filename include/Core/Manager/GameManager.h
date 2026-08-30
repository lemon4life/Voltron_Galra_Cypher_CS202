#pragma once

#include "Core/DepthRenderItem.h"
#include "Core/Manager/EffectManager.h"
#include "Core/Manager/WaveManager.h"
#include "Core/Manager/LevelManager.h"
#include "Core/Manager/ObjectManager.h"
#include "Core/Manager/PathFindingManager.h"
#include "UI/ComboMeter.h"
#include "raylib.h"

#include <memory>
#include <string>
#include <vector>

enum class GameState {
    MAIN_MENU,
    HUB,
    GAMEPLAY,
    PAUSE,
    SETTINGS,
    ROOM_EDITOR,
    GAME_OVER,
    VICTORY
};

class GameObject;
class IGameState;
class Projectile;
class Rover;
class TeamManager;
enum class PaladinId;

// Design Patterns - Singleton, Facade, Service Locator, and State Context:
// The single GameManager owns the core managers with RAII and exposes high-level
// world/session operations as a Facade. Get*Manager accessors make it a Service
// Locator. currentStateObj is the Context that delegates to IGameState objects.
class GameManager {
private:
    GameState currentState = GameState::MAIN_MENU;
    GameState previousGameState = GameState::MAIN_MENU;
    std::unique_ptr<IGameState> currentStateObj;
    std::unique_ptr<IGameState> overlayBackgroundStateObj;
    GameState overlayBackgroundGameState = GameState::MAIN_MENU;

    int targetFPS = 0;
    float hitstopTimer = 0.0f;
    int currentFloor = 1;
    bool hasTalkedToShiro = false;
    ComboMeter comboMeter;

    LevelManager levelManager;
    ObjectManager objectManager;
    PathFindingManager pathFindingManager;
    EffectManager effectManager;
    WaveManager waveManager;
    std::unique_ptr<TeamManager> teamManager;

    /// Creates a GameManager instance from the supplied configuration.
    GameManager();
    /// Releases resources owned by this GameManager instance.
    ~GameManager();

public:
    static constexpr int MAX_FLOORS = 5;
    /// Returns the process-wide singleton instance of this manager.
    static GameManager& GetInstance();

    /// Creates a GameManager instance from the supplied configuration.
    GameManager(const GameManager&) = delete;
    GameManager& operator=(const GameManager&) = delete;
    /// Creates a GameManager instance from the supplied configuration.
    GameManager(GameManager&&) = delete;
    GameManager& operator=(GameManager&&) = delete;

    /// Updates the stored state.
    void SetState(GameState newState);
    /// Returns the current state.
    GameState GetState() const;
    /// Updates the stored current state obj.
    void SetCurrentStateObj(std::unique_ptr<IGameState> state);
    /// Returns the current current state obj.
    IGameState* GetCurrentStateObj() const;
    /// Preserves current state for overlay.
    void PreserveCurrentStateForOverlay(GameState backgroundState);
    /// Returns the current overlay background state.
    IGameState* GetOverlayBackgroundState() const;
    /// Restores overlay background state.
    bool RestoreOverlayBackgroundState(GameState state);
    /// Clears overlay background state.
    void ClearOverlayBackgroundState();
    /// Reports whether this component has overlay background state.
    bool HasOverlayBackgroundState() const;
    /// Pauses game.
    bool PauseGame();
    /// Resumes game.
    bool ResumeGame();
    /// Reports whether the paused condition is satisfied.
    bool IsPaused() const;
    /// Returns the current previous game state.
    GameState GetPreviousGameState() const;
    /// Opens enhance menu.
    void OpenEnhanceMenu(PaladinId paladinId);
    /// Reports whether the enhance menu open condition is satisfied.
    bool IsEnhanceMenuOpen() const;

    /// Triggers hitstop.
    void TriggerHitstop(float duration) { hitstopTimer = duration; }
    /// Returns the current hitstop timer.
    float GetHitstopTimer() const { return hitstopTimer; }
    /// Clears hitstop.
    void ClearHitstop() { hitstopTimer = 0.0f; }
    /// Updates hitstop.
    void UpdateHitstop(float deltaTime) {
        if (hitstopTimer > 0.0f) hitstopTimer -= deltaTime;
    }

    /// Returns the current current floor.
    int GetCurrentFloor() const { return currentFloor; }
    /// Advances floor count.
    void AdvanceFloorCount() { ++currentFloor; }
    /// Resets floor count.
    void ResetFloorCount() { currentFloor = 1; }

    /// Reports whether this component has talked to shiro.
    bool HasTalkedToShiro() const { return hasTalkedToShiro; }
    /// Updates the stored talked to shiro.
    void SetTalkedToShiro(bool talked) { hasTalkedToShiro = talked; }
    /// Returns the current combo meter.
    ComboMeter& GetComboMeter() { return comboMeter; }

    /// Updates target fps.
    void UpdateTargetFPS(int fps) { targetFPS = fps; SetTargetFPS(fps); }
    /// Returns the current target fps.
    int GetTargetFPS() const { return targetFPS; }

    /// Returns the current level manager.
    LevelManager* GetLevelManager() { return &levelManager; }
    /// Returns the current level manager.
    const LevelManager* GetLevelManager() const { return &levelManager; }
    /// Returns the current object manager.
    ObjectManager& GetObjectManager() { return objectManager; }
    /// Returns the current object manager.
    const ObjectManager& GetObjectManager() const { return objectManager; }
    /// Returns the current path finding manager.
    PathFindingManager& GetPathFindingManager() {
        return pathFindingManager;
    }
    /// Returns the current path finding manager.
    const PathFindingManager& GetPathFindingManager() const {
        return pathFindingManager;
    }
    /// Returns the current effect manager.
    EffectManager& GetEffectManager() { return effectManager; }
    /// Returns the current effect manager.
    const EffectManager& GetEffectManager() const { return effectManager; }
    /// Returns the current wave manager.
    WaveManager& GetWaveManager() { return waveManager; }
    /// Returns the current team manager.
    TeamManager* GetTeamManager() const { return teamManager.get(); }
    /// Updates the stored team manager.
    void SetTeamManager(std::unique_ptr<TeamManager> team);

    /// Returns the current level width.
    float GetLevelWidth() const { return levelManager.GetLevelWidth(); }
    /// Returns the current level height.
    float GetLevelHeight() const { return levelManager.GetLevelHeight(); }

    /// Clears the active world, loads a level definition, and creates its runtime objects.
    void LoadLevel(const std::string& path);
    /// Builds the current procedural floor, bakes its rooms into level layers,
    /// creates special-room entities, and exposes all resulting dynamic spawns.
    void GenerateDungeon();
    /// Resets world.
    void ResetWorld();
    /// Resets transient state.
    void ResetTransientState();

    /// Advances managed entities and commits lifecycle changes without invalidating active iteration.
    void UpdateDynamicEntities(float deltaTime);
    /// Adds depth render items.
    void AddDepthRenderItems(std::vector<DepthRenderItem>& items);
    /// Renders debug overlays.
    void DrawDebugOverlays(TeamManager* team = nullptr) const;

    // Transitional creation adapters used by existing entity code.
    /// Adds projectile.
    void AddProjectile(std::unique_ptr<Projectile> projectile);
    /// Clears projectiles.
    void ClearProjectiles();
    /// Advances projectiles, resolves their collisions, and removes inactive shots safely.
    void UpdateProjectiles(float deltaTime, TeamManager* team = nullptr);
    /// Adds rover.
    void AddRover(std::unique_ptr<Rover> rover);
    /// Updates assists.
    void UpdateAssists(float deltaTime, TeamManager* team = nullptr);
    /// Spawns quintessence orb.
    void SpawnQuintessenceOrb(Vector2 position);
    /// Updates orbs.
    void UpdateOrbs(float deltaTime, TeamManager* team = nullptr);
    /// Renders orbs.
    void DrawOrbs();
    /// Clears orbs.
    void ClearOrbs();
    /// Updates the stored bullet impact texture.
    void SetBulletImpactTexture(Texture2D texture);
    /// Adds effect.
    void AddEffect(
        Vector2 position,
        Texture2D texture,
        int frames,
        float lifetime,
        bool drawBehind = false,
        Color tint = WHITE
    );
    /// Adds impact effect.
    void AddImpactEffect(Vector2 position);
    /// Updates effects.
    void UpdateEffects(float deltaTime);
    /// Renders effects.
    void DrawEffects(bool background);
    /// Renders particles.
    void DrawParticles();
};
