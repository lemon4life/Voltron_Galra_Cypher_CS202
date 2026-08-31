#include "UI/PaladinSelectionMenu.h"
#include "UI/UIUtils.h"

#include "Core/Manager/AssetManager.h"
#include "Core/Manager/AudioManager.h"
#include "Core/Manager/TeamManager.h"
#include "Entities/Player/Paladin.h"
#include "UI/GUIStatBar.h"
#include "UI/PaladinPortrait.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <vector>

namespace {
constexpr Rectangle CONTAINER = {8.0f, 8.0f, 667.0f, 496.0f};
constexpr Rectangle LEFT_PANEL = {20.0f, 50.0f, 188.0f, 270.0f};
constexpr Rectangle CENTER_PANEL = {218.0f, 50.0f, 247.0f, 270.0f};
constexpr Rectangle RIGHT_PANEL = {475.0f, 50.0f, 188.0f, 270.0f};
constexpr Rectangle DEMO_BUTTON = {288.0f, 323.0f, 106.0f, 22.0f};
constexpr Rectangle BACK_BUTTON = {575.0f, 466.0f, 76.0f, 28.0f};
constexpr float TEAM_CARD_X = 20.0f;
constexpr float TEAM_CARD_Y = 368.0f;
constexpr float TEAM_CARD_WIDTH = 205.0f;
constexpr float TEAM_CARD_HEIGHT = 68.0f;
constexpr float TEAM_CARD_GAP = 14.0f;

/// Calculates and returns team card bounds.
Rectangle TeamCardBounds(std::size_t index) {
    return {
        TEAM_CARD_X + index * (TEAM_CARD_WIDTH + TEAM_CARD_GAP),
        TEAM_CARD_Y,
        TEAM_CARD_WIDTH,
        TEAM_CARD_HEIGHT
    };
}

/// Renders stylized demo button.
void DrawDemoButton(
    Rectangle bounds,
    Vector2 mousePosition
) {
    bool hovered = UIUtils::IsHovered(bounds);
    Color background = hovered ? Color{48, 76, 112, 255}
                               : Color{28, 44, 68, 255};
    Color border = hovered ? GOLD : Color{80, 130, 190, 255};
    UIUtils::DrawPanel(bounds, background);
    DrawRectangleLinesEx(bounds, 1.5f, border);
    UIUtils::DrawCenteredText(
        "PixeloidBold",
        "[ DEMO ]",
        { bounds.x + bounds.width * 0.5f, bounds.y + bounds.height * 0.5f },
        static_cast<UIUtils::FontSize>(12),
        hovered ? GOLD : RAYWHITE
    );
}

/// Renders panel.
void DrawPanel(Rectangle bounds, const char* title) {
    UIUtils::DrawPanel(bounds, Color{24, 31, 44, 248});
    UIUtils::DrawCenteredText("PixeloidBold", title, { bounds.x + bounds.width * 0.5f, bounds.y + 16.0f }, UIUtils::FontSize::SMALL, Color{225, 234, 248, 255});
}

/// Renders wrapped text.
void DrawWrappedText(
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
            y += fontSize + 3.0f;
        } else {
            line = candidate;
        }
    }
    if (!line.empty() && y + fontSize <= bounds.y + bounds.height) {
        UIUtils::DrawText("PixeloidSans", line, { bounds.x, y }, static_cast<UIUtils::FontSize>(fontSize), color);
    }
}

/// Renders texture aspect fit.
void DrawTextureAspectFit(Texture2D texture, Rectangle bounds) {
    if (texture.id == 0) {
        return;
    }
    float scale = std::min(
        bounds.width / texture.width,
        bounds.height / texture.height
    );
    Rectangle destination = {
        bounds.x + (bounds.width - texture.width * scale) * 0.5f,
        bounds.y + (bounds.height - texture.height * scale) * 0.5f,
        texture.width * scale,
        texture.height * scale
    };
    DrawTexturePro(
        texture,
        {0.0f, 0.0f, (float)texture.width, (float)texture.height},
        destination,
        {0.0f, 0.0f},
        0.0f,
        WHITE
    );
}

/// Formats number.
std::string FormatNumber(float value, int precision = 0) {
    std::ostringstream output;
    output << std::fixed << std::setprecision(precision) << value;
    return output.str();
}

/// Renders simple button.
void DrawSimpleButton(
    Rectangle bounds,
    const char* label,
    Vector2 mousePosition
) {
    bool hovered = UIUtils::IsHovered(bounds);
    Color background = hovered ? Color{67, 88, 119, 255}
                               : Color{43, 56, 76, 255};
    UIUtils::DrawPanel(bounds, background);
    UIUtils::DrawCenteredText("PixeloidSans", label, { bounds.x + bounds.width * 0.5f, bounds.y + bounds.height * 0.5f }, static_cast<UIUtils::FontSize>(14), RAYWHITE);
}
}

