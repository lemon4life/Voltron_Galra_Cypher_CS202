#pragma once
#include "raylib.h"
#include <map>
#include <string>

class AudioManager {
private:
    std::map<std::string, Sound> sounds;
    std::map<std::string, Music> music;

    AudioManager(); // Private constructor (Initializes Raylib Audio)
    ~AudioManager(); // Private destructor (Closes Raylib Audio)

public:
    static AudioManager& GetInstance();

    // Delete copy and assignment operators to enforce singleton behavior
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;
    AudioManager(AudioManager&&) = delete;
    AudioManager& operator=(AudioManager&&) = delete;

    void LoadSound(const std::string& name, const std::string& filepath);
    void PlaySoundEffect(const std::string& name);
    void PlaySoundEffectPitch(const std::string& name, float pitch);
    
    void LoadMusic(const std::string& name, const std::string& filepath);
    void PlayMusicTrack(const std::string& name);
    void UpdateMusicStream();
};
