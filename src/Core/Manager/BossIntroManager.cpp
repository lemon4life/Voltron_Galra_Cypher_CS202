#include "Core/Manager/BossIntroManager.h"
#include "Core/Manager/AudioManager.h"
#include "Core/Manager/AssetManager.h"
#include "UI/UIUtils.h"
#include "Core/Constants.h"
#include "Entities/EnemyEntities/Boss.h"
#include "raymath.h"
#include <cmath>
#include <iostream>

BossIntroManager& BossIntroManager::GetInstance() {
    static BossIntroManager instance;
    return instance;
}

BossIntroManager::BossIntroManager()
    : isPlaying(false),
      timer(0.0f),
      activeBoss(nullptr),
      bossName("COMMANDER PROROK"),
      bossTitle("GALRA EMPIRE CYBERNETIC WARLORD"),
      bannerColor(Color{ 105, 18, 120, 255 }) { // Deep Galra Imperial Violet
}

BossIntroManager::~BossIntroManager() {
}

void BossIntroManager::PlayIntro(Boss* boss, const std::string& name, const std::string& title) {
    activeBoss = boss;
    bossName = name;
    bossTitle = title;
    isPlaying = true;
    timer = 0.0f;
    bannerColor = Color{ 110, 15, 65, 255 }; // Crimson Galra Warlord theme
    
    // Play intro sounds and dramatic swell
    AudioManager::GetInstance().PlaySoundEffect("ui_opening");
    AudioManager::GetInstance().PlayMusicTrack("bg_boss", 1.0f);
}

void BossIntroManager::Update(float deltaTime) {
    if (!isPlaying) return;
    
    timer += deltaTime;
    
    if (timer >= 2.2f) {
        isPlaying = false;
    }
}

