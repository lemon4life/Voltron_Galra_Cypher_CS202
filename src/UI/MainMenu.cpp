#include "UI/MainMenu.h"
#include "UI/UIUtils.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/AudioManager.h"
#include "raymath.h"

#include <algorithm>
#include <cmath>

MainMenu::MainMenu() : currentSlideIndex(0), slideTimer(0.0f), panTimer(0.0f), switchedIndex(false) {
    logoTex.id = 0;
}

MainMenu::~MainMenu() {
}

void MainMenu::LoadCurrentBackground() {
    if (backgroundTexture.id != 0) {
        UnloadTexture(backgroundTexture);
        backgroundTexture = {};
    }
    std::string path = "assets/img/Background/bg_" +
        std::to_string(currentSlideIndex + 1) + ".png";
    backgroundTexture = LoadTexture(path.c_str());
    if (backgroundTexture.id != 0) {
        SetTextureFilter(backgroundTexture, TEXTURE_FILTER_BILINEAR);
    }
}

void MainMenu::Initialize() {
    AssetManager& assets = AssetManager::GetInstance();
    currentSlideIndex = 0;
    LoadCurrentBackground();
    
    // Load logo with point filter to keep logo crisp if needed, or false.
    // The previous implementation used true for logo. We'll stick to true.
    logoTex = assets.LoadTexture2D("Voltron_logo", "assets/img/Background/Voltron_logo.png", true);
    
    // Initialize buttons
    RebuildButtons();
}

void MainMenu::Shutdown() {
    if (backgroundTexture.id != 0) {
        UnloadTexture(backgroundTexture);
        backgroundTexture = {};
    }
}

void MainMenu::RebuildButtons() {
    buttons.clear();
    std::vector<std::string> titles;
    if (continueAvailable) {
        titles.push_back("Continue");
    }
    titles.push_back("Start Game");
    titles.push_back("Settings");
    titles.push_back("Room Editor");
    titles.push_back("About us");
    titles.push_back("Exit Game");

    for (const std::string& title : titles) {
        MenuButton btn;
        btn.text = title;
        btn.bounds = { 0, 0, 250, 50 }; 
        btn.currentScale = baseScale;
        btn.currentXOffset = 0.0f;
        btn.currentColor = baseColor;
        buttons.push_back(btn);
    }
}

void MainMenu::Update(float deltaTime) {
    // Slideshow timer logic
    slideTimer += deltaTime;
    panTimer += deltaTime;
    
    if (slideTimer >= 10.15f && !switchedIndex) {
        currentSlideIndex = (currentSlideIndex + 1) % BACKGROUND_COUNT;
        LoadCurrentBackground();
        switchedIndex = true;
        panTimer = 0.0f;
    }
    
    if (slideTimer >= 10.3f) {
        slideTimer -= 10.3f;
        switchedIndex = false;
    }

    if (currentState == MenuState::LOADING) {
        float actualProgress = 0.0f;
        bool done = AssetManager::GetInstance().UpdateLoading(actualProgress);
        
        // Artificial trailing progress for visual polish (max 33% per second = 3s total load time minimum)
        loadingProgress = std::min(loadingProgress + (deltaTime * 0.33f), actualProgress);
        
        if (done && loadingProgress >= 0.999f) {
            loadingProgress = 1.0f; // Snap to exactly 100%
            transitionTimer += deltaTime;
            if (transitionTimer > 1.5f) { // 1.5s buffer (stays 1s longer than before)
                currentState = MenuState::TRANSITIONING;
                transitionTimer = 0.0f;
            }
        }
    } else if (currentState == MenuState::TRANSITIONING) {
        transitionTimer += deltaTime;
        float duration = 1.0f; // 1 second transition
        float t = std::min(transitionTimer / duration, 1.0f);
        
        // Ease out cubic
        float easeOut = 1.0f - std::pow(1.0f - t, 3.0f);
        uiAlpha = easeOut;
        
        if (t >= 1.0f) {
            currentState = MenuState::ACTIVE;
            isReady = true;
        }
    } else if (currentState == MenuState::ACTIVE) {
        uiAlpha = 1.0f;

        if (IsKeyPressed(KEY_ENTER)) {
            AudioManager::GetInstance().PlayRandomClick();
            pendingAction = continueAvailable
                ? MainMenuAction::Continue
                : MainMenuAction::StartGame;
            return;
        }
        
        // Update buttons
        Vector2 mousePos = GetMousePosition();
        for (size_t i = 0; i < buttons.size(); i++) {
            auto& btn = buttons[i];
            bool hovered = CheckCollisionPointRec(mousePos, btn.bounds);

            if (hovered) {
                btn.currentScale = Lerp(btn.currentScale, hoverScale, 15.0f * deltaTime);
                btn.currentXOffset = Lerp(btn.currentXOffset, hoverXOffset, 15.0f * deltaTime);
                
                btn.currentColor.r = (unsigned char)Lerp(btn.currentColor.r, hoverColor.r, 15.0f * deltaTime);
                btn.currentColor.g = (unsigned char)Lerp(btn.currentColor.g, hoverColor.g, 15.0f * deltaTime);
                btn.currentColor.b = (unsigned char)Lerp(btn.currentColor.b, hoverColor.b, 15.0f * deltaTime);
                btn.currentColor.a = (unsigned char)Lerp(btn.currentColor.a, hoverColor.a, 15.0f * deltaTime);

                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    if (btn.text == "Start Game") {
                        AudioManager::GetInstance().PlaySoundEffect("sfx_ui_select");
                        pendingAction = MainMenuAction::StartGame;
                    } else if (btn.text == "Continue") {
                        AudioManager::GetInstance().PlaySoundEffect("sfx_ui_select");
                        pendingAction = MainMenuAction::Continue;
                    } else if (btn.text == "Settings") {
                        AudioManager::GetInstance().PlaySoundEffect("sfx_ui_select");
                        GameManager::GetInstance().SetState(GameState::SETTINGS);
                    } else if (btn.text == "Room Editor") {
                        AudioManager::GetInstance().PlaySoundEffect("sfx_ui_select");
                        pendingAction = MainMenuAction::OpenEditor;
                    } else if (btn.text == "Exit Game") {
                        AudioManager::GetInstance().PlaySoundEffect("sfx_ui_select");
                        quitRequested = true;
                    }
                }
            } else {
                btn.currentScale = Lerp(btn.currentScale, baseScale, 15.0f * deltaTime);
                btn.currentXOffset = Lerp(btn.currentXOffset, 0.0f, 15.0f * deltaTime);
                
                btn.currentColor.r = (unsigned char)Lerp(btn.currentColor.r, baseColor.r, 15.0f * deltaTime);
                btn.currentColor.g = (unsigned char)Lerp(btn.currentColor.g, baseColor.g, 15.0f * deltaTime);
                btn.currentColor.b = (unsigned char)Lerp(btn.currentColor.b, baseColor.b, 15.0f * deltaTime);
                btn.currentColor.a = (unsigned char)Lerp(btn.currentColor.a, baseColor.a, 15.0f * deltaTime);
            }
        }
    }
}

