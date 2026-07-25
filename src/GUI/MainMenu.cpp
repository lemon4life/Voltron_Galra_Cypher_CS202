#include "GUI/MainMenu.h"

#include "Core/Manager/AudioManager.h"

namespace {
constexpr float UI_WIDTH = 683.0f;
}

MainMenu::MainMenu()
    : playButton(Rectangle{(UI_WIDTH - 240.0f) * 0.5f, 260.0f, 240.0f, 48.0f}, "Play"),
      quitButton(Rectangle{(UI_WIDTH - 240.0f) * 0.5f, 326.0f, 240.0f, 48.0f}, "Quit") {
}

MainMenuAction MainMenu::Update(Vector2 mousePosition) {
    if (playButton.Update(mousePosition) || IsKeyPressed(KEY_ENTER)) {
        AudioManager::GetInstance().PlayRandomClick();
        return MainMenuAction::Play;
    }

    if (quitButton.Update(mousePosition)) {
        AudioManager::GetInstance().PlayRandomClick();
        return MainMenuAction::Quit;
    }

    return MainMenuAction::None;
}

void MainMenu::Draw(Vector2 mousePosition) const {
    ClearBackground(Color{21, 28, 39, 255});

    constexpr int titleFontSize = 38;
    constexpr const char* title = "VOLTRON: GALRA CYPHER";
    const int titleWidth = MeasureText(title, titleFontSize);
    DrawText(title,
             static_cast<int>((UI_WIDTH - titleWidth) * 0.5f),
             135,
             titleFontSize,
             RAYWHITE);

    constexpr int subtitleFontSize = 18;
    constexpr const char* subtitle = "Choose your next mission";
    const int subtitleWidth = MeasureText(subtitle, subtitleFontSize);
    DrawText(subtitle,
             static_cast<int>((UI_WIDTH - subtitleWidth) * 0.5f),
             195,
             subtitleFontSize,
             LIGHTGRAY);

    playButton.Draw(mousePosition);
    quitButton.Draw(mousePosition);
}
