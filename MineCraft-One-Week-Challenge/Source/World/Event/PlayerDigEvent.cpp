#include "PlayerDigEvent.h"

#include "../../Item/Material.h"
#include "../../Player/Player.h"
#include "../World.h"

PlayerDigEvent::PlayerDigEvent(sf::Mouse::Button button,
                               const glm::vec3 &location, Player &player)
    : m_buttonPress(button)
    , m_digSpot(location)
    , m_pPlayer(&player)
{
}

void PlayerDigEvent::handle(World &world)
{
    auto chunkLocation = World::getChunkXZ(static_cast<int>(m_digSpot.x),
                                           static_cast<int>(m_digSpot.z));

    if (world.getChunkManager().chunkLoadedAt(chunkLocation.x,
                                              chunkLocation.z)) {
        dig(world);
    }
}

void PlayerDigEvent::dig(World &world)
{
    int x = static_cast<int>(m_digSpot.x);
    int y = static_cast<int>(m_digSpot.y);
    int z = static_cast<int>(m_digSpot.z);
    switch (m_buttonPress) {
        case sf::Mouse::Button::Left: {
            auto block = world.getBlock(x, y, z);
            world.spawnDrop(glm::ivec3(x, y, z), (BlockId)block.id);
            world.updateChunk(x, y, z);
            world.setBlock(x, y, z, 0);
            break;
        }

        case sf::Mouse::Button::Right: {
            auto &stack = m_pPlayer->getHeldItems();
            auto &material = stack.getMaterial();

            if (material.id == Material::ID::Nothing) {
                return;
            }
            else {
                stack.remove();
                world.updateChunk(x, y, z);
                world.setBlock(x, y, z, material.toBlockID());
                break;
            }
        }
        default:
            break;
    }
}
