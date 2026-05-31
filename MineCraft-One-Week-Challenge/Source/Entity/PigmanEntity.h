// ===================================================================
// PigmanEntity = 猪人 = Zoglin (Minecraft zombified piglin)
// 模型: Res/Models/Zoglin/ (CC-BY-4.0 by trmhtk2 on Sketchfab)
// ===================================================================

#ifndef PIGMANENTITY_H_INCLUDED
#define PIGMANENTITY_H_INCLUDED

#include "../Maths/glm.h"
#include "../Physics/AABB.h"
#include <vector>

class Model;

struct PigmanEntity {
    glm::vec3 position{0, 0, 0};
    glm::vec3 velocity{0, 0, 0};
    glm::vec3 rotation{0, 0, 0};
    AABB box{glm::vec3(0.4f, 1.0f, 0.4f)};

    int hp = 30;
    int maxHp = 30;
    float moveSpeed = 4.0f;

    enum State { Patrol, Chase, Attack, Hurt, Dead };
    State state = Patrol;
    float stateTimer = 0.0f;
    float attackCooldown = 1.5f;
    float hurtTimer = 0.0f;
    float stuckTimer = 0.0f;
    glm::vec3 stuckPosition{0, 0, 0};
    float respawnTimer = -1.0f;

    glm::vec3 patrolOrigin{0, 0, 0};
    glm::vec3 patrolTarget{0, 0, 0};
    std::vector<glm::ivec3> path;
    int pathIndex = 0;

    float deathAnimTimer = 0.0f;
};

#endif
