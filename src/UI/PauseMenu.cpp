#include "UI/PauseMenu.h"

#include "Core/Manager/AudioManager.h"

namespace {
constexpr float UI_WIDTH = 683.0f;
constexpr float UI_HEIGHT = 512.0f;
}

PauseMenu::PauseMenu()
    : containerBounds{(UI_WIDTH - 400.0f) * 0.5f, 36.0f, 400.0f, 440.0f},
      soundEffectsSlider(
          Rectangle{(UI_WIDTH - 260.0f) * 0.5f, 150.0f, 260.0f, 7.0f},
          "Sound Effects",
          AudioManager::GetInstance().GetSoundEffectsVolume()),
      musicSlider(
          Rectangle{(UI_WIDTH - 260.0f) * 0.5f, 215.0f, 260.0f, 7.0f},
          "Music",
          AudioManager::GetInstance().GetMusicVolumeLevel()),
      resumeButton(
          Rectangle{(UI_WIDTH - 280.0f) * 0.5f, 265.0f, 280.0f, 44.0f},
          "Resume"),
      backToMainMenuButton(
          Rectangle{(UI_WIDTH - 280.0f) * 0.5f, 325.0f, 280.0f, 44.0f},
          "Back to Main Menu"),
      quitButton(
          Rectangle{(UI_WIDTH - 280.0f) * 0.5f, 385.0f, 280.0f, 44.0f},
          "Quit Game") {
}

PauseMenuAction PauseMenu::Update(Vector2 mousePosition) {
    AudioManager& audioManager = AudioManager::GetInstance();

    if (soundEffectsSlider.Update(mousePosition)) {
        audioManager.SetSoundEffectsVolume(soundEffectsSlider.GetValue());
    }

    if (musicSlider.Update(mousePosition)) {
        audioManager.SetMusicVolumeLevel(musicSlider.GetValue());
    }

    if (resumeButton.Update(mousePosition)) {
        audioManager.PlayRandomClick();
        return PauseMenuAction::Resume;
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
    DrawRectangleRec(Rectangle{0.0f, 0.0f, UI_WIDTH, UI_HEIGHT},
                     Color{0, 0, 0, 175});
    DrawRectangleRec(containerBounds, Color{25, 31, 43, 248});
    DrawRectangleLinesEx(containerBounds, 2.0f, Color{145, 156, 178, 255});

    constexpr int titleFontSize = 30;
    constexpr const char* title = "PAUSED";
    const int titleWidth = MeasureText(title, titleFontSize);
    DrawText(title,
             static_cast<int>(containerBounds.x +
                              (containerBounds.width - titleWidth) * 0.5f),
             70,
             titleFontSize,
             RAYWHITE);

    soundEffectsSlider.Draw();
    musicSlider.Draw();
    resumeButton.Draw(mousePosition);
    backToMainMenuButton.Draw(mousePosition);
    quitButton.Draw(mousePosition);
}
