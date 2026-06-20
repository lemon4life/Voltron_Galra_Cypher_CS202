#pragma once
#include <string>
#include <vector>
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
    ~DialogueManager() = default;

    std::vector<DialogueNode> currentTree;
    bool isDialogueActive;
    int currentNode;
    int selectedOption;
    bool missionRequested;

public:
    static DialogueManager& GetInstance();
    
    // Delete copy and assignment operators
    DialogueManager(const DialogueManager&) = delete;
    DialogueManager& operator=(const DialogueManager&) = delete;

    void StartDialogue(const std::vector<DialogueNode>& tree);
    void Update(float deltaTime);
    void Draw();
    
    bool IsActive() const { return isDialogueActive; }
    bool IsMissionRequested() const { return missionRequested; }
    void ClearMissionRequest() { missionRequested = false; }
};
