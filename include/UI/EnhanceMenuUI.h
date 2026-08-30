#pragma once

#include "Entities/Player/PaladinDefinition.h"
#include "raylib.h"
#include <cstddef>
#include <string>

class TeamManager;
class Paladin;

class EnhanceMenuUI {
private:
    bool open = false;
    PaladinId inspectedPaladin = PaladinId::Lance;
    std::string feedbackText;
    float feedbackTimer = 0.0f;

    /// Updates the stored feedback.
    void SetFeedback(const std::string& text);

public:
    /// Creates a EnhanceMenuUI instance from the supplied configuration.
    EnhanceMenuUI();
    /// Releases resources owned by this EnhanceMenuUI instance.
    ~EnhanceMenuUI() = default;

    /// Opens this menu and prepares its current selection.
    void Open(PaladinId paladinId);
    /// Closes this menu and clears transient interaction state.
    void Close();
    /// Reports whether the open condition is satisfied.
    bool IsOpen() const { return open; }

    /// Advances this component's state for the current frame.
    void Update(
        float deltaTime,
        Vector2 mousePosition,
        TeamManager& teamManager
    );
    /// Renders this component using its current state and visual resources.
    void Draw(Vector2 mousePosition, const TeamManager& teamManager) const;
};
