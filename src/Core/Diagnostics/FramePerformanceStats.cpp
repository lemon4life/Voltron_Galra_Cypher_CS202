#include "Core/Diagnostics/FramePerformanceStats.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

namespace {
constexpr float SAMPLE_WINDOW_SECONDS = 30.0f;
constexpr float RECALCULATION_INTERVAL_SECONDS = 0.25f;
constexpr float HITCH_THRESHOLD_SECONDS = 0.050f;

float FpsFromFrameTime(float frameTime) {
    return frameTime > 0.0f ? 1.0f / frameTime : 0.0f;
}

float SlowestAverageFps(
    const std::vector<float>& sortedFrameTimes,
    float fraction
) {
    if (sortedFrameTimes.empty()) return 0.0f;
    std::size_t count = std::max<std::size_t>(
        1,
        static_cast<std::size_t>(
            std::ceil(sortedFrameTimes.size() * fraction)
        )
    );
    float total = std::accumulate(
        sortedFrameTimes.end() - count,
        sortedFrameTimes.end(),
        0.0f
    );
    return FpsFromFrameTime(total / static_cast<float>(count));
}

float PercentileMilliseconds(
    const std::vector<float>& sortedFrameTimes,
    float percentile
) {
    if (sortedFrameTimes.empty()) return 0.0f;
    std::size_t index = static_cast<std::size_t>(std::ceil(
        percentile * static_cast<float>(sortedFrameTimes.size())
    ));
    index = std::clamp<std::size_t>(index, 1, sortedFrameTimes.size()) - 1;
    return sortedFrameTimes[index] * 1000.0f;
}
}

FramePerformanceStats& FramePerformanceStats::GetInstance() {
    static FramePerformanceStats instance;
    return instance;
}

void FramePerformanceStats::Reset() {
    frameTimes.clear();
    accumulatedTime = 0.0f;
    calculationTimer = 0.0f;
    snapshot = {};
}

void FramePerformanceStats::Update(float deltaTime, int targetFps) {
    if (!std::isfinite(deltaTime) || deltaTime <= 0.0f) return;

    frameTimes.push_back(deltaTime);
    accumulatedTime += deltaTime;
    calculationTimer += deltaTime;
    while (accumulatedTime > SAMPLE_WINDOW_SECONDS &&
           frameTimes.size() > 1) {
        accumulatedTime -= frameTimes.front();
        frameTimes.pop_front();
    }

    snapshot.currentFrameMilliseconds = deltaTime * 1000.0f;
    snapshot.currentFps = FpsFromFrameTime(deltaTime);
    if (calculationTimer >= RECALCULATION_INTERVAL_SECONDS ||
        snapshot.sampleCount == 0) {
        calculationTimer = 0.0f;
        Recalculate(targetFps);
    }
}

void FramePerformanceStats::Recalculate(int targetFps) {
    if (frameTimes.empty()) {
        snapshot = {};
        return;
    }

    std::vector<float> sorted(frameTimes.begin(), frameTimes.end());
    std::sort(sorted.begin(), sorted.end());
    const float total = std::accumulate(sorted.begin(), sorted.end(), 0.0f);
    const float average = total / static_cast<float>(sorted.size());
    const float targetFrameTime = targetFps > 0
        ? 1.0f / static_cast<float>(targetFps)
        : 0.0f;

    std::size_t belowTarget = 0;
    std::size_t hitches = 0;
    float squaredDeviation = 0.0f;
    for (float frameTime : frameTimes) {
        if (targetFrameTime > 0.0f && frameTime > targetFrameTime) {
            ++belowTarget;
        }
        if (frameTime >= HITCH_THRESHOLD_SECONDS) ++hitches;
        float deviation = frameTime - average;
        squaredDeviation += deviation * deviation;
    }

    snapshot.averageFps = FpsFromFrameTime(average);
    snapshot.lowestFps = FpsFromFrameTime(sorted.back());
    snapshot.highestFps = FpsFromFrameTime(sorted.front());
    snapshot.onePercentLowFps = SlowestAverageFps(sorted, 0.01f);
    snapshot.pointOnePercentLowFps = SlowestAverageFps(sorted, 0.001f);
    snapshot.belowTargetPercent = 100.0f * static_cast<float>(belowTarget) /
        static_cast<float>(frameTimes.size());
    snapshot.hitchPercent = 100.0f * static_cast<float>(hitches) /
        static_cast<float>(frameTimes.size());
    snapshot.averageFrameMilliseconds = average * 1000.0f;
    snapshot.p95FrameMilliseconds = PercentileMilliseconds(sorted, 0.95f);
    snapshot.p99FrameMilliseconds = PercentileMilliseconds(sorted, 0.99f);
    snapshot.maximumFrameMilliseconds = sorted.back() * 1000.0f;
    snapshot.frameTimeDeviationMilliseconds = std::sqrt(
        squaredDeviation / static_cast<float>(frameTimes.size())
    ) * 1000.0f;
    snapshot.windowSeconds = accumulatedTime;
    snapshot.sampleCount = frameTimes.size();
}
