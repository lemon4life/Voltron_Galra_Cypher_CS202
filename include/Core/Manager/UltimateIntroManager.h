#pragma once
#include "raylib.h"
#include <string>

class Paladin; // Forward declaration

class UltimateIntroManager {
private:
    bool isPlaying;
    float timer;
    Paladin* activePaladin;
    
    UltimateIntroManager();
    ~UltimateIntroManager();

public:
    static UltimateIntroManager& GetInstance();
    
    // Delete copy and assignment
    UltimateIntroManager(const UltimateIntroManager&) = delete;
    UltimateIntroManager& operator=(const UltimateIntroManager&) = delete;
    UltimateIntroManager(UltimateIntroManager&&) = delete;
    UltimateIntroManager& operator=(UltimateIntroManager&&) = delete;
    
    void PlayIntro(Paladin* paladin);
    void Update(float deltaTime);
    void Draw();
    
    bool IsPlaying() const { return isPlaying; }
};
