#include "Core/Manager/AudioManager.h"

#include <algorithm>
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
        laserSounds.push_back(::LoadSound(
            ("assets/audio/Weapon/Firearm/laserSmall_" + num + ".ogg").c_str()
        ));
        footstepSounds.push_back(::LoadSound(
            ("assets/audio/Impact/Footstep/footstep_concrete_" + num + ".ogg").c_str()
        ));
        clickSounds.push_back(::LoadSound(
            ("assets/audio/UI/Button/click_" + num + ".ogg").c_str()
        ));
    }

    // Load BGM Tracks
    LoadMusic("bgm_starter_menu", "assets/audio/BGM/bgm_starter_menu.mp3");
    LoadMusic("bgm_story_mode", "assets/audio/BGM/bgm_story_mode.mp3");
    LoadMusic("bgm_battle", "assets/audio/BGM/bgm_battle.mp3");
    LoadMusic("bgm_boss_theme", "assets/audio/BGM/bgm_boss_theme.mp3");

    currentFootstepIndex = 0;
    SetSoundEffectsVolume(soundEffectsVolume);
    SetMusicVolumeLevel(musicVolume);
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
        ::SetSoundVolume(sounds[name], soundEffectsVolume);
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
        ::SetMusicVolume(music[name], musicVolume);
    }
}

void AudioManager::PlayMusicTrack(const std::string& name, float fadeTime) {
    if (music.find(name) == music.end()) return; // Track not found
    if (name == currentMusicName && currentFadeState != MusicFadeState::FADING_OUT) return; // Already playing

    if (currentMusicName.empty()) {
        // No music currently playing, start immediately
        currentMusicName = name;
        currentTrackVolume = 1.0f;
        currentFadeState = MusicFadeState::NONE;
        ::SetMusicVolume(music[currentMusicName], currentTrackVolume * musicVolume);
        ::PlayMusicStream(music[currentMusicName]);
    } else {
        // Fade out current track, queue next
        nextMusicName = name;
        fadeDuration = (fadeTime > 0.0f) ? fadeTime : 0.001f;
        fadeTimer = fadeDuration;
        currentFadeState = MusicFadeState::FADING_OUT;
    }
}

void AudioManager::UpdateMusicStream() {
    float dt = GetFrameTime();

    if (currentFadeState == MusicFadeState::FADING_OUT) {
        fadeTimer -= dt;
        if (fadeTimer <= 0.0f) {
            // Fade out complete, switch tracks
            if (!currentMusicName.empty()) {
                ::StopMusicStream(music[currentMusicName]);
            }
            currentMusicName = nextMusicName;
            nextMusicName = "";
            currentTrackVolume = 0.0f;
            currentFadeState = MusicFadeState::FADING_IN;
            fadeTimer = fadeDuration;
            
            if (!currentMusicName.empty()) {
                ::SetMusicVolume(music[currentMusicName], 0.0f);
                ::PlayMusicStream(music[currentMusicName]);
            }
        } else {
            currentTrackVolume = fadeTimer / fadeDuration;
            if (!currentMusicName.empty()) {
                ::SetMusicVolume(music[currentMusicName], currentTrackVolume * musicVolume);
            }
        }
    } else if (currentFadeState == MusicFadeState::FADING_IN) {
        fadeTimer -= dt;
        if (fadeTimer <= 0.0f) {
            // Fade in complete
            currentTrackVolume = 1.0f;
            currentFadeState = MusicFadeState::NONE;
        } else {
            currentTrackVolume = 1.0f - (fadeTimer / fadeDuration);
        }
        if (!currentMusicName.empty()) {
            ::SetMusicVolume(music[currentMusicName], currentTrackVolume * musicVolume);
        }
    }

    // Always update playing streams
    for (auto& pair : music) {
        if (IsMusicStreamPlaying(pair.second)) {
            ::UpdateMusicStream(pair.second);
        }
    }
}

void AudioManager::SetSoundEffectsVolume(float volume) {
    soundEffectsVolume = std::clamp(volume, 0.0f, 1.0f);

    for (auto& pair : sounds) {
        ::SetSoundVolume(pair.second, soundEffectsVolume);
    }
    for (Sound& sound : laserSounds) {
        ::SetSoundVolume(sound, soundEffectsVolume);
    }
    for (Sound& sound : footstepSounds) {
        ::SetSoundVolume(sound, soundEffectsVolume);
    }
    for (Sound& sound : clickSounds) {
        ::SetSoundVolume(sound, soundEffectsVolume);
    }
}

void AudioManager::SetMusicVolumeLevel(float volume) {
    musicVolume = std::clamp(volume, 0.0f, 1.0f);

    // Only update the currently active track's volume according to its fade state.
    // Inactive tracks remain at 0 volume so they don't unexpectedly play.
    if (!currentMusicName.empty() && music.find(currentMusicName) != music.end()) {
        ::SetMusicVolume(music[currentMusicName], currentTrackVolume * musicVolume);
    }
}

float AudioManager::GetSoundEffectsVolume() const {
    return soundEffectsVolume;
}

float AudioManager::GetMusicVolumeLevel() const {
    return musicVolume;
}