/// Creates a PaladinSelectionMenu instance from the supplied configuration.
PaladinSelectionMenu::PaladinSelectionMenu()
    : open(false),
      inspectedPaladin(PaladinId::Lance),
      feedbackTimer(0.0f) {
}

/// Opens this menu and prepares its current selection.
void PaladinSelectionMenu::Open(PaladinId paladinId) {
    open = true;
    inspectedPaladin = paladinId;
    feedbackText.clear();
    feedbackTimer = 0.0f;
    demoModal.Close();
}

/// Closes this menu and clears transient interaction state.
void PaladinSelectionMenu::Close() {
    open = false;
    feedbackText.clear();
    feedbackTimer = 0.0f;
    demoModal.Close();
}

/// Updates the stored feedback.
void PaladinSelectionMenu::SetFeedback(const std::string& text) {
    feedbackText = text;
    feedbackTimer = 2.0f;
}

/// Advances this component's state for the current frame.
void PaladinSelectionMenu::Update(
    float deltaTime,
    Vector2 mousePosition,
    TeamManager& teamManager
) {
    if (!open) {
        return;
    }

    if (demoModal.IsOpen()) {
        demoModal.Update(deltaTime, mousePosition);
        return;
    }

    if (feedbackTimer > 0.0f) {
        feedbackTimer -= deltaTime;
        if (feedbackTimer <= 0.0f) {
            feedbackText.clear();
        }
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        AudioManager::GetInstance().PlayRandomClick();
        Close();
        return;
    }

    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        return;
    }

    AudioManager& audioManager = AudioManager::GetInstance();
    if (CheckCollisionPointRec(mousePosition, BACK_BUTTON)) {
        audioManager.PlayRandomClick();
        Close();
        return;
    }

    if (CheckCollisionPointRec(mousePosition, DEMO_BUTTON)) {
        audioManager.PlayRandomClick();
        demoModal.Open(inspectedPaladin);
        return;
    }

    const std::vector<Paladin*>& team = teamManager.GetTeam();
    for (std::size_t index = 0; index < team.size(); ++index) {
        if (CheckCollisionPointRec(mousePosition, TeamCardBounds(index))) {
            audioManager.PlayRandomClick();
            int previousIndex = teamManager.FindMemberIndex(inspectedPaladin);
            if (teamManager.AssignPaladinToSlot(inspectedPaladin, index)) {
                if (previousIndex == static_cast<int>(index)) {
                    SetFeedback("Paladin is already in that slot");
                } else {
                    SetFeedback("Paladin assigned to selected slot");
                }
            } else {
                SetFeedback("Unable to assign that slot");
            }
            return;
        }
    }
}

