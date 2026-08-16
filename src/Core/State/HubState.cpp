#include "Core/State/HubState.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/DialogueManager.h"
#include "Core/Manager/ParticleManager.h"
#include "Core/Manager/CameraManager.h"
#include "Core/Manager/InputManager.h"
#include "Core/Constants.h"
#include "Entities/Player/Paladin.h"
#include "Entities/Hub/HubPaladinStand.h"
#include "Entities/NPC.h"
#include "UI/UIUtils.h"
#include <algorithm>

namespace {
    GameObject* FindNearestHubInteractable(const std::vector<GameObject*>& entities, Vector2 playerPosition) {
        GameObject* nearest = nullptr;
        float nearestDistance = 50.0f; // interaction range

        for (GameObject* entity : entities) {
            if (entity->GetObjectType() != GameObjectType::HubPaladinStand &&
                entity->GetObjectType() != GameObjectType::NPC) {
                continue;
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

    void DrawHubInteractionPrompt(GameObject* interactable) {
        if (!interactable) return;

        std::string text = "Press F to talk";
        if (interactable->GetObjectType() == GameObjectType::HubPaladinStand) {
            HubPaladinStand* stand = static_cast<HubPaladinStand*>(interactable);
            text = std::string("Press F to inspect ") + stand->GetDisplayName();
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
        DialogueManager::GetInstance().ClearMissionRequest();
        GameManager::GetInstance().ClearProjectiles();
        GameManager::GetInstance().ResetFloorCount();
        levelManager->GenerateDungeon(teamManager);
        waveManager->Reset(1, 0, 0);
        GameManager::GetInstance().SetState(GameState::GAMEPLAY);
    } else {
        levelManager->UpdateLevel(deltaTime, teamManager->GetActivePaladin()->GetPosition());
        teamManager->Update(deltaTime);

        if (InputManager::IsInteractPressed()) {
            GameObject* interactable = FindNearestHubInteractable(
                GameManager::GetInstance().GetLevelEntities(),
                teamManager->GetActivePaladin()->GetPosition()
            );

            if (interactable) {
                if (interactable->GetObjectType() == GameObjectType::HubPaladinStand) {
                    HubPaladinStand* stand = static_cast<HubPaladinStand*>(interactable);
                    paladinSelectionMenu->Open(stand->GetPaladinId());
                } else if (interactable->GetObjectType() == GameObjectType::NPC) {
                    // NPC* npc = static_cast<NPC*>(interactable);
                    DialogueManager::GetInstance().StartDialogue();
                }
            } else if (levelManager->IsPlayerInExitRoom(teamManager->GetActivePaladin()->GetPosition())) {
                GameManager::GetInstance().AdvanceFloorCount();
                if (GameManager::GetInstance().GetCurrentFloor() > GameManager::MAX_FLOORS) {
                    // This is handled by GameApplication when State changes to HUB again
                    GameManager::GetInstance().SetState(GameState::HUB);
                } else {
                    GameManager::GetInstance().ClearProjectiles();
                    levelManager->GenerateDungeon(teamManager);
                    waveManager->Reset(0, 0, 0);
                }
            }
        }
    }
    
    GameManager::GetInstance().UpdateEffects(deltaTime);
    ParticleManager::GetInstance().Update(deltaTime);
}

void HubState::Draw() {
    BeginMode2D(CameraManager::GetInstance().GetRenderCamera());

    levelManager->DrawLevelBase();

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

    ParticleManager::GetInstance().Draw();
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
    } else if (!paladinSelectionMenu->IsOpen()) {
        GameObject* interactable = FindNearestHubInteractable(
            GameManager::GetInstance().GetLevelEntities(),
            teamManager->GetActivePaladin()->GetPosition()
        );
        if (interactable) {
            DrawHubInteractionPrompt(interactable);
        } else if (levelManager->IsPlayerInExitRoom(teamManager->GetActivePaladin()->GetPosition())) {
            DrawInteractionPrompt("Press F to go to the next floor");
        }
    }
    EndMode2D();
}
