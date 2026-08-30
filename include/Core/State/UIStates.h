#pragma once
#include "Core/State/IGameState.h"
#include "UI/MainMenu.h"
#include "UI/PauseMenu.h"
#include "UI/SettingsMenu.h"
#include "Core/Level/RoomEditorState.h"
#include "Core/GameApplication.h" // For callbacks like StartNewGame

class MainMenuState : public IGameState {
public:
    /// Creates a MainMenuState instance from the supplied configuration.
    MainMenuState(MainMenu* menu, GameApplication* app);
    /// Advances this component's state for the current frame.
    void Update(float deltaTime) override;
    /// Renders this component using its current state and visual resources.
    void Draw() override;
private:
    MainMenu* menu;
    GameApplication* app;
};

class PauseState : public IGameState {
public:
    /// Creates a PauseState instance from the supplied configuration.
    PauseState(PauseMenu* menu, GameApplication* app, IGameState* backgroundState);
    /// Advances this component's state for the current frame.
    void Update(float deltaTime) override;
    /// Renders this component using its current state and visual resources.
    void Draw() override;
private:
    PauseMenu* menu;
    GameApplication* app;
    IGameState* backgroundState;
};

class SettingsState : public IGameState {
public:
    /// Creates a SettingsState instance from the supplied configuration.
    SettingsState(SettingsMenu* menu, GameApplication* app, IGameState* backgroundState);
    /// Advances this component's state for the current frame.
    void Update(float deltaTime) override;
    /// Renders this component using its current state and visual resources.
    void Draw() override;
private:
    SettingsMenu* menu;
    GameApplication* app;
    IGameState* backgroundState;
    bool closeRequested;
};

class GameOverState : public IGameState {
public:
    /// Creates a GameOverState instance from the supplied configuration.
    GameOverState(GameApplication* app, IGameState* backgroundState);
    /// Advances this component's state for the current frame.
    void Update(float deltaTime) override;
    /// Renders this component using its current state and visual resources.
    void Draw() override;
private:
    GameApplication* app;
    IGameState* backgroundState;
};

class VictoryState : public IGameState {
public:
    /// Creates a VictoryState instance from the supplied configuration.
    VictoryState(GameApplication* app, IGameState* backgroundState);
    /// Advances this component's state for the current frame.
    void Update(float deltaTime) override;
    /// Renders this component using its current state and visual resources.
    void Draw() override;
private:
    GameApplication* app;
    IGameState* backgroundState;
};

// Design Pattern - Adapter:
// Adaptee: RoomEditorState. Target interface: IGameState. This adapter translates
// the game loop's Update/Draw calls so the editor can participate as a game state.
class RoomEditorStateAdapter : public IGameState {
public:
    /// Creates a RoomEditorStateAdapter instance from the supplied configuration.
    RoomEditorStateAdapter(RoomEditorState* editor);
    /// Advances this component's state for the current frame.
    void Update(float deltaTime) override;
    /// Renders this component using its current state and visual resources.
    void Draw() override;
private:
    RoomEditorState* editor;
};
