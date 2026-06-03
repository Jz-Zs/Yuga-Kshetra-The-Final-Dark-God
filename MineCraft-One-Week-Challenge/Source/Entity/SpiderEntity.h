#ifndef SPIDERENTITY_H_INCLUDED
#define SPIDERENTITY_H_INCLUDED

#include "../Maths/glm.h"
#include "../Physics/AABB.h"
#include <vector>

struct SpiderEntity {
    glm::vec3 position{0, 0, 0};
    glm::vec3 velocity{0, 0, 0};
    glm::vec3 rotation{0, 0, 0};
    AABB box{glm::vec3(0.7f, 0.65f, 0.7f)}; // Spider, raised Y for raycast hit

    int hp = 80;
    int maxHp = 80;
    float moveSpeed = 6.0f;

    enum State { Patrol, Chase, MeleeAttack, Hurt, Dead };
    State state = Patrol;
    float stateTimer = 0.0f;
    float meleeCooldown = 1.5f;
    float rangedCooldown = 2.5f;
    float freezeTimer = 0.0f;      // Post-shot freeze (0.2s)
    float hurtTimer = 0.0f;
    float stuckTimer = 0.0f;
    float aggroTimer = 0.0f;  // forced chase on arrow hit
    glm::vec3 stuckPosition{0, 0, 0};
    float respawnTimer = -1.0f;
    float m_slowTimer = 0.0f;     // slow debuff from silk arrow
    float m_slowFactor = 1.0f;    // 1.0=normal, 0.8=slowed

    glm::vec3 patrolOrigin{0, 0, 0};
    std::vector<glm::ivec3> path;
    int pathIndex = 0;

    float deathAnimTimer = 0.0f;
};

#endif
