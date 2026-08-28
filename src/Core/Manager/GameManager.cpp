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

namespace {
void DrawObjectCollisionDebug(const GameObject& object) {
    Color hitboxColor = PURPLE;
    Color collisionColor = GOLD;
    if (object.GetObjectType() == GameObjectType::Player) {
        hitboxColor = BLUE;
        collisionColor = GREEN;
    } else if (object.GetObjectType() == GameObjectType::Enemy) {
        hitboxColor = RED;
        collisionColor = ORANGE;
    }

    DrawRectangleLinesEx(
        object.GetBoundingBox(),
        Constants::DEBUG_COLLISION_LINE_THICKNESS * 2.0f,
        hitboxColor
    );
    DrawRectangleLinesEx(
        object.GetCollisionBox(),
        Constants::DEBUG_COLLISION_LINE_THICKNESS,
        collisionColor
    );
}
}

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

GameManager::~GameManager() {
    pathFindingManager.Clear();
    objectManager.Clear();
    effectManager.ClearSession();
    levelManager.ClearLevel();
}

GameManager& GameManager::GetInstance() {
    static GameManager instance;
    return instance;
}

bool GameManager::PauseGame() {
    if (currentState != GameState::HUB &&
        currentState != GameState::GAMEPLAY) {
        return false;
    }
    previousGameState = currentState;
    currentState = GameState::PAUSE;
    return true;
}

bool GameManager::ResumeGame() {
    if (currentState != GameState::PAUSE) return false;
    currentState = previousGameState;
    return true;
}

bool GameManager::IsPaused() const {
    return currentState == GameState::PAUSE;
}

GameState GameManager::GetPreviousGameState() const {
    return previousGameState;
}

GameState GameManager::GetState() const {
    return currentState;
}

void GameManager::SetState(GameState newState) {
    currentState = newState;
}

void GameManager::SetCurrentStateObj(std::unique_ptr<IGameState> state) {
    currentStateObj = std::move(state);
}

IGameState* GameManager::GetCurrentStateObj() const {
    return currentStateObj.get();
}

std::unique_ptr<IGameState> GameManager::TakeCurrentStateObj() {
    return std::move(currentStateObj);
}

GameState GameManager::GetRenderState() const {
    return IsPaused() ? previousGameState : currentState;
}

void GameManager::SetTeamManager(std::unique_ptr<TeamManager> team) {
    teamManager = std::move(team);
    objectManager.SetTeamManager(teamManager.get());
}

void GameManager::LoadLevel(const std::string& path) {
    MemoryDiagnostics::Capture("before_load_level", *this);
    pathFindingManager.Clear();
    objectManager.Clear();
    effectManager.ClearSession();
    DynamicSpawnList spawns = levelManager.LoadLevel(path);
    objectManager.SpawnAll(spawns);
    MemoryDiagnostics::Capture("after_load_level", *this);
}

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

void GameManager::ResetWorld() {
    MemoryDiagnostics::Capture("before_reset_world", *this);
    pathFindingManager.Clear();
    objectManager.Clear();
    effectManager.ClearSession();
    levelManager.ClearLevel();
    encounterManager.Reset(0, 0, 0);
    hitstopTimer = 0.0f;
    MemoryDiagnostics::Capture("after_reset_world", *this);
}

void GameManager::ResetTransientState() {
    MemoryDiagnostics::Capture("before_reset_transient", *this);
    pathFindingManager.Clear();
    objectManager.Clear();
    effectManager.ClearSession();
    hitstopTimer = 0.0f;
    hasTalkedToShiro = false;
    MemoryDiagnostics::Capture("after_reset_transient", *this);
}

void GameManager::UpdateDynamicEntities(float deltaTime) {
    pathFindingManager.Update(deltaTime);
    objectManager.UpdateEntities(deltaTime);
}

void GameManager::AddProjectile(Projectile* projectile) {
    objectManager.AddProjectile(projectile);
}

void GameManager::ClearProjectiles() {
    objectManager.ClearProjectiles();
}

void GameManager::UpdateProjectiles(float deltaTime, TeamManager*) {
    objectManager.UpdateProjectiles(deltaTime);
}

void GameManager::AddRover(std::unique_ptr<Rover> rover) {
    objectManager.AddRover(std::move(rover));
}

void GameManager::UpdateAssists(float deltaTime, TeamManager*) {
    objectManager.UpdateAssists(deltaTime);
}

void GameManager::SpawnQuintessenceOrb(Vector2 position) {
    objectManager.SpawnQuintessenceOrb(position);
}

void GameManager::UpdateOrbs(float deltaTime, TeamManager*) {
    objectManager.UpdateOrbs(deltaTime);
}

void GameManager::DrawOrbs() {
    objectManager.DrawOrbs();
}

void GameManager::ClearOrbs() {
    objectManager.ClearOrbs();
}

void GameManager::SetBulletImpactTexture(Texture2D texture) {
    effectManager.SetBulletImpactTexture(texture);
}

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

void GameManager::AddImpactEffect(Vector2 position) {
    effectManager.AddImpactEffect(position);
}

void GameManager::UpdateEffects(float deltaTime) {
    effectManager.Update(deltaTime);
}

void GameManager::DrawEffects(bool background) {
    effectManager.Draw(background);
}

void GameManager::DrawParticles() {
    effectManager.DrawParticles();
}

void GameManager::AddDepthRenderItems(
    std::vector<DepthRenderItem>& items
) {
    objectManager.AddDepthRenderItems(items);
}

void GameManager::DrawDebugOverlays(TeamManager* team) const {
    TeamManager* debugTeam = team ? team : teamManager.get();
    if (Constants::DEBUG_DRAW_ENTITY_COLLISION_BOXES && debugTeam) {
        for (const Paladin* paladin : debugTeam->GetTeam()) {
            if (paladin) DrawObjectCollisionDebug(*paladin);
        }
    }
    objectManager.DrawDebugOverlays();
    levelManager.DrawMapCollisionDebug();
    if (Constants::DEBUG_DRAW_LINE_OF_SIGHT) {
        levelManager.DrawLineOfSightDebug();
    }
}

void GameManager::OpenEnhanceMenu(PaladinId paladinId) {
    if (auto* gameplay = dynamic_cast<GameplayState*>(currentStateObj.get())) {
        gameplay->OpenEnhanceMenu(paladinId);
    }
}

bool GameManager::IsEnhanceMenuOpen() const {
    if (auto* gameplay = dynamic_cast<GameplayState*>(currentStateObj.get())) {
        return gameplay->GetEnhanceMenuUI().IsOpen();
    }
    return false;
}
