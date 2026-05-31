#ifndef PIGMANAI_H_INCLUDED
#define PIGMANAI_H_INCLUDED

#include "PigmanEntity.h"
#include <vector>

class World;
class Player;

namespace PigmanAI {

std::vector<glm::ivec3> findPath(World& world,
                                  const glm::vec3& from,
                                  const glm::vec3& to);

void update(PigmanEntity& e, float dt, Player& player, World& world);

} // namespace PigmanAI

#endif
