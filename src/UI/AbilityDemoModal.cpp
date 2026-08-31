#include "UI/AbilityDemoModal.h"
#include "UI/UIUtils.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Manager/AudioManager.h"
#include "Core/Constants.h"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace {
void DrawWrappedDemoText(
    const std::string& text,
    Rectangle bounds,
    int fontSize,
    Color color
) {
    std::istringstream stream(text);
    std::string word;
    std::string line;
    float y = bounds.y;
    while (stream >> word && y + fontSize <= bounds.y + bounds.height) {
        std::string candidate = line.empty() ? word : line + " " + word;
        if (!line.empty() && UIUtils::MeasureText("PixeloidSans", candidate, static_cast<UIUtils::FontSize>(fontSize)).x > bounds.width) {
            UIUtils::DrawText("PixeloidSans", line, { bounds.x, y }, static_cast<UIUtils::FontSize>(fontSize), color);
            line = word;
            y += fontSize + 2.0f;
        } else {
            line = candidate;
        }
    }
    if (!line.empty() && y + fontSize <= bounds.y + bounds.height) {
        UIUtils::DrawText("PixeloidSans", line, { bounds.x, y }, static_cast<UIUtils::FontSize>(fontSize), color);
    }
}
}


ModalLayout AbilityDemoModal::CalculateLayout(int screenWidth, int screenHeight) {
    ModalLayout layout;
    layout.scale = std::clamp(
        std::min(static_cast<float>(screenWidth) / 1280.0f, static_cast<float>(screenHeight) / 720.0f),
        0.75f,
        1.25f
    );

    // Design resolution reference: 683x512 virtual canvas
    float targetW = (screenWidth <= Constants::GAME_WIDTH + 10) ? static_cast<float>(Constants::GAME_WIDTH) : static_cast<float>(screenWidth);
    float targetH = (screenHeight <= Constants::GAME_HEIGHT + 10) ? static_cast<float>(Constants::GAME_HEIGHT) : static_cast<float>(screenHeight);

    float baseContainerW = targetW - 12.0f;
    float baseContainerH = targetH - 12.0f;

    float containerX = (targetW - baseContainerW) * 0.5f;
    float containerY = (targetH - baseContainerH) * 0.5f;
    layout.container = { containerX, containerY, baseContainerW, baseContainerH };

    float padX = 10.0f;
    float topPad = 32.0f;
    float bottomPad = 44.0f;
    float innerH = baseContainerH - topPad - bottomPad;
    float totalInnerW = baseContainerW - padX * 3.0f;

    // Stage gets 72% width, cards get 28%
    float stageW = std::floor(totalInnerW * 0.72f);
    float cardsW = totalInnerW - stageW;

    layout.stagePanel = { containerX + padX, containerY + topPad, stageW, innerH };
    layout.cardsPanel = { containerX + padX * 2.0f + stageW, containerY + topPad, cardsW, innerH };

    layout.cardGap = 10.0f;
    layout.cardHeight = (innerH - layout.cardGap * 2.0f) / 3.0f;

    float btnW = 85.0f;
    float btnH = 26.0f;
    layout.backButton = {
        containerX + baseContainerW - padX - btnW,
        containerY + baseContainerH - btnH - 10.0f,
        btnW,
        btnH
    };

    layout.titleFontSize = 14.0f;
    layout.cardTitleFontSize = 13.0f;
    layout.cardTagFontSize = 9.0f;
    layout.cardDescFontSize = 9.0f;
    layout.backBtnFontSize = 12.0f;

    return layout;
}

AbilityDemoModal::AbilityDemoModal()
    : open(false),
      currentPaladin(PaladinId::Lance) {
}

AbilityDemoModal::~AbilityDemoModal() = default;

void AbilityDemoModal::Open(PaladinId paladinId) {
    open = true;
    currentPaladin = paladinId;
    sandbox.Init(paladinId);
}

void AbilityDemoModal::Close() {
    open = false;
    sandbox.Reset();
}

void AbilityDemoModal::Update(float deltaTime, Vector2 mousePosition) {
    if (!open) return;

    if (IsKeyPressed(KEY_ESCAPE)) {
        AudioManager::GetInstance().PlayRandomClick();
        Close();
        return;
    }

    ModalLayout layout = CalculateLayout(Constants::GAME_WIDTH, Constants::GAME_HEIGHT);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (CheckCollisionPointRec(mousePosition, layout.backButton)) {
            AudioManager::GetInstance().PlayRandomClick();
            Close();
            return;
        }

        for (int i = 0; i < 3; ++i) {
            Rectangle cardBounds = layout.GetCardBounds(i);
            if (CheckCollisionPointRec(mousePosition, cardBounds)) {
                DemoPreviewMode selected = static_cast<DemoPreviewMode>(i);
                if (sandbox.GetMode() != selected) {
                    AudioManager::GetInstance().PlayRandomClick();
                    sandbox.SetMode(selected);
                }
                return;
            }
        }
    }

    sandbox.Update(deltaTime);
}

