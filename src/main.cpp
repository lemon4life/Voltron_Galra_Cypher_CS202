#include "Core/GameApplication.h"

int main() {
    GameApplication app;
    app.Initialize();
    app.RunLoop();
    app.Shutdown();
    return 0;
}
