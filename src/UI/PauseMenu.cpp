#include "UI/PauseMenu.h"
#include "UI/UIUtils.h"
#include "Core/Manager/AssetManager.h"

#include "Core/Constants.h"
#include "Core/Manager/AudioManager.h"

namespace {
constexpr float CONTAINER_WIDTH = 320.0f;
constexpr float CONTAINER_HEIGHT = 256.0f;
constexpr float BUTTON_WIDTH = 230.0f;
constexpr float BUTTON_HEIGHT = 34.0f;
}

PauseMenu::PauseMenu()
    : containerBounds{
          (Constants::GAME_WIDTH - CONTAINER_WIDTH) * 0.5f,
          (Constants::GAME_HEIGHT - CONTAINER_HEIGHT) * 0.5f,
          CONTAINER_WIDTH,
          CONTAINER_HEIGHT},
      resumeButton(
          Rectangle{
              (Constants::GAME_WIDTH - BUTTON_WIDTH) * 0.5f,
              192.0f,
              BUTTON_WIDTH,
              BUTTON_HEIGHT},
          "Resume"),
      settingsButton(
          Rectangle{
              (Constants::GAME_WIDTH - BUTTON_WIDTH) * 0.5f,
              234.0f,
              BUTTON_WIDTH,
              BUTTON_HEIGHT},
          "Settings"),
      backToMainMenuButton(
          Rectangle{
              (Constants::GAME_WIDTH - BUTTON_WIDTH) * 0.5f,
              276.0f,
              BUTTON_WIDTH,
              BUTTON_HEIGHT},
          "Back to Main Menu"),
      quitButton(
          Rectangle{
              (Constants::GAME_WIDTH - BUTTON_WIDTH) * 0.5f,
              318.0f,
              BUTTON_WIDTH,
              BUTTON_HEIGHT},
          "Quit Game") {
}

PauseMenuAction PauseMenu::Update(Vector2 mousePosition) {
    AudioManager& audioManager = AudioManager::GetInstance();

    if (resumeButton.Update(mousePosition)) {
        audioManager.PlayRandomClick();
        return PauseMenuAction::Resume;
    }

    if (settingsButton.Update(mousePosition)) {
        audioManager.PlayRandomClick();
        return PauseMenuAction::Settings;
    }

    if (backToMainMenuButton.Update(mousePosition)) {
        audioManager.PlayRandomClick();
        return PauseMenuAction::BackToMainMenu;
    }

    if (quitButton.Update(mousePosition)) {
        audioManager.PlayRandomClick();
        return PauseMenuAction::Quit;
    }

    return PauseMenuAction::None;
}

void PauseMenu::Draw(Vector2 mousePosition) const {
    DrawRectangleRec(containerBounds, Color{25, 31, 43, 248});
    DrawRectangleLinesEx(containerBounds, 2.0f, Color{145, 156, 178, 255});

    constexpr float titleFontSize = 26.0f;
    constexpr const char* title = "PAUSED";
    Font fontBold = AssetManager::GetInstance().GetCustomFont("PixeloidBold");
    Vector2 titleSize = MeasureTextEx(fontBold, title, titleFontSize, 1.0f);
    UIUtils::DrawText("PixeloidBold", title, { containerBounds.x + (containerBounds.width - titleSize.x) * 0.5f, containerBounds.y + 16.0f }, static_cast<UIUtils::FontSize>(titleFontSize), RAYWHITE);

    resumeButton.Draw(mousePosition);
    settingsButton.Draw(mousePosition);
    backToMainMenuButton.Draw(mousePosition);
    quitButton.Draw(mousePosition);
}
