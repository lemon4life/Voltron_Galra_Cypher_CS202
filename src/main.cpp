#include "Core/GameApplication.h"
#include "UI/ErrorDialog.h"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

int main() {
    std::unique_ptr<GameApplication> app;
    bool initialized = false;
    bool shutdownStarted = false;

    try {
        app = std::make_unique<GameApplication>();
        app->Initialize();
        initialized = true;
        app->RunLoop();
        shutdownStarted = true;
        app->Shutdown();
        return EXIT_SUCCESS;
    } catch (...) {
        const std::string errorMessage =
            ErrorDialog::GetCurrentExceptionMessage();

        try {
            ErrorDialog::Show(errorMessage);
        } catch (...) {
            std::cerr << "Fatal error: " << errorMessage << '\n';
        }

        if (app && initialized && !shutdownStarted) {
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

        return EXIT_FAILURE;
    }
}
