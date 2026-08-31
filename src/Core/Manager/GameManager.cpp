#include "Core/Manager/GameManager.h"

#include "Core/Constants.h"
#include "Core/Diagnostics/MemoryDiagnostics.h"
#include "Core/Manager/TeamManager.h"
#include "Core/State/IGameState.h"
#include "Core/State/GameplayState.h"
#include "Entities/GameObject.h"
#include "Entities/Player/Paladin.h"
#include "Entities/Projectile.h"
#include "Entities/Rover.h"

#include <algorithm>

/// Creates a GameManager instance from the supplied configuration.
GameManager::GameManager()
    : pathFindingManager(levelManager, objectManager) {
    effectManager.Configure(levelManager);
    objectManager.Configure(
        levelManager,
        pathFindingManager,
        effectManager,
        nullptr
    );
    objectManager.SetHitstopCallback(
        [this](float duration) { TriggerHitstop(duration); }
    );
}

/// Releases resources owned by this GameManager instance.
GameManager::~GameManager() {
    pathFindingManager.Clear();
    objectManager.Clear();
    effectManager.ClearSession();
    levelManager.ClearLevel();
}

/// Returns the process-wide singleton instance of this manager.
GameManager& GameManager::GetInstance() {
    static GameManager instance;
    return instance;
}

/// Pauses game.
bool GameManager::PauseGame() {
    if (currentState != GameState::HUB &&
        currentState != GameState::GAMEPLAY) {
        return false;
    }
    previousGameState = currentState;
    currentState = GameState::PAUSE;
    return true;
}

/// Resumes game.
bool GameManager::ResumeGame() {
    if (currentState != GameState::PAUSE) return false;
    currentState = previousGameState;
    return true;
}

/// Reports whether the paused condition is satisfied.
bool GameManager::IsPaused() const {
    return currentState == GameState::PAUSE;
}

/// Returns the current previous game state.
GameState GameManager::GetPreviousGameState() const {
    return previousGameState;
}

/// Returns the current state.
GameState GameManager::GetState() const {
    return currentState;
}

/// Updates the stored state.
void GameManager::SetState(GameState newState) {
    currentState = newState;
}

/// Updates the stored current state obj.
void GameManager::SetCurrentStateObj(std::unique_ptr<IGameState> state) {
    currentStateObj = std::move(state);
}

/// Returns the current current state obj.
IGameState* GameManager::GetCurrentStateObj() const {
    return currentStateObj.get();
}

/// Preserves current state for overlay.
void GameManager::PreserveCurrentStateForOverlay(GameState backgroundState) {
    if (overlayBackgroundStateObj || !currentStateObj) return;
    overlayBackgroundGameState = backgroundState;
    overlayBackgroundStateObj = std::move(currentStateObj);
}

/// Returns the current overlay background state.
IGameState* GameManager::GetOverlayBackgroundState() const {
    return overlayBackgroundStateObj.get();
}

/// Restores overlay background state.
bool GameManager::RestoreOverlayBackgroundState(GameState state) {
    if (!overlayBackgroundStateObj || state != overlayBackgroundGameState) {
        return false;
    }
    currentStateObj = std::move(overlayBackgroundStateObj);
    return true;
}

/// Clears overlay background state.
void GameManager::ClearOverlayBackgroundState() {
    overlayBackgroundStateObj.reset();
}

/// Reports whether this component has overlay background state.
bool GameManager::HasOverlayBackgroundState() const {
    return static_cast<bool>(overlayBackgroundStateObj);
}

/// Updates the stored team manager.
void GameManager::SetTeamManager(std::unique_ptr<TeamManager> team) {
    teamManager = std::move(team);
    objectManager.SetTeamManager(teamManager.get());
}

/// Replaces the active world with a static/layered level.
/// LevelManager owns map geometry; its dynamic spawn requests are handed to
/// ObjectManager so moving entities never become LevelManager-owned objects.
void GameManager::LoadLevel(const std::string& path) {
    MemoryDiagnostics::Capture("before_load_level", *this);
    pathFindingManager.Clear();
    objectManager.Clear();
    effectManager.ClearSession();
    DynamicSpawnList spawns = levelManager.LoadLevel(path);
    objectManager.SpawnAll(spawns);
    MemoryDiagnostics::Capture("after_load_level", *this);
}

