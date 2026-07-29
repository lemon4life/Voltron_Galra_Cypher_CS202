#pragma once

#include "raylib.h"

#include <string>
#include <vector>

struct MenuButton {
    Rectangle bounds;
    std::string text;
    float currentScale;
    float currentXOffset;
    Color currentColor;
};


enum class MenuState { LOADING, TRANSITIONING, ACTIVE };

class MainMenu {
private:
    MenuState currentState = MenuState::LOADING;
    float uiAlpha = 0.0f;
    float loadingProgress = 0.0f;
    float transitionTimer = 0.0f;
    bool isReady = false;
    bool quitRequested = false;

    std::vector<Texture2D> bgSlides;
    int currentSlideIndex;
    float slideTimer;
    float panTimer;
    bool switchedIndex;

    Texture2D logoTex;
    std::vector<MenuButton> buttons;
    
    // Lerping parameters
    float baseScale = 1.0f;
    float hoverScale = 1.1f;
    float hoverXOffset = -15.0f;
    Color baseColor = { 200, 200, 200, 255 }; // Dull white
    Color hoverColor = { 255, 255, 255, 255 }; // Bright white

public:
    MainMenu();
    ~MainMenu();

    void Initialize();
    void Update(float deltaTime);
    bool IsReady() const { return isReady; }
    MenuState GetState() const { return currentState; }
    bool ConsumeQuitRequest();
    void Draw(int screenWidth, int screenHeight);
};
