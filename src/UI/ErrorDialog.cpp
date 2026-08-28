#include "UI/ErrorDialog.h"

#include "raylib.h"

#include <algorithm>
#include <exception>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

constexpr int ERROR_WINDOW_WIDTH = 900;
constexpr int ERROR_WINDOW_HEIGHT = 560;
constexpr float MIN_ERROR_FONT_SIZE = 12.0f;

bool TextFits(const Font& font,
              const std::string& text,
              float fontSize,
              float spacing,
              float maxWidth) {
    return MeasureTextEx(font, text.c_str(), fontSize, spacing).x <= maxWidth;
}

std::vector<std::string> WrapErrorText(const std::string& text,
                                       const Font& font,
                                       float fontSize,
                                       float spacing,
                                       float maxWidth) {
    std::vector<std::string> lines;
    std::istringstream paragraphs(text);
    std::string paragraph;

    while (std::getline(paragraphs, paragraph)) {
        if (paragraph.empty()) {
            lines.emplace_back();
            continue;
        }

        std::istringstream words(paragraph);
        std::string word;
        std::string currentLine;

        while (words >> word) {
            std::string candidate = currentLine.empty()
                ? word
                : currentLine + " " + word;

            if (TextFits(font, candidate, fontSize, spacing, maxWidth)) {
                currentLine = std::move(candidate);
                continue;
            }

            if (!currentLine.empty()) {
                lines.push_back(currentLine);
                currentLine.clear();
            }

            while (!word.empty() &&
                   !TextFits(font, word, fontSize, spacing, maxWidth)) {
                std::size_t splitPosition = 1;
                while (splitPosition < word.size() &&
                       TextFits(font,
                                word.substr(0, splitPosition + 1),
                                fontSize,
                                spacing,
                                maxWidth)) {
                    ++splitPosition;
                }

                lines.push_back(word.substr(0, splitPosition));
                word.erase(0, splitPosition);
            }

            currentLine = word;
        }

        if (!currentLine.empty()) {
            lines.push_back(currentLine);
        }
    }

    if (lines.empty()) {
        lines.emplace_back("Unknown error.");
    }
    return lines;
}

bool EnsureErrorWindow() {
    if (IsWindowReady()) {
        return true;
    }

    InitWindow(ERROR_WINDOW_WIDTH,
               ERROR_WINDOW_HEIGHT,
               "Voltron Mission - Unexpected Error");
    if (!IsWindowReady()) {
        return false;
    }

    SetTargetFPS(60);
    return true;
}

} // namespace