bool MainMenu::ConsumeQuitRequest() {
    bool requested = quitRequested;
    quitRequested = false;
    return requested;
}

MainMenuAction MainMenu::ConsumeAction() {
    MainMenuAction action = pendingAction;
    pendingAction = MainMenuAction::None;
    return action;
}

void MainMenu::SetContinueAvailable(bool available) {
    if (continueAvailable == available) return;

    continueAvailable = available;
    RebuildButtons();
}

void MainMenu::Draw(int screenWidth, int screenHeight) {
    // 1. Draw Slideshow Background with Pan
    if (backgroundTexture.id != 0) {
        Texture2D& currentBg = backgroundTexture;
        
        if (currentBg.id != 0) {
            float scaleX = (float)screenWidth / currentBg.width;
            float scaleY = (float)screenHeight / currentBg.height;
            float panDistance = screenWidth * 0.05f;
            float reqScaleX = (screenWidth + panDistance) / currentBg.width;
            float scale = std::max(reqScaleX, scaleY);
            float drawnWidth = currentBg.width * scale;
            float drawnHeight = currentBg.height * scale;
            float panProgress = panTimer / 10.3f;
            float currentPan = panProgress * panDistance;
            float maxOffsetY = drawnHeight - screenHeight;
            float offsetX = 0.0f;
            
            if (currentSlideIndex % 2 == 0) {
                offsetX = -currentPan;
            } else {
                offsetX = -(drawnWidth - screenWidth) + currentPan;
            }
            
            Rectangle sourceRec = { 0.0f, 0.0f, (float)currentBg.width, (float)currentBg.height };
            Rectangle destRec = { offsetX, -(maxOffsetY / 2.0f), drawnWidth, drawnHeight };
            DrawTexturePro(currentBg, sourceRec, destRec, {0,0}, 0.0f, WHITE);
        }
    } else {
        ClearBackground(DARKGRAY);
    }

    // Overlay transition fade
    if (slideTimer > 10.0f) {
        float fadeAlpha = 0.0f;
        if (slideTimer < 10.15f) {
            fadeAlpha = (slideTimer - 10.0f) / 0.15f;
        } else {
            fadeAlpha = 1.0f - ((slideTimer - 10.15f) / 0.15f);
        }
        fadeAlpha = std::max(0.0f, std::min(1.0f, fadeAlpha));
        DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, fadeAlpha));
    }

    float panelWidth = screenWidth * 0.35f;
    float panelX = screenWidth - panelWidth;

    // Draw Loading Indicator
    float loadingAlpha = 1.0f;
    float loadingSlideY = 0.0f;
    if (currentState == MenuState::TRANSITIONING || currentState == MenuState::ACTIVE) {
        loadingAlpha = 1.0f - uiAlpha;
        loadingSlideY = uiAlpha * 50.0f;
    }
    
    if (loadingAlpha > 0.01f) {
        int barWidth = 600;
        int barHeight = 6;
        int barX = (screenWidth - barWidth) / 2;
        int barY = screenHeight * 0.75f + loadingSlideY;
        
        UIUtils::DrawProgressBar(
            { (float)barX, (float)barY, (float)barWidth, (float)barHeight },
            loadingProgress, 1.0f,
            Fade(RAYWHITE, loadingAlpha * 0.3f), // Assuming bg color slightly faded
            Fade(RAYWHITE, loadingAlpha)
        );
        
        const char* text = TextFormat("LOADING... %d%%", (int)(loadingProgress * 100));
        UIUtils::DrawCenteredText("PixeloidSans", text, { barX + barWidth / 2.0f, (float)barY - 30.0f + 10.0f }, UIUtils::FontSize::SMALL, Fade(RAYWHITE, loadingAlpha));
    }

    // Calculate Logo dynamic position (Lerp from top-center to panel layout)
    
    // Target Logo State
    float logoTargetWidth = panelWidth * 0.8f;
    float logoTargetScale = logoTargetWidth / logoTex.width;
    float logoTargetHeight = logoTex.height * logoTargetScale;
    float logoTargetX = panelX + (panelWidth - logoTargetWidth) / 2.0f;
    float logoTargetY = screenHeight * 0.15f;
    
    // Init Logo State (Loading Phase)
    float logoInitWidth = screenWidth * 0.35f;
    float logoInitScale = logoInitWidth / logoTex.width;
    float logoInitHeight = logoTex.height * logoInitScale;
    float logoInitX = (screenWidth - logoInitWidth) / 2.0f;
    float logoInitY = screenHeight * 0.3f;
    
    float currentLogoX = Lerp(logoInitX, logoTargetX, uiAlpha);
    float currentLogoY = Lerp(logoInitY, logoTargetY, uiAlpha);
    float currentLogoWidth = Lerp(logoInitWidth, logoTargetWidth, uiAlpha);
    float currentLogoHeight = Lerp(logoInitHeight, logoTargetHeight, uiAlpha);

    // Draw UI Panel (only if uiAlpha > 0)
    if (uiAlpha > 0.01f) {
        DrawRectangleGradientH(panelX, 0, panelWidth, screenHeight, BLANK, Fade(BLACK, 0.9f * uiAlpha));

        float scaleFactor = (float)screenHeight / 720.0f;

        // Start buttons perfectly spaced below the sliding logo
        float startY = currentLogoY + currentLogoHeight + (40.0f * scaleFactor);
        float btnSpacing = 55.0f * scaleFactor;
        
        float btnSlideY = (1.0f - uiAlpha) * (30.0f * scaleFactor); // Buttons slide up as they fade in
        
        for (size_t i = 0; i < buttons.size(); i++) {
            auto& btn = buttons[i];
            
            float baseFontSize = static_cast<float>(UIUtils::FontSize::HEADER) * scaleFactor;
            float textWidth = UIUtils::MeasureText("PixeloidSans", btn.text, static_cast<UIUtils::FontSize>(baseFontSize)).x;
            
            float drawTextWidth = textWidth * btn.currentScale;
            float drawFontSize = baseFontSize * btn.currentScale;
            
            float btnWidth = textWidth + (80.0f * scaleFactor);
            float btnHeight = baseFontSize + (20.0f * scaleFactor);
            float targetX = panelX + (panelWidth / 2.0f) - (btnWidth / 2.0f);
            
            btn.bounds = { targetX, startY + (i * btnSpacing), btnWidth, btnHeight };
            
            float drawX = targetX + btn.currentXOffset;
            float drawY = btn.bounds.y + btnSlideY;
            
            float adjustedY = drawY - (drawFontSize - baseFontSize) / 2.0f;
            float adjustedX = drawX - (drawTextWidth - textWidth) / 2.0f;

            Color fadeColor = btn.currentColor;
            fadeColor.a = (unsigned char)(fadeColor.a * uiAlpha);

            // Shadow
            UIUtils::DrawText("PixeloidSans", btn.text, { adjustedX + 2, adjustedY + 2 }, static_cast<UIUtils::FontSize>(drawFontSize), Fade(BLACK, fadeColor.a / 255.0f));
            // Text
            UIUtils::DrawText("PixeloidSans", btn.text, { adjustedX, adjustedY }, static_cast<UIUtils::FontSize>(drawFontSize), fadeColor);
        }
    }
    
    // Draw Logo
    if (logoTex.id != 0) {
        Rectangle dest = { currentLogoX, currentLogoY, currentLogoWidth, currentLogoHeight };
        Rectangle src = { 0.0f, 0.0f, (float)logoTex.width, (float)logoTex.height };
        DrawTexturePro(logoTex, src, dest, {0,0}, 0.0f, WHITE);
    }
}
