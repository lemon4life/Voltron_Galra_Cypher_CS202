#include "Core/State/GameplayState.h"
#include "Core/Manager/UltimateIntroManager.h"
#include "Core/Manager/ParticleManager.h"
#include "Core/Manager/CameraManager.h"
#include "Core/Manager/InputManager.h"
#include "Core/Manager/DecalManager.h"
#include "Core/Constants.h"
#include "Entities/Player/Paladin.h"
#include "Entities/Enemy.h"
#include "UI/MinimapRenderer.h"
#include "UI/UIUtils.h"

GameplayState::GameplayState(TeamManager* teamManager, LevelManager* levelManager, WaveManager* waveManager)
    : teamManager(teamManager), levelManager(levelManager), waveManager(waveManager) {
}

void GameplayState::Update(float deltaTime) {
    if (InputManager::IsToggleAutoAimPressed()) {
        Constants::isAutoAimEnabled = !Constants::isAutoAimEnabled;
    }

    if (UltimateIntroManager::GetInstance().IsPlaying()) {
        UltimateIntroManager::GetInstance().Update(deltaTime);
    } else if (GameManager::GetInstance().GetHitstopTimer() > 0.0f) {
        GameManager::GetInstance().UpdateHitstop(deltaTime);
    } else {
        levelManager->UpdateLevel(deltaTime, teamManager->GetActivePaladin()->GetPosition());
        if (levelManager->NeedsPlayerNudge()) {
            teamManager->GetActivePaladin()->SetPosition(levelManager->ConsumeNudge());
        }
        teamManager->Update(deltaTime);
        
        if (levelManager->IsPlayerInExitRoom(teamManager->GetActivePaladin()->GetPosition())) {
            if (InputManager::IsInteractPressed()) {
                GameManager::GetInstance().AdvanceFloorCount();
                if (GameManager::GetInstance().GetCurrentFloor() > GameManager::MAX_FLOORS) {
                    GameManager::GetInstance().SetState(GameState::VICTORY);
                } else {
                    GameManager::GetInstance().ClearProjectiles();
                    levelManager->GenerateDungeon(teamManager);
                    waveManager->Reset(0, 0, 0);
                }
            }
        }
        
        GameManager::GetInstance().UpdateProjectiles(deltaTime, teamManager);
        GameManager::GetInstance().UpdateAssists(deltaTime, teamManager);
        waveManager->Update(deltaTime, teamManager, levelManager);
    }
    GameManager::GetInstance().UpdateOrbs(deltaTime, teamManager);
    DecalManager::GetInstance().Update(deltaTime);
    GameManager::GetInstance().UpdateEffects(deltaTime);
    ParticleManager::GetInstance().Update(deltaTime);
}

void GameplayState::Draw() {
    BeginMode2D(CameraManager::GetInstance().GetRenderCamera());
    levelManager->DrawLevelBase();
    DecalManager::GetInstance().Draw();

    GameManager::GetInstance().DrawEffects(true);

    std::vector<DepthRenderItem> renderItems;
    
    if (teamManager && teamManager->GetActivePaladin()) {
        teamManager->AddDepthRenderItems(renderItems);
    }
    
    GameManager::GetInstance().AddDepthRenderItems(renderItems);
    levelManager->GetDepthRenderItems(renderItems);

    std::sort(
        renderItems.begin(),
        renderItems.end(),
        [](const DepthRenderItem& a, const DepthRenderItem& b) {
            return a.ySort < b.ySort;
        }
    );

    for (const auto& item : renderItems) {
        if (item.drawFunc) {
            item.drawFunc();
        }
    }

    GameManager::GetInstance().DrawEffects(false);
    GameManager::GetInstance().DrawOrbs();
    ParticleManager::GetInstance().Draw();

    if (Constants::isAutoAimEnabled) {
        Paladin* activePaladin = teamManager->GetActivePaladin();
        if (activePaladin && activePaladin->GetLockedEnemy()) {
            Vector2 targetPos = activePaladin->GetLockedEnemy()->GetPosition();
            DrawCircleLines(static_cast<int>(targetPos.x), static_cast<int>(targetPos.y), 20.0f, RED);
            DrawLine(targetPos.x - 25, targetPos.y, targetPos.x + 25, targetPos.y, RED);
            DrawLine(targetPos.x, targetPos.y - 25, targetPos.x, targetPos.y + 25, RED);
        }
    } else if (InputManager::GetMode() != InputMode::KEYBOARD_ONLY) {
        Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), CameraManager::GetInstance().GetCamera());
        DrawCircleLines(static_cast<int>(mouseWorld.x), static_cast<int>(mouseWorld.y), 10.0f, GREEN);
        DrawLine(mouseWorld.x - 15, mouseWorld.y, mouseWorld.x + 15, mouseWorld.y, GREEN);
        DrawLine(mouseWorld.x, mouseWorld.y - 15, mouseWorld.x, mouseWorld.y + 15, GREEN);
        DrawCircle(static_cast<int>(mouseWorld.x), static_cast<int>(mouseWorld.y), 2.0f, GREEN);
    }

    GameManager::GetInstance().DrawDebugOverlays(teamManager);
    
    EndMode2D();

    // UI Rendering
    float viewportScale = std::min((float)GetScreenWidth() / Constants::GAME_WIDTH, (float)GetScreenHeight() / Constants::GAME_HEIGHT);
    Camera2D uiCamera = UIUtils::CreateCenteredUICamera(viewportScale);

    BeginMode2D(uiCamera);
    
    waveManager->DrawHUD();

    if (levelManager->IsProceduralDungeon()) {
        int currentGridX = 3;
        int currentGridY = 3;
        auto paladin = teamManager->GetActivePaladin();
        if (paladin) {
            float tileW = Constants::RENDER_TILE_SIZE;
            int roomOuterSize = Constants::MAX_ROOM_TILE_SIZE + Constants::CORRIDOR_LENGTH;
            currentGridX = (int)(paladin->GetPosition().x / (roomOuterSize * tileW));
            currentGridY = (int)(paladin->GetPosition().y / (roomOuterSize * tileW));
        }
        float padding = 10.0f;
        float minimapSize = 100.0f; // Width and height of minimap area
        float anchorX = (GetScreenWidth() - uiCamera.offset.x) / viewportScale - minimapSize - padding;
        float anchorY = padding - uiCamera.offset.y / viewportScale;
        MinimapRenderer::Draw(
            levelManager->GetLevelMap(),
            currentGridX,
            currentGridY,
            { anchorX, anchorY },
            GameManager::GetInstance().GetCurrentFloor()
        );
    }

    if (levelManager->IsPlayerInExitRoom(teamManager->GetActivePaladin()->GetPosition())) {
        float textWidth = UIUtils::MeasureText("PixeloidSans", "Press F to go to the next floor", UIUtils::FontSize::SMALL).x;
        Rectangle background = {
            (Constants::GAME_WIDTH - textWidth) * 0.5f - 10.0f,
            Constants::GAME_HEIGHT - 44.0f,
            textWidth + 20.0f,
            28.0f
        };
        UIUtils::DrawPanel(background, Color{15, 20, 29, 220});
        UIUtils::DrawCenteredText("PixeloidSans", "Press F to go to the next floor", { background.x + background.width * 0.5f, background.y + background.height * 0.5f }, UIUtils::FontSize::SMALL, RAYWHITE);
    }

    EndMode2D();
}
