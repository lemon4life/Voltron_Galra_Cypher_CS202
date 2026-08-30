#pragma once
#include <string>
#include <vector>
#include <map>
#include "raylib.h"

struct DialogueNode {
    std::string speakerName;
    std::string text;
    std::vector<std::string> options;
    std::vector<int> nextNodeIndices;
};

// Design Pattern - Singleton:
// DialogueManager owns the one active dialogue graph/session. GetInstance gives
// UI and game states coordinated access while copying is disabled.
class DialogueManager {
private:
    /// Creates a DialogueManager instance from the supplied configuration.
    DialogueManager();
    /// Releases resources owned by this DialogueManager instance.
    ~DialogueManager();

    std::vector<DialogueNode> currentTree;
    DialogueNode transientResponse;
    bool showingTransientResponse = false;
    bool isDialogueActive;
    int currentNode;
    int selectedOption;
    bool missionRequested;
    int requestedMissionId;

    // Typewriter effect
    float typewriterTimer;
    int visibleCharCount;

    // Visual Assets
    std::map<std::string, Texture2D> portraits;

public:
    /// Returns the process-wide singleton instance of this manager.
    static DialogueManager& GetInstance();

    // Delete copy and assignment operators
    /// Creates a DialogueManager instance from the supplied configuration.
    DialogueManager(const DialogueManager&) = delete;
    DialogueManager& operator=(const DialogueManager&) = delete;

    /// Initializes assets.
    void InitializeAssets();
    /// Loads dialogue tree.
    void LoadDialogueTree(const std::string& filepath);

    /// Starts dialogue.
    void StartDialogue();
    /// Resets session.
    void ResetSession();
    /// Advances this component's state for the current frame.
    void Update(float deltaTime);
    /// Renders this component using its current state and visual resources.
    void Draw(int screenWidth, int screenHeight);

    /// Reports whether the active condition is satisfied.
    bool IsActive() const { return isDialogueActive; }
    /// Reports whether the mission requested condition is satisfied.
    bool IsMissionRequested() const { return missionRequested; }
    /// Returns the current requested mission id.
    int GetRequestedMissionId() const { return requestedMissionId; }
    /// Clears mission request.
    void ClearMissionRequest() { missionRequested = false; }
    /// Returns the current dialogue node count.
    std::size_t GetDialogueNodeCount() const { return currentTree.size(); }
    /// Reports whether this component has transient response.
    bool HasTransientResponse() const { return showingTransientResponse; }
};