/// Renders this component using its current state and visual resources.
void PaladinSelectionMenu::Draw(
    Vector2 mousePosition,
    const TeamManager& teamManager
) const {
    if (!open) {
        return;
    }

    if (demoModal.IsOpen()) {
        demoModal.Draw(mousePosition);
        return;
    }

    const PaladinDefinition& definition =
        PaladinCatalog::Get(inspectedPaladin);
    const auto& catalog = PaladinCatalog::GetAll();

    UIUtils::DrawPanel(CONTAINER, Color{13, 18, 27, 252});
    const char* title = "PALADIN SELECTION";
    UIUtils::DrawCenteredText("PixeloidBold", title, { 683.0f * 0.5f, 17.0f + 12.0f }, UIUtils::FontSize::BODY, GOLD);

    DrawPanel(LEFT_PANEL, "PALADIN STATS");
    DrawPanel(CENTER_PANEL, definition.name.c_str());
    DrawPanel(RIGHT_PANEL, definition.weapon.name.c_str());

    float maxHealth = 0.0f;
    float maxSpeed = 0.0f;
    float maxEx = 0.0f;
    float maxAttackSpeed = 0.0f;
    float maxDamage = 0.0f;
    float maxRecoil = 0.0f;
    for (const PaladinDefinition& paladin : catalog) {
        maxHealth = std::max(maxHealth, BaseStats::HP * paladin.hpScalar);
        maxSpeed = std::max(maxSpeed, BaseStats::Speed * paladin.speedScalar);
        maxEx = std::max(maxEx, paladin.maxExEnergy);
        maxAttackSpeed = std::max(
            maxAttackSpeed,
            1.0f / (BaseStats::AttackCooldown * paladin.attackCooldownScalar)
        );
        maxDamage = std::max(maxDamage, BaseStats::Damage * paladin.weapon.maxDamageScalar);
        if (paladin.weapon.recoilApplicable) {
            maxRecoil = std::max(maxRecoil, paladin.weapon.recoil);
        }
    }

    GUIStatBar::Draw(
        {32.0f, 100.0f, 164.0f, 12.0f},
        "Max Health",
        std::to_string(static_cast<int>(BaseStats::HP * definition.hpScalar)),
        BaseStats::HP * definition.hpScalar,
        maxHealth,
        Color{66, 190, 100, 255}
    );
    GUIStatBar::Draw(
        {32.0f, 151.0f, 164.0f, 12.0f},
        "Speed",
        FormatNumber(BaseStats::Speed * definition.speedScalar),
        BaseStats::Speed * definition.speedScalar,
        maxSpeed,
        Color{77, 180, 225, 255}
    );
    GUIStatBar::Draw(
        {32.0f, 202.0f, 164.0f, 12.0f},
        "EX Capacity",
        FormatNumber(definition.maxExEnergy),
        definition.maxExEnergy,
        maxEx,
        Color{163, 92, 224, 255}
    );
    float attacksPerSecond = 1.0f / (BaseStats::AttackCooldown * definition.attackCooldownScalar);
    GUIStatBar::Draw(
        {32.0f, 253.0f, 164.0f, 12.0f},
        "Attack Speed",
        FormatNumber(attacksPerSecond, 1) + "/s",
        attacksPerSecond,
        maxAttackSpeed,
        Color{235, 172, 59, 255}
    );

    Texture2D idleTexture = AssetManager::GetInstance().GetTexture(
        definition.idleTextureKey
    );
    DrawPaladinFullBody(idleTexture, {239.0f, 82.0f, 205.0f, 170.0f});
    DrawWrappedText(
        definition.description,
        {231.0f, 265.0f, 221.0f, 44.0f},
        12,
        Color{210, 220, 235, 255}
    );

    Texture2D weaponTexture = AssetManager::GetInstance().GetTexture(
        definition.weapon.textureKey
    );
    DrawTextureAspectFit(weaponTexture, {495.0f, 81.0f, 148.0f, 72.0f});

    std::string damageText =
        definition.weapon.minDamageScalar == definition.weapon.maxDamageScalar
        ? std::to_string(static_cast<int>(BaseStats::Damage * definition.weapon.maxDamageScalar))
        : std::to_string(static_cast<int>(BaseStats::Damage * definition.weapon.minDamageScalar)) + "-" +
          std::to_string(static_cast<int>(BaseStats::Damage * definition.weapon.maxDamageScalar));
    GUIStatBar::Draw(
        {487.0f, 188.0f, 164.0f, 12.0f},
        "Damage",
        damageText,
        BaseStats::Damage * definition.weapon.maxDamageScalar,
        maxDamage,
        Color{220, 76, 70, 255}
    );
    GUIStatBar::Draw(
        {487.0f, 235.0f, 164.0f, 12.0f},
        "Recoil",
        definition.weapon.recoilApplicable
            ? FormatNumber(definition.weapon.recoil)
            : "N/A (melee)",
        definition.weapon.recoil,
        maxRecoil,
        Color{235, 142, 54, 255},
        definition.weapon.recoilApplicable
    );
    DrawWrappedText(
        definition.weapon.description,
        {487.0f, 263.0f, 164.0f, 47.0f},
        11,
        Color{205, 215, 230, 255}
    );

    DrawDemoButton(DEMO_BUTTON, mousePosition);

    std::string instruction =
        "Click a portrait card to place " + definition.name +
        " in that slot";
    UIUtils::DrawCenteredText("PixeloidSans", instruction, { 683.0f * 0.5f, 350.0f }, static_cast<UIUtils::FontSize>(13), Color{218, 226, 240, 255});

    const std::vector<Paladin*>& team = teamManager.GetTeam();
    for (std::size_t index = 0; index < team.size(); ++index) {
        Paladin* paladin = team[index];
        Rectangle card = TeamCardBounds(index);
        bool hovered = CheckCollisionPointRec(mousePosition, card);
        bool inspected =
            paladin && paladin->GetPaladinId() == inspectedPaladin;
        Color background = hovered
            ? Color{56, 75, 103, 255}
            : Color{35, 46, 64, 255};
        UIUtils::DrawPanel(card, background);

        if (paladin) {
            DrawPaladinPortrait(
                paladin,
                {card.x + 7.0f, card.y + 8.0f, 62.0f, 45.0f}
            );
            const PaladinDefinition& memberDefinition =
                PaladinCatalog::Get(paladin->GetPaladinId());
            std::string slotLabel = "SLOT " + std::to_string(index + 1);
            UIUtils::DrawText("PixeloidSans", slotLabel, { card.x + 79.0f, card.y + 10.0f }, static_cast<UIUtils::FontSize>(12), GRAY);
            UIUtils::DrawText("PixeloidBold", memberDefinition.name, { card.x + 79.0f, card.y + 29.0f }, static_cast<UIUtils::FontSize>(18), RAYWHITE);
            UIUtils::DrawText("PixeloidSans", "Click to assign", { card.x + 79.0f, card.y + 50.0f }, static_cast<UIUtils::FontSize>(10), Color{170, 184, 204, 255});
        }
    }

    DrawSimpleButton(BACK_BUTTON, "BACK", mousePosition);
    if (!feedbackText.empty()) {
        UIUtils::DrawText("PixeloidSans", feedbackText, { 22.0f, 473.0f }, static_cast<UIUtils::FontSize>(13), GOLD);
    }
}
