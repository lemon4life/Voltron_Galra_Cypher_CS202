#pragma once

// Design Pattern - State:
// Context: GameManager/GameApplication. State interface: IGameState.
// Concrete states: GameplayState, HubState, menu/overlay states, and
// RoomEditorStateAdapter. The context delegates each frame's Update/Draw.
class IGameState {
public:
    /// Releases resources owned by this IGameState instance.
    virtual ~IGameState() = default;
    /// Advances this component's state for the current frame.
    virtual void Update(float deltaTime) = 0;
    /// Renders this component using its current state and visual resources.
    virtual void Draw() = 0;
};
