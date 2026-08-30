#include "UI/EnhanceMenuUI.h"
#include "UI/UIUtils.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Manager/AudioManager.h"
#include "Core/Manager/TeamManager.h"
#include "Core/Manager/InputManager.h"
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
constexpr Rectangle BACK_BUTTON = {575.0f, 466.0f, 76.0f, 28.0f};

// Unified Level Up Button centered horizontally between top inspector and bottom slot list
constexpr Rectangle LEVEL_UP_BUTTON = {(683.0f - 160.0f) * 0.5f, 326.0f, 160.0f, 40.0f};

constexpr float TEAM_CARD_X = 20.0f;
constexpr float TEAM_CARD_Y = 390.0f;
constexpr float TEAM_CARD_WIDTH = 205.0f;
constexpr float TEAM_CARD_HEIGHT = 64.0f;
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
    if (texture.id == 0) return;
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

/// Renders level up button.
void DrawLevelUpButton(
    Rectangle bounds,
    int paladinLevel,
    int maxLevel,
    int cost,
    int currentCoins,
    Vector2 mousePosition
) {
    if (paladinLevel >= maxLevel) {
        UIUtils::DrawPanel(bounds, Color{45, 52, 65, 220});
        DrawRectangleLinesEx(bounds, 1.0f, Color{80, 90, 105, 255});
        UIUtils::DrawCenteredText("PixeloidBold", "[ MAX LEVEL ]", { bounds.x + bounds.width * 0.5f, bounds.y + bounds.height * 0.5f }, static_cast<UIUtils::FontSize>(12), Color{140, 150, 165, 255});
    } else if (currentCoins >= cost) {
        bool hovered = CheckCollisionPointRec(mousePosition, bounds);
        Color background = hovered ? Color{52, 152, 219, 255} : Color{35, 110, 170, 255};
        Color border = hovered ? GOLD : Color{90, 185, 255, 255};
        UIUtils::DrawPanel(bounds, background);
        DrawRectangleLinesEx(bounds, hovered ? 2.0f : 1.0f, border);

        // Line 1: "Level Up"
        UIUtils::DrawCenteredText("PixeloidBold", "LEVEL UP", { bounds.x + bounds.width * 0.5f, bounds.y + 13.0f }, static_cast<UIUtils::FontSize>(12), RAYWHITE);

        // Line 2: Coin Icon + Cost
        Texture2D coinIcon = AssetManager::GetInstance().GetTexture("coin_icon");
        std::string costStr = std::to_string(cost);
        Font fontBold = AssetManager::GetInstance().GetCustomFont("PixeloidBold");
        Vector2 numSize = MeasureTextEx(fontBold, costStr.c_str(), 11.0f, 1.0f);
        float iconSize = 12.0f;
        float totalW = (coinIcon.id != 0 ? (iconSize + 4.0f) : 0.0f) + numSize.x;
        float startX = bounds.x + (bounds.width - totalW) * 0.5f;
        float line2Y = bounds.y + 22.0f;

        if (coinIcon.id != 0) {
            DrawTexturePro(
                coinIcon,
                { 0.0f, 0.0f, (float)coinIcon.width, (float)coinIcon.height },
                { startX, line2Y, iconSize, iconSize },
                { 0.0f, 0.0f },
                0.0f,
                WHITE
            );
            UIUtils::DrawText("PixeloidBold", costStr.c_str(), { startX + iconSize + 4.0f, line2Y }, static_cast<UIUtils::FontSize>(11), Color{255, 223, 80, 255});
        } else {
            UIUtils::DrawText("PixeloidBold", (costStr + " Coins").c_str(), { startX, line2Y }, static_cast<UIUtils::FontSize>(11), Color{255, 223, 80, 255});
        }
    } else {
        bool hovered = CheckCollisionPointRec(mousePosition, bounds);
        Color background = hovered ? Color{65, 30, 35, 240} : Color{48, 24, 28, 220};
        UIUtils::DrawPanel(bounds, background);
        DrawRectangleLinesEx(bounds, 1.0f, Color{120, 50, 55, 200});

        // Line 1: "Level Up" (Dimmed)
        UIUtils::DrawCenteredText("PixeloidBold", "LEVEL UP", { bounds.x + bounds.width * 0.5f, bounds.y + 13.0f }, static_cast<UIUtils::FontSize>(12), Color{180, 180, 190, 220});

        // Line 2: Coin Icon (Dimmed) + Cost
        Texture2D coinIcon = AssetManager::GetInstance().GetTexture("coin_icon");
        std::string costStr = std::to_string(cost);
        Font fontBold = AssetManager::GetInstance().GetCustomFont("PixeloidBold");
        Vector2 numSize = MeasureTextEx(fontBold, costStr.c_str(), 11.0f, 1.0f);
        float iconSize = 12.0f;
        float totalW = (coinIcon.id != 0 ? (iconSize + 4.0f) : 0.0f) + numSize.x;
        float startX = bounds.x + (bounds.width - totalW) * 0.5f;
        float line2Y = bounds.y + 22.0f;

        if (coinIcon.id != 0) {
            DrawTexturePro(
                coinIcon,
                { 0.0f, 0.0f, (float)coinIcon.width, (float)coinIcon.height },
                { startX, line2Y, iconSize, iconSize },
                { 0.0f, 0.0f },
                0.0f,
                ColorAlpha(WHITE, 0.6f)
            );
            UIUtils::DrawText("PixeloidBold", costStr.c_str(), { startX + iconSize + 4.0f, line2Y }, static_cast<UIUtils::FontSize>(11), Color{235, 120, 120, 240});
        } else {
            UIUtils::DrawText("PixeloidBold", (costStr + " Coins").c_str(), { startX, line2Y }, static_cast<UIUtils::FontSize>(11), Color{235, 120, 120, 240});
        }
    }
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

/// Creates a EnhanceMenuUI instance from the supplied configuration.
EnhanceMenuUI::EnhanceMenuUI()
    : open(false),
      inspectedPaladin(PaladinId::Lance),
      feedbackTimer(0.0f) {
}

/// Opens this menu and prepares its current selection.
void EnhanceMenuUI::Open(PaladinId paladinId) {
    open = true;
    inspectedPaladin = paladinId;
    feedbackText.clear();
    feedbackTimer = 0.0f;
}

/// Closes this menu and clears transient interaction state.
void EnhanceMenuUI::Close() {
    open = false;
    feedbackText.clear();
    feedbackTimer = 0.0f;
}

/// Updates the stored feedback.
void EnhanceMenuUI::SetFeedback(const std::string& text) {
    feedbackText = text;
    feedbackTimer = 2.5f;
}

/// Advances this component's state for the current frame.
void EnhanceMenuUI::Update(
    float deltaTime,
    Vector2 mousePosition,
    TeamManager& teamManager
) {
    if (!open) return;

    if (feedbackTimer > 0.0f) {
        feedbackTimer -= deltaTime;
        if (feedbackTimer <= 0.0f) {
            feedbackText.clear();
        }
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        Close();
        return;
    }

    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        return;
    }

    AudioManager& audioManager = AudioManager::GetInstance();

    // Back Button Click
    if (CheckCollisionPointRec(mousePosition, BACK_BUTTON)) {
        audioManager.PlayRandomClick();
        Close();
        return;
    }

    // Team Slot Selection
    const std::vector<Paladin*>& team = teamManager.GetTeam();
    for (std::size_t index = 0; index < team.size(); ++index) {
        if (CheckCollisionPointRec(mousePosition, TeamCardBounds(index))) {
            if (team[index]) {
                audioManager.PlayRandomClick();
                inspectedPaladin = team[index]->GetPaladinId();
            }
            return;
        }
    }

    // Locate active inspected Paladin
    Paladin* inspected = nullptr;
    for (Paladin* p : team) {
        if (p && p->GetPaladinId() == inspectedPaladin) {
            inspected = p;
            break;
        }
    }

    if (!inspected) return;

    // Single Unified Level Up Button Click
    if (CheckCollisionPointRec(mousePosition, LEVEL_UP_BUTTON)) {
        if (inspected->IsMaxLevel()) {
            // Already maxed
        } else if (!inspected->CanLevelUp(teamManager.GetCoins())) {
            audioManager.PlaySoundEffect("fx_button_click");
            SetFeedback("Not enough Coins!");
        } else if (inspected->LevelUp()) {
            audioManager.PlaySoundEffect("fx_button_click");
            const std::string& name = PaladinCatalog::Get(inspected->GetPaladinId()).name;
            SetFeedback(name + " leveled up to Tier " + std::to_string(inspected->GetPaladinLevel()) + "!");
        }
        return;
    }
}

