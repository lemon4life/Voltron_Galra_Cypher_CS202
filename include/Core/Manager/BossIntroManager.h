#pragma once
#include "raylib.h"
#include <string>

class Boss; // Forward declaration

// Design Pattern - Singleton:
// BossIntroManager coordinates the cinematic boss entrance banner overlay
// with the boss name on the left and boss illustration on the right.
class BossIntroManager {
private:
    bool isPlaying;
    float timer;
    Boss* activeBoss;
    std::string bossName;
    std::string bossTitle;
    Color bannerColor;
    
    BossIntroManager();
    ~BossIntroManager();

public:
    static BossIntroManager& GetInstance();
    
    // Delete copy and assignment
    BossIntroManager(const BossIntroManager&) = delete;
    BossIntroManager& operator=(const BossIntroManager&) = delete;
    BossIntroManager(BossIntroManager&&) = delete;
    BossIntroManager& operator=(BossIntroManager&&) = delete;
    
    /// Starts the cinematic boss intro.
    void PlayIntro(Boss* boss, const std::string& name = "COMMANDER PROROK", const std::string& title = "GALRA EMPIRE CYBERNETIC WARLORD");
    
    /// Advances intro timer and animation states.
    void Update(float deltaTime);
    
    /// Renders the cinematic intro overlay.
    void Draw();
    
    /// Reports whether the boss intro is currently playing.
    bool IsPlaying() const { return isPlaying; }
};
