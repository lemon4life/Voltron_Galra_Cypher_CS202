#include "UI/UIManager.h"
#include "UI/GameplayHUDLayout.h"
#include "UI/UIUtils.h"
#include "Core/Constants.h"
#include "Core/Manager/InputManager.h"
#include "Core/Manager/TeamManager.h"
#include "Entities/Player/Paladin.h"
#include "Entities/EnemyEntities/Boss.h"
#include "Core/Manager/GameManager.h"
#include "UI/PaladinPortrait.h"
#include "Core/Manager/AssetManager.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

/// Creates a UIManager instance from the supplied configuration.
UIManager::UIManager() : teamManager(nullptr) {
    statsShell.id = 0;
    statsShellBack.id = 0;
}

/// Releases resources owned by this UIManager instance.
UIManager::~UIManager() {
}

/// Initializes the resources and collaborators required before this component can run.
void UIManager::Initialize() {
    AssetManager& assets = AssetManager::GetInstance();
    statsShell = assets.LoadTexture2D(
        "Team_StatsShell",
        "assets/UI/Team_StatsShell.png"
    );
    statsShellBack = assets.LoadTexture2D(
        "Team_StatsShell_Back",
        "assets/UI/Team_StatsShell_Back.png"
    );
}

/// Reports whether the pause button pressed condition is satisfied.
bool UIManager::IsPauseButtonPressed(Rectangle windowBounds, Vector2 mousePosition) const {
    if (!teamManager) return false;
    GameplayHUDLayout::Result layout = GameplayHUDLayout::Calculate(
        windowBounds,
        teamManager->GetTeam().size()
    );
    return CheckCollisionPointRec(
               mousePosition,
               layout.pauseButtonBounds
           ) &&
           IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

/// Handles the player stats changed event.
void UIManager::OnPlayerStatsChanged(const PlayerStatsSnapshot& stats, int slotIndex) {
    if (slotIndex >= static_cast<int>(cachedPlayerStats.size())) {
        cachedPlayerStats.resize(slotIndex + 1);
    }
    cachedPlayerStats[slotIndex] = stats;
    hasReceivedStats = true;
}

/// Handles the team stats changed event.
void UIManager::OnTeamStatsChanged(const TeamStatsSnapshot& stats) {
    cachedTeamStats = stats;
    hasReceivedStats = true;
}

/// Renders hud.
void UIManager::DrawHUD(Rectangle windowBounds, Vector2 mousePosition) {
    if (!teamManager) return;
    DrawTeamHUD(teamManager, windowBounds, mousePosition);

    Boss* primaryBoss = GameManager::GetInstance().GetObjectManager().FindPrimaryBoss();
    if (primaryBoss && !primaryBoss->IsDead() && !primaryBoss->IsSpawnSequenceActive()) {
        DrawBossHealthBar(primaryBoss, windowBounds, GetFrameTime());
    }
}

/// Renders team hud.
void UIManager::DrawTeamHUD(
    TeamManager* team,
    Rectangle windowBounds,
    Vector2 mousePosition
) {
    if (!team) return;
    Paladin* active = team->GetActivePaladin();
    if (!active) return;
    
    std::vector<Paladin*> roster = team->GetTeam();
    int numPaladins = roster.size();
    if (numPaladins == 0) return;

    GameplayHUDLayout::Result layout = GameplayHUDLayout::Calculate(
        windowBounds,
        roster.size()
    );
    float S = layout.scale;
    float baseShellWidth = layout.baseShellWidth;
    float shellWidth = layout.teamShellBounds.width;
    float startX = layout.teamShellBounds.x;
    float startY = layout.teamShellBounds.y;

    // --- Pause Button (Top Left of HUD) ---
    Rectangle pauseBtn = layout.pauseButtonBounds;
    bool isHovered = CheckCollisionPointRec(mousePosition, pauseBtn);
    Texture2D pauseTex = AssetManager::GetInstance().GetTexture("button_pause");
    if (pauseTex.id != 0) {
        float frameW = pauseTex.width / 2.0f;
        float frameH = (float)pauseTex.height;
        Rectangle source = { isHovered ? frameW : 0.0f, 0.0f, frameW, frameH };
        DrawTexturePro(pauseTex, source, pauseBtn, {0.0f, 0.0f}, 0.0f, WHITE);
    } else {
        DrawRectangleRec(pauseBtn, isHovered ? Fade(GRAY, 0.8f) : Fade(BLACK, 0.5f));
        DrawRectangleLinesEx(pauseBtn, 2.0f, WHITE);
        DrawRectangle(pauseBtn.x + 8*S, pauseBtn.y + 6*S, 4*S, 18*S, WHITE);
        DrawRectangle(pauseBtn.x + 18*S, pauseBtn.y + 6*S, 4*S, 18*S, WHITE);
    }

    // Prepare off-field characters
    Paladin* offField1 = nullptr;
    Paladin* offField2 = nullptr;
    int activeIdx = hasReceivedStats ? cachedTeamStats.activeIndex : team->GetActiveIndex();
    
    int offIndex = 0;
    int offField1Idx = -1;
    int offField2Idx = -1;
    for (int i = 0; i < numPaladins; i++) {
        if (i == activeIdx) continue;
        if (offIndex == 0) { offField1 = roster[i]; offField1Idx = i; }
        else if (offIndex == 1) { offField2 = roster[i]; offField2Idx = i; }
        offIndex++;
    }

    // --- Layer 0: Lowest Layer (Stats Shell Back) ---
    if (statsShellBack.id != 0) {
        Rectangle sourceRec = {
            0,
            0,
            baseShellWidth * GameplayHUDLayout::HUD_TEXTURE_SOURCE_SCALE,
            GameplayHUDLayout::HUD_BASE_HEIGHT *
                GameplayHUDLayout::HUD_TEXTURE_SOURCE_SCALE
        };
        DrawTexturePro(statsShellBack, sourceRec, {startX, startY, shellWidth, 48.0f * S}, {0.0f, 0.0f}, 0.0f, WHITE);
    }

    // --- Layer 1: Portraits ---
    
    Font fontMono = AssetManager::GetInstance().GetCustomFont("PixeloidMono");
    Font fontSans = AssetManager::GetInstance().GetCustomFont("PixeloidSans");
    Font fontBold = AssetManager::GetInstance().GetCustomFont("PixeloidBold");

    auto DrawPortraitNumber = [&](int idx, Rectangle dest) {
        int num = idx + 1;
        DrawCircle(dest.x + dest.width - 8*S, dest.y + dest.height - 8*S, 8*S, Fade(BLACK, 0.7f));
        int fSize = std::max(1, (int)std::round(10.0f * S));
        Vector2 numSize = MeasureTextEx(fontMono, TextFormat("%d", num), fSize, 1.0f);
        UIUtils::DrawText("PixeloidMono", TextFormat("%d", num), { dest.x + dest.width - 8*S - numSize.x/2, dest.y + dest.height - 8*S - numSize.y/2 }, static_cast<UIUtils::FontSize>(fSize), WHITE);
    };

    auto DrawCardPortrait = [&](Paladin* p, int slotIdx, Rectangle dest) {
        if (!p) return;
        Texture2D cardTex = AssetManager::GetInstance().GetTexture(p->GetIntroData().portraitTextureID);
        Rectangle sourceRec = p->GetHudPortraitSlice();
        bool isDowned = (hasReceivedStats && slotIdx >= 0 && slotIdx < static_cast<int>(cachedPlayerStats.size()))
            ? cachedPlayerStats[slotIdx].isDowned
            : (p->GetHealth() <= 0);
        Color tint = isDowned ? DARKGRAY : WHITE;
        DrawTexturePro(cardTex, sourceRec, dest, {0.0f, 0.0f}, 0.0f, tint);
    };

    Rectangle activeDest = {startX + 4*S, startY + 4*S, 90*S, 40*S};
    DrawCardPortrait(active, activeIdx, activeDest);
    DrawPortraitNumber(activeIdx, activeDest);

    if (offField1) {
        Rectangle off1Dest = {startX + 226*S, startY + 4*S, 60*S, 28*S};
        DrawCardPortrait(offField1, offField1Idx, off1Dest);
        DrawPortraitNumber(offField1Idx, off1Dest);
    }
    if (offField2) {
        Rectangle off2Dest = {startX + 354*S, startY + 4*S, 60*S, 28*S};
        DrawCardPortrait(offField2, offField2Idx, off2Dest);
        DrawPortraitNumber(offField2Idx, off2Dest);
    }

    Texture2D checkTex = AssetManager::GetInstance().GetTexture("stats_checkpoint");
    float frameW = checkTex.id != 0 ? checkTex.width / 2.0f : 0.0f;
    float frameH = checkTex.id != 0 ? checkTex.height : 0.0f;

    // Helper lambda for HP (consuming Observer cached stats)
    auto DrawHP = [&](Paladin* p, int slotIdx, float x, float y, float maxW, float h) {
        float hp = 0.0f;
        float displayedHp = 0.0f;
        float ghost = 0.0f;
        float maxHp = 0.0f;

        if (hasReceivedStats && slotIdx >= 0 && slotIdx < static_cast<int>(cachedPlayerStats.size())) {
            const auto& s = cachedPlayerStats[slotIdx];
            if (s.isDowned) return;
            hp = static_cast<float>(s.health);
            displayedHp = s.displayedHp;
            ghost = s.ghostHp;
            maxHp = static_cast<float>(s.maxHealth);
        } else {
            if (!p || p->GetHealth() <= 0) return;
            hp = static_cast<float>(p->GetHealth());
            displayedHp = p->GetDisplayedHp();
            ghost = p->GetGhostHp();
            maxHp = static_cast<float>(p->GetMaxHealth());
        }
        
        float pctGhost = maxHp > 0 ? (ghost / maxHp) : 0.0f;
        float pctReal = maxHp > 0 ? (displayedHp / maxHp) : 0.0f;
        
        // Draw Ghost HP (Red Base)
        DrawRectangle(startX + x, startY + y, (int)(maxW * pctGhost), h, RED);
        
        // Draw Real HP (Gradient Overlay)
        Rectangle hpBounds = { startX + x, startY + y, maxW, h };
        bool isLowHp = hp < (maxHp * 0.3f);
        UIUtils::DrawGradientPulseBar(hpBounds, pctReal, UIUtils::HP_GRADIENT_LEFT, UIUtils::HP_GRADIENT_RIGHT, isLowHp, false);
    };

    // --- Layer 2: HP Bars (Doubled) ---
    DrawHP(active, activeIdx, 98*S, 4*S, 124*S, 26*S);
    DrawHP(offField1, offField1Idx, 290*S, 4*S, 60*S, 12*S);
    DrawHP(offField2, offField2Idx, 418*S, 4*S, 60*S, 12*S);

    // --- Layer 3: Mask Rectangle ---
    Color maskColor = { 57, 57, 68, 255 }; // #5b5b67
    DrawRectangle(startX + 146*S, startY + 20*S, 76*S, 12*S, maskColor);

    // Helper lambda for EX Energy (BLUE — for Skills, consuming Observer cached stats)
    auto DrawEX = [&](Paladin* p, int slotIdx, float x, float y, float maxW, float h) {
        float ex = 0.0f;
        float displayedEx = 0.0f;
        float maxEx = 0.0f;
        float exThreshold = 0.0f;

        if (hasReceivedStats && slotIdx >= 0 && slotIdx < static_cast<int>(cachedPlayerStats.size())) {
            const auto& s = cachedPlayerStats[slotIdx];
            ex = s.exEnergy;
            displayedEx = s.displayedEx;
            maxEx = s.maxEx;
            exThreshold = s.skillCost;
        } else {
            if (!p) return;
            ex = p->GetExEnergy();
            displayedEx = p->GetDisplayedExEnergy();
            maxEx = p->GetMaxExEnergy();
            exThreshold = p->GetSkillCost();
        }

        float pct = maxEx > 0 ? (displayedEx / maxEx) : 0;
        
        Rectangle exBounds = { startX + x, startY + y, maxW, h };
        bool isReady = ex >= exThreshold;
        UIUtils::DrawGradientPulseBar(exBounds, pct, UIUtils::EX_GRADIENT_LEFT, UIUtils::EX_GRADIENT_RIGHT, isReady, !isReady);
    };

    // --- Layer 4: EX Bars (BLUE) ---
    DrawEX(active, activeIdx, 146*S, 20*S, 76*S, 10*S);
    DrawEX(offField1, offField1Idx, 290*S, 20*S, 60*S, 10*S);
    DrawEX(offField2, offField2Idx, 418*S, 20*S, 60*S, 10*S);

    // --- Layer 4.5: Quintessence Bar (PURPLE — shared team ultimate fuel, 3 cells) ---
    Rectangle qBar = { startX + 146*S, startY + 34*S, 332*S, 12*S };
    float quint = hasReceivedStats ? cachedTeamStats.currentQuintessence : team->GetQuintessence();
    float displayedQuint = hasReceivedStats ? cachedTeamStats.displayedQuintessence : team->GetDisplayedQuintessence();
    float maxQuint = hasReceivedStats ? cachedTeamStats.maxQuintessence : team->GetMaxQuintessence();
    float quintPct = maxQuint > 0 ? (displayedQuint / maxQuint) : 0.0f;
    bool quintReady = quint >= TeamManager::ULTIMATE_COST;
    UIUtils::DrawGradientPulseBar(qBar, quintPct, UIUtils::QUINT_GRADIENT_LEFT, UIUtils::QUINT_GRADIENT_RIGHT, quintReady, !quintReady);

    // --- Layer 5: Stats Shell Overlay ---
    if (statsShell.id != 0) {
        Rectangle sourceRec = {
            0,
            0,
            baseShellWidth * GameplayHUDLayout::HUD_TEXTURE_SOURCE_SCALE,
            GameplayHUDLayout::HUD_BASE_HEIGHT *
                GameplayHUDLayout::HUD_TEXTURE_SOURCE_SCALE
        };
        DrawTexturePro(statsShell, sourceRec, {startX, startY, shellWidth, 48.0f * S}, {0.0f, 0.0f}, 0.0f, WHITE);
    }

    // --- Layer 6: Checkpoint Markers (Over Shell) ---
    if (checkTex.id != 0) {
        auto DrawExCheckpoint = [&](Paladin* p, int slotIdx, Rectangle exBounds) {
            float ex = 0.0f;
            float maxEx = 0.0f;
            float exThreshold = 0.0f;

            if (hasReceivedStats && slotIdx >= 0 && slotIdx < static_cast<int>(cachedPlayerStats.size())) {
                const auto& s = cachedPlayerStats[slotIdx];
                ex = s.exEnergy;
                maxEx = s.maxEx;
                exThreshold = s.skillCost;
            } else {
                if (!p) return;
                ex = p->GetExEnergy();
                maxEx = p->GetMaxExEnergy();
                exThreshold = p->GetSkillCost();
            }

            if (maxEx > 0) {
                bool isReady = ex >= exThreshold;
                float markerX = exBounds.x + (exBounds.width * (exThreshold / maxEx));
                Rectangle srcRec = { isReady ? frameW : 0.0f, 0.0f, frameW, frameH };
                Rectangle destRec = { markerX, exBounds.y + exBounds.height / 2.0f, frameW, frameH };
                DrawTexturePro(checkTex, srcRec, destRec, { frameW / 2.0f, frameH / 2.0f }, 0.0f, WHITE);
            }
        };

        // EX Checkpoints
        DrawExCheckpoint(active, activeIdx, { startX + 146*S, startY + 20*S, 76*S, 10*S });
        DrawExCheckpoint(offField1, offField1Idx, { startX + 290*S, startY + 20*S, 60*S, 10*S });
        DrawExCheckpoint(offField2, offField2Idx, { startX + 418*S, startY + 20*S, 60*S, 10*S });
        
        // Quintessence Checkpoints
        if (maxQuint > 0) {
            for (int i = 1; i <= 3; i++) {
                float targetQuint = TeamManager::ULTIMATE_COST * i;
                if (targetQuint > maxQuint) break;
                
                bool isReady = quint >= targetQuint;
                float qMarkerX = qBar.x + (qBar.width * (targetQuint / maxQuint));
                Rectangle qSrcRec = { isReady ? frameW : 0.0f, 0.0f, frameW, frameH };
                Rectangle qDestRec = { qMarkerX, qBar.y + qBar.height / 2.0f, frameW, frameH };
                DrawTexturePro(checkTex, qSrcRec, qDestRec, { frameW / 2.0f, frameH / 2.0f }, 0.0f, WHITE);
            }
        }
    }

    // --- Active HP Text ---
    char hpText[32];
    if (hasReceivedStats && activeIdx >= 0 && activeIdx < static_cast<int>(cachedPlayerStats.size())) {
        snprintf(hpText, sizeof(hpText), "%d/%d", cachedPlayerStats[activeIdx].health, cachedPlayerStats[activeIdx].maxHealth);
    } else {
        snprintf(hpText, sizeof(hpText), "%d/%d", active->GetHealth(), active->GetMaxHealth());
    }
    
    int fontSize = std::max(1, (int)std::round(10.0f * S));
    Vector2 textSize = MeasureTextEx(fontMono, hpText, fontSize, 1.0f);
    float textX = startX + 98*S + (44*S - textSize.x) / 2;
    float textY = startY + 34*S + (12*S - textSize.y) / 2;
    UIUtils::DrawText("PixeloidMono", hpText, { textX, textY }, static_cast<UIUtils::FontSize>(fontSize), WHITE);

    if (InputManager::GetMode() == InputMode::KEYBOARD_ONLY && !Constants::isAutoAimEnabled) {
        const char* hint = "Auto-Aim ('T') Recommended for Keyboard Only";
        Vector2 hintSize = MeasureTextEx(fontSans, hint, 20, 1.0f);
        UIUtils::DrawText("PixeloidSans", hint, { windowBounds.x + (windowBounds.width - hintSize.x) / 2, windowBounds.y + windowBounds.height - 100.0f }, static_cast<UIUtils::FontSize>(20), Fade(WHITE, 0.7f));
    }


    // --- Coin Counter Panel (Directly to the Left of Minimap) ---
    DrawCoinHUD(layout.coinCounterBounds, team->GetCoins());
}

/// Renders coin hud.
void UIManager::DrawCoinHUD(Rectangle bounds, int coins) {
    // Rounded background & border matching minimap style
    DrawRectangleRounded(bounds, 0.25f, 6, ColorAlpha(Color{ 10, 10, 15, 255 }, 0.8f));
    DrawRectangleRoundedLinesEx(bounds, 0.25f, 6, 1.0f, ColorAlpha(GRAY, 0.4f));

    Texture2D coinIcon = AssetManager::GetInstance().GetTexture("coin_icon");
    if (coinIcon.id != 0) {
        float iconSize = std::min(bounds.height - 8.0f, (float)coinIcon.height);
        float iconScale = (coinIcon.height > 0) ? (iconSize / coinIcon.height) : 1.0f;
        float iconW = coinIcon.width * iconScale;
        float iconH = coinIcon.height * iconScale;
        Rectangle dest = {
            bounds.x + 8.0f,
            bounds.y + (bounds.height - iconH) * 0.5f,
            iconW,
            iconH
        };
        DrawTexturePro(
            coinIcon,
            { 0.0f, 0.0f, (float)coinIcon.width, (float)coinIcon.height },
            dest,
            { 0.0f, 0.0f },
            0.0f,
            WHITE
        );
    }

    std::string coinText = std::to_string(coins);
    Font fontMono = AssetManager::GetInstance().GetCustomFont("PixeloidMono");
    float fontSize = 14.0f;
    Vector2 textSize = MeasureTextEx(fontMono, coinText.c_str(), fontSize, 1.0f);
    Vector2 textPos = {
        bounds.x + bounds.width - textSize.x - 10.0f,
        bounds.y + (bounds.height - textSize.y) * 0.5f
    };
    UIUtils::DrawText("PixeloidMono", coinText.c_str(), textPos, static_cast<UIUtils::FontSize>(fontSize), Color{ 255, 223, 80, 255 });
}

/// Renders modal overlay.
void UIManager::DrawModalOverlay() {
    DrawRectangle(-10000, -10000, 20000, 20000, Fade(BLACK, 0.6f));
}

/// Renders Soul Knight style boss health bar fixed at top of screen.
void UIManager::DrawBossHealthBar(Boss* boss, Rectangle windowBounds, float deltaTime) {
    if (!boss || boss->IsDead()) return;

    // Track ghost HP for smooth damage animation
    static float s_ghostHp = -1.0f;
    static Boss* s_lastBoss = nullptr;
    if (s_lastBoss != boss || s_ghostHp < 0.0f) {
        s_lastBoss = boss;
        s_ghostHp = (float)boss->GetHealth();
    }

    float currentHp = (float)boss->GetHealth();
    float maxHp = (float)std::max(1, boss->GetMaxHealth());

    if (s_ghostHp < currentHp) {
        s_ghostHp = currentHp;
    } else if (s_ghostHp > currentHp) {
        s_ghostHp = std::max(currentHp, s_ghostHp - deltaTime * 400.0f);
    }

    float healthPercent = std::clamp(currentHp / maxHp, 0.0f, 1.0f);
    float ghostPercent = std::clamp(s_ghostHp / maxHp, 0.0f, 1.0f);

    // Soul Knight Style Top-Center Boss Bar (Centered, longer, without dividers)
    float barWidth = 330.0f;
    float barHeight = 15.0f;
    float startX = windowBounds.x + (windowBounds.width - barWidth) * 0.5f;
    float startY = 68.0f; // Fixed below Player Team HUD to avoid overlap

    // 1. Dark outer frame / shadow
    Rectangle outerFrame = { startX - 3.0f, startY - 3.0f, barWidth + 6.0f, barHeight + 6.0f };
    DrawRectangleRec(outerFrame, Color{ 14, 14, 18, 230 });
    DrawRectangleLinesEx(outerFrame, 1.5f, Color{ 55, 55, 65, 255 });

    // 2. Inner background fill (dark crimson empty bar)
    Rectangle innerBar = { startX, startY, barWidth, barHeight };
    DrawRectangleRec(innerBar, Color{ 35, 12, 16, 255 });

    // 3. Ghost HP Bar (Yellow/Orange trailing damage)
    if (ghostPercent > healthPercent) {
        DrawRectangleRec(
            { startX, startY, barWidth * ghostPercent, barHeight },
            Color{ 230, 165, 45, 240 }
        );
    }

    // 4. Current HP Bar (Vibrant Soul Knight Red)
    DrawRectangleRec(
        { startX, startY, barWidth * healthPercent, barHeight },
        Color{ 220, 35, 35, 255 }
    );
}
