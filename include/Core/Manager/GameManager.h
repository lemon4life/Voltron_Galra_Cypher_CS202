#pragma once

#include "Core/DepthRenderItem.h"
#include "Core/Manager/EffectManager.h"
#include "Core/Manager/EncounterManager.h"
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

class GameManager {
private:
    GameState currentState = GameState::MAIN_MENU;
    GameState previousGameState = GameState::MAIN_MENU;
    std::unique_ptr<IGameState> currentStateObj;

    int targetFPS = 0;
    float hitstopTimer = 0.0f;
    int currentFloor = 1;
    bool hasTalkedToShiro = false;
    ComboMeter comboMeter;

    LevelManager levelManager;
    ObjectManager objectManager;
    PathFindingManager pathFindingManager;
    EffectManager effectManager;
    EncounterManager encounterManager;
    std::unique_ptr<TeamManager> teamManager;

    GameManager();
    ~GameManager();

public:
    static constexpr int MAX_FLOORS = 5;
    static GameManager& GetInstance();

    GameManager(const GameManager&) = delete;
    GameManager& operator=(const GameManager&) = delete;
    GameManager(GameManager&&) = delete;
    GameManager& operator=(GameManager&&) = delete;

    void SetState(GameState newState);
    GameState GetState() const;
    void SetCurrentStateObj(std::unique_ptr<IGameState> state);
    IGameState* GetCurrentStateObj() const;
    std::unique_ptr<IGameState> TakeCurrentStateObj();
    bool PauseGame();
    bool ResumeGame();
    bool IsPaused() const;
    GameState GetPreviousGameState() const;
    GameState GetRenderState() const;
    void OpenEnhanceMenu(PaladinId paladinId);
    bool IsEnhanceMenuOpen() const;

    void TriggerHitstop(float duration) { hitstopTimer = duration; }
    float GetHitstopTimer() const { return hitstopTimer; }
    void ClearHitstop() { hitstopTimer = 0.0f; }
    void UpdateHitstop(float deltaTime) {
        if (hitstopTimer > 0.0f) hitstopTimer -= deltaTime;
    }

    int GetCurrentFloor() const { return currentFloor; }
    void AdvanceFloorCount() { ++currentFloor; }
    void ResetFloorCount() { currentFloor = 1; }

    bool HasTalkedToShiro() const { return hasTalkedToShiro; }
    void SetTalkedToShiro(bool talked) { hasTalkedToShiro = talked; }
    ComboMeter& GetComboMeter() { return comboMeter; }

    void UpdateTargetFPS(int fps) { targetFPS = fps; SetTargetFPS(fps); }
    int GetTargetFPS() const { return targetFPS; }

    LevelManager* GetLevelManager() { return &levelManager; }
    const LevelManager* GetLevelManager() const { return &levelManager; }
    ObjectManager& GetObjectManager() { return objectManager; }
    const ObjectManager& GetObjectManager() const { return objectManager; }
    PathFindingManager& GetPathFindingManager() {
        return pathFindingManager;
    }
    const PathFindingManager& GetPathFindingManager() const {
        return pathFindingManager;
    }
    EffectManager& GetEffectManager() { return effectManager; }
    const EffectManager& GetEffectManager() const { return effectManager; }
    EncounterManager& GetEncounterManager() { return encounterManager; }
    TeamManager* GetTeamManager() const { return teamManager.get(); }
    void SetTeamManager(std::unique_ptr<TeamManager> team);

    float GetLevelWidth() const { return levelManager.GetLevelWidth(); }
    float GetLevelHeight() const { return levelManager.GetLevelHeight(); }

    void LoadLevel(const std::string& path);
    void GenerateDungeon();
    void ResetWorld();
    void ResetTransientState();

    void UpdateDynamicEntities(float deltaTime);
    void AddDepthRenderItems(std::vector<DepthRenderItem>& items);
    void DrawDebugOverlays(TeamManager* team = nullptr) const;

    // Transitional creation adapters used by existing entity code.
    void AddProjectile(Projectile* projectile);
    void ClearProjectiles();
    void UpdateProjectiles(float deltaTime, TeamManager* team = nullptr);
    void AddRover(std::unique_ptr<Rover> rover);
    void UpdateAssists(float deltaTime, TeamManager* team = nullptr);
    void SpawnQuintessenceOrb(Vector2 position);
    void UpdateOrbs(float deltaTime, TeamManager* team = nullptr);
    void DrawOrbs();
    void ClearOrbs();
    void SetBulletImpactTexture(Texture2D texture);
    void AddEffect(
        Vector2 position,
        Texture2D texture,
        int frames,
        float lifetime,
        bool drawBehind = false,
        Color tint = WHITE
    );
    void AddImpactEffect(Vector2 position);
    void UpdateEffects(float deltaTime);
    void DrawEffects(bool background);
    void DrawParticles();
};
