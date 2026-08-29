#pragma once

#include <stdexcept>

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
    static TEnemy* RequireTypedEnemy(Enemy* enemy) {
        TEnemy* typedEnemy = dynamic_cast<TEnemy*>(enemy);
        if (!typedEnemy) {
            throw std::logic_error(
                "Enemy state was applied to an incompatible enemy type"
            );
        }
        return typedEnemy;
    }

    void Enter(Enemy* enemy) final override {
        Enter(RequireTypedEnemy(enemy));
    }

    void Update(Enemy* enemy, float deltaTime) final override {
        Update(RequireTypedEnemy(enemy), deltaTime);
    }

    void Exit(Enemy* enemy) final override {
        Exit(RequireTypedEnemy(enemy));
    }
};
