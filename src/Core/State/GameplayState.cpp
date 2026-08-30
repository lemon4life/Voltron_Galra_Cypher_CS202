#include "Core/State/GameplayState.h"
#include "Core/Manager/UltimateIntroManager.h"
#include "Core/Manager/CameraManager.h"
#include "Core/Manager/DecalManager.h"
#include "Core/Manager/InputManager.h"
#include "Core/Constants.h"
#include "Entities/Player/Paladin.h"
#include "Entities/Enemy.h"
#include "UI/GameplayHUDLayout.h"
#include "UI/MinimapRenderer.h"
#include "UI/UIUtils.h"
#include "Entities/Props/Pot.h"
#include "Entities/Props/EnhanceMachine.h"
#include "Entities/Props/Chest.h"
#include "raymath.h"
#include <cmath>
#include "Core/Manager/AssetManager.h"

/// Creates a GameplayState instance from the supplied configuration.
GameplayState::GameplayState(TeamManager* teamManager, LevelManager* levelManager, WaveManager* waveManager)
    : teamManager(teamManager), levelManager(levelManager), waveManager(waveManager) {
}

/// Orchestrates one gameplay frame in dependency order.
/// Modal/cinematic states freeze simulation; otherwise the level, team, waves,
/// paths/entities, projectiles, assists, and deferred object mutations advance.
void GameplayState::Update(float deltaTime) {
    if (enhanceMenuUI.IsOpen()) {
        float viewportScale = std::min(
            (float)GetScreenWidth() / Constants::GAME_WIDTH,
            (float)GetScreenHeight() / Constants::GAME_HEIGHT
        );
        Camera2D uiCamera = UIUtils::CreateCenteredUICamera(viewportScale);
        Vector2 uiMousePosition = UIUtils::GetVirtualMousePosition(uiCamera);

        enhanceMenuUI.Update(deltaTime, uiMousePosition, *teamManager);
        return; // Freeze/modal-block gameplay input while enhance screen is open
    }

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
        Paladin* activePaladin = teamManager->GetActivePaladin();
        levelManager->UpdateLevel(
            deltaTime,
            activePaladin->GetPosition(),
            activePaladin->GetCollisionBox()
        );
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
                    GameManager::GetInstance().GenerateDungeon();
                    waveManager->Reset(0, 0, 0);
                }
            }
        }
        
        Paladin* active = teamManager->GetActivePaladin();
        ObjectManager& objects =
            GameManager::GetInstance().GetObjectManager();
        Pot* nearestPot = objects.FindNearestPickup(
            active->GetPosition(),
            50.0f
        );
        
        if (nearestPot && InputManager::IsInteractPressed()) {
            nearestPot->OnConsume(teamManager);
        }
        
        waveManager->Update(deltaTime, teamManager, levelManager);
        GameManager::GetInstance().UpdateDynamicEntities(deltaTime);
        GameManager::GetInstance().UpdateProjectiles(deltaTime, teamManager);
        GameManager::GetInstance().UpdateAssists(deltaTime, teamManager);
        objects.CommitPendingChanges();
        
        GameManager::GetInstance().GetComboMeter().Update(deltaTime);
    }
    GameManager::GetInstance().UpdateOrbs(deltaTime, teamManager);
    GameManager::GetInstance().UpdateEffects(deltaTime);
}

