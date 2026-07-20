#pragma once

class Paladin; // Forward declaration

class IPlayerState {
public:
    virtual ~IPlayerState() = default;
    virtual void Enter(Paladin* player) = 0;
    virtual void Update(Paladin* player, float deltaTime) = 0;
    virtual void Exit(Paladin* player) = 0;
};

class PlayerIdleState : public IPlayerState {
public:
    void Enter(Paladin* player) override;
    void Update(Paladin* player, float deltaTime) override;
    void Exit(Paladin* player) override;
};

class PlayerRunState : public IPlayerState {
public:
    void Enter(Paladin* player) override;
    void Update(Paladin* player, float deltaTime) override;
    void Exit(Paladin* player) override;
};

class PlayerParryState : public IPlayerState {
private:
    float parryTimer;
public:
    void Enter(Paladin* player) override;
    void Update(Paladin* player, float deltaTime) override;
    void Exit(Paladin* player) override;
};