namespace ErrorDialog {

void Show(const std::string& errorText) {
    if (!EnsureErrorWindow()) {
        std::cerr << "Fatal error: " << errorText << '\n';
        return;
    }

    SetExitKey(KEY_NULL);
    float scrollOffset = 0.0f;
    bool closeRequested = false;

    while (!closeRequested && !WindowShouldClose()) {
        const float screenWidth = static_cast<float>(GetScreenWidth());
        const float screenHeight = static_cast<float>(GetScreenHeight());
        const float outerMargin = std::clamp(
            std::min(screenWidth, screenHeight) * 0.04f,
            12.0f,
            36.0f
        );
        const float panelWidth = std::min(
            1000.0f,
            std::max(280.0f, screenWidth - outerMargin * 2.0f)
        );
        const float panelHeight = std::min(
            720.0f,
            std::max(210.0f, screenHeight - outerMargin * 2.0f)
        );
        const Rectangle panel = {
            (screenWidth - panelWidth) * 0.5f,
            (screenHeight - panelHeight) * 0.5f,
            panelWidth,
            panelHeight
        };
        const float padding = std::clamp(panelWidth * 0.035f, 12.0f, 28.0f);
        const float titleFontSize = std::clamp(
            panelHeight * 0.065f,
            20.0f,
            34.0f
        );
        const float buttonHeight = std::clamp(
            panelHeight * 0.10f,
            36.0f,
            50.0f
        );
        const float buttonWidth = std::clamp(
            panelWidth * 0.28f,
            150.0f,
            230.0f
        );
        const Rectangle closeButton = {
            panel.x + (panel.width - buttonWidth) * 0.5f,
            panel.y + panel.height - padding - buttonHeight,
            buttonWidth,
            buttonHeight
        };
        const Rectangle textArea = {
            panel.x + padding,
            panel.y + padding + titleFontSize + 18.0f,
            panel.width - padding * 2.0f,
            std::max(
                40.0f,
                closeButton.y -
                    (panel.y + padding + titleFontSize + 18.0f) -
                    18.0f
            )
        };

        const Font font = GetFontDefault();
        float bodyFontSize = std::clamp(
            panelHeight * 0.038f,
            14.0f,
            24.0f
        );
        constexpr float spacing = 1.0f;
        std::vector<std::string> lines;
        float lineHeight = 0.0f;
        float textHeight = 0.0f;

        do {
            lines = WrapErrorText(errorText,
                                  font,
                                  bodyFontSize,
                                  spacing,
                                  textArea.width);
            lineHeight = bodyFontSize * 1.35f;
            textHeight = static_cast<float>(lines.size()) * lineHeight;
            if (textHeight <= textArea.height ||
                bodyFontSize <= MIN_ERROR_FONT_SIZE) {
                break;
            }
            bodyFontSize -= 1.0f;
        } while (true);

        const float maximumScroll = std::max(
            0.0f,
            textHeight - textArea.height
        );
        scrollOffset = std::clamp(scrollOffset, 0.0f, maximumScroll);

        if (CheckCollisionPointRec(GetMousePosition(), textArea)) {
            scrollOffset -= GetMouseWheelMove() * lineHeight * 3.0f;
        }
        if (IsKeyDown(KEY_DOWN)) {
            scrollOffset += lineHeight * 6.0f * GetFrameTime();
        }
        if (IsKeyDown(KEY_UP)) {
            scrollOffset -= lineHeight * 6.0f * GetFrameTime();
        }
        scrollOffset = std::clamp(scrollOffset, 0.0f, maximumScroll);

        const bool buttonHovered = CheckCollisionPointRec(
            GetMousePosition(),
            closeButton
        );
        closeRequested = (buttonHovered &&
                          IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) ||
                         IsKeyPressed(KEY_ENTER) ||
                         IsKeyPressed(KEY_ESCAPE);

        BeginDrawing();
        ClearBackground(Color{13, 15, 22, 255});
        DrawRectangleRounded(panel, 0.025f, 8, Color{31, 34, 45, 255});
        DrawRectangleRoundedLines(
            panel,
            0.025f,
            8,
            Color{220, 76, 76, 255}
        );
        DrawTextEx(font,
                   "The game encountered an unexpected error",
                   Vector2{panel.x + padding, panel.y + padding},
                   titleFontSize,
                   spacing,
                   Color{255, 224, 224, 255});

        BeginScissorMode(static_cast<int>(textArea.x),
                         static_cast<int>(textArea.y),
                         static_cast<int>(textArea.width),
                         static_cast<int>(textArea.height));
        for (std::size_t index = 0; index < lines.size(); ++index) {
            const float lineY = textArea.y +
                                static_cast<float>(index) * lineHeight -
                                scrollOffset;
            if (lineY + lineHeight < textArea.y ||
                lineY > textArea.y + textArea.height) {
                continue;
            }
            DrawTextEx(font,
                       lines[index].c_str(),
                       Vector2{textArea.x, lineY},
                       bodyFontSize,
                       spacing,
                       Color{231, 233, 239, 255});
        }
        EndScissorMode();

        if (maximumScroll > 0.0f) {
            const float scrollTrackHeight = textArea.height;
            const float scrollThumbHeight = std::max(
                20.0f,
                scrollTrackHeight * (textArea.height / textHeight)
            );
            const float scrollThumbY = textArea.y +
                (scrollTrackHeight - scrollThumbHeight) *
                (scrollOffset / maximumScroll);
            DrawRectangle(
                static_cast<int>(textArea.x + textArea.width - 3.0f),
                static_cast<int>(textArea.y),
                3,
                static_cast<int>(scrollTrackHeight),
                Color{65, 69, 82, 255}
            );
            DrawRectangle(
                static_cast<int>(textArea.x + textArea.width - 3.0f),
                static_cast<int>(scrollThumbY),
                3,
                static_cast<int>(scrollThumbHeight),
                Color{220, 76, 76, 255}
            );
        }

        DrawRectangleRounded(
            closeButton,
            0.18f,
            8,
            buttonHovered ? Color{205, 70, 70, 255}
                          : Color{171, 55, 55, 255}
        );
        const char* buttonText = "Close safely";
        const float buttonFontSize = std::clamp(
            buttonHeight * 0.42f,
            15.0f,
            20.0f
        );
        const Vector2 buttonTextSize = MeasureTextEx(
            font,
            buttonText,
            buttonFontSize,
            spacing
        );
        DrawTextEx(
            font,
            buttonText,
            Vector2{
                closeButton.x +
                    (closeButton.width - buttonTextSize.x) * 0.5f,
                closeButton.y +
                    (closeButton.height - buttonTextSize.y) * 0.5f
            },
            buttonFontSize,
            spacing,
            WHITE
        );
        EndDrawing();
    }
}

std::string GetCurrentExceptionMessage() {
    try {
        throw;
    } catch (const std::exception& error) {
        return error.what();
    } catch (...) {
        return "An unknown non-standard exception was thrown.";
    }
}

} // namespace ErrorDialog
