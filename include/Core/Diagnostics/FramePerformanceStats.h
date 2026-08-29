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

class FramePerformanceStats {
public:
    static FramePerformanceStats& GetInstance();

    void Update(float deltaTime, int targetFps);
    void Reset();
    const FramePerformanceSnapshot& GetSnapshot() const { return snapshot; }

private:
    void Recalculate(int targetFps);

    std::deque<float> frameTimes;
    std::vector<float> sortedScratch;
    float accumulatedTime = 0.0f;
    float calculationTimer = 0.0f;
    FramePerformanceSnapshot snapshot;
};