/// Builds the current procedural floor, bakes its rooms into level layers,
/// creates special-room entities, and transfers dynamic spawns to ObjectManager.
/// The active Paladin is then placed at the generated spawn-room center.
void GameManager::GenerateDungeon() {
    MemoryDiagnostics::Capture("before_generate_dungeon", *this);
    pathFindingManager.Clear();
    objectManager.Clear();
    effectManager.ClearSession();
    DynamicSpawnList spawns = levelManager.GenerateDungeon(currentFloor);
    objectManager.SpawnAll(spawns);

    if (teamManager && teamManager->GetActivePaladin() &&
        levelManager.GetLevelMap().spawnRoom) {
        Rectangle bounds =
            levelManager.GetLevelMap().spawnRoom->GetWorldBounds();
        teamManager->GetActivePaladin()->SetPosition({
            bounds.x + bounds.width * 0.5f,
            bounds.y + bounds.height * 0.5f
        });
        teamManager->StartSpawnAnimation();
    }
    MemoryDiagnostics::Capture("after_generate_dungeon", *this);
}

/// Clears path, object, effect, level, wave, hitstop, and overlay session data.
/// Manager instances remain alive so references held by states stay valid.
void GameManager::ResetWorld() {
    MemoryDiagnostics::Capture("before_reset_world", *this);
    pathFindingManager.Clear();
    objectManager.Clear();
    effectManager.ClearSession();
    levelManager.ClearLevel();
    waveManager.Reset(0, 0, 0);
    hitstopTimer = 0.0f;
    ClearOverlayBackgroundState();
    MemoryDiagnostics::Capture("after_reset_world", *this);
}

/// Resets transient state.
void GameManager::ResetTransientState() {
    MemoryDiagnostics::Capture("before_reset_transient", *this);
    pathFindingManager.Clear();
    objectManager.Clear();
    effectManager.ClearSession();
    hitstopTimer = 0.0f;
    hasTalkedToShiro = false;
    MemoryDiagnostics::Capture("after_reset_transient", *this);
}

/// A checkpoint applies only to a generated floor with a living team.
bool GameManager::HasCheckpointableMission() const {
    return levelManager.IsProceduralDungeon() && teamManager &&
        teamManager->GetActivePaladin() && !teamManager->IsTeamDead() &&
        currentFloor >= 1 && currentFloor <= MAX_FLOORS;
}

/// Captures stable ownership only. Enemies, bullets, effects, paths, and current
/// wave counters are intentionally absent so combat restarts from room entry.
MissionSaveData GameManager::CaptureCheckpointState() const {
    MissionSaveData saved;
    saved.floor = currentFloor;
    saved.talkedToShiro = hasTalkedToShiro;
    saved.autoAim = Constants::isAutoAimEnabled;
    saved.level = levelManager.CaptureCheckpointState();
    if (teamManager) saved.team = teamManager->CaptureCheckpointState();
    saved.utilityObjects = objectManager.CaptureCheckpointObjects();
    return saved;
}

/// Rebuilds the static floor before the team and utility objects, then resets
/// the wave controller so the next battle begins through its normal entry flow.
bool GameManager::RestoreCheckpointState(const MissionSaveData& saved) {
    pathFindingManager.Clear();
    objectManager.Clear();
    effectManager.ClearSession();
    ClearOverlayBackgroundState();
    if (!teamManager ||
        !levelManager.RestoreCheckpointState(saved.level) ||
        !teamManager->RestoreCheckpointState(saved.team) ||
        !objectManager.RestoreCheckpointObjects(saved.utilityObjects)) {
        pathFindingManager.Clear();
        objectManager.Clear();
        levelManager.ClearLevel();
        waveManager.Reset(0, 0, 0);
        return false;
    }

    currentFloor = std::clamp(saved.floor, 1, MAX_FLOORS);
    hasTalkedToShiro = saved.talkedToShiro;
    Constants::isAutoAimEnabled = saved.autoAim;
    waveManager.Reset(0, 0, 0);
    hitstopTimer = 0.0f;
    previousGameState = GameState::GAMEPLAY;
    currentState = GameState::GAMEPLAY;
    currentStateObj.reset();
    teamManager->RefreshAimStrategies();
    teamManager->NotifyObservers();
    return true;
}

