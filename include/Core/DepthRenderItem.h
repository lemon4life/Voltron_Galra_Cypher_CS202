#pragma once

#include <functional>

// Design Pattern - Lightweight Command:
// Invoker: GameplayState/HubState after depth sorting. Command: drawFunc.
// Receivers are captured world objects or renderers, allowing heterogeneous
// drawing operations to be queued and executed uniformly in y-order.
struct DepthRenderItem {
    float ySort;
    std::function<void()> drawFunc;
};
