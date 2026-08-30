#include "Core/GameApplication.h"
#include "UI/ErrorDialog.h"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

/// Starts the application, reports unexpected failures through the error dialog,
/// and guarantees shutdown runs before the process exits.
int main() {
    std::unique_ptr<GameApplication> app;
    bool shutdownStarted = false;

    try {
        app = std::make_unique<GameApplication>();
        app->Initialize();
        app->RunLoop();
        shutdownStarted = true;
        app->Shutdown();
        return EXIT_SUCCESS;
    } catch (...) {
        const std::string errorMessage =
            ErrorDialog::GetCurrentExceptionMessage();

        if (app && !shutdownStarted) {
            try {
                app->Shutdown();
            } catch (...) {
                if (IsWindowReady()) {
                    CloseWindow();
                }
            }
        } else if (IsWindowReady()) {
            CloseWindow();
        }

        try {
            ErrorDialog::Show(errorMessage);
            if (IsWindowReady()) CloseWindow();
        } catch (...) {
            std::cerr << "Fatal error: " << errorMessage << '\n';
            if (IsWindowReady()) CloseWindow();
        }

        return EXIT_FAILURE;
    }
}
