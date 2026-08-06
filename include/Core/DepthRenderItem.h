#pragma once

#include <functional>

struct DepthRenderItem {
    float ySort;
    std::function<void()> drawFunc;
};
