#include "Core/AudioManager.h"

AudioManager::AudioManager() {
    InitAudioDevice(); // Initialize Raylib Audio Device
}

AudioManager::~AudioManager() {
    CloseAudioDevice(); // Close Raylib Audio Device
}

AudioManager& AudioManager::GetInstance() {
    // Thread-safe in C++11+ (Meyers' Singleton)
    static AudioManager instance;
    return instance;
}
