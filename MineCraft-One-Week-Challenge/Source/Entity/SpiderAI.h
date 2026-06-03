#ifndef SPIDERAI_H_INCLUDED
#define SPIDERAI_H_INCLUDED

#include "SpiderEntity.h"
#include <vector>

class World;
class Player;

namespace SpiderAI {

void update(SpiderEntity& e, float dt, Player& player, World& world);

} // namespace SpiderAI

#endif
