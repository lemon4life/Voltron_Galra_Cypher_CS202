#include "UI/MainMenu.h"
#include "UI/UIUtils.h"
#include "Core/Manager/AssetManager.h"
#include "Core/Manager/GameManager.h"
#include "Core/Manager/AudioManager.h"
#include "raymath.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {
    struct RoomListLayout {
        Rectangle panel;
        Rectangle list;
        Rectangle backButton;
        Rectangle editButton;
        Rectangle deleteButton;
        float scale;
        float rowHeight;
    };

    struct InfoModalLayout {
        Rectangle panel;
        Rectangle content;
        Rectangle closeButton;
        float scale;
    };

    struct InstructionLine {
        const char* text;
        bool heading;
    };

    constexpr InstructionLine INSTRUCTION_LINES[] = {
        { "OBJECTIVE", true },
        { "Clear every {O:enemy wave} so the {O:room gates} reopen.", false },
        { "Explore each floor, then use its {O:portal} to continue.", false },
        { "Clear all rooms and defeat the final {O:Boss} on {O:Floor 5}.", false },
        { "", false },
        { "CONTROLS", true },
        { "Move: {K:W}/{K:A}/{K:S}/{K:D} or {K:Arrow Keys}", false },
        { "Aim: {K:Mouse} | Toggle Auto-Aim: {K:T}", false },
        { "Attack: {K:Left Mouse} or {K:J}", false },
        { "Parry: Hold {K:Right Mouse} or {K:L}", false },
        { "Dash: {K:Space}", false },
        { "Skill: {K:E} | Ultimate: {K:Q}", false },
        { "Switch Paladin: {K:Tab} | Direct slots: {K:1}, {K:2}, {K:3}", false },
        { "Interact: {K:F} | Pause: {K:P} or {K:Escape}", false },
        { "Dialogue: {K:W}/{K:S} or {K:Arrows}; {K:Enter} to select", false },
        { "Main Menu: {K:Enter} to Start/Continue", false },
        { "Game Over: {K:R} restart | Victory: {K:Space} return", false },
        { "Developer Admin Panel: {K:F1} (when enabled)", false },
        { "", false },
        { "SKILLS AND ULTIMATES", true },
        { "Skills need personal {O:EX} and cannot restart while active.", false },
        { "Skill EX: {C:Keith} 70%, {C:Lance} 50%, {C:Hunk} 30%, {C:Pidge} 70%.", false },
        { "Deal damage or use {O:EX pots} to refill {O:EX}.", false },
        { "Ultimates need 100 {O:Quintessence} and a ready 5s cooldown.", false },
        { "Enemy rewards and {O:Quintessence pots} refill the shared meter.", false },
        { "", false },
        { "BEGINNER TIPS", true },
        { "Use {K:F} near {O:NPCs}, {O:Paladin stands}, {O:pots}, and {O:portals}.", false },
        { "{O:HP pots} heal the team; {O:EX pots} refill skill energy.", false },
        { "{O:Chests} open when approached and release a reward.", false },
        { "{O:Enhancement machines} spend {O:coins} on permanent upgrades.", false },
        { "Switch Paladins to use different health, skills, and weapons.", false },
        { "Downed Paladins cannot be selected; protect the whole team.", false },
        { "Use the minimap to locate unexplored and special rooms.", false }
    };

    /// Returns the current room list layout.
    RoomListLayout GetRoomListLayout(int screenWidth, int screenHeight) {
        float scale = std::clamp(
            std::min(screenWidth / 1280.0f, screenHeight / 720.0f),
            0.7f,
            1.5f
        );
        float panelWidth = std::min(
            760.0f * scale,
            screenWidth - 40.0f * scale
        );
        float panelHeight = std::min(
            560.0f * scale,
            screenHeight - 40.0f * scale
        );
        Rectangle panel = {
            (screenWidth - panelWidth) * 0.5f,
            (screenHeight - panelHeight) * 0.5f,
            panelWidth,
            panelHeight
        };
        float padding = 24.0f * scale;
        float headerHeight = 70.0f * scale;
        float footerHeight = 82.0f * scale;
        Rectangle list = {
            panel.x + padding,
            panel.y + headerHeight,
            panel.width - padding * 2.0f,
            panel.height - headerHeight - footerHeight
        };
        float gap = 12.0f * scale;
        float buttonWidth = (list.width - gap * 2.0f) / 3.0f;
        float buttonHeight = 42.0f * scale;
        float buttonY = panel.y + panel.height -
            padding - buttonHeight;
        return {
            panel,
            list,
            { list.x, buttonY, buttonWidth, buttonHeight },
            { list.x + buttonWidth + gap, buttonY, buttonWidth, buttonHeight },
            { list.x + (buttonWidth + gap) * 2.0f, buttonY, buttonWidth, buttonHeight },
            scale,
            44.0f * scale
        };
    }

    /// Returns the current info modal layout.
    InfoModalLayout GetInfoModalLayout(int screenWidth, int screenHeight) {
        float scale = std::clamp(
            std::min(screenWidth / 1280.0f, screenHeight / 720.0f),
            0.65f,
            1.4f
        );
        float panelWidth = std::min(
            900.0f * scale,
            screenWidth - 30.0f * scale
        );
        float panelHeight = std::min(
            650.0f * scale,
            screenHeight - 30.0f * scale
        );
        Rectangle panel = {
            (screenWidth - panelWidth) * 0.5f,
            (screenHeight - panelHeight) * 0.5f,
            panelWidth,
            panelHeight
        };
        float padding = 28.0f * scale;
        float headerHeight = 72.0f * scale;
        float footerHeight = 70.0f * scale;
        Rectangle content = {
            panel.x + padding,
            panel.y + headerHeight,
            panel.width - padding * 2.0f,
            panel.height - headerHeight - footerHeight
        };
        float buttonWidth = std::min(220.0f * scale, content.width);
        float buttonHeight = 40.0f * scale;
        Rectangle closeButton = {
            panel.x + (panel.width - buttonWidth) * 0.5f,
            panel.y + panel.height - padding - buttonHeight,
            buttonWidth,
            buttonHeight
        };
        return { panel, content, closeButton, scale };
    }

    /// Returns the current instruction content height.
    float GetInstructionContentHeight(float scale) {
        float height = 0.0f;
        for (const InstructionLine& line : INSTRUCTION_LINES) {
            height += (line.heading ? 42.0f : 31.0f) * scale;
        }
        return height + 24.0f * scale;
    }

    /// Renders room list text.
    void DrawRoomListText(
        const std::string& text,
        Vector2 position,
        float fontSize,
        Color color,
        bool centered = false
    ) {
        Font font = AssetManager::GetInstance().GetCustomFont("PixeloidSans");
        Vector2 size = MeasureTextEx(font, text.c_str(), fontSize, 1.0f);
        if (centered) {
            position.x -= size.x * 0.5f;
            position.y -= size.y * 0.5f;
        }
        DrawTextEx(font, text.c_str(), position, fontSize, 1.0f, color);
    }

    /// Renders info text.
    void DrawInfoText(
        const std::string& fontKey,
        const std::string& text,
        Vector2 position,
        float fontSize,
        Color color,
        bool centered = false
    ) {
        Font font = AssetManager::GetInstance().GetCustomFont(fontKey);
        Vector2 size = MeasureTextEx(font, text.c_str(), fontSize, 1.0f);
        if (centered) {
            position.x -= size.x * 0.5f;
            position.y -= size.y * 0.5f;
        }
        DrawTextEx(font, text.c_str(), position, fontSize, 1.0f, color);
    }

    /// Returns the current character name color.
    Color GetCharacterNameColor(const std::string& name) {
        if (name == "Keith") return Color{ 255, 104, 104, 255 };
        if (name == "Lance") return Color{ 100, 190, 255, 255 };
        if (name == "Hunk") return Color{ 255, 211, 82, 255 };
        if (name == "Pidge") return Color{ 105, 224, 120, 255 };
        return Color{ 235, 164, 255, 255 };
    }

    /// Renders rich instruction line.
    void DrawRichInstructionLine(
        const std::string& markup,
        Vector2 position,
        float fontSize,
        float scale
    ) {
        AssetManager& assets = AssetManager::GetInstance();
        Font normalFont = assets.GetCustomFont("PixeloidSans");
        Font monoFont = assets.GetCustomFont("PixeloidMono");
        Font boldFont = assets.GetCustomFont("PixeloidBold");
        float x = position.x;

        auto drawNormal = [&](const std::string& text) {
            if (text.empty()) return;
            DrawTextEx(
                normalFont,
                text.c_str(),
                { x, position.y },
                fontSize,
                1.0f,
                RAYWHITE
            );
            x += MeasureTextEx(
                normalFont,
                text.c_str(),
                fontSize,
                1.0f
            ).x;
        };

        std::size_t cursor = 0;
        while (cursor < markup.size()) {
            std::size_t marker = markup.find('{', cursor);
            if (marker == std::string::npos) {
                drawNormal(markup.substr(cursor));
                break;
            }
            drawNormal(markup.substr(cursor, marker - cursor));

            std::size_t end = markup.find('}', marker + 1);
            if (end == std::string::npos || marker + 3 >= end ||
                markup[marker + 2] != ':') {
                drawNormal(markup.substr(marker, 1));
                cursor = marker + 1;
                continue;
            }

            char style = markup[marker + 1];
            std::string text = markup.substr(marker + 3, end - marker - 3);
            if (style == 'K') {
                float keyFontSize = fontSize * 0.84f;
                Vector2 textSize = MeasureTextEx(
                    monoFont,
                    text.c_str(),
                    keyFontSize,
                    1.0f
                );
                float paddingX = 6.0f * scale;
                float paddingY = 3.0f * scale;
                Rectangle keycap = {
                    x,
                    position.y - 2.0f * scale,
                    textSize.x + paddingX * 2.0f,
                    textSize.y + paddingY * 2.0f
                };
                DrawRectangleRounded(
                    keycap,
                    0.25f,
                    4,
                    Color{ 244, 211, 94, 255 }
                );
                DrawRectangleLinesEx(
                    keycap,
                    std::max(1.0f, scale),
                    Color{ 108, 76, 22, 255 }
                );
                DrawTextEx(
                    monoFont,
                    text.c_str(),
                    {
                        keycap.x + paddingX,
                        keycap.y + paddingY
                    },
                    keyFontSize,
                    1.0f,
                    Color{ 28, 24, 18, 255 }
                );
                x += keycap.width + 4.0f * scale;
            } else if (style == 'C') {
                Color characterColor = GetCharacterNameColor(text);
                DrawTextEx(
                    boldFont,
                    text.c_str(),
                    { x + scale, position.y + scale },
                    fontSize,
                    1.0f,
                    Color{ 20, 12, 25, 220 }
                );
                DrawTextEx(
                    boldFont,
                    text.c_str(),
                    { x, position.y },
                    fontSize,
                    1.0f,
                    characterColor
                );
                x += MeasureTextEx(
                    boldFont,
                    text.c_str(),
                    fontSize,
                    1.0f
                ).x + 2.0f * scale;
            } else if (style == 'O') {
                float objectFontSize = fontSize * 0.9f;
                Vector2 textSize = MeasureTextEx(
                    boldFont,
                    text.c_str(),
                    objectFontSize,
                    1.0f
                );
                float paddingX = 5.0f * scale;
                Rectangle badge = {
                    x,
                    position.y - scale,
                    textSize.x + paddingX * 2.0f,
                    textSize.y + 3.0f * scale
                };
                DrawRectangleRounded(
                    badge,
                    0.3f,
                    4,
                    Color{ 31, 78, 105, 230 }
                );
                DrawTextEx(
                    boldFont,
                    text.c_str(),
                    { badge.x + paddingX, position.y },
                    objectFontSize,
                    1.0f,
                    Color{ 123, 224, 255, 255 }
                );
                x += badge.width + 3.0f * scale;
            } else {
                drawNormal(text);
            }
            cursor = end + 1;
        }
    }
}

