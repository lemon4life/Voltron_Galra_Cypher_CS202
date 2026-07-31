#include "UI/SettingsMenu.h"

#include "Core/Constants.h"
#include "Core/Manager/AudioManager.h"

namespace {
constexpr float CONTAINER_WIDTH = 360.0f;
constexpr float CONTAINER_HEIGHT = 300.0f;
constexpr float SLIDER_WIDTH = 230.0f;
constexpr float BUTTON_WIDTH = 180.0f;
constexpr float BUTTON_HEIGHT = 34.0f;
}

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
      backButton(
          Rectangle{
              (Constants::GAME_WIDTH - BUTTON_WIDTH) * 0.5f,
              335.0f,
              BUTTON_WIDTH,
              BUTTON_HEIGHT},
          "Back") {
}

bool SettingsMenu::Update(Vector2 mousePosition) {
    AudioManager& audioManager = AudioManager::GetInstance();

    if (soundEffectsSlider.Update(mousePosition)) {
        audioManager.SetSoundEffectsVolume(soundEffectsSlider.GetValue());
    }

    if (musicSlider.Update(mousePosition)) {
        audioManager.SetMusicVolumeLevel(musicSlider.GetValue());
    }

    if (backButton.Update(mousePosition)) {
        audioManager.PlayRandomClick();
        return true;
    }

    return false;
}

void SettingsMenu::Draw(Vector2 mousePosition) const {
    DrawRectangleRec(containerBounds, Color{25, 31, 43, 248});
    DrawRectangleLinesEx(containerBounds, 2.0f, Color{145, 156, 178, 255});

    constexpr int titleFontSize = 26;
    constexpr const char* title = "SETTINGS";
    const int titleWidth = MeasureText(title, titleFontSize);
    DrawText(
        title,
        static_cast<int>(
            containerBounds.x +
            (containerBounds.width - titleWidth) * 0.5f
        ),
        static_cast<int>(containerBounds.y + 18.0f),
        titleFontSize,
        RAYWHITE
    );
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
    backButton.Draw(mousePosition);
}