void AbilityDemoModal::Draw(Vector2 mousePosition) const {
    if (!open) return;

    ModalLayout layout = CalculateLayout(Constants::GAME_WIDTH, Constants::GAME_HEIGHT);

    // 1. Semi-transparent dark backdrop overlay
    DrawRectangle(-1000, -1000, 3000, 3000, ColorAlpha(BLACK, 0.75f));

    // 2. Main modal container
    UIUtils::DrawPanel(layout.container, Color{ 14, 18, 26, 252 });
    
    // Header Title
    const PaladinDefinition& def = sandbox.GetPaladinDefinition();
    std::string title = "ABILITY DEMO — " + def.name;
    UIUtils::DrawCenteredText("PixeloidBold", title, { layout.container.x + layout.container.width * 0.5f, layout.container.y + 14.0f }, UIUtils::FontSize::SMALL, GOLD);

    // 3. Left Showcase Arena with Scissor Clipping to avoid any out-of-bounds rendering
    float viewportScale = std::min(
        (float)GetScreenWidth() / Constants::GAME_WIDTH,
        (float)GetScreenHeight() / Constants::GAME_HEIGHT
    );
    Camera2D uiCamera = UIUtils::CreateCenteredUICamera(viewportScale);
    Vector2 pMin = GetWorldToScreen2D({ layout.stagePanel.x, layout.stagePanel.y }, uiCamera);
    Vector2 pMax = GetWorldToScreen2D({ layout.stagePanel.x + layout.stagePanel.width, layout.stagePanel.y + layout.stagePanel.height }, uiCamera);

    int scissorX = (int)std::floor(std::min(pMin.x, pMax.x));
    int scissorY = (int)std::floor(std::min(pMin.y, pMax.y));
    int scissorW = (int)std::ceil(std::fabs(pMax.x - pMin.x));
    int scissorH = (int)std::ceil(std::fabs(pMax.y - pMin.y));

    BeginScissorMode(scissorX, scissorY, scissorW, scissorH);
    sandbox.Draw(layout.stagePanel);
    EndScissorMode();

    // Crisp stage border over clipped contents
    DrawRectangleLinesEx(layout.stagePanel, 1.5f, Color{ 45, 62, 85, 255 });

    // 4. Right Action Cards (Const-correct, zero const_cast)
    DrawRightActionCards(layout, mousePosition);

    // 5. Back Button
    bool hovered = CheckCollisionPointRec(mousePosition, layout.backButton);
    Color btnColor = hovered ? Color{ 70, 92, 122, 255 } : Color{ 44, 58, 78, 255 };
    UIUtils::DrawPanel(layout.backButton, btnColor);
    UIUtils::DrawCenteredText("PixeloidSans", "BACK", { layout.backButton.x + layout.backButton.width * 0.5f, layout.backButton.y + layout.backButton.height * 0.5f }, static_cast<UIUtils::FontSize>(12), RAYWHITE);
}

void AbilityDemoModal::DrawRightActionCards(const ModalLayout& layout, Vector2 mousePosition) const {
    const PaladinDefinition& def = sandbox.GetPaladinDefinition();

    struct CardInfo {
        std::string tag;
        std::string title;
        std::string desc;
        Color tagColor;
    };

    CardInfo cards[3] = {
        { "BASIC ATTACK", def.weapon.name, def.weapon.description, Color{ 80, 190, 255, 255 } },
        { "SKILL (E)", def.skill.name, def.skill.description, Color{ 255, 180, 50, 255 } },
        { "ULTIMATE (Q)", def.ultimate.name, def.ultimate.description, Color{ 255, 80, 80, 255 } }
    };

    for (int i = 0; i < 3; ++i) {
        Rectangle card = layout.GetCardBounds(i);
        bool selected = (static_cast<int>(sandbox.GetMode()) == i);
        bool hovered = CheckCollisionPointRec(mousePosition, card);

        Color bg = selected ? Color{ 36, 48, 68, 255 }
                 : (hovered ? Color{ 28, 38, 54, 255 } : Color{ 18, 24, 34, 255 });
        Color border = selected ? GOLD : (hovered ? Color{ 90, 120, 160, 255 } : Color{ 40, 54, 76, 255 });

        UIUtils::DrawPanel(card, bg);
        DrawRectangleLinesEx(card, selected ? 2.0f : 1.0f, border);

        // Badge Tag
        UIUtils::DrawText("PixeloidBold", cards[i].tag, { card.x + 10.0f, card.y + 8.0f }, static_cast<UIUtils::FontSize>(9), cards[i].tagColor);
        // Card Title
        UIUtils::DrawText("PixeloidBold", cards[i].title, { card.x + 10.0f, card.y + 22.0f }, static_cast<UIUtils::FontSize>(12), RAYWHITE);
        // Card Description (Clean qualitative text without numeric values)
        DrawWrappedDemoText(cards[i].desc, { card.x + 10.0f, card.y + 40.0f, card.width - 20.0f, card.height - 46.0f }, 9, Color{ 185, 205, 225, 255 });
    }
}