/// Builds the world frame from base tiles through depth-sorted actors and props.
/// Effects are split behind/in front of actors, then debug overlays and the
/// screen-space enhance menu are drawn in their appropriate camera spaces.
void GameplayState::Draw() {
    BeginMode2D(CameraManager::GetInstance().GetRenderCamera());
    levelManager->DrawLevelBase();
    DecalManager::GetInstance().Draw();

    if (Constants::isAutoAimEnabled && teamManager && teamManager->GetActivePaladin()) {
        Paladin* activePaladin = teamManager->GetActivePaladin();
        if (activePaladin->GetLockedEnemy()) {
            Enemy* targetEnemy = activePaladin->GetLockedEnemy();
            Vector2 footPos = targetEnemy->GetRenderFootPosition();

            Texture2D enemyCircle = AssetManager::GetInstance().GetTexture("Enemy_Circle");
            if (enemyCircle.id != 0) {
                float scale = (targetEnemy->GetEnemyType() == EnemyType::BOSS) ? 2.0f : 1.0f;
                float w = enemyCircle.width * scale;
                float h = enemyCircle.height * scale;
                Rectangle dest = { footPos.x, footPos.y, w, h };
                Vector2 circleOrigin = { w / 2.0f, h / 2.0f };
                DrawTexturePro(enemyCircle, {0, 0, (float)enemyCircle.width, (float)enemyCircle.height}, dest, circleOrigin, 0.0f, WHITE);
            }
        }
    }

    GameManager::GetInstance().DrawEffects(true);

    std::vector<DepthRenderItem> renderItems;
    renderItems.reserve(
        256 + GameManager::GetInstance().GetObjectManager().GetEnemyCount()
    );
    
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
    GameManager::GetInstance().DrawParticles();

    for (Enemy* enemy : GameManager::GetInstance()
             .GetObjectManager().GetEnemies()) {
        if (!enemy ||
            !enemy->GetStatusComponent().HasEffect(EffectType::FREEZE)) {
            continue;
        }
        Texture2D tex = enemy->GetEnemyType() == EnemyType::BOSS
            ? AssetManager::GetInstance().GetTexture("Freeze_Big")
            : AssetManager::GetInstance().GetTexture("Freeze");
        if (tex.id != 0) {
            Vector2 footPos = enemy->GetRenderFootPosition();
            Rectangle dest = {
                footPos.x,
                footPos.y,
                (float)tex.width,
                (float)tex.height
            };
            Vector2 origin = {
                tex.width / 2.0f,
                (float)tex.height - 5.0f
            };
            DrawTexturePro(
                tex,
                { 0, 0, (float)tex.width, (float)tex.height },
                dest,
                origin,
                0.0f,
                WHITE
            );
        }
    }

    if (Constants::isAutoAimEnabled) {
        Paladin* activePaladin = teamManager->GetActivePaladin();
        if (activePaladin && activePaladin->GetLockedEnemy()) {
            Enemy* targetEnemy = activePaladin->GetLockedEnemy();
            Vector2 targetPos = targetEnemy->GetPosition();

            Vector2 weaponPos = activePaladin->GetWeaponPivot();
            Vector2 dir = Vector2Subtract(targetPos, weaponPos);
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
    Rectangle virtualWindowBounds = {
        -uiCamera.offset.x / uiCamera.zoom,
        -uiCamera.offset.y / uiCamera.zoom,
        GetScreenWidth() / uiCamera.zoom,
        GetScreenHeight() / uiCamera.zoom
    };
    GameplayHUDLayout::Result gameplayHudLayout =
        GameplayHUDLayout::Calculate(
            virtualWindowBounds,
            teamManager ? teamManager->GetTeam().size() : 0
        );

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
        MinimapRenderer::Draw(
            levelManager->GetLevelMap(),
            currentGridX,
            currentGridY,
            gameplayHudLayout.minimapBounds,
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
        ObjectManager& objects = GameManager::GetInstance().GetObjectManager();

        EnhanceMachine* nearestMachine = nullptr;
        float machineDist = 60.0f;
        Chest* nearestChest = nullptr;
        float chestDist = 50.0f;

        for (GameObject* obj : objects.GetInteractables()) {
            if (!obj) continue;
            if (auto* machine = dynamic_cast<EnhanceMachine*>(obj)) {
                float d = Vector2Distance(active->GetPosition(), machine->GetPosition());
                if (d < machineDist) {
                    machineDist = d;
                    nearestMachine = machine;
                }
            } else if (auto* chest = dynamic_cast<Chest*>(obj)) {
                if (!chest->IsOpened()) {
                    float d = Vector2Distance(active->GetPosition(), chest->GetPosition());
                    if (d < chestDist) {
                        chestDist = d;
                        nearestChest = chest;
                    }
                }
            }
        }

        Pot* nearestPot = objects.FindNearestPickup(active->GetPosition(), 50.0f);
        float potDist = nearestPot ? Vector2Distance(active->GetPosition(), nearestPot->GetPosition()) : 9999.0f;

        if (nearestMachine && (machineDist <= potDist) && (machineDist <= (nearestChest ? chestDist : 9999.0f))) {
            Vector2 screenPos = GetWorldToScreen2D(nearestMachine->GetPosition(), CameraManager::GetInstance().GetRenderCamera());
            Vector2 uiPos = GetScreenToWorld2D(screenPos, uiCamera);
            float yOffset = std::sin(GetTime() * 5.0f) * 3.0f;
            uiPos.y -= 44.0f + yOffset;

            Texture2D selectTex = AssetManager::GetInstance().GetTexture("Select");
            if (selectTex.id != 0) {
                Vector2 origin = { selectTex.width / 2.0f, selectTex.height / 2.0f };
                Rectangle dest = { uiPos.x, uiPos.y, (float)selectTex.width, (float)selectTex.height };
                DrawTexturePro(selectTex, {0, 0, (float)selectTex.width, (float)selectTex.height}, dest, origin, 0.0f, WHITE);
            }
            UIUtils::DrawCenteredText("PixeloidSans", "Enhance Machine", { uiPos.x, uiPos.y - 12.0f }, UIUtils::FontSize::SMALL, RAYWHITE);

            float textWidth = UIUtils::MeasureText("PixeloidSans", "Press F to enhance", UIUtils::FontSize::SMALL).x;
            Rectangle background = {
                (Constants::GAME_WIDTH - textWidth) * 0.5f - 10.0f,
                Constants::GAME_HEIGHT - 44.0f,
                textWidth + 20.0f,
                28.0f
            };
            UIUtils::DrawPanel(background, Color{15, 20, 29, 220});
            UIUtils::DrawCenteredText("PixeloidSans", "Press F to enhance", { background.x + background.width * 0.5f, background.y + background.height * 0.5f }, UIUtils::FontSize::SMALL, RAYWHITE);
        }
        else if (nearestChest && (chestDist <= potDist)) {
            Vector2 screenPos = GetWorldToScreen2D(nearestChest->GetPosition(), CameraManager::GetInstance().GetRenderCamera());
            Vector2 uiPos = GetScreenToWorld2D(screenPos, uiCamera);
            float yOffset = std::sin(GetTime() * 5.0f) * 3.0f;
            uiPos.y -= 25.0f + yOffset;

            Texture2D selectTex = AssetManager::GetInstance().GetTexture("Select");
            if (selectTex.id != 0) {
                Vector2 origin = { selectTex.width / 2.0f, selectTex.height / 2.0f };
                Rectangle dest = { uiPos.x, uiPos.y, (float)selectTex.width, (float)selectTex.height };
                DrawTexturePro(selectTex, {0, 0, (float)selectTex.width, (float)selectTex.height}, dest, origin, 0.0f, WHITE);
            }
            UIUtils::DrawCenteredText("PixeloidSans", "Chest", { uiPos.x, uiPos.y - 12.0f }, UIUtils::FontSize::SMALL, RAYWHITE);

            float textWidth = UIUtils::MeasureText("PixeloidSans", "Press F to open", UIUtils::FontSize::SMALL).x;
            Rectangle background = {
                (Constants::GAME_WIDTH - textWidth) * 0.5f - 10.0f,
                Constants::GAME_HEIGHT - 44.0f,
                textWidth + 20.0f,
                28.0f
            };
            UIUtils::DrawPanel(background, Color{15, 20, 29, 220});
            UIUtils::DrawCenteredText("PixeloidSans", "Press F to open", { background.x + background.width * 0.5f, background.y + background.height * 0.5f }, UIUtils::FontSize::SMALL, RAYWHITE);
        }
        else if (nearestPot) {
            std::string name = "Potion";
            if (dynamic_cast<HpPot*>(nearestPot)) name = "HP Potion";
            else if (dynamic_cast<ExPot*>(nearestPot)) name = "EX Potion";
            else if (dynamic_cast<QuintPot*>(nearestPot)) name = "Quintessence";

            Vector2 screenPos = GetWorldToScreen2D(nearestPot->GetPosition(), CameraManager::GetInstance().GetRenderCamera());
            Vector2 uiPos = GetScreenToWorld2D(screenPos, uiCamera);
            float yOffset = std::sin(GetTime() * 5.0f) * 3.0f;
            uiPos.y -= 25.0f + yOffset;

            Texture2D selectTex = AssetManager::GetInstance().GetTexture("Select");
            if (selectTex.id != 0) {
                Vector2 origin = { selectTex.width / 2.0f, selectTex.height / 2.0f };
                Rectangle dest = { uiPos.x, uiPos.y, (float)selectTex.width, (float)selectTex.height };
                DrawTexturePro(selectTex, {0, 0, (float)selectTex.width, (float)selectTex.height}, dest, origin, 0.0f, WHITE);
            }
            UIUtils::DrawCenteredText("PixeloidSans", name, { uiPos.x, uiPos.y - 12.0f }, UIUtils::FontSize::SMALL, RAYWHITE);

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

    if (enhanceMenuUI.IsOpen()) {
        Vector2 mouseUI = GetScreenToWorld2D(GetMousePosition(), uiCamera);
        enhanceMenuUI.Draw(mouseUI, *teamManager);
    }

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