void BossIntroManager::Draw() {
    if (!isPlaying) return;
    
    float screenW = (float)Constants::GAME_WIDTH;
    float screenH = (float)Constants::GAME_HEIGHT;
    
    // 1. Darken Screen & Cinematic Letterbox
    float darkAlpha = 0.0f;
    if (timer < 0.35f) {
        darkAlpha = (timer / 0.35f) * 0.70f;
    } else if (timer > 1.8f) {
        darkAlpha = ((2.2f - timer) / 0.4f) * 0.70f;
    } else {
        darkAlpha = 0.70f;
    }
    DrawRectangle(0, 0, Constants::SCREEN_WIDTH, Constants::SCREEN_HEIGHT, ColorAlpha(BLACK, darkAlpha));
    
    // 2. Banner Calculations
    float bannerY = screenH / 2.0f;
    float bannerHeight = 320.0f;
    float bannerAngle = -4.0f; // Slanted angle
    
    float slideOffset = 0.0f;
    if (timer < 0.35f) {
        // Slide in from bottom
        float t = timer / 0.35f;
        t = 1.0f - powf(1.0f - t, 3.0f); // ease out cubic
        slideOffset = (1.0f - t) * screenH;
    } else if (timer > 1.8f) {
        // Slide out to top
        float t = (timer - 1.8f) / 0.4f;
        t = t * t * t; // ease in cubic
        slideOffset = -t * screenH;
    }
    
    // Draw slanted banner background
    Rectangle bannerRec = { screenW / 2.0f, bannerY + slideOffset, screenW * 2.2f, bannerHeight };
    Vector2 bannerOrigin = { screenW * 1.1f, bannerHeight / 2.0f };
    
    // Accent edge strips
    Rectangle topBorderRec = { screenW / 2.0f, bannerY + slideOffset - bannerHeight * 0.5f - 4.0f, screenW * 2.2f, 8.0f };
    Vector2 borderOrigin = { screenW * 1.1f, 4.0f };
    DrawRectanglePro(topBorderRec, borderOrigin, bannerAngle, Color{ 255, 204, 0, 240 }); // Gold top trim
    
    Rectangle botBorderRec = { screenW / 2.0f, bannerY + slideOffset + bannerHeight * 0.5f + 4.0f, screenW * 2.2f, 8.0f };
    DrawRectanglePro(botBorderRec, borderOrigin, bannerAngle, Color{ 255, 60, 60, 240 }); // Crimson bottom trim
    
    // Main banner body
    DrawRectanglePro(bannerRec, bannerOrigin, bannerAngle, bannerColor);
    
    // 3. Name on LEFT, Boss Portrait on RIGHT
    float startTextX = -450.0f;
    float targetTextX = screenW * 0.08f; // Well to the left (x ~ 51px)
    
    float startPortraitX = screenW + 450.0f;
    float targetPortraitX = screenW * 0.82f; // Well to the right (x ~ 525px)
    
    float textX = startTextX;
    float portraitX = startPortraitX;
    
    if (timer >= 0.35f && timer <= 1.8f) {
        float phase2T = (timer - 0.35f) / 1.45f;
        float easeT = 1.0f - powf(1.0f - phase2T, 3.0f);
        textX = startTextX + easeT * (targetTextX - startTextX);
        portraitX = startPortraitX - easeT * (startPortraitX - targetPortraitX);
    } else if (timer > 1.8f) {
        float phase3T = (timer - 1.8f) / 0.4f;
        float easeT = phase3T * phase3T;
        textX = targetTextX - easeT * 1000.0f; // fly left
        portraitX = targetPortraitX + easeT * 1000.0f; // fly right
    }
    
    if (timer >= 0.35f) {
        // --- Draw Boss Name on LEFT Side ---
        // Shadow for name
        UIUtils::DrawTextPro(
            "PixeloidBold",
            bossName,
            { textX + 2.0f, bannerY + slideOffset - 22.0f + 2.0f },
            { 0, 0 },
            bannerAngle,
            UIUtils::FontSize::HEADER,
            Color{ 10, 10, 15, 230 }
        );
        // Main name text
        UIUtils::DrawTextPro(
            "PixeloidBold",
            bossName,
            { textX, bannerY + slideOffset - 22.0f },
            { 0, 0 },
            bannerAngle,
            UIUtils::FontSize::HEADER,
            WHITE
        );
        
        // Subtitle shadow
        UIUtils::DrawTextPro(
            "PixeloidBold",
            bossTitle,
            { textX + 1.5f, bannerY + slideOffset + 24.0f + 1.5f },
            { 0, 0 },
            bannerAngle,
            UIUtils::FontSize::SMALL,
            Color{ 10, 10, 15, 230 }
        );
        // Subtitle main
        UIUtils::DrawTextPro(
            "PixeloidBold",
            bossTitle,
            { textX, bannerY + slideOffset + 24.0f },
            { 0, 0 },
            bannerAngle,
            UIUtils::FontSize::SMALL,
            Color{ 255, 204, 0, 255 }
        );
        
        // --- Draw Boss Portrait on RIGHT Side ---
        Texture2D bossIdleTex = AssetManager::GetInstance().GetTexture("Boss_Idle");
        if (bossIdleTex.id != 0) {
            constexpr float FRAME_W = 64.0f;
            constexpr float FRAME_H = 72.0f;
            float targetHeight = 220.0f;
            float scale = targetHeight / FRAME_H;
            float destWidth = FRAME_W * scale;
            
            // Frame 0 of idle animation, flipped horizontally to face left towards text
            Rectangle pSrc = { 0, 0, -FRAME_W, FRAME_H };
            Rectangle pDest = { portraitX, bannerY + slideOffset, destWidth, targetHeight };
            Vector2 pOrigin = { destWidth * 0.5f, targetHeight * 0.5f };
            
            // White sticker-style outline effect (Soul Knight style)
            constexpr float OUTLINE_PX = 2.5f;
            Vector2 outlineOffsets[] = {
                { -OUTLINE_PX, 0.0f },
                { OUTLINE_PX, 0.0f },
                { 0.0f, -OUTLINE_PX },
                { 0.0f, OUTLINE_PX },
                { -OUTLINE_PX, -OUTLINE_PX },
                { OUTLINE_PX, OUTLINE_PX },
                { -OUTLINE_PX, OUTLINE_PX },
                { OUTLINE_PX, -OUTLINE_PX }
            };
            
            for (const auto& offset : outlineOffsets) {
                Rectangle oDest = { pDest.x + offset.x, pDest.y + offset.y, pDest.width, pDest.height };
                DrawTexturePro(bossIdleTex, pSrc, oDest, pOrigin, bannerAngle, WHITE);
            }
            
            // Main colored sprite on top
            DrawTexturePro(bossIdleTex, pSrc, pDest, pOrigin, bannerAngle, WHITE);
        }
    }
}