/// Creates a MainMenu instance from the supplied configuration.
MainMenu::MainMenu() : currentSlideIndex(0), slideTimer(0.0f), panTimer(0.0f), switchedIndex(false) {
    logoTex.id = 0;
}

/// Releases resources owned by this MainMenu instance.
MainMenu::~MainMenu() {
}

/// Loads current background.
void MainMenu::LoadCurrentBackground() {
    if (backgroundTexture.id != 0) {
        UnloadTexture(backgroundTexture);
        backgroundTexture = {};
    }
    std::string path = "assets/img/Background/bg_" +
        std::to_string(currentSlideIndex + 1) + ".png";
    backgroundTexture = LoadTexture(path.c_str());
    if (backgroundTexture.id == 0) {
        throw std::runtime_error(
            "Failed to load required menu background: " + path
        );
    }
    SetTextureFilter(backgroundTexture, TEXTURE_FILTER_BILINEAR);
}

/// Initializes the resources and collaborators required before this component can run.
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

/// Releases resources owned by this component and leaves it safe to destroy.
void MainMenu::Shutdown() {
    if (backgroundTexture.id != 0) {
        UnloadTexture(backgroundTexture);
        backgroundTexture = {};
    }
}

/// Rebuilds buttons.
void MainMenu::RebuildButtons() {
    buttons.clear();
    std::vector<std::string> titles;
    if (continueAvailable) {
        titles.push_back("Continue");
    }
    titles.push_back("Start Game");
    titles.push_back("Settings");
    titles.push_back("Room Editor");
    titles.push_back("Room List");
    titles.push_back("Instructions");
    titles.push_back("About Us");
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

/// Advances this component's state for the current frame.
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
        bool done = AssetManager::GetInstance().UpdateLoading(
            loadingProgress,
            loadingStatus
        );
        if (done) {
            loadingProgress = 1.0f;
            currentState = MenuState::TRANSITIONING;
            transitionTimer = 0.0f;
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

        if (openModal != MainMenuModal::None) {
            UpdateInfoModal();
            return;
        }

        if (roomListOpen) {
            UpdateRoomList();
            return;
        }

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
                    AudioManager::GetInstance().PlayRandomClick();
                        pendingAction = MainMenuAction::StartGame;
                    } else if (btn.text == "Continue") {
                    AudioManager::GetInstance().PlayRandomClick();
                        pendingAction = MainMenuAction::Continue;
                    } else if (btn.text == "Settings") {
                    AudioManager::GetInstance().PlayRandomClick();
                        GameManager::GetInstance().SetState(GameState::SETTINGS);
                    } else if (btn.text == "Room Editor") {
                    AudioManager::GetInstance().PlayRandomClick();
                        pendingAction = MainMenuAction::OpenEditor;
                    } else if (btn.text == "Room List") {
                    AudioManager::GetInstance().PlayRandomClick();
                        OpenRoomList();
                    } else if (btn.text == "Instructions") {
                    AudioManager::GetInstance().PlayRandomClick();
                        OpenInfoModal(MainMenuModal::Instructions);
                    } else if (btn.text == "About Us") {
                    AudioManager::GetInstance().PlayRandomClick();
                        OpenInfoModal(MainMenuModal::About);
                    } else if (btn.text == "Exit Game") {
                    AudioManager::GetInstance().PlayRandomClick();
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

/// Consumes and returns quit request.
bool MainMenu::ConsumeQuitRequest() {
    bool requested = quitRequested;
    quitRequested = false;
    return requested;
}

/// Consumes and returns action.
MainMenuAction MainMenu::ConsumeAction() {
    MainMenuAction action = pendingAction;
    pendingAction = MainMenuAction::None;
    return action;
}

/// Consumes and returns selected room path.
std::string MainMenu::ConsumeSelectedRoomPath() {
    std::string path = selectedRoomPath;
    selectedRoomPath.clear();
    return path;
}

/// Opens room list.
void MainMenu::OpenRoomList() {
    openModal = MainMenuModal::None;
    roomListOpen = true;
    roomListScroll = 0.0f;
    RefreshRoomList();
}

/// Opens info modal.
void MainMenu::OpenInfoModal(MainMenuModal modal) {
    roomListOpen = false;
    openModal = modal;
    instructionsScroll = 0.0f;
}

/// Updates info modal.
void MainMenu::UpdateInfoModal() {
    if (IsKeyPressed(KEY_ESCAPE)) {
        openModal = MainMenuModal::None;
        return;
    }

    InfoModalLayout layout = GetInfoModalLayout(
        GetScreenWidth(),
        GetScreenHeight()
    );
    Vector2 mouse = GetMousePosition();
    if (openModal == MainMenuModal::Instructions &&
        CheckCollisionPointRec(mouse, layout.content)) {
        float minimumScroll = std::min(
            0.0f,
            layout.content.height -
                GetInstructionContentHeight(layout.scale)
        );
        instructionsScroll = std::clamp(
            instructionsScroll +
                GetMouseWheelMove() * 38.0f * layout.scale,
            minimumScroll,
            0.0f
        );
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
        CheckCollisionPointRec(mouse, layout.closeButton)) {
        AudioManager::GetInstance().PlayRandomClick();
        openModal = MainMenuModal::None;
    }
}

/// Refreshes room list.
void MainMenu::RefreshRoomList() {
    std::string previousSelection;
    if (selectedRoomIndex >= 0 &&
        selectedRoomIndex < static_cast<int>(savedRooms.size())) {
        previousSelection = savedRooms[selectedRoomIndex].path;
    }

    savedRooms = LevelIO::ListSavedRooms();
    selectedRoomIndex = -1;
    for (std::size_t index = 0; index < savedRooms.size(); ++index) {
        if (savedRooms[index].path == previousSelection) {
            selectedRoomIndex = static_cast<int>(index);
            break;
        }
    }
    if (selectedRoomIndex < 0 && !savedRooms.empty()) {
        selectedRoomIndex = 0;
    }
}

/// Updates room list.
void MainMenu::UpdateRoomList() {
    if (IsKeyPressed(KEY_ESCAPE)) {
        roomListOpen = false;
        return;
    }

    RoomListLayout layout = GetRoomListLayout(
        GetScreenWidth(),
        GetScreenHeight()
    );
    Vector2 mouse = GetMousePosition();
    float contentHeight = savedRooms.size() * layout.rowHeight;
    float minimumScroll = std::min(0.0f, layout.list.height - contentHeight);
    if (CheckCollisionPointRec(mouse, layout.list)) {
        roomListScroll = std::clamp(
            roomListScroll + GetMouseWheelMove() * 30.0f * layout.scale,
            minimumScroll,
            0.0f
        );
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            float localY = mouse.y - layout.list.y - roomListScroll;
            int index = static_cast<int>(std::floor(
                localY / layout.rowHeight
            ));
            if (index >= 0 && index < static_cast<int>(savedRooms.size())) {
                selectedRoomIndex = index;
            }
        }
    }

    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) return;
    if (CheckCollisionPointRec(mouse, layout.backButton)) {
        AudioManager::GetInstance().PlayRandomClick();
        roomListOpen = false;
        return;
    }
    if (CheckCollisionPointRec(mouse, layout.editButton) &&
        selectedRoomIndex >= 0 &&
        selectedRoomIndex < static_cast<int>(savedRooms.size())) {
        AudioManager::GetInstance().PlayRandomClick();
        selectedRoomPath = savedRooms[selectedRoomIndex].path;
        pendingAction = MainMenuAction::OpenSavedRoomEditor;
        roomListOpen = false;
        return;
    }
    if (CheckCollisionPointRec(mouse, layout.deleteButton) &&
        selectedRoomIndex >= 0 &&
        selectedRoomIndex < static_cast<int>(savedRooms.size())) {
        AudioManager::GetInstance().PlayRandomClick();
        LevelIO::DeleteSavedRoom(savedRooms[selectedRoomIndex].path);
        RefreshRoomList();
        contentHeight = savedRooms.size() * layout.rowHeight;
        minimumScroll = std::min(0.0f, layout.list.height - contentHeight);
        roomListScroll = std::clamp(
            roomListScroll,
            minimumScroll,
            0.0f
        );
    }
}