/// Renders this component using its current state and visual resources.
void EnhanceMenuUI::Draw(
    Vector2 mousePosition,
    const TeamManager& teamManager
) const {
    if (!open) return;

    const PaladinDefinition& definition = PaladinCatalog::Get(inspectedPaladin);
    const auto& catalog = PaladinCatalog::GetAll();

    // Find live inspected Paladin instance for current upgraded stats
    const Paladin* livePaladin = nullptr;
    const std::vector<Paladin*>& team = teamManager.GetTeam();
    for (const Paladin* p : team) {
        if (p && p->GetPaladinId() == inspectedPaladin) {
            livePaladin = p;
            break;
        }
    }

    // Overall Container
    UIUtils::DrawPanel(CONTAINER, Color{13, 18, 27, 252});
    UIUtils::DrawCenteredText("PixeloidBold", "ENHANCE MACHINE", { 683.0f * 0.5f, 27.0f }, UIUtils::FontSize::BODY, GOLD);

    // Header Coin Balance Badge
    int userCoins = teamManager.GetCoins();
    Rectangle coinBadge = {
        CONTAINER.x + CONTAINER.width - 105.0f,
        16.0f,
        90.0f,
        22.0f
    };
    DrawRectangleRounded(coinBadge, 0.3f, 4, ColorAlpha(Color{10, 14, 22, 255}, 0.9f));
    DrawRectangleRoundedLinesEx(coinBadge, 0.3f, 4, 1.0f, ColorAlpha(GRAY, 0.4f));

    Texture2D coinIcon = AssetManager::GetInstance().GetTexture("coin_icon");
    if (coinIcon.id != 0) {
        float iconSize = 14.0f;
        DrawTexturePro(
            coinIcon,
            { 0.0f, 0.0f, (float)coinIcon.width, (float)coinIcon.height },
            { coinBadge.x + 6.0f, coinBadge.y + (coinBadge.height - iconSize) * 0.5f, iconSize, iconSize },
            { 0.0f, 0.0f },
            0.0f,
            WHITE
        );
    }
    std::string coinStr = std::to_string(userCoins);
    UIUtils::DrawText("PixeloidMono", coinStr.c_str(), { coinBadge.x + 24.0f, coinBadge.y + 4.0f }, static_cast<UIUtils::FontSize>(11), Color{255, 223, 80, 255});

    DrawPanel(LEFT_PANEL, "PALADIN STATS");
    DrawPanel(CENTER_PANEL, definition.name.c_str());
    DrawPanel(RIGHT_PANEL, definition.weapon.name.c_str());

    // Max Stat references for progress bar scaling
    float maxHealthRef = 0.0f;
    float maxSpeedRef = 0.0f;
    float maxExRef = 0.0f;
    float maxAttackSpeedRef = 0.0f;
    float maxDamageRef = 0.0f;
    float maxRecoilRef = 0.0f;

    for (const PaladinDefinition& paladin : catalog) {
        maxHealthRef = std::max(maxHealthRef, BaseStats::HP * paladin.hpScalar * 2.5f);
        maxSpeedRef = std::max(maxSpeedRef, BaseStats::Speed * paladin.speedScalar * 1.6f);
        maxExRef = std::max(maxExRef, paladin.maxExEnergy);
        maxAttackSpeedRef = std::max(
            maxAttackSpeedRef,
            1.0f / (BaseStats::AttackCooldown * paladin.attackCooldownScalar * 0.4f)
        );
        maxDamageRef = std::max(maxDamageRef, BaseStats::Damage * paladin.weapon.maxDamageScalar * 2.5f);
        if (paladin.weapon.recoilApplicable) {
            maxRecoilRef = std::max(maxRecoilRef, paladin.weapon.recoil);
        }
    }

    // Current live values
    int currentHp = livePaladin ? livePaladin->GetMaxHealth() : static_cast<int>(BaseStats::HP * definition.hpScalar);
    float currentSpd = livePaladin ? livePaladin->GetSpeed() : (BaseStats::Speed * definition.speedScalar);
    float currentEx = livePaladin ? livePaladin->GetMaxExEnergy() : definition.maxExEnergy;
    float currentCooldown = livePaladin ? livePaladin->GetBaseAttackCooldown() : (BaseStats::AttackCooldown * definition.attackCooldownScalar);
    float currentAtkSpd = (currentCooldown > 0.001f) ? (1.0f / currentCooldown) : 2.0f;

    float currentDmgScalar = livePaladin ? livePaladin->GetDamageScalar() : 1.0f;
    int currentMinDmg = static_cast<int>(BaseStats::Damage * definition.weapon.minDamageScalar * currentDmgScalar);
    int currentMaxDmg = static_cast<int>(BaseStats::Damage * definition.weapon.maxDamageScalar * currentDmgScalar);

    // --- Left Panel: Paladin Stats (Cleaned of individual buttons, perfectly spaced) ---
    // 1. Health
    GUIStatBar::Draw(
        {32.0f, 100.0f, 164.0f, 12.0f},
        "Max Health",
        std::to_string(currentHp),
        (float)currentHp,
        maxHealthRef,
        Color{66, 190, 100, 255}
    );

    // 2. Speed
    GUIStatBar::Draw(
        {32.0f, 151.0f, 164.0f, 12.0f},
        "Speed",
        FormatNumber(currentSpd),
        currentSpd,
        maxSpeedRef,
        Color{77, 180, 225, 255}
    );

    // 3. EX Capacity
    GUIStatBar::Draw(
        {32.0f, 202.0f, 164.0f, 12.0f},
        "EX Capacity",
        FormatNumber(currentEx),
        currentEx,
        maxExRef,
        Color{163, 92, 224, 255}
    );

    // 4. Attack Speed
    GUIStatBar::Draw(
        {32.0f, 253.0f, 164.0f, 12.0f},
        "Attack Speed",
        FormatNumber(currentAtkSpd, 1) + "/s",
        currentAtkSpd,
        maxAttackSpeedRef,
        Color{235, 172, 59, 255}
    );

    // --- Center Panel: Paladin Bio & Sprite ---
    Texture2D idleTexture = AssetManager::GetInstance().GetTexture(definition.idleTextureKey);
    DrawPaladinFullBody(idleTexture, {239.0f, 78.0f, 205.0f, 170.0f});
    DrawWrappedText(
        definition.description,
        {231.0f, 258.0f, 221.0f, 52.0f},
        12,
        Color{210, 220, 235, 255}
    );

    // --- Right Panel: Weapon Stats ---
    Texture2D weaponTexture = AssetManager::GetInstance().GetTexture(definition.weapon.textureKey);
    DrawTextureAspectFit(weaponTexture, {495.0f, 81.0f, 148.0f, 72.0f});

    std::string damageText = (currentMinDmg == currentMaxDmg)
        ? std::to_string(currentMaxDmg)
        : (std::to_string(currentMinDmg) + "-" + std::to_string(currentMaxDmg));

    // 1. Damage
    GUIStatBar::Draw(
        {487.0f, 188.0f, 164.0f, 12.0f},
        "Damage",
        damageText,
        (float)currentMaxDmg,
        maxDamageRef,
        Color{220, 76, 70, 255}
    );

    // 2. Recoil
    GUIStatBar::Draw(
        {487.0f, 235.0f, 164.0f, 12.0f},
        "Recoil",
        definition.weapon.recoilApplicable ? FormatNumber(definition.weapon.recoil) : "N/A (melee)",
        definition.weapon.recoil,
        maxRecoilRef,
        Color{235, 142, 54, 255},
        definition.weapon.recoilApplicable
    );
    DrawWrappedText(
        definition.weapon.description,
        {487.0f, 262.0f, 164.0f, 48.0f},
        11,
        Color{205, 215, 230, 255}
    );

    // --- Middle Section: Single Unified Level Up Button ---
    int paladinLvl = livePaladin ? livePaladin->GetPaladinLevel() : 1;
    int upgradeCost = livePaladin ? livePaladin->GetUpgradeCost() : 5;
    DrawLevelUpButton(LEVEL_UP_BUTTON, paladinLvl, Paladin::MAX_PALADIN_LEVEL, upgradeCost, userCoins, mousePosition);

    // --- Bottom Section: Strike Team Slots ---
    UIUtils::DrawCenteredText("PixeloidSans", "Select a Paladin below to inspect and enhance stats", { 683.0f * 0.5f, 374.0f }, static_cast<UIUtils::FontSize>(12), Color{218, 226, 240, 255});

    for (std::size_t index = 0; index < team.size(); ++index) {
        Paladin* paladin = team[index];
        Rectangle card = TeamCardBounds(index);
        bool hovered = CheckCollisionPointRec(mousePosition, card);
        bool inspected = paladin && (paladin->GetPaladinId() == inspectedPaladin);

        Color background = inspected
            ? Color{60, 95, 135, 255}
            : (hovered ? Color{56, 75, 103, 255} : Color{35, 46, 64, 255});

        UIUtils::DrawPanel(card, background);
        if (inspected) {
            DrawRectangleLinesEx(card, 2.0f, GOLD);
        }

        if (paladin) {
            DrawPaladinPortrait(
                paladin,
                {card.x + 7.0f, card.y + 7.0f, 62.0f, 50.0f}
            );
            const PaladinDefinition& memberDefinition = PaladinCatalog::Get(paladin->GetPaladinId());
            std::string slotLabel = "SLOT " + std::to_string(index + 1) + " (Tier " + std::to_string(paladin->GetPaladinLevel()) + ")";
            UIUtils::DrawText("PixeloidSans", slotLabel, { card.x + 77.0f, card.y + 8.0f }, static_cast<UIUtils::FontSize>(11), GRAY);
            UIUtils::DrawText("PixeloidBold", memberDefinition.name, { card.x + 77.0f, card.y + 24.0f }, static_cast<UIUtils::FontSize>(16), RAYWHITE);
            UIUtils::DrawText("PixeloidSans", inspected ? "Selected" : "Click to select", { card.x + 77.0f, card.y + 44.0f }, static_cast<UIUtils::FontSize>(10), inspected ? GOLD : Color{170, 184, 204, 255});
        }
    }

    DrawSimpleButton(BACK_BUTTON, "BACK", mousePosition);
    if (!feedbackText.empty()) {
        UIUtils::DrawText("PixeloidSans", feedbackText, { 22.0f, 473.0f }, static_cast<UIUtils::FontSize>(13), GOLD);
    }
}
