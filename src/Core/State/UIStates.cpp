#include "Core/State/UIStates.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/AudioManager.h"
#include "Core/Manager/CameraManager.h"
#include "Core/Manager/InputManager.h"
#include "UI/UIManager.h"
#include "UI/UIUtils.h"
#include "Core/Constants.h"

// --- MainMenuState ---
MainMenuState::MainMenuState(MainMenu* menu, GameApplication* app) : menu(menu), app(app) {}

void MainMenuState::Update(float deltaTime) {
    menu->Update(deltaTime);
    MainMenuAction menuAction = menu->ConsumeAction();
    if (menu->ConsumeQuitRequest()) {
        app->quitRequested = true;
    }
    
    if (menuAction == MainMenuAction::StartGame && app->systemInitialized) {
        app->paladinSelectionMenu.Close();
        app->StartNewGame();
    } else if (menuAction == MainMenuAction::Continue && app->hasContinuableSession) {
        app->ContinueSuspendedSession();
    } else if (menuAction == MainMenuAction::OpenEditor) {
        app->roomEditor.Initialize();
        GameManager::GetInstance().SetState(GameState::ROOM_EDITOR);
    } else if (menuAction == MainMenuAction::OpenSavedRoomEditor) {
        std::string roomPath = menu->ConsumeSelectedRoomPath();
        if (!roomPath.empty() && app->roomEditor.LoadRoom(roomPath)) {
            GameManager::GetInstance().SetState(GameState::ROOM_EDITOR);
        }
    }
    
    if (app->systemInitialized && IsKeyPressed(KEY_R)) {
        app->paladinSelectionMenu.Close();
        app->ResetDemoGame();
    }
}

void MainMenuState::Draw() {
    menu->Draw(GetScreenWidth(), GetScreenHeight());
}

// --- PauseState ---
PauseState::PauseState(PauseMenu* menu, GameApplication* app, std::unique_ptr<IGameState> backgroundState) 
    : menu(menu), app(app), backgroundState(std::move(backgroundState)) {}

void PauseState::Update(float deltaTime) {
    float viewportScale = std::min((float)GetScreenWidth() / Constants::GAME_WIDTH, (float)GetScreenHeight() / Constants::GAME_HEIGHT);
    Camera2D uiCamera = UIUtils::CreateCenteredUICamera(viewportScale);
    Vector2 modalMousePosition = UIUtils::GetVirtualMousePosition(uiCamera);

    PauseMenuAction action = menu->Update(modalMousePosition);
    switch (action) {
        case PauseMenuAction::Resume:
            GameManager::GetInstance().ResumeGame();
            break;
        case PauseMenuAction::Settings:
            app->settingsReturnState = GameState::PAUSE;
            GameManager::GetInstance().SetState(GameState::SETTINGS);
            break;
        case PauseMenuAction::BackToMainMenu:
        case PauseMenuAction::Quit:
            app->SuspendSessionToMainMenu();
            break;
        case PauseMenuAction::None:
            break;
    }
}

void PauseState::Draw() {
    if (backgroundState) {
        backgroundState->Draw();
    }
    
    float viewportScale = std::min((float)GetScreenWidth() / Constants::GAME_WIDTH, (float)GetScreenHeight() / Constants::GAME_HEIGHT);
    Camera2D uiCamera = UIUtils::CreateCenteredUICamera(viewportScale);
    Vector2 uiMousePosition = UIUtils::GetVirtualMousePosition(uiCamera);

    BeginMode2D(uiCamera);
    UIManager::DrawModalOverlay();
    menu->Draw(uiMousePosition);
    EndMode2D();
}

// --- SettingsState ---
SettingsState::SettingsState(SettingsMenu* menu, GameApplication* app, std::unique_ptr<IGameState> backgroundState)
    : menu(menu), app(app), backgroundState(std::move(backgroundState)) {}

void SettingsState::Update(float deltaTime) {
    float viewportScale = std::min((float)GetScreenWidth() / Constants::GAME_WIDTH, (float)GetScreenHeight() / Constants::GAME_HEIGHT);
    Camera2D uiCamera = UIUtils::CreateCenteredUICamera(viewportScale);
    Vector2 modalMousePosition = UIUtils::GetVirtualMousePosition(uiCamera);

    if (menu->Update(modalMousePosition)) {
        GameManager::GetInstance().SetState(app->settingsReturnState);
    }
}