/// Updates the stored continue available.
void MainMenu::SetContinueAvailable(bool available) {
    if (continueAvailable == available) return;

    continueAvailable = available;
    RebuildButtons();
}

/// Renders this component using its current state and visual resources.
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
        int barWidth = static_cast<int>(std::clamp(
            screenWidth * 0.52f,
            320.0f,
            760.0f
        ));
        int barHeight = std::max(4, static_cast<int>(screenHeight / 120.0f));
        int barX = (screenWidth - barWidth) / 2;
        int barY = screenHeight * 0.75f + loadingSlideY;
        
        UIUtils::DrawProgressBar(
            { (float)barX, (float)barY, (float)barWidth, (float)barHeight },
            loadingProgress, 1.0f,
            Fade(RAYWHITE, loadingAlpha * 0.3f), // Assuming bg color slightly faded
            Fade(RAYWHITE, loadingAlpha)
        );
        
        float statusFontSize = std::clamp(
            screenHeight / 45.0f,
            12.0f,
            22.0f
        );
        DrawRoomListText(
            loadingStatus,
            {
                barX + barWidth * 0.5f,
                static_cast<float>(barY) - 24.0f
            },
            statusFontSize,
            Fade(RAYWHITE, loadingAlpha),
            true
        );
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
        float baseMenuFontSize =
            static_cast<float>(UIUtils::FontSize::HEADER) * scaleFactor;
        float baseButtonHeight = baseMenuFontSize + 20.0f * scaleFactor;
        float availableButtonHeight = std::max(
            0.0f,
            screenHeight - startY - baseButtonHeight - 14.0f * scaleFactor
        );
        float btnSpacing = 55.0f * scaleFactor;
        if (buttons.size() > 1) {
            btnSpacing = std::min(
                btnSpacing,
                availableButtonHeight /
                    static_cast<float>(buttons.size() - 1)
            );
        }
        
        float btnSlideY = (1.0f - uiAlpha) * (30.0f * scaleFactor); // Buttons slide up as they fade in
        
        for (size_t i = 0; i < buttons.size(); i++) {
            auto& btn = buttons[i];
            
            float baseFontSize = baseMenuFontSize;
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

    if (roomListOpen) {
        DrawRoomList(screenWidth, screenHeight);
    }
    if (openModal != MainMenuModal::None) {
        DrawInfoModal(screenWidth, screenHeight);
    }
}

