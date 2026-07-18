#pragma once

class Enemy;

class IEnemyState {
public:
    virtual ~IEnemyState() = default;

    virtual void Enter(Enemy* enemy) = 0;
    virtual void Update(Enemy* enemy, float deltaTime) = 0;
    virtual void Exit(Enemy* enemy) = 0;
};

template <typename TEnemy>
class ITypedEnemyState : public IEnemyState {
public:
    virtual void Enter(TEnemy* enemy) = 0;
    virtual void Update(TEnemy* enemy, float deltaTime) = 0;
    virtual void Exit(TEnemy* enemy) = 0;

private:
    void Enter(Enemy* enemy) final override {
        Enter(static_cast<TEnemy*>(enemy));
    }

    void Update(Enemy* enemy, float deltaTime) final override {
        Update(static_cast<TEnemy*>(enemy), deltaTime);
    }

    void Exit(Enemy* enemy) final override {
        Exit(static_cast<TEnemy*>(enemy));
    }
};
