#pragma once

#include <cstddef>
#include <deque>
#include <vector>

struct FramePerformanceSnapshot {
    float currentFps = 0.0f;
    float averageFps = 0.0f;
    float lowestFps = 0.0f;
    float highestFps = 0.0f;
    float onePercentLowFps = 0.0f;
    float pointOnePercentLowFps = 0.0f;
    float belowTargetPercent = 0.0f;
    float hitchPercent = 0.0f;
    float currentFrameMilliseconds = 0.0f;
    float averageFrameMilliseconds = 0.0f;
    float p95FrameMilliseconds = 0.0f;
    float p99FrameMilliseconds = 0.0f;
    float maximumFrameMilliseconds = 0.0f;
    float frameTimeDeviationMilliseconds = 0.0f;
    float windowSeconds = 0.0f;
    std::size_t sampleCount = 0;
};

// Design Pattern - Singleton:
// FramePerformanceStats is the single process-wide accumulator for frame samples,
// ensuring gameplay and diagnostics read the same rolling measurements.
class FramePerformanceStats {
public:
    /// Returns the process-wide singleton instance of this manager.
    static FramePerformanceStats& GetInstance();

    /// Advances this component's state for the current frame.
    void Update(float deltaTime, int targetFps);
    /// Restores this component to its initial runtime state.
    void Reset();
    /// Returns the current snapshot.
    const FramePerformanceSnapshot& GetSnapshot() const { return snapshot; }

private:
    /// Recomputes cached values from the latest runtime samples.
    void Recalculate(int targetFps);

    std::deque<float> frameTimes;
    std::vector<float> sortedScratch;
    float accumulatedTime = 0.0f;
    float calculationTimer = 0.0f;
    FramePerformanceSnapshot snapshot;
};