/// Renders info modal.
void MainMenu::DrawInfoModal(int screenWidth, int screenHeight) {
    InfoModalLayout layout = GetInfoModalLayout(screenWidth, screenHeight);
    DrawRectangle(
        0,
        0,
        screenWidth,
        screenHeight,
        ColorAlpha(BLACK, 0.82f)
    );
    DrawRectangleRec(layout.panel, Color{ 25, 31, 43, 252 });
    DrawRectangleLinesEx(
        layout.panel,
        std::max(1.0f, 2.0f * layout.scale),
        Color{ 145, 156, 178, 255 }
    );

    const char* title = openModal == MainMenuModal::About
        ? "ABOUT US"
        : "INSTRUCTIONS";
    DrawInfoText(
        "PixeloidBold",
        title,
        {
            layout.panel.x + layout.panel.width * 0.5f,
            layout.panel.y + 36.0f * layout.scale
        },
        34.0f * layout.scale,
        RAYWHITE,
        true
    );

    DrawRectangleRec(layout.content, Color{ 14, 18, 27, 235 });
    BeginScissorMode(
        static_cast<int>(layout.content.x),
        static_cast<int>(layout.content.y),
        static_cast<int>(layout.content.width),
        static_cast<int>(layout.content.height)
    );

    if (openModal == MainMenuModal::About) {
        float centerX = layout.content.x + layout.content.width * 0.5f;
        float y = layout.content.y + 46.0f * layout.scale;
        DrawInfoText(
            "PixeloidBold",
            "VOLTRON MISSION - GALRA CYPHER",
            { centerX, y },
            27.0f * layout.scale,
            GOLD,
            true
        );
        y += 64.0f * layout.scale;
        DrawInfoText(
            "PixeloidSans",
            "Developed by",
            { centerX, y },
            21.0f * layout.scale,
            LIGHTGRAY,
            true
        );
        y += 42.0f * layout.scale;
        DrawInfoText(
            "PixeloidBold",
            "Tran Phuc Khanh",
            { centerX, y },
            25.0f * layout.scale,
            RAYWHITE,
            true
        );
        y += 38.0f * layout.scale;
        DrawInfoText(
            "PixeloidBold",
            "Hoang Nguyen Anh",
            { centerX, y },
            25.0f * layout.scale,
            RAYWHITE,
            true
        );
        y += 62.0f * layout.scale;
        DrawInfoText(
            "PixeloidSans",
            "A course project for CS202",
            { centerX, y },
            21.0f * layout.scale,
            LIGHTGRAY,
            true
        );
        y += 34.0f * layout.scale;
        DrawInfoText(
            "PixeloidSans",
            "Advanced Program in Computer Science (APCS)",
            { centerX, y },
            19.0f * layout.scale,
            LIGHTGRAY,
            true
        );
        y += 34.0f * layout.scale;
        DrawInfoText(
            "PixeloidSans",
            "HCMUS - Academic Year 2025-2026",
            { centerX, y },
            19.0f * layout.scale,
            LIGHTGRAY,
            true
        );
    } else {
        float textX = layout.content.x + 22.0f * layout.scale;
        float y = layout.content.y + instructionsScroll +
            12.0f * layout.scale;
        for (const InstructionLine& line : INSTRUCTION_LINES) {
            float lineHeight = (line.heading ? 42.0f : 31.0f) *
                layout.scale;
            if (line.text[0] != '\0') {
                if (line.heading) {
                    DrawInfoText(
                        "PixeloidBold",
                        line.text,
                        { textX, y },
                        21.0f * layout.scale,
                        GOLD
                    );
                } else {
                    DrawRichInstructionLine(
                        line.text,
                        { textX, y },
                        16.0f * layout.scale,
                        layout.scale
                    );
                }
            }
            y += lineHeight;
        }
    }
    EndScissorMode();

    Vector2 mouse = GetMousePosition();
    Color closeColor = Color{ 108, 116, 132, 255 };
    if (CheckCollisionPointRec(mouse, layout.closeButton)) {
        closeColor = ColorBrightness(closeColor, 0.18f);
    }
    DrawRectangleRec(layout.closeButton, closeColor);
    DrawRectangleLinesEx(layout.closeButton, 1.0f, BLACK);
    DrawInfoText(
        "PixeloidBold",
        "CLOSE",
        {
            layout.closeButton.x + layout.closeButton.width * 0.5f,
            layout.closeButton.y + layout.closeButton.height * 0.5f
        },
        18.0f * layout.scale,
        WHITE,
        true
    );

    if (openModal == MainMenuModal::Instructions &&
        GetInstructionContentHeight(layout.scale) > layout.content.height) {
        DrawInfoText(
            "PixeloidSans",
            "Scroll for more",
            {
                layout.content.x + layout.content.width * 0.5f,
                layout.content.y + layout.content.height -
                    14.0f * layout.scale
            },
            13.0f * layout.scale,
            GRAY,
            true
        );
    }
}

