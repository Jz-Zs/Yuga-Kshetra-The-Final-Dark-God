#ifndef SPIDERPROJECTILE_H_INCLUDED
#define SPIDERPROJECTILE_H_INCLUDED

#include "../Maths/glm.h"

struct SpiderProjectile {
    glm::vec3 position;
    glm::vec3 velocity;  // horizontal only, speed 4.0
    int damage = 5;
    float lifetime = 0.0f;
    float maxLifetime = 2.0f;
    bool alive = true;
};

#endif
