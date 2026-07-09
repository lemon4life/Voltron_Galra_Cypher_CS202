#pragma once

class Player; // Forward declaration

class IPlayerState {
public:
    virtual ~IPlayerState() = default;
    virtual void Enter(Player* player) = 0;
    virtual void Update(Player* player, float deltaTime) = 0;
    virtual void Exit(Player* player) = 0;
};

class PlayerIdleState : public IPlayerState {
public:
    void Enter(Player* player) override;
    void Update(Player* player, float deltaTime) override;
    void Exit(Player* player) override;
};

class PlayerRunState : public IPlayerState {
public:
    void Enter(Player* player) override;
    void Update(Player* player, float deltaTime) override;
    void Exit(Player* player) override;
};