/// Renders room list.
void MainMenu::DrawRoomList(int screenWidth, int screenHeight) {
    RoomListLayout layout = GetRoomListLayout(screenWidth, screenHeight);
    DrawRectangle(
        0,
        0,
        screenWidth,
        screenHeight,
        ColorAlpha(BLACK, 0.78f)
    );
    DrawRectangleRec(layout.panel, Color{ 26, 31, 40, 250 });
    DrawRectangleLinesEx(layout.panel, 2.0f * layout.scale, LIGHTGRAY);
    DrawRoomListText(
        "SAVED ROOMS",
        {
            layout.panel.x + layout.panel.width * 0.5f,
            layout.panel.y + 34.0f * layout.scale
        },
        28.0f * layout.scale,
        WHITE,
        true
    );

    DrawRectangleRec(layout.list, Color{ 12, 16, 23, 230 });
    BeginScissorMode(
        static_cast<int>(layout.list.x),
        static_cast<int>(layout.list.y),
        static_cast<int>(layout.list.width),
        static_cast<int>(layout.list.height)
    );
    if (savedRooms.empty()) {
        DrawRoomListText(
            "No saved rooms",
            {
                layout.list.x + layout.list.width * 0.5f,
                layout.list.y + layout.list.height * 0.5f
            },
            18.0f * layout.scale,
            GRAY,
            true
        );
    } else {
        Vector2 mouse = GetMousePosition();
        for (std::size_t index = 0; index < savedRooms.size(); ++index) {
            Rectangle row = {
                layout.list.x + 4.0f * layout.scale,
                layout.list.y + roomListScroll +
                    index * layout.rowHeight,
                layout.list.width - 8.0f * layout.scale,
                layout.rowHeight - 4.0f * layout.scale
            };
            bool selected = selectedRoomIndex == static_cast<int>(index);
            bool hovered = CheckCollisionPointRec(mouse, row) &&
                CheckCollisionPointRec(mouse, layout.list);
            Color rowColor = selected
                ? Color{ 58, 104, 145, 255 }
                : Color{ 48, 54, 65, 245 };
            if (hovered) rowColor = ColorBrightness(rowColor, 0.18f);
            DrawRectangleRec(row, rowColor);
            DrawRectangleLinesEx(row, 1.0f, selected ? SKYBLUE : DARKGRAY);

            DrawRoomListText(
                savedRooms[index].name,
                {
                    row.x + 14.0f * layout.scale,
                    row.y + row.height * 0.5f
                },
                16.0f * layout.scale,
                WHITE,
                false
            );
        }
    }
    EndScissorMode();

    struct ModalButton {
        Rectangle bounds;
        const char* label;
        Color color;
        bool enabled;
    };
    bool hasSelection = selectedRoomIndex >= 0 &&
        selectedRoomIndex < static_cast<int>(savedRooms.size());
    ModalButton buttons[] = {
        { layout.backButton, "BACK", Color{ 115, 122, 134, 255 }, true },
        { layout.editButton, "EDIT", GREEN, hasSelection },
        { layout.deleteButton, "DELETE", RED, hasSelection }
    };
    Vector2 mouse = GetMousePosition();
    for (const ModalButton& button : buttons) {
        Color color = button.enabled ? button.color : DARKGRAY;
        if (button.enabled && CheckCollisionPointRec(mouse, button.bounds)) {
            color = ColorBrightness(color, 0.18f);
        }
        DrawRectangleRec(button.bounds, color);
        DrawRectangleLinesEx(button.bounds, 1.0f * layout.scale, BLACK);
        DrawRoomListText(
            button.label,
            {
                button.bounds.x + button.bounds.width * 0.5f,
                button.bounds.y + button.bounds.height * 0.5f
            },
            16.0f * layout.scale,
            button.enabled ? WHITE : GRAY,
            true
        );
    }
}
