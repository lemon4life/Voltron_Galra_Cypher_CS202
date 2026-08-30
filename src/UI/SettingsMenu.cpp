#include "UI/SettingsMenu.h"
#include "UI/UIUtils.h"
#include "Core/Manager/AssetManager.h"

#include "Core/Constants.h"
#include "Core/Manager/AudioManager.h"

namespace {
constexpr float CONTAINER_WIDTH = 360.0f;
constexpr float CONTAINER_HEIGHT = 300.0f;
constexpr float SLIDER_WIDTH = 230.0f;
constexpr float BUTTON_WIDTH = 180.0f;
constexpr float BUTTON_HEIGHT = 34.0f;
}

/// Creates a SettingsMenu instance from the supplied configuration.
SettingsMenu::SettingsMenu()
    : containerBounds{
          (Constants::GAME_WIDTH - CONTAINER_WIDTH) * 0.5f,
          (Constants::GAME_HEIGHT - CONTAINER_HEIGHT) * 0.5f,
          CONTAINER_WIDTH,
          CONTAINER_HEIGHT},
      soundEffectsSlider(
          Rectangle{
              (Constants::GAME_WIDTH - SLIDER_WIDTH) * 0.5f,
              210.0f,
              SLIDER_WIDTH,
              7.0f},
          "Sound Effects",
          AudioManager::GetInstance().GetSoundEffectsVolume()),
      musicSlider(
          Rectangle{
              (Constants::GAME_WIDTH - SLIDER_WIDTH) * 0.5f,
              270.0f,
              SLIDER_WIDTH,
              7.0f},
          "Music",
          AudioManager::GetInstance().GetMusicVolumeLevel()),
      autoAimToggleBounds{
          (Constants::GAME_WIDTH - SLIDER_WIDTH) * 0.5f,
          310.0f,
          24.0f,
          24.0f},
      backButton(
          Rectangle{
              (Constants::GAME_WIDTH - BUTTON_WIDTH) * 0.5f,
              355.0f,
              BUTTON_WIDTH,
              BUTTON_HEIGHT},
          "Back") {
}

/// Advances this component's state for the current frame.
bool SettingsMenu::Update(Vector2 mousePosition) {
    AudioManager& audioManager = AudioManager::GetInstance();

    if (soundEffectsSlider.Update(mousePosition)) {
        audioManager.SetSoundEffectsVolume(soundEffectsSlider.GetValue());
    }

    if (musicSlider.Update(mousePosition)) {
        audioManager.SetMusicVolumeLevel(musicSlider.GetValue());
    }
    
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mousePosition, autoAimToggleBounds)) {
        Constants::isAutoAimEnabled = !Constants::isAutoAimEnabled;
        audioManager.PlayRandomClick();
    }

    if (backButton.Update(mousePosition)) {
        audioManager.PlayRandomClick();
        return true;
    }

    return false;
}

/// Renders this component using its current state and visual resources.
void SettingsMenu::Draw(Vector2 mousePosition) const {
    DrawRectangleRec(containerBounds, Color{25, 31, 43, 248});
    DrawRectangleLinesEx(containerBounds, 2.0f, Color{145, 156, 178, 255});

    constexpr float titleFontSize = 26.0f;
    constexpr const char* title = "SETTINGS";
    Font fontBold = AssetManager::GetInstance().GetCustomFont("PixeloidBold");
    Vector2 titleSize = MeasureTextEx(fontBold, title, titleFontSize, 1.0f);
    UIUtils::DrawText("PixeloidBold", title, { containerBounds.x + (containerBounds.width - titleSize.x) * 0.5f, containerBounds.y + 18.0f }, static_cast<UIUtils::FontSize>(titleFontSize), RAYWHITE);
    DrawLine(
        static_cast<int>(containerBounds.x + 20.0f),
        static_cast<int>(containerBounds.y + 60.0f),
        static_cast<int>(
            containerBounds.x + containerBounds.width - 20.0f
        ),
        static_cast<int>(containerBounds.y + 60.0f),
        Color{100, 108, 123, 255}
    );

    soundEffectsSlider.Draw();
    musicSlider.Draw();
    
    // Draw Auto-Aim Toggle
    Font fontSans = AssetManager::GetInstance().GetCustomFont("PixeloidSans");
    UIUtils::DrawText("PixeloidSans", "Auto-Aim", { autoAimToggleBounds.x + 35.0f, autoAimToggleBounds.y + 2.0f }, static_cast<UIUtils::FontSize>(20), RAYWHITE);
    DrawRectangleLinesEx(autoAimToggleBounds, 2.0f, RAYWHITE);
    if (Constants::isAutoAimEnabled) {
        Rectangle fillRec = { autoAimToggleBounds.x + 4, autoAimToggleBounds.y + 4, autoAimToggleBounds.width - 8, autoAimToggleBounds.height - 8 };
        DrawRectangleRec(fillRec, GREEN);
    }
    
    backButton.Draw(mousePosition);
}
