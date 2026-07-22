# Repository Guidelines

## Project Structure & Module Organization
This is a C++17 raylib game built with CMake. Runtime code lives in `src/`, with matching public headers in `include/`. Major areas include `src/AI`, `src/Core`, `src/Core/Manager`, `src/Entities`, `src/Combat`, and `src/UI`. Game assets are stored under `assets/`, with sprite sheets also present in `sprites/`. Design notes and planning documents live in `tmp/implementation_plan/`. Build output belongs in `build/` and should not be edited manually.

## Build, Test, and Development Commands
Configure the project:
`cmake -S . -B build`

Build the executable:
`cmake --build build --config Release`

Run locally from the repository root or build folder:
`build/VoltronMissionGalraCypher.exe`

CMake downloads raylib through `FetchContent`, compiles all `src/*.cpp` files, and copies `assets/` into the build directory.

## Coding Style & Naming Conventions
Use C++17 and follow the existing style. Classes and major types use PascalCase, such as `GameManager`, `LevelManager`, and `EnemyChaser`. Functions generally use PascalCase for public methods, such as `UpdateLevel()` and `GetInstance()`. Local variables use camelCase. Use 4-space indentation, keep includes grouped by project headers then standard headers, and avoid unrelated refactors in feature commits.

## Testing Guidelines
There is no formal unit test framework currently configured. Treat a clean CMake build as the minimum validation. For gameplay changes, run the game and smoke-test the affected flow, such as player movement, enemy spawning, pathfinding, combat, map loading, and menu transitions. Add focused tests or debug harnesses only when they can be built repeatably.

## Commit & Pull Request Guidelines
Recent history uses short imperative messages with optional prefixes, for example `feat:`, `fix:`, `chore:`, `config:`, and `clean:`. Keep commits focused on one change. Pull requests should summarize the gameplay/code change, mention affected systems, list manual test steps, and include screenshots or recordings for visible gameplay or UI changes.


## Implementation Plans & AI Usage Notes
Whenever an implementation plan is created, you MUST automatically copy it to the project's physical `tmp/implementation_plan/` directory in numbered order (e.g., `plan_9_...`), in addition to creating the standard artifact. Do not modify old plans. Additionally, automatically update the AI Usage Note for the corresponding week in the `AI Usage Note/` folder with the prompts used, purpose, and content generated for that feature. Each week X is followed by a file declaration named `AI Usage Notes - Week X - Phuc Khanh`

Note that week 7 starts from 20 July, 2026


## Fallback
If any of the build run failed, stop and return the answer, do need to force build success