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
enum class MainMenuModal { None, Instructions, About };
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
    std::string loadingStatus = "Preparing startup";
    float transitionTimer = 0.0f;
    bool isReady = false;
    bool quitRequested = false;
    bool continueAvailable = false;
    bool roomListOpen = false;
    MainMenuModal openModal = MainMenuModal::None;
    float instructionsScroll = 0.0f;
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

    Texture2D logoTex = {};
    std::vector<MenuButton> buttons;

    // About modal author portraits animation
    float hnaBlinkTimer = 0.0f;
    float tpkBlinkTimer = 0.0f;
    int hnaFrame = 0;
    int tpkFrame = 0;
    float hnaNextBlinkInterval = 3.5f;
    float tpkNextBlinkInterval = 5.0f;
    static constexpr float BLINK_FRAME_DURATION = 0.08f;

    /// Rebuilds buttons.
    void RebuildButtons();
    /// Loads current background.
    void LoadCurrentBackground();
    /// Opens room list.
    void OpenRoomList();
    /// Refreshes room list.
    void RefreshRoomList();
    /// Updates room list.
    void UpdateRoomList();
    /// Renders room list.
    void DrawRoomList(int screenWidth, int screenHeight);
    /// Opens info modal.
    void OpenInfoModal(MainMenuModal modal);
    /// Updates info modal.
    void UpdateInfoModal();
    /// Renders info modal.
    void DrawInfoModal(int screenWidth, int screenHeight);
    
    // Lerping parameters
    float baseScale = 1.0f;
    float hoverScale = 1.1f;
    float hoverXOffset = -15.0f;
    Color baseColor = { 200, 200, 200, 255 }; // Dull white
    Color hoverColor = { 255, 255, 255, 255 }; // Bright white

public:
    /// Creates a MainMenu instance from the supplied configuration.
    MainMenu();
    /// Releases resources owned by this MainMenu instance.
    ~MainMenu();

    /// Initializes the resources and collaborators required before this component can run.
    void Initialize();
    /// Releases resources owned by this component and leaves it safe to destroy.
    void Shutdown();
    /// Advances this component's state for the current frame.
    void Update(float deltaTime);
    /// Reports whether the ready condition is satisfied.
    bool IsReady() const { return isReady; }
    /// Returns the current state.
    MenuState GetState() const { return currentState; }
    /// Consumes and returns quit request.
    bool ConsumeQuitRequest();
    /// Consumes and returns action.
    MainMenuAction ConsumeAction();
    /// Consumes and returns selected room path.
    std::string ConsumeSelectedRoomPath();
    /// Updates the stored continue available.
    void SetContinueAvailable(bool available);
    /// Renders this component using its current state and visual resources.
    void Draw(int screenWidth, int screenHeight);
};