void SettingsState::Draw() {
    if (backgroundState) {
        backgroundState->Draw();
    }

    float viewportScale = std::min((float)GetScreenWidth() / Constants::GAME_WIDTH, (float)GetScreenHeight() / Constants::GAME_HEIGHT);
    Camera2D uiCamera = UIUtils::CreateCenteredUICamera(viewportScale);
    Vector2 uiMousePosition = UIUtils::GetVirtualMousePosition(uiCamera);

    BeginMode2D(uiCamera);
    UIManager::DrawModalOverlay();
    menu->Draw(uiMousePosition);
    EndMode2D();
}

// --- GameOverState ---
GameOverState::GameOverState(GameApplication* app, std::unique_ptr<IGameState> backgroundState)
    : app(app), backgroundState(std::move(backgroundState)) {}

void GameOverState::Update(float deltaTime) {
    if (IsKeyPressed(KEY_R)) {
        app->ResetGame();
    }
}

void GameOverState::Draw() {
    if (backgroundState) {
        backgroundState->Draw();
    }

    float viewportScale = std::min((float)GetScreenWidth() / Constants::GAME_WIDTH, (float)GetScreenHeight() / Constants::GAME_HEIGHT);
    Camera2D uiCamera = UIUtils::CreateCenteredUICamera(viewportScale);

    BeginMode2D(uiCamera);
    UIManager::DrawModalOverlay();
    UIUtils::DrawCenteredText("PixeloidSansBold", "GAME OVER", { Constants::GAME_WIDTH * 0.5f, Constants::GAME_HEIGHT * 0.4f }, UIUtils::FontSize::TITLE, RED);
    UIUtils::DrawCenteredText("PixeloidSans", "Press R to Restart", { Constants::GAME_WIDTH * 0.5f, Constants::GAME_HEIGHT * 0.5f }, UIUtils::FontSize::BODY, RAYWHITE);
    EndMode2D();
}

// --- VictoryState ---
VictoryState::VictoryState(GameApplication* app, std::unique_ptr<IGameState> backgroundState)
    : app(app), backgroundState(std::move(backgroundState)) {}

void VictoryState::Update(float deltaTime) {
    if (IsKeyPressed(KEY_SPACE)) {
        app->ReturnToHub();
    }
}

void VictoryState::Draw() {
    if (backgroundState) {
        backgroundState->Draw();
    }

    float viewportScale = std::min((float)GetScreenWidth() / Constants::GAME_WIDTH, (float)GetScreenHeight() / Constants::GAME_HEIGHT);
    Camera2D uiCamera = UIUtils::CreateCenteredUICamera(viewportScale);

    BeginMode2D(uiCamera);
    UIManager::DrawModalOverlay();
    UIUtils::DrawCenteredText("PixeloidSansBold", "MISSION COMPLETE", { Constants::GAME_WIDTH * 0.5f, Constants::GAME_HEIGHT * 0.4f }, UIUtils::FontSize::TITLE, GOLD);
    UIUtils::DrawCenteredText("PixeloidSans", "Press SPACE to Return", { Constants::GAME_WIDTH * 0.5f, Constants::GAME_HEIGHT * 0.5f }, UIUtils::FontSize::BODY, RAYWHITE);
    EndMode2D();
}

// --- RoomEditorStateAdapter ---
RoomEditorStateAdapter::RoomEditorStateAdapter(RoomEditorState* editor) : editor(editor) {}

void RoomEditorStateAdapter::Update(float deltaTime) {
    editor->Update(deltaTime);
    if (IsKeyPressed(KEY_ESCAPE)) {
        GameManager::GetInstance().SetState(GameState::MAIN_MENU);
        AudioManager::GetInstance().PlayMusicTrack("bgm_starter_menu", 1.0f);
    }
}

void RoomEditorStateAdapter::Draw() {
    BeginMode2D(CameraManager::GetInstance().GetRenderCamera());
    editor->Draw();
    EndMode2D();
}
