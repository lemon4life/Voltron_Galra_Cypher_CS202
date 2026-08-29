#pragma once

#include "raylib.h"
#include "Core/Level/LevelIO.h"

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
enum class MainMenuAction {
    None,
    StartGame,
    Continue,
    OpenEditor,
    OpenSavedRoomEditor
};

class MainMenu {
private:
    MenuState currentState = MenuState::LOADING;
    float uiAlpha = 0.0f;
    float loadingProgress = 0.0f;
    float transitionTimer = 0.0f;
    bool isReady = false;
    bool quitRequested = false;
    bool continueAvailable = false;
    bool roomListOpen = false;
    MainMenuAction pendingAction = MainMenuAction::None;
    std::vector<SavedRoomInfo> savedRooms;
    int selectedRoomIndex = -1;
    float roomListScroll = 0.0f;
    std::string selectedRoomPath;

    Texture2D backgroundTexture = {};
    static constexpr int BACKGROUND_COUNT = 9;
    int currentSlideIndex;
    float slideTimer;
    float panTimer;
    bool switchedIndex;

    Texture2D logoTex;
    std::vector<MenuButton> buttons;

    void RebuildButtons();
    void LoadCurrentBackground();
    void OpenRoomList();
    void RefreshRoomList();
    void UpdateRoomList();
    void DrawRoomList(int screenWidth, int screenHeight);
    
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
    void Shutdown();
    void Update(float deltaTime);
    bool IsReady() const { return isReady; }
    MenuState GetState() const { return currentState; }
    bool ConsumeQuitRequest();
    MainMenuAction ConsumeAction();
    std::string ConsumeSelectedRoomPath();
    void SetContinueAvailable(bool available);
    void Draw(int screenWidth, int screenHeight);
};
