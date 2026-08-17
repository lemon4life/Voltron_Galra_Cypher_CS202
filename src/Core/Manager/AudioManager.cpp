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
    laserGunSounds.clear();
    footstepSounds.clear();
    clickSounds.clear();
    swordSlashSounds.clear();

    for (int i = 0; i < 5; ++i) {
        std::string num_str = std::to_string(i);
        laserGunSounds.push_back(::LoadSound(
            ("assets/audio/SFX/Combat/fx_laser_gun_" + num_str + ".ogg").c_str()
        ));
        std::string num = "00" + std::to_string(i);
        footstepSounds.push_back(::LoadSound(
            ("assets/audio/SFX/Environment/footstep_concrete_" + num + ".ogg").c_str()
        ));
        clickSounds.push_back(::LoadSound(
            ("assets/audio/SFX/UI/click_" + num + ".ogg").c_str()
        ));
    }
    
    for (int i = 0; i < 2; ++i) {
        std::string num = std::to_string(i);
        swordSlashSounds.push_back(::LoadSound(
            ("assets/audio/SFX/Combat/fx_sword_slash_" + num + ".wav").c_str()
        ));
    }

    for (int i = 0; i < 6; ++i) {
        std::string num = std::to_string(i);
        laserSounds.push_back(::LoadSound(
            ("assets/audio/SFX/Combat/fx_laser_small_" + num + ".wav").c_str()
        ));
    }

    // Load BGM Tracks
    LoadMusic("bgm_starter_menu", "assets/audio/BGM/bgm_starter_menu.mp3");
    LoadMusic("bgm_story_mode", "assets/audio/BGM/bgm_story_mode.mp3");
    LoadMusic("bgm_battle", "assets/audio/BGM/bgm_battle.mp3");

    // Load Enemy SFX correctly
    LoadSound("knight_dead_0", "assets/audio/SFX/Enemy/knight_dead_0.wav");
    LoadSound("knight_dead_1", "assets/audio/SFX/Enemy/knight_dead_1.wav");
    LoadSound("drone_dead_0", "assets/audio/SFX/Enemy/drone_dead_0.wav");
    LoadSound("drone_dead_1", "assets/audio/SFX/Enemy/drone_dead_1.wav");

    // SFX - Combat
    LoadSound("fx_laser_bullet", "assets/audio/SFX/Combat/fx_laser_bullet.wav");
    LoadSound("fx_explode_big", "assets/audio/SFX/Combat/fx_explode_big.wav");
    LoadSound("fx_explode_small", "assets/audio/SFX/Combat/fx_explode_small.wav");
    LoadSound("fx_shield_hit", "assets/audio/SFX/Combat/fx_shield_hit.wav");
    // SFX - Environment & Items
    LoadSound("fx_energy", "assets/audio/SFX/Item/fx_energy.wav");
    LoadSound("fx_coin", "assets/audio/SFX/Item/fx_coin.wav");
    LoadSound("fx_pickup", "assets/audio/SFX/Item/fx_pickup.wav");
    LoadSound("fx_box_destroy", "assets/audio/SFX/Environment/fx_box_destroy.wav");
    LoadSound("fx_chest_open", "assets/audio/SFX/Environment/fx_chest_open.wav");
    LoadSound("fx_cage_open", "assets/audio/SFX/Environment/fx_cage_open.wav");
    LoadSound("fx_doorgate", "assets/audio/SFX/Environment/fx_doorgate.wav");
    
    // SFX - Character Skills & Ults
    LoadSound("fx_lance_skill", "assets/audio/SFX/Character/fx_lance_skill.wav");
    LoadSound("fx_lance_ult", "assets/audio/SFX/Character/fx_lance_ult.wav");
    LoadSound("fx_pidge_ult", "assets/audio/SFX/Character/fx_pidge_ult.wav");
    LoadSound("fx_hunk_skill", "assets/audio/SFX/Character/fx_hunk_skill.wav");
    LoadSound("fx_keith_ult", "assets/audio/SFX/Character/fx_keith_ult.wav");
    LoadSound("fx_switch_character", "assets/audio/SFX/UI/fx_switch_character.wav");
    LoadSound("fx_get_buff", "assets/audio/SFX/Character/fx_get_buff.wav");
    LoadSound("fx_ice_explode", "assets/audio/SFX/Character/fx_ice_explode.wav");
    LoadSound("fx_ice_hit", "assets/audio/SFX/Character/fx_ice_hit.wav");
    LoadSound("fx_fire", "assets/audio/SFX/Character/fx_fire.wav");
    LoadSound("fx_flash_lighting", "assets/audio/SFX/Character/fx_flash_lighting.wav");

    LoadMusic("bgm_boss_theme", "assets/audio/BGM/bgm_boss_theme.mp3");

    // Load Voicelines
    LoadSound("vl_lance_skill", "assets/audio/Voice/lance_skill.wav");
    LoadSound("vl_lance_ult", "assets/audio/Voice/lance_ult.wav");
    LoadSound("vl_pidge_ult", "assets/audio/Voice/pidge_ult.wav");
    LoadSound("vl_keith_ult", "assets/audio/Voice/keith_ult.wav");
    LoadSound("vl_hunk_ult", "assets/audio/Voice/hunk_ult.wav");

    currentFootstepIndex = 0;
    SetSoundEffectsVolume(soundEffectsVolume);
    SetMusicVolumeLevel(musicVolume);
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
    SetSoundVolume(clickSounds[idx], soundEffectsVolume);
    ::PlaySound(clickSounds[idx]);
}

void AudioManager::PlayRandomSwordSlash() {
    if (swordSlashSounds.empty()) return;
    int index = GetRandomValue(0, swordSlashSounds.size() - 1);
    float pitch = GetRandomValue(90, 110) / 100.0f;
    SetSoundPitch(swordSlashSounds[index], pitch);
    SetSoundVolume(swordSlashSounds[index], soundEffectsVolume);
    ::PlaySound(swordSlashSounds[index]);
}

void AudioManager::PlayRandomLaser() {
    if (laserSounds.empty()) return;
    int index = GetRandomValue(0, laserSounds.size() - 1);
    float pitch = GetRandomValue(90, 110) / 100.0f;
    SetSoundPitch(laserSounds[index], pitch);
    SetSoundVolume(laserSounds[index], soundEffectsVolume);
    ::PlaySound(laserSounds[index]);
}

void AudioManager::PlayRandomLaserGun() {
    if (laserGunSounds.empty()) return;
    int index = GetRandomValue(0, laserGunSounds.size() - 1);
    float pitch = GetRandomValue(90, 110) / 100.0f;
    SetSoundPitch(laserGunSounds[index], pitch);
    SetSoundVolume(laserGunSounds[index], soundEffectsVolume);
    ::PlaySound(laserGunSounds[index]);
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

void AudioManager::PlaySoundEffectVolume(const std::string& name, float volumeScale) {
    if (sounds.find(name) != sounds.end()) {
        SetSoundPitch(sounds[name], 1.0f);
        ::SetSoundVolume(sounds[name], soundEffectsVolume * volumeScale);
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