/// Advances global pathfinding first, then lets each enemy consume its latest
/// route. Additions/removals remain queued until GameplayState commits them.
void GameManager::UpdateDynamicEntities(float deltaTime) {
    pathFindingManager.Update(deltaTime);
    objectManager.UpdateEntities(deltaTime);
}

/// Adds projectile.
void GameManager::AddProjectile(std::unique_ptr<Projectile> projectile) {
    objectManager.AddProjectile(std::move(projectile));
}

/// Clears projectiles.
void GameManager::ClearProjectiles() {
    objectManager.ClearProjectiles();
}

/// Advances projectiles, resolves their collisions, and removes inactive shots safely.
void GameManager::UpdateProjectiles(float deltaTime, TeamManager*) {
    objectManager.UpdateProjectiles(deltaTime);
}

/// Adds rover.
void GameManager::AddRover(std::unique_ptr<Rover> rover) {
    objectManager.AddRover(std::move(rover));
}

/// Updates assists.
void GameManager::UpdateAssists(float deltaTime, TeamManager*) {
    objectManager.UpdateAssists(deltaTime);
}

/// Spawns quintessence orb.
void GameManager::SpawnQuintessenceOrb(Vector2 position) {
    objectManager.SpawnQuintessenceOrb(position);
}

/// Updates orbs.
void GameManager::UpdateOrbs(float deltaTime, TeamManager*) {
    objectManager.UpdateOrbs(deltaTime);
}

/// Renders orbs.
void GameManager::DrawOrbs() {
    objectManager.DrawOrbs();
}

/// Clears orbs.
void GameManager::ClearOrbs() {
    objectManager.ClearOrbs();
}

/// Updates the stored bullet impact texture.
void GameManager::SetBulletImpactTexture(Texture2D texture) {
    effectManager.SetBulletImpactTexture(texture);
}

/// Adds effect.
void GameManager::AddEffect(
    Vector2 position,
    Texture2D texture,
    int frames,
    float lifetime,
    bool drawBehind,
    Color tint
) {
    effectManager.AddEffect(
        position,
        texture,
        frames,
        lifetime,
        drawBehind,
        tint
    );
}

/// Adds impact effect.
void GameManager::AddImpactEffect(Vector2 position) {
    effectManager.AddImpactEffect(position);
}

/// Updates effects.
void GameManager::UpdateEffects(float deltaTime) {
    effectManager.Update(deltaTime);
}

/// Renders effects.
void GameManager::DrawEffects(bool background) {
    effectManager.Draw(background);
}

/// Renders particles.
void GameManager::DrawParticles() {
    effectManager.DrawParticles();
}

/// Adds depth render items.
void GameManager::AddDepthRenderItems(
    std::vector<DepthRenderItem>& items
) {
    objectManager.AddDepthRenderItems(items);
}

/// Renders debug overlays.
void GameManager::DrawDebugOverlays(TeamManager* team) const {
    TeamManager* debugTeam = team ? team : teamManager.get();
    if (Constants::DEBUG_DRAW_ENTITY_COLLISION_BOXES && debugTeam) {
        for (const Paladin* paladin : debugTeam->GetTeam()) {
            if (!paladin) continue;
            DrawRectangleLinesEx(
                paladin->GetBoundingBox(),
                Constants::DEBUG_COLLISION_LINE_THICKNESS * 2.0f,
                BLUE
            );
            DrawRectangleLinesEx(
                paladin->GetCollisionBox(),
                Constants::DEBUG_COLLISION_LINE_THICKNESS,
                GREEN
            );
        }
    }
    objectManager.DrawDebugOverlays();
    levelManager.DrawMapCollisionDebug();
    if (Constants::DEBUG_DRAW_LINE_OF_SIGHT) {
        levelManager.DrawLineOfSightDebug();
    }
}

/// Opens enhance menu.
void GameManager::OpenEnhanceMenu(PaladinId paladinId) {
    if (auto* gameplay = dynamic_cast<GameplayState*>(currentStateObj.get())) {
        gameplay->OpenEnhanceMenu(paladinId);
    }
}

/// Reports whether the enhance menu open condition is satisfied.
bool GameManager::IsEnhanceMenuOpen() const {
    if (auto* gameplay = dynamic_cast<GameplayState*>(currentStateObj.get())) {
        return gameplay->GetEnhanceMenuUI().IsOpen();
    }
    return false;
}
