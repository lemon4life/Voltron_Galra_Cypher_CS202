#pragma once
#include "raylib.h"
#include <string>

class Paladin; // Forward declaration

// Design Pattern - Singleton:
// UltimateIntroManager coordinates the one global cinematic overlay and pending
// Paladin ultimate action; GetInstance prevents simultaneous intro controllers.
class UltimateIntroManager {
private:
    bool isPlaying;
    float timer;
    Paladin* activePaladin;
    
    /// Creates a UltimateIntroManager instance from the supplied configuration.
    UltimateIntroManager();
    /// Releases resources owned by this UltimateIntroManager instance.
    ~UltimateIntroManager();

public:
    /// Returns the process-wide singleton instance of this manager.
    static UltimateIntroManager& GetInstance();
    
    // Delete copy and assignment
    /// Creates a UltimateIntroManager instance from the supplied configuration.
    UltimateIntroManager(const UltimateIntroManager&) = delete;
    UltimateIntroManager& operator=(const UltimateIntroManager&) = delete;
    /// Creates a UltimateIntroManager instance from the supplied configuration.
    UltimateIntroManager(UltimateIntroManager&&) = delete;
    UltimateIntroManager& operator=(UltimateIntroManager&&) = delete;
    
    /// Plays intro.
    void PlayIntro(Paladin* paladin);
    /// Advances this component's state for the current frame.
    void Update(float deltaTime);
    /// Renders this component using its current state and visual resources.
    void Draw();
    
    /// Reports whether the playing condition is satisfied.
    bool IsPlaying() const { return isPlaying; }
};
