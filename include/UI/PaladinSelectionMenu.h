#pragma once

#include "Entities/Player/PaladinDefinition.h"
#include "UI/AbilityDemoModal.h"
#include "raylib.h"

#include <cstddef>
#include <string>

class TeamManager;

class PaladinSelectionMenu {
private:
    bool open;
    PaladinId inspectedPaladin;
    std::string feedbackText;
    float feedbackTimer;
    AbilityDemoModal demoModal;

    /// Updates the stored feedback.
    void SetFeedback(const std::string& text);

public:
    /// Creates a PaladinSelectionMenu instance from the supplied configuration.
    PaladinSelectionMenu();

    /// Opens this menu and prepares its current selection.
    void Open(PaladinId paladinId);
    /// Closes this menu and clears transient interaction state.
    void Close();
    /// Reports whether the open condition is satisfied.
    bool IsOpen() const { return open; }
    /// Reports whether the ability demo modal is open.
    bool IsDemoOpen() const { return demoModal.IsOpen(); }

    /// Advances this component's state for the current frame.
    void Update(
        float deltaTime,
        Vector2 mousePosition,
        TeamManager& teamManager
    );
    /// Renders this component using its current state and visual resources.
    void Draw(Vector2 mousePosition, const TeamManager& teamManager) const;
};
