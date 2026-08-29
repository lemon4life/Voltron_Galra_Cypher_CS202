#include "Core/State/HubState.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/DialogueManager.h"
#include "Core/Manager/CameraManager.h"
#include "Core/Manager/InputManager.h"
#include "Core/Constants.h"
#include "Entities/Player/Paladin.h"
#include "Entities/Hub/HubPaladinStand.h"
#include "Entities/NPC.h"
#include "UI/UIUtils.h"
#include <algorithm>
#include <cmath>
#include "raymath.h"
#include "Core/Manager/AssetManager.h"

namespace {
    GameObject* FindNearestHubInteractable(const std::vector<GameObject*>& entities, Vector2 playerPosition) {
        GameObject* nearest = nullptr;
        float nearestDistance = 50.0f; // interaction range

        for (GameObject* entity : entities) {
            if (entity->GetObjectType() != GameObjectType::HubPaladinStand &&
                entity->GetObjectType() != GameObjectType::NPC) {
                continue;
            }
            if (entity->GetObjectType() == GameObjectType::NPC) {
                NPC* npc = static_cast<NPC*>(entity);
                if (npc->GetNpcId() == NpcId::Shiro && GameManager::GetInstance().HasTalkedToShiro()) continue;
                if (npc->GetNpcId() == NpcId::Allura && !GameManager::GetInstance().HasTalkedToShiro()) continue;
            }

            float distance = Vector2Distance(playerPosition, entity->GetPosition());
            if (distance < nearestDistance) {
                nearestDistance = distance;
                nearest = entity;
            }
        }
        return nearest;
    }

    void DrawInteractionPrompt(const std::string& text) {
        float textWidth = UIUtils::MeasureText("PixeloidSans", text, UIUtils::FontSize::SMALL).x;
        Rectangle background = {
            (Constants::GAME_WIDTH - textWidth) * 0.5f - 10.0f,
            Constants::GAME_HEIGHT - 44.0f,
            textWidth + 20.0f,
            28.0f
        };
        UIUtils::DrawPanel(background, Color{15, 20, 29, 220});
        UIUtils::DrawCenteredText("PixeloidSans", text, { background.x + background.width * 0.5f, background.y + background.height * 0.5f }, UIUtils::FontSize::SMALL, RAYWHITE);
    }

    void DrawHubInteractionPrompt(GameObject* interactable, Camera2D uiCamera) {
        if (!interactable) return;

        Vector2 screenPos = GetWorldToScreen2D(interactable->GetPosition(), CameraManager::GetInstance().GetRenderCamera());
        Vector2 uiPos = GetScreenToWorld2D(screenPos, uiCamera);

        float yOffset = std::sin(GetTime() * 5.0f) * 3.0f;
        uiPos.y -= 35.0f + yOffset;

        std::string text = "Press F to talk";
        if (interactable->GetObjectType() == GameObjectType::HubPaladinStand) {
            HubPaladinStand* stand = static_cast<HubPaladinStand*>(interactable);
            text = std::string("Press F to inspect ") + stand->GetDisplayName();

            Texture2D selectTex = AssetManager::GetInstance().GetTexture("Select");
            if (selectTex.id != 0) {
                Vector2 origin = { selectTex.width / 2.0f, selectTex.height / 2.0f };
                Rectangle dest = { uiPos.x, uiPos.y, (float)selectTex.width, (float)selectTex.height };
                DrawTexturePro(selectTex, {0, 0, (float)selectTex.width, (float)selectTex.height}, dest, origin, 0.0f, WHITE);
            }
            UIUtils::DrawCenteredText("PixeloidSans", stand->GetDisplayName(), { uiPos.x, uiPos.y - 12.0f }, UIUtils::FontSize::SMALL, RAYWHITE);
        }

        DrawInteractionPrompt(text);
    }
}

HubState::HubState(TeamManager* teamManager, LevelManager* levelManager, WaveManager* waveManager, PaladinSelectionMenu* paladinSelectionMenu)
    : teamManager(teamManager), levelManager(levelManager), waveManager(waveManager), paladinSelectionMenu(paladinSelectionMenu) {
}

