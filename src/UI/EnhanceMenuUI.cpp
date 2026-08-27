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

// Upgrade Buttons (centered below stat bars, width 88px, height 14px, 10px font)
constexpr Rectangle BTN_HEALTH = {70.0f, 110.0f, 88.0f, 14.0f};
constexpr Rectangle BTN_SPEED = {70.0f, 162.0f, 88.0f, 14.0f};
constexpr Rectangle BTN_ATTACK_SPEED = {70.0f, 256.0f, 88.0f, 14.0f};
constexpr Rectangle BTN_DAMAGE = {525.0f, 168.0f, 88.0f, 14.0f};

constexpr float TEAM_CARD_X = 20.0f;
constexpr float TEAM_CARD_Y = 350.0f;
constexpr float TEAM_CARD_WIDTH = 205.0f;
constexpr float TEAM_CARD_HEIGHT = 68.0f;
constexpr float TEAM_CARD_GAP = 14.0f;

Rectangle TeamCardBounds(std::size_t index) {
    return {
        TEAM_CARD_X + index * (TEAM_CARD_WIDTH + TEAM_CARD_GAP),
        TEAM_CARD_Y,
        TEAM_CARD_WIDTH,
        TEAM_CARD_HEIGHT
    };
}

void DrawPanel(Rectangle bounds, const char* title) {
    UIUtils::DrawPanel(bounds, Color{24, 31, 44, 248});
    UIUtils::DrawCenteredText("PixeloidBold", title, { bounds.x + bounds.width * 0.5f, bounds.y + 16.0f }, UIUtils::FontSize::SMALL, Color{225, 234, 248, 255});
}

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

std::string FormatNumber(float value, int precision = 0) {
    std::ostringstream output;
    output << std::fixed << std::setprecision(precision) << value;
    return output.str();
}

void DrawUpgradeButton(
    Rectangle bounds,
    bool canUpgrade,
    Vector2 mousePosition
) {
    if (canUpgrade) {
        bool hovered = CheckCollisionPointRec(mousePosition, bounds);
        Color background = hovered ? Color{52, 152, 219, 255} : Color{41, 128, 185, 255};
        UIUtils::DrawPanel(bounds, background);
        UIUtils::DrawCenteredText("PixeloidBold", "+ LEVEL UP", { bounds.x + bounds.width * 0.5f, bounds.y + bounds.height * 0.5f }, static_cast<UIUtils::FontSize>(10), RAYWHITE);
    } else {
        UIUtils::DrawPanel(bounds, Color{45, 52, 65, 200});
        UIUtils::DrawCenteredText("PixeloidSans", "[ MAX ]", { bounds.x + bounds.width * 0.5f, bounds.y + bounds.height * 0.5f }, static_cast<UIUtils::FontSize>(10), Color{130, 140, 155, 255});
    }
}

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

EnhanceMenuUI::EnhanceMenuUI()
    : open(false),
      inspectedPaladin(PaladinId::Lance),
      feedbackTimer(0.0f) {
}

void EnhanceMenuUI::Open(PaladinId paladinId) {
    open = true;
    inspectedPaladin = paladinId;
    feedbackText.clear();
    feedbackTimer = 0.0f;
}

void EnhanceMenuUI::Close() {
    open = false;
    feedbackText.clear();
    feedbackTimer = 0.0f;
}

