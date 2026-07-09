# Base GameObject Architecture

## Goal
Establish the foundational Object-Oriented structure for all entities in the game, providing a consistent interface for position tracking, collision, and the game loop.

## Proposed Changes

### 1. GameObject Interface
**[NEW] include/Entities/GameObject.h & src/Entities/GameObject.cpp**
- Create an abstract base class `GameObject`.
- **Protected Members**: 
  - `Vector2 position`
- **Public Methods**:
  - `virtual void Update(float deltaTime) = 0;` (Pure virtual)
  - `virtual void Draw() = 0;` (Pure virtual)
  - `virtual Rectangle GetBoundingBox() const = 0;` (Pure virtual)
  - `Vector2 GetPosition() const` and `void SetPosition(Vector2)`

### 2. Character Base Class
**[NEW] include/Entities/Character.h & src/Entities/Character.cpp**
- Create `Character` inheriting from `GameObject`.
- **Protected Members**:
  - `int health`
  - `int maxHealth`
  - `float speed`
  - `int damage`
- This class will serve as the parent for both the `Player` and `Enemy` entities, standardizing health and speed across all moving characters.

## Verification Plan
### Automated Tests
- No automated tests required for pure virtual interfaces.

### Manual Verification
- Instantiate a temporary concrete class inheriting from `Character`.
- Verify that calling `GetPosition()` and modifying `speed` works correctly.
- Ensure the game compiles successfully with the new abstract structures.
