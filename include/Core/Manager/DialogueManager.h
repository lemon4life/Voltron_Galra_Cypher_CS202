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

class DialogueManager {
private:
    DialogueManager();
    ~DialogueManager();

    std::vector<DialogueNode> currentTree;
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
    static DialogueManager& GetInstance();

    // Delete copy and assignment operators
    DialogueManager(const DialogueManager&) = delete;
    DialogueManager& operator=(const DialogueManager&) = delete;

    void InitializeAssets();
    void LoadDialogueTree(const std::string& filepath);

    void StartDialogue();
    void ResetSession();
    void Update(float deltaTime);
    void Draw(int screenWidth, int screenHeight);

    bool IsActive() const { return isDialogueActive; }
    bool IsMissionRequested() const { return missionRequested; }
    int GetRequestedMissionId() const { return requestedMissionId; }
    void ClearMissionRequest() { missionRequested = false; }
};