void EnhanceMenuUI::SetFeedback(const std::string& text) {
    feedbackText = text;
    feedbackTimer = 2.0f;
}

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

    if (IsKeyPressed(KEY_ESCAPE) || InputManager::IsInteractPressed()) {
        AudioManager::GetInstance().PlayRandomClick();
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

    // Health Upgrade
    if (CheckCollisionPointRec(mousePosition, BTN_HEALTH)) {
        if (inspected->UpgradeStat(StatType::Health)) {
            audioManager.PlaySoundEffect("fx_button_click");
            audioManager.PlaySoundEffect("fx_get_buff");
            SetFeedback("Max Health enhanced!");
        }
        return;
    }

    // Speed Upgrade
    if (CheckCollisionPointRec(mousePosition, BTN_SPEED)) {
        if (inspected->UpgradeStat(StatType::Speed)) {
            audioManager.PlaySoundEffect("fx_button_click");
            audioManager.PlaySoundEffect("fx_get_buff");
            SetFeedback("Movement Speed enhanced!");
        }
        return;
    }

    // Attack Speed Upgrade
    if (CheckCollisionPointRec(mousePosition, BTN_ATTACK_SPEED)) {
        if (inspected->UpgradeStat(StatType::AttackSpeed)) {
            audioManager.PlaySoundEffect("fx_button_click");
            audioManager.PlaySoundEffect("fx_get_buff");
            SetFeedback("Attack Speed enhanced!");
        }
        return;
    }

    // Damage Upgrade
    if (CheckCollisionPointRec(mousePosition, BTN_DAMAGE)) {
        if (inspected->UpgradeStat(StatType::Damage)) {
            audioManager.PlaySoundEffect("fx_button_click");
            audioManager.PlaySoundEffect("fx_get_buff");
            SetFeedback("Weapon Damage enhanced!");
        }
        return;
    }
}

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

    // --- Left Panel: Paladin Stats ---
    // 1. Health
    GUIStatBar::Draw(
        {32.0f, 96.0f, 164.0f, 10.0f},
        "Max Health",
        std::to_string(currentHp),
        (float)currentHp,
        maxHealthRef,
        Color{66, 190, 100, 255}
    );
    DrawUpgradeButton(BTN_HEALTH, livePaladin ? livePaladin->CanUpgradeStat(StatType::Health) : false, mousePosition);

    // 2. Speed
    GUIStatBar::Draw(
        {32.0f, 148.0f, 164.0f, 10.0f},
        "Speed",
        FormatNumber(currentSpd),
        currentSpd,
        maxSpeedRef,
        Color{77, 180, 225, 255}
    );
    DrawUpgradeButton(BTN_SPEED, livePaladin ? livePaladin->CanUpgradeStat(StatType::Speed) : false, mousePosition);

    // 3. EX Capacity
    GUIStatBar::Draw(
        {32.0f, 200.0f, 164.0f, 10.0f},
        "EX Capacity",
        FormatNumber(currentEx),
        currentEx,
        maxExRef,
        Color{163, 92, 224, 255}
    );

    // 4. Attack Speed
    GUIStatBar::Draw(
        {32.0f, 242.0f, 164.0f, 10.0f},
        "Attack Speed",
        FormatNumber(currentAtkSpd, 1) + "/s",
        currentAtkSpd,
        maxAttackSpeedRef,
        Color{235, 172, 59, 255}
    );
    DrawUpgradeButton(BTN_ATTACK_SPEED, livePaladin ? livePaladin->CanUpgradeStat(StatType::AttackSpeed) : false, mousePosition);

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
    DrawTextureAspectFit(weaponTexture, {495.0f, 76.0f, 148.0f, 56.0f});

    std::string damageText = (currentMinDmg == currentMaxDmg)
        ? std::to_string(currentMaxDmg)
        : (std::to_string(currentMinDmg) + "-" + std::to_string(currentMaxDmg));

    // 1. Damage
    GUIStatBar::Draw(
        {487.0f, 154.0f, 164.0f, 10.0f},
        "Damage",
        damageText,
        (float)currentMaxDmg,
        maxDamageRef,
        Color{220, 76, 70, 255}
    );
    DrawUpgradeButton(BTN_DAMAGE, livePaladin ? livePaladin->CanUpgradeStat(StatType::Damage) : false, mousePosition);

    // 2. Recoil
    GUIStatBar::Draw(
        {487.0f, 206.0f, 164.0f, 10.0f},
        "Recoil",
        definition.weapon.recoilApplicable ? FormatNumber(definition.weapon.recoil) : "N/A (melee)",
        definition.weapon.recoil,
        maxRecoilRef,
        Color{235, 142, 54, 255},
        definition.weapon.recoilApplicable
    );
    DrawWrappedText(
        definition.weapon.description,
        {487.0f, 238.0f, 164.0f, 65.0f},
        11,
        Color{205, 215, 230, 255}
    );

    // --- Bottom Section: Strike Team Slots ---
    UIUtils::DrawCenteredText("PixeloidSans", "Select a Paladin below to inspect and enhance stats", { 683.0f * 0.5f, 335.0f }, static_cast<UIUtils::FontSize>(14), Color{218, 226, 240, 255});

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
                {card.x + 7.0f, card.y + 8.0f, 62.0f, 45.0f}
            );
            const PaladinDefinition& memberDefinition = PaladinCatalog::Get(paladin->GetPaladinId());
            std::string slotLabel = "SLOT " + std::to_string(index + 1);
            UIUtils::DrawText("PixeloidSans", slotLabel, { card.x + 79.0f, card.y + 10.0f }, static_cast<UIUtils::FontSize>(12), GRAY);
            UIUtils::DrawText("PixeloidBold", memberDefinition.name, { card.x + 79.0f, card.y + 29.0f }, static_cast<UIUtils::FontSize>(18), RAYWHITE);
            UIUtils::DrawText("PixeloidSans", inspected ? "Active" : "Click to select", { card.x + 79.0f, card.y + 50.0f }, static_cast<UIUtils::FontSize>(10), inspected ? GOLD : Color{170, 184, 204, 255});
        }
    }

    DrawSimpleButton(BACK_BUTTON, "BACK", mousePosition);
    if (!feedbackText.empty()) {
        UIUtils::DrawText("PixeloidSans", feedbackText, { 22.0f, 473.0f }, static_cast<UIUtils::FontSize>(13), GOLD);
    }
}
