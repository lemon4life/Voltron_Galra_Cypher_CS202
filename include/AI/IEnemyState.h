#pragma once

#include <stdexcept>

class Enemy;

// Design Pattern - State:
// Context: Enemy and its subclasses. State interface: IEnemyState. Concrete
// states in AI headers implement chase, attack, patrol, boss offense, and other
// behaviors; Enemy::ChangeState performs the shared Exit/Enter transition.
class IEnemyState {
public:
    /// Releases resources owned by this IEnemyState instance.
    virtual ~IEnemyState() = default;

    /// Prepares this state when it becomes active.
    virtual void Enter(Enemy* enemy) = 0;
    /// Advances this component's state for the current frame.
    virtual void Update(Enemy* enemy, float deltaTime) = 0;
    /// Cleans up this state before control moves elsewhere.
    virtual void Exit(Enemy* enemy) = 0;
};

// Design Pattern - Adapter:
// ITypedEnemyState adapts the generic IEnemyState Enemy* API to a concrete
// enemy type. RequireTypedEnemy validates the conversion before forwarding to
// Enter/Update/Exit implementations written for Boss, Drone, DemonTHA, etc.
template <typename TEnemy>
class ITypedEnemyState : public IEnemyState {
public:
    /// Prepares this state when it becomes active.
    virtual void Enter(TEnemy* enemy) = 0;
    /// Advances this component's state for the current frame.
    virtual void Update(TEnemy* enemy, float deltaTime) = 0;
    /// Cleans up this state before control moves elsewhere.
    virtual void Exit(TEnemy* enemy) = 0;

private:
    /// Requires and returns typed enemy.
    static TEnemy* RequireTypedEnemy(Enemy* enemy) {
        TEnemy* typedEnemy = dynamic_cast<TEnemy*>(enemy);
        if (!typedEnemy) {
            throw std::logic_error(
                "Enemy state was applied to an incompatible enemy type"
            );
        }
        return typedEnemy;
    }

    /// Prepares this state when it becomes active.
    void Enter(Enemy* enemy) final override {
        /// Prepares this state when it becomes active.
        Enter(RequireTypedEnemy(enemy));
    }

    /// Advances this component's state for the current frame.
    void Update(Enemy* enemy, float deltaTime) final override {
        /// Advances this component's state for the current frame.
        Update(RequireTypedEnemy(enemy), deltaTime);
    }

    /// Cleans up this state before control moves elsewhere.
    void Exit(Enemy* enemy) final override {
        /// Cleans up this state before control moves elsewhere.
        Exit(RequireTypedEnemy(enemy));
    }
};
