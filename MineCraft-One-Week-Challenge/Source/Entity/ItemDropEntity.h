#ifndef ITEMDROPENTITY_H_INCLUDED
#define ITEMDROPENTITY_H_INCLUDED

#include "../Maths/glm.h"
#include "../Item/Material.h"

struct ItemDropEntity {
    glm::vec3 position;
    glm::vec3 velocity;
    const Material* material;
    float lifeTime = 0.0f;
    bool onGround = false;
    bool alive = true;
};

#endif // ITEMDROPENTITY_H_INCLUDED
