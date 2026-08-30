#pragma once
#include "raylib.h"
#include <map>
#include <string>
#include <vector>

// Design Patterns - Singleton and Object Pool:
// GetInstance owns the audio device/resources. SoundVoicePool is a fixed set of
// reusable aliases; PlayPolyphonicSoundEffect selects an idle voice or advances
// cyclically, allowing overlap without loading/allocating a sound per play.
class AudioManager {
private:
    struct SoundVoicePool {
        std::vector<Sound> voices;
        std::size_t nextVoice = 0;
    };

    std::map<std::string, Sound> sounds;
    std::map<std::string, SoundVoicePool> soundVoicePools;
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

    /// Creates a AudioManager instance from the supplied configuration.
    AudioManager(); // Private constructor (Initializes Raylib Audio)
    /// Releases resources owned by this AudioManager instance.
    ~AudioManager(); // Private destructor (Closes Raylib Audio)

    /// Creates sound voice pool.
    void CreateSoundVoicePool(
        const std::string& name,
        std::size_t voiceCount
    );

public:
    /// Returns the process-wide singleton instance of this manager.
    static AudioManager& GetInstance();

    // Delete copy and assignment operators to enforce singleton behavior
    /// Creates a AudioManager instance from the supplied configuration.
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;
    /// Creates a AudioManager instance from the supplied configuration.
    AudioManager(AudioManager&&) = delete;
    AudioManager& operator=(AudioManager&&) = delete;

    /// Initializes the resources and collaborators required before this component can run.
    void Initialize();
    /// Releases resources owned by this component and leaves it safe to destroy.
    void Shutdown();
    /// Plays random laser.
    void PlayRandomLaser();
    /// Plays sequential footstep.
    void PlaySequentialFootstep();
    /// Plays random click.
    void PlayRandomClick();

    /// Plays random sword slash.
    void PlayRandomSwordSlash();
    /// Plays random laser gun.
    void PlayRandomLaserGun();

    // Volume Control
    /// Loads sound.
    void LoadSound(const std::string& name, const std::string& filepath);
    /// Plays sound effect.
    void PlaySoundEffect(const std::string& name);
    /// Plays polyphonic sound effect.
    void PlayPolyphonicSoundEffect(const std::string& name);
    /// Plays sound effect volume.
    void PlaySoundEffectVolume(const std::string& name, float volumeScale);
    /// Plays sound effect pitch.
    void PlaySoundEffectPitch(const std::string& name, float pitch);

    enum class MusicFadeState { NONE, FADING_OUT, FADING_IN };
    MusicFadeState currentFadeState = MusicFadeState::NONE;
    std::string currentMusicName = "";
    std::string nextMusicName = "";
    float fadeTimer = 0.0f;
    float fadeDuration = 1.0f;
    float currentTrackVolume = 1.0f;

    /// Loads music.
    void LoadMusic(const std::string& name, const std::string& filepath);
    /// Plays music track.
    void PlayMusicTrack(const std::string& name, float fadeTime = 1.0f);
    /// Updates music stream.
    void UpdateMusicStream();
    /// Returns the current current music name.
    const std::string& GetCurrentMusicName() const { return currentMusicName; }

    /// Updates the stored sound effects volume.
    void SetSoundEffectsVolume(float volume);
    /// Updates the stored music volume level.
    void SetMusicVolumeLevel(float volume);
    /// Returns the current sound effects volume.
    float GetSoundEffectsVolume() const;
    /// Returns the current music volume level.
    float GetMusicVolumeLevel() const;
    /// Returns the current sound count.
    std::size_t GetSoundCount() const;
    /// Returns the current music count.
    std::size_t GetMusicCount() const { return music.size(); }
};
