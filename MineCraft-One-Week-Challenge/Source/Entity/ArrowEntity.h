#ifndef ARROWENTITY_H_INCLUDED
#define ARROWENTITY_H_INCLUDED

#include "../Maths/glm.h"
#include "../Item/Material.h"

struct ArrowEntity {
    glm::vec3 position;
    glm::vec3 velocity{0, 0, 0}; // horizontal, speed 10.0
    int damage = 10;
    float lifetime = 0.0f;
    float maxLifetime = 1.2f;  // 12 range / 10 speed
    bool alive = true;
    bool isSilkArrow = false;  // applies slow on hit
};

#endif
