#include "Core/AudioManager.h"
#include <iostream>

AudioManager::AudioManager() {
    InitAudioDevice(); // Initialize Raylib Audio Device
}

AudioManager::~AudioManager() {
    for (auto& pair : sounds) {
        UnloadSound(pair.second);
    }
    sounds.clear();

    for (auto& s : laserSounds) UnloadSound(s);
    laserSounds.clear();

    for (auto& s : footstepSounds) UnloadSound(s);
    footstepSounds.clear();

    for (auto& s : clickSounds) UnloadSound(s);
    clickSounds.clear();

    for (auto& pair : music) {
        UnloadMusicStream(pair.second);
    }
    music.clear();

    CloseAudioDevice(); // Close Raylib Audio Device
}

AudioManager& AudioManager::GetInstance() {
    // Thread-safe in C++11+ (Meyers' Singleton)
    static AudioManager instance;
    return instance;
}

void AudioManager::Initialize() {
    laserSounds.clear();
    footstepSounds.clear();
    clickSounds.clear();

    for (int i = 0; i < 5; ++i) {
        std::string num = "00" + std::to_string(i);
        laserSounds.push_back(::LoadSound(("assets/audio/laserSmall_" + num + ".ogg").c_str()));
        footstepSounds.push_back(::LoadSound(("assets/audio/footstep_concrete_" + num + ".ogg").c_str()));
        clickSounds.push_back(::LoadSound(("assets/audio/click_" + num + ".ogg").c_str()));
    }
    currentFootstepIndex = 0;
}

void AudioManager::PlayRandomLaser() {
    if (laserSounds.empty()) return;
    int idx = GetRandomValue(0, laserSounds.size() - 1);
    float pitch = GetRandomValue(90, 110) / 100.0f;
    SetSoundPitch(laserSounds[idx], pitch);
    ::PlaySound(laserSounds[idx]);
}

void AudioManager::PlaySequentialFootstep() {
    if (footstepSounds.empty()) return;
    float pitch = GetRandomValue(95, 105) / 100.0f;
    SetSoundPitch(footstepSounds[currentFootstepIndex], pitch);
    ::PlaySound(footstepSounds[currentFootstepIndex]);
    
    currentFootstepIndex = (currentFootstepIndex + 1) % footstepSounds.size();
}

void AudioManager::PlayRandomClick() {
    if (clickSounds.empty()) return;
    int idx = GetRandomValue(0, clickSounds.size() - 1);
    float pitch = GetRandomValue(90, 110) / 100.0f;
    SetSoundPitch(clickSounds[idx], pitch);
    ::PlaySound(clickSounds[idx]);
}

void AudioManager::LoadSound(const std::string& name, const std::string& filepath) {
    if (sounds.find(name) == sounds.end()) {
        sounds[name] = ::LoadSound(filepath.c_str());
    }
}

void AudioManager::PlaySoundEffect(const std::string& name) {
    if (sounds.find(name) != sounds.end()) {
        SetSoundPitch(sounds[name], 1.0f); // Reset pitch in case it was altered
        ::PlaySound(sounds[name]);
    }
}

void AudioManager::PlaySoundEffectPitch(const std::string& name, float pitch) {
    if (sounds.find(name) != sounds.end()) {
        SetSoundPitch(sounds[name], pitch);
        ::PlaySound(sounds[name]);
    }
}

void AudioManager::LoadMusic(const std::string& name, const std::string& filepath) {
    if (music.find(name) == music.end()) {
        music[name] = LoadMusicStream(filepath.c_str());
    }
}

void AudioManager::PlayMusicTrack(const std::string& name) {
    if (music.find(name) != music.end()) {
        PlayMusicStream(music[name]);
    }
}

void AudioManager::UpdateMusicStream() {
    for (auto& pair : music) {
        if (IsMusicStreamPlaying(pair.second)) {
            ::UpdateMusicStream(pair.second);
        }
    }
}
