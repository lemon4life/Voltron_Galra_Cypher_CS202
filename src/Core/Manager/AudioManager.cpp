#include "Core/Manager/AudioManager.h"
#include <iostream>

AudioManager::AudioManager() {
    InitAudioDevice(); // Initialize Raylib Audio Device
}

AudioManager::~AudioManager() {
    for (auto& pair : sounds) {
        UnloadSound(pair.second);
    }
    sounds.clear();

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
