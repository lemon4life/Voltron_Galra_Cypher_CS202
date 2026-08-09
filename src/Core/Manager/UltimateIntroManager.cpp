#include "Core/Manager/UltimateIntroManager.h"
#include "Core/Manager/AudioManager.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Constants.h"
#include "Entities/Player/Paladin.h"
#include "Entities/Player/Keith.h"
#include "Entities/Player/Lance.h"
#include "Entities/Player/Hunk.h"
#include "raymath.h"
#include <iostream>

UltimateIntroManager& UltimateIntroManager::GetInstance() {
    static UltimateIntroManager instance;
    return instance;
}

UltimateIntroManager::UltimateIntroManager() : isPlaying(false), timer(0.0f), activePaladin(nullptr) {
}

UltimateIntroManager::~UltimateIntroManager() {
}

void UltimateIntroManager::PlayIntro(Paladin* paladin) {
    activePaladin = paladin;
    isPlaying = true;
    timer = 0.0f;
    
    // Play placeholder sound
    AudioManager::GetInstance().PlaySoundEffect("sfx_button_click"); 
    // Wait, the user asked for a voiceline. I can use PlaySoundEffect with a generic name for now.
    // e.g. "keith_ult_voice" if dynamic cast works.
    if (dynamic_cast<Keith*>(paladin)) AudioManager::GetInstance().PlaySoundEffect("keith_ult_voice");
    else if (dynamic_cast<Lance*>(paladin)) AudioManager::GetInstance().PlaySoundEffect("lance_ult_voice");
    else if (dynamic_cast<Hunk*>(paladin)) AudioManager::GetInstance().PlaySoundEffect("hunk_ult_voice");
}

void UltimateIntroManager::Update(float deltaTime) {
    if (!isPlaying) return;
    
    timer += deltaTime;
    
    if (timer >= 1.8f) {
        isPlaying = false;
        if (activePaladin) {
            activePaladin->ExecuteUltimateAction();
        }
    }
}

void UltimateIntroManager::Draw() {
    if (!isPlaying || !activePaladin) return;
    
    float screenW = (float)Constants::GAME_WIDTH;
    float screenH = (float)Constants::GAME_HEIGHT;
    
    // 1. Darken Screen
    float darkAlpha = 0.0f;
    if (timer < 0.3f) {
        darkAlpha = (timer / 0.3f) * 0.6f;
    } else if (timer > 1.5f) {
        darkAlpha = ((1.8f - timer) / 0.3f) * 0.6f;
    } else {
        darkAlpha = 0.6f;
    }
    DrawRectangle(0, 0, Constants::SCREEN_WIDTH, Constants::SCREEN_HEIGHT, ColorAlpha(BLACK, darkAlpha));
    
    // 2. Banner Color based on Paladin
    Color bannerColor = GRAY;
    std::string nameText = "PALADIN";
    std::string ultText = "ULTIMATE";
    
    if (dynamic_cast<Keith*>(activePaladin)) { bannerColor = RED; nameText = "KEITH"; ultText = "EXCALIBUR"; }
    else if (dynamic_cast<Lance*>(activePaladin)) { bannerColor = BLUE; nameText = "LANCE"; ultText = "ABSOLUTE ZERO"; }
    else if (dynamic_cast<Hunk*>(activePaladin)) { bannerColor = YELLOW; nameText = "HUNK"; ultText = "AEGIS SHIELD"; }
    
    // 3. Banner sliding
    float bannerY = screenH / 2.0f;
    float bannerHeight = .0f;
    
    float slideOffset = 0.0f;
    if (timer < 0.3f) {
        // Slide in from bottom
        float t = timer / 0.3f;
        // ease out cubic
        t = 1.0f - powf(1.0f - t, 3.0f);
        slideOffset = (1.0f - t) * screenH;
    } else if (timer > 1.5f) {
        // Slide out to top
        float t = (timer - 1.5f) / 0.3f;
        // ease in cubic
        t = t * t * t;
        slideOffset = -t * screenH;
    }
    
    // Draw slanted banner
    // A skewed rectangle. We can use DrawTriangleStrip or DrawPoly, or just DrawRectanglePro rotated
    Rectangle bannerRec = { screenW / 2.0f, bannerY + slideOffset, screenW * 2.0f, bannerHeight };
    Vector2 bannerOrigin = { screenW, bannerHeight / 2.0f };
    DrawRectanglePro(bannerRec, bannerOrigin, -5.0f, bannerColor);
    
    // 4. Portrait & Text Lerping
    // Portrait slides in from left (Phase 2), stays, slides out (Phase 3)
    float startPortraitX = -400.0f;
    float targetPortraitX = screenW * 0.20f;
    
    float startTextX = screenW + 400.0f;
    float targetTextX = screenW * 0.65f;
    
    float portraitX = startPortraitX;
    float textX = startTextX;
    
    if (timer >= 0.3f && timer <= 1.5f) {
        float phase2T = (timer - 0.3f) / 1.2f;
        float easeT = 1.0f - powf(1.0f - phase2T, 3.0f);
        portraitX = startPortraitX + easeT * (targetPortraitX - startPortraitX);
        textX = startTextX - easeT * (startTextX - targetTextX);
    } else if (timer > 1.5f) {
        float phase3T = (timer - 1.5f) / 0.3f;
        float easeT = phase3T * phase3T; // ease in
        portraitX = targetPortraitX + easeT * 1000.0f; // fly right
        textX = targetTextX - easeT * 1000.0f; // fly left
    }
    
    if (timer >= 0.3f) {
        // Draw Portrait
        Texture2D portraitTex;
        if (dynamic_cast<Keith*>(activePaladin)) portraitTex = AssetManager::GetInstance().GetTexture("Card_Keith");
        else if (dynamic_cast<Lance*>(activePaladin)) portraitTex = AssetManager::GetInstance().GetTexture("Card_Lance");
        else if (dynamic_cast<Hunk*>(activePaladin)) portraitTex = AssetManager::GetInstance().GetTexture("Card_Hunk");
        
        if (portraitTex.id != 0) {
            float targetHeight = 400.0f;
            float scale = targetHeight / portraitTex.height;
            float destWidth = portraitTex.width * scale;
            
            Rectangle pSrc = { 0, 0, (float)portraitTex.width, (float)portraitTex.height };
            Rectangle pDest = { portraitX, bannerY + slideOffset, destWidth, targetHeight };
            // The bottom of the banner is bannerHeight / 2.0f (which is 200/2 = 100) below the rotation center.
            // We want the bottom of the image to also be 100.0f below the rotation center.
            Vector2 pOrigin = { destWidth / 2.0f, targetHeight - 200.0f }; 
            DrawTexturePro(portraitTex, pSrc, pDest, pOrigin, -5.0f, WHITE);
        }
        
        // Draw Text
        DrawTextPro(GetFontDefault(), nameText.c_str(), {textX, bannerY + slideOffset - 25.0f}, {0,0}, -5.0f, 40, 5, WHITE);
        DrawTextPro(GetFontDefault(), ultText.c_str(), {textX, bannerY + slideOffset + 15.0f}, {0,0}, -5.0f, 20, 3, WHITE);
    }
}