void HubState::Update(float deltaTime) {
    if (paladinSelectionMenu->IsOpen()) {
        float viewportScale = std::min(
            (float)GetScreenWidth() / Constants::GAME_WIDTH,
            (float)GetScreenHeight() / Constants::GAME_HEIGHT
        );
        Camera2D uiCamera = UIUtils::CreateCenteredUICamera(viewportScale);
        Vector2 uiMousePosition = UIUtils::GetVirtualMousePosition(uiCamera);

        paladinSelectionMenu->Update(
            deltaTime,
            uiMousePosition,
            *teamManager
        );
        if (InputManager::IsInteractPressed()) {
            paladinSelectionMenu->Close();
        }
    } else if (DialogueManager::GetInstance().IsActive()) {
        DialogueManager::GetInstance().Update(deltaTime);
    } else if (DialogueManager::GetInstance().IsMissionRequested()) {
        int missionId = DialogueManager::GetInstance().GetRequestedMissionId();
        DialogueManager::GetInstance().ClearMissionRequest();

        if (missionId == -2) {
            GameManager::GetInstance().ClearProjectiles();
            GameManager::GetInstance().ResetFloorCount();
            GameManager::GetInstance().GenerateDungeon();
            waveManager->Reset(0, 0, 0, 0);
            GameManager::GetInstance().SetState(GameState::GAMEPLAY);
        }
    } else {
        levelManager->UpdateLevel(deltaTime, teamManager->GetActivePaladin()->GetPosition());
        teamManager->Update(deltaTime);
        GameManager::GetInstance().UpdateDynamicEntities(deltaTime);
        GameManager::GetInstance().GetObjectManager().CommitPendingChanges();

        if (InputManager::IsInteractPressed()) {
            GameObject* interactable = FindNearestHubInteractable(
                GameManager::GetInstance()
                    .GetObjectManager().GetInteractables(),
                teamManager->GetActivePaladin()->GetPosition()
            );

            if (interactable) {
                if (interactable->GetObjectType() == GameObjectType::HubPaladinStand) {
                    HubPaladinStand* stand = static_cast<HubPaladinStand*>(interactable);
                    paladinSelectionMenu->Open(stand->GetPaladinId());
                } else if (interactable->GetObjectType() == GameObjectType::NPC) {
                    NPC* npc = static_cast<NPC*>(interactable);
                    if (npc->GetNpcId() == NpcId::Shiro) {
                        DialogueManager::GetInstance().LoadDialogueTree("assets/story/Shiro.txt");
                        GameManager::GetInstance().SetTalkedToShiro(true);
                    } else {
                        DialogueManager::GetInstance().LoadDialogueTree("assets/story/Allura.txt");
                    }
                    DialogueManager::GetInstance().StartDialogue();
                }
            } else if (levelManager->IsPlayerInExitRoom(teamManager->GetActivePaladin()->GetPosition())) {
                GameManager::GetInstance().AdvanceFloorCount();
                if (GameManager::GetInstance().GetCurrentFloor() > GameManager::MAX_FLOORS) {
                    // This is handled by GameApplication when State changes to HUB again
                    GameManager::GetInstance().SetState(GameState::HUB);
                } else {
                    GameManager::GetInstance().ClearProjectiles();
                    GameManager::GetInstance().GenerateDungeon();
                    waveManager->Reset(0, 0, 0);
                }
            }
        }
    }
    
    GameManager::GetInstance().UpdateEffects(deltaTime);
}

void HubState::Draw() {
    BeginMode2D(CameraManager::GetInstance().GetRenderCamera());

    levelManager->DrawLevelBase();
    GameManager::GetInstance().DrawEffects(true);

    std::vector<DepthRenderItem> renderItems;
    renderItems.reserve(
        128 + GameManager::GetInstance().GetObjectManager().GetEnemyCount()
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
    GameManager::GetInstance().DrawParticles();
    GameManager::GetInstance().DrawDebugOverlays(teamManager);
    
    EndMode2D();

    // Draw Hub specific UI overlays
    float viewportScale = std::min(
        (float)GetScreenWidth() / Constants::GAME_WIDTH,
        (float)GetScreenHeight() / Constants::GAME_HEIGHT
    );
    Camera2D uiCamera = UIUtils::CreateCenteredUICamera(viewportScale);
    
    BeginMode2D(uiCamera);
    if (DialogueManager::GetInstance().IsActive()) {
        DialogueManager::GetInstance().Draw(Constants::GAME_WIDTH, Constants::GAME_HEIGHT);
    } else if (paladinSelectionMenu->IsOpen()) {
        Vector2 uiMousePosition = UIUtils::GetVirtualMousePosition(uiCamera);
        paladinSelectionMenu->Draw(uiMousePosition, *teamManager);
    } else {
        // Draw interact.png above all NPCs
        for (GameObject* entity : GameManager::GetInstance()
                 .GetObjectManager().GetInteractables()) {
            if (entity->GetObjectType() == GameObjectType::NPC) {
                NPC* npc = static_cast<NPC*>(entity);
                bool isActive = (npc->GetNpcId() == NpcId::Shiro && !GameManager::GetInstance().HasTalkedToShiro()) ||
                                (npc->GetNpcId() == NpcId::Allura && GameManager::GetInstance().HasTalkedToShiro());
                if (!isActive) continue;

                Vector2 screenPos = GetWorldToScreen2D(entity->GetPosition(), CameraManager::GetInstance().GetRenderCamera());
                Vector2 uiPos = GetScreenToWorld2D(screenPos, uiCamera);
                float yOffset = std::sin(GetTime() * 5.0f) * 3.0f;
                uiPos.y -= 35.0f + yOffset;

                Texture2D interactTex = AssetManager::GetInstance().GetTexture("Interact");
                if (interactTex.id != 0) {
                    Vector2 origin = { interactTex.width / 2.0f, interactTex.height / 2.0f };
                    Rectangle dest = { uiPos.x, uiPos.y, (float)interactTex.width, (float)interactTex.height };
                    DrawTexturePro(interactTex, {0, 0, (float)interactTex.width, (float)interactTex.height}, dest, origin, 0.0f, WHITE);
                }
            }
        }

        GameObject* interactable = FindNearestHubInteractable(
            GameManager::GetInstance()
                .GetObjectManager().GetInteractables(),
            teamManager->GetActivePaladin()->GetPosition()
        );
        if (interactable) {
            DrawHubInteractionPrompt(interactable, uiCamera);
        } else if (levelManager->IsPlayerInExitRoom(teamManager->GetActivePaladin()->GetPosition())) {
            DrawInteractionPrompt("Press F to go to the next floor");
        }
    }
    EndMode2D();
}
