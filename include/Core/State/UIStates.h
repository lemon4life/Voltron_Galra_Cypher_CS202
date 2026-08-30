#pragma once
#include "Core/State/IGameState.h"
#include "UI/MainMenu.h"
#include "UI/PauseMenu.h"
#include "UI/SettingsMenu.h"
#include "Core/Level/RoomEditorState.h"
#include "Core/GameApplication.h" // For callbacks like StartNewGame

class MainMenuState : public IGameState {
public:
    MainMenuState(MainMenu* menu, GameApplication* app);
    void Update(float deltaTime) override;
    void Draw() override;
private:
    MainMenu* menu;
    GameApplication* app;
};

class PauseState : public IGameState {
public:
    PauseState(PauseMenu* menu, GameApplication* app, IGameState* backgroundState);
    void Update(float deltaTime) override;
    void Draw() override;
private:
    PauseMenu* menu;
    GameApplication* app;
    IGameState* backgroundState;
};

class SettingsState : public IGameState {
public:
    SettingsState(SettingsMenu* menu, GameApplication* app, IGameState* backgroundState);
    void Update(float deltaTime) override;
    void Draw() override;
private:
    SettingsMenu* menu;
    GameApplication* app;
    IGameState* backgroundState;
    bool closeRequested;
};

class GameOverState : public IGameState {
public:
    GameOverState(GameApplication* app, IGameState* backgroundState);
    void Update(float deltaTime) override;
    void Draw() override;
private:
    GameApplication* app;
    IGameState* backgroundState;
};

class VictoryState : public IGameState {
public:
    VictoryState(GameApplication* app, IGameState* backgroundState);
    void Update(float deltaTime) override;
    void Draw() override;
private:
    GameApplication* app;
    IGameState* backgroundState;
};

class RoomEditorStateAdapter : public IGameState {
public:
    RoomEditorStateAdapter(RoomEditorState* editor);
    void Update(float deltaTime) override;
    void Draw() override;
private:
    RoomEditorState* editor;
};
