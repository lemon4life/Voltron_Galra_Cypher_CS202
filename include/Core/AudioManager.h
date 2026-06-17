#pragma once
#include "raylib.h"

class AudioManager {
private:
    AudioManager(); // Private constructor (Initializes Raylib Audio)
    ~AudioManager(); // Private destructor (Closes Raylib Audio)

public:
    static AudioManager& GetInstance();

    // Delete copy and assignment operators to enforce singleton behavior
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;
    AudioManager(AudioManager&&) = delete;
    AudioManager& operator=(AudioManager&&) = delete;
};
