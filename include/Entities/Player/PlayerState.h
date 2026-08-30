#pragma once

class Paladin; // Forward declaration

// Design Pattern - State:
// Context: Paladin. State interface: IPlayerState. Concrete states handle idle,
// run, attack, dash, parry, and down behavior; Paladin::ChangeState coordinates
// Exit/Enter so Paladin does not contain one large movement/combat state switch.
class IPlayerState {
public:
    /// Releases resources owned by this IPlayerState instance.
    virtual ~IPlayerState() = default;
    /// Prepares this state when it becomes active.
    virtual void Enter(Paladin* player) = 0;
    /// Advances this component's state for the current frame.
    virtual void Update(Paladin* player, float deltaTime) = 0;
    /// Cleans up this state before control moves elsewhere.
    virtual void Exit(Paladin* player) = 0;
};

class PlayerIdleState : public IPlayerState {
public:
    /// Prepares this state when it becomes active.
    void Enter(Paladin* player) override;
    /// Advances this component's state for the current frame.
    void Update(Paladin* player, float deltaTime) override;
    /// Cleans up this state before control moves elsewhere.
    void Exit(Paladin* player) override;
};

class PlayerRunState : public IPlayerState {
public:
    /// Prepares this state when it becomes active.
    void Enter(Paladin* player) override;
    /// Advances this component's state for the current frame.
    void Update(Paladin* player, float deltaTime) override;
    /// Cleans up this state before control moves elsewhere.
    void Exit(Paladin* player) override;
};

class PlayerParryState : public IPlayerState {
private:
    float parryTimer;
public:
    /// Prepares this state when it becomes active.
    void Enter(Paladin* player) override;
    /// Advances this component's state for the current frame.
    void Update(Paladin* player, float deltaTime) override;
    /// Cleans up this state before control moves elsewhere.
    void Exit(Paladin* player) override;
};

class PlayerDownState : public IPlayerState {
private:
    float bounceTimer;
    float initialY;
public:
    /// Prepares this state when it becomes active.
    void Enter(Paladin* player) override;
    /// Advances this component's state for the current frame.
    void Update(Paladin* player, float deltaTime) override;
    /// Cleans up this state before control moves elsewhere.
    void Exit(Paladin* player) override;
};
