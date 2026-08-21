#pragma once
#include "raylib.h"
#include <map>
#include <string>
#include <vector>

class AudioManager {
private:
    std::map<std::string, Sound> sounds;
    std::map<std::string, Music> music;

    std::vector<Sound> laserSounds;
    std::vector<Sound> laserGunSounds;
    std::vector<Sound> footstepSounds;
    std::vector<Sound> clickSounds;
    std::vector<Sound> swordSlashSounds;
    bool initialized = false;

    int currentFootstepIndex = 0;
    float soundEffectsVolume = 0.4f;
    float musicVolume = 0.05f;

    AudioManager(); // Private constructor (Initializes Raylib Audio)
    ~AudioManager(); // Private destructor (Closes Raylib Audio)

public:
    static AudioManager& GetInstance();

    // Delete copy and assignment operators to enforce singleton behavior
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;
    AudioManager(AudioManager&&) = delete;
    AudioManager& operator=(AudioManager&&) = delete;

    void Initialize();
    void Shutdown();
    void PlayRandomLaser();
    void PlaySequentialFootstep();
    void PlayRandomClick();

    void PlayRandomSwordSlash();
    void PlayRandomLaserGun();

    // Volume Control
    void LoadSound(const std::string& name, const std::string& filepath);
    void PlaySoundEffect(const std::string& name);
    void PlaySoundEffectVolume(const std::string& name, float volumeScale);
    void PlaySoundEffectPitch(const std::string& name, float pitch);

    enum class MusicFadeState { NONE, FADING_OUT, FADING_IN };
    MusicFadeState currentFadeState = MusicFadeState::NONE;
    std::string currentMusicName = "";
    std::string nextMusicName = "";
    float fadeTimer = 0.0f;
    float fadeDuration = 1.0f;
    float currentTrackVolume = 1.0f;

    void LoadMusic(const std::string& name, const std::string& filepath);
    void PlayMusicTrack(const std::string& name, float fadeTime = 1.0f);
    void UpdateMusicStream();
    const std::string& GetCurrentMusicName() const { return currentMusicName; }

    void SetSoundEffectsVolume(float volume);
    void SetMusicVolumeLevel(float volume);
    float GetSoundEffectsVolume() const;
    float GetMusicVolumeLevel() const;
    std::size_t GetSoundCount() const;
    std::size_t GetMusicCount() const { return music.size(); }
};
