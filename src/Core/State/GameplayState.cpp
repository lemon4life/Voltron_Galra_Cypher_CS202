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
#include "Entities/Props/Pot.h"
#include "raymath.h"
#include "Core/Manager/AssetManager.h"

GameplayState::GameplayState(TeamManager* teamManager, LevelManager* levelManager, WaveManager* waveManager)
    : teamManager(teamManager), levelManager(levelManager), waveManager(waveManager) {
}

void GameplayState::Update(float deltaTime) {
    UltimateIntroManager::GetInstance().Update(deltaTime);
    if (UltimateIntroManager::GetInstance().IsPlaying()) {
        return; // Freeze gameplay while the cinematic plays
    }

    if (InputManager::IsToggleAutoAimPressed()) {
        Constants::isAutoAimEnabled = !Constants::isAutoAimEnabled;
    }

    if (GameManager::GetInstance().GetHitstopTimer() > 0.0f) {
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
        
        Paladin* active = teamManager->GetActivePaladin();
        Pot* nearestPot = nullptr;
        float closestDistSq = 50.0f * 50.0f;
        for (GameObject* obj : levelManager->GetEntities()) {
            if (obj->GetObjectType() == GameObjectType::Prop) {
                Pot* pot = dynamic_cast<Pot*>(obj);
                if (pot && !pot->IsConsumed()) {
                    float distSq = Vector2DistanceSqr(active->GetPosition(), pot->GetPosition());
                    if (distSq < closestDistSq) {
                        closestDistSq = distSq;
                        nearestPot = pot;
                    }
                }
            }
        }
        
        if (nearestPot && InputManager::IsInteractPressed()) {
            nearestPot->OnConsume(teamManager);
        }
        
        GameManager::GetInstance().UpdateProjectiles(deltaTime, teamManager);
        GameManager::GetInstance().UpdateAssists(deltaTime, teamManager);
        waveManager->Update(deltaTime, teamManager, levelManager);
        
        GameManager::GetInstance().GetComboMeter().Update(deltaTime);
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

    for (GameObject* obj : GameManager::GetInstance().GetLevelEntities()) {
        if (obj->GetObjectType() == GameObjectType::Enemy) {
            Enemy* enemy = static_cast<Enemy*>(obj);
            if (enemy->GetStatusComponent().HasEffect(EffectType::FREEZE)) {
                Texture2D tex = enemy->GetEnemyType() == EnemyType::BOSS ? AssetManager::GetInstance().GetTexture("Freeze_Big") : AssetManager::GetInstance().GetTexture("Freeze");
                if (tex.id != 0) {
                    Vector2 footPos = enemy->GetRenderFootPosition();
                    Rectangle dest = { footPos.x, footPos.y, (float)tex.width, (float)tex.height };
                    // Anchor to the bottom-center of the freeze texture
                    Vector2 origin = { tex.width / 2.0f, (float)tex.height - 5.0f };
                    DrawTexturePro(tex, { 0, 0, (float)tex.width, (float)tex.height }, dest, origin, 0.0f, WHITE);
                }
            }
        }
    }

    if (Constants::isAutoAimEnabled) {
        Paladin* activePaladin = teamManager->GetActivePaladin();
        if (activePaladin && activePaladin->GetLockedEnemy()) {
            Vector2 targetPos = activePaladin->GetLockedEnemy()->GetPosition();
            DrawCircleLines(static_cast<int>(targetPos.x), static_cast<int>(targetPos.y), 20.0f, RED);
            DrawLine(targetPos.x - 25, targetPos.y, targetPos.x + 25, targetPos.y, RED);
            DrawLine(targetPos.x, targetPos.y - 25, targetPos.x, targetPos.y + 25, RED);
        }
    } else if (InputManager::GetMode() != InputMode::KEYBOARD_ONLY) {
        Paladin* activePaladin = teamManager->GetActivePaladin();
        if (activePaladin) {
            Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), CameraManager::GetInstance().GetCamera());
            Vector2 weaponPos = activePaladin->GetWeaponPivot();
            
            Vector2 dir = Vector2Subtract(mouseWorld, weaponPos);
            float dist = Vector2Length(dir);
            if (dist > 10.0f) {
                dir = Vector2Normalize(dir);
                float dotSpacing = 15.0f;
                Color dotColor = { 255, 255, 255, (unsigned char)(255 * 0.15f) };
                for (float d = dotSpacing; d < dist; d += dotSpacing) {
                    Vector2 dotPos = Vector2Add(weaponPos, Vector2Scale(dir, d));
                    DrawCircleV(dotPos, 1.0f, dotColor);
                }
            }
        }
    }

    GameManager::GetInstance().DrawDebugOverlays(teamManager);
    
    EndMode2D();

    // UI Rendering
    float viewportScale = std::min((float)GetScreenWidth() / Constants::GAME_WIDTH, (float)GetScreenHeight() / Constants::GAME_HEIGHT);
    Camera2D uiCamera = UIUtils::CreateCenteredUICamera(viewportScale);

    BeginMode2D(uiCamera);
    
    waveManager->DrawHUD();
    
    float leftEdge = -uiCamera.offset.x / viewportScale;
    float topEdge = -uiCamera.offset.y / viewportScale;
    GameManager::GetInstance().GetComboMeter().Draw({ leftEdge + 20.0f, topEdge + 150.0f });

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
    else {
        Paladin* active = teamManager->GetActivePaladin();
        Pot* nearestPot = nullptr;
        float closestDistSq = 50.0f * 50.0f;
        for (GameObject* obj : levelManager->GetEntities()) {
            if (obj->GetObjectType() == GameObjectType::Prop) {
                Pot* pot = dynamic_cast<Pot*>(obj);
                if (pot && !pot->IsConsumed()) {
                    float distSq = Vector2DistanceSqr(active->GetPosition(), pot->GetPosition());
                    if (distSq < closestDistSq) {
                        closestDistSq = distSq;
                        nearestPot = pot;
                    }
                }
            }
        }
        
        if (nearestPot) {
            float textWidth = UIUtils::MeasureText("PixeloidSans", "Press F to consume", UIUtils::FontSize::SMALL).x;
            Rectangle background = {
                (Constants::GAME_WIDTH - textWidth) * 0.5f - 10.0f,
                Constants::GAME_HEIGHT - 44.0f,
                textWidth + 20.0f,
                28.0f
            };
            UIUtils::DrawPanel(background, Color{15, 20, 29, 220});
            UIUtils::DrawCenteredText("PixeloidSans", "Press F to consume", { background.x + background.width * 0.5f, background.y + background.height * 0.5f }, UIUtils::FontSize::SMALL, RAYWHITE);
        }
    }

    UltimateIntroManager::GetInstance().Draw();

    if (!Constants::isAutoAimEnabled && InputManager::GetMode() != InputMode::KEYBOARD_ONLY) {
        Vector2 mouseUI = GetScreenToWorld2D(GetMousePosition(), uiCamera);
        Texture2D aimTex = AssetManager::GetInstance().GetTexture("Aim");
        if (aimTex.id != 0) {
            Vector2 origin = { (float)aimTex.width / 2.0f, (float)aimTex.height / 2.0f };
            Rectangle source = { 0.0f, 0.0f, (float)aimTex.width, (float)aimTex.height };
            Rectangle dest = { mouseUI.x, mouseUI.y, (float)aimTex.width, (float)aimTex.height };
            DrawTexturePro(aimTex, source, dest, origin, 0.0f, WHITE);
        }
    }

    EndMode2D();
}
