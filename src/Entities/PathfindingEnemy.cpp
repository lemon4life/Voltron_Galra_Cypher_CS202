#include "Entities/PathfindingEnemy.h"

PathfindingEnemy::PathfindingEnemy(
    Vector2 position,
    TeamManager* targetTeam,
    int maxHealth,
    float speed,
    int damage,
    float attackCooldown,
    IEntityRemovalAccess* removalAccess,
    IEnemyPathAccess* pathAccess
)
    : Enemy(
          position,
          targetTeam,
          maxHealth,
          speed,
          damage,
          attackCooldown,
          removalAccess
      ),
      pathAccess(pathAccess) {}

PathfindingEnemy::~PathfindingEnemy() {
    EndPathFinding();
}

void PathfindingEnemy::StartPathFinding() {
    if (usePathFinding) return;

    usePathFinding = true;
    if (pathAccess) {
        pathAccess->BeginPathFinding(this);
    }
}

void PathfindingEnemy::EndPathFinding() {
    if (!usePathFinding) return;

    usePathFinding = false;
    ClearTargetPosition();
    if (pathAccess) {
        pathAccess->EndPathFinding(this);
    }
}
