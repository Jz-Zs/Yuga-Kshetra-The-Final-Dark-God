#include "PlayerDigEvent.h"

#include "../../Item/Material.h"
#include "../../Player/Player.h"
#include "../../Util/Random.h"
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
            auto& data = block.getData();

            // Determine drop: use dropBlockId if set, otherwise drop self
            BlockId dropId = (data.dropBlockId != BlockId::Air)
                ? data.dropBlockId : (BlockId)block.id;

            // Probability-based drop (use integer random for float probability)
            bool shouldDrop = data.dropProbability >= 1.0f;
            if (!shouldDrop) {
                shouldDrop = RandomSingleton::get().intInRange(0, 99)
                             < static_cast<int>(data.dropProbability * 100);
            }
            if (shouldDrop) {
                world.spawnDrop(glm::ivec3(x, y, z), dropId);
            }

            // Special: OakLeaf 10% chance to also drop WildFruit
            if (block.id == (int)BlockId::OakLeaf
                && RandomSingleton::get().intInRange(0, 99) < 10) {
                world.spawnDrop(glm::ivec3(x, y, z), BlockId::WildFruit);
            }

            // 挖掘熔炉：委托 onFurnaceMined 统一处理掉落+清理
            if (block.id == (int)BlockId::Furnace) {
                m_pPlayer->onFurnaceMined();
            }

            world.updateChunk(x, y, z);
            world.setBlock(x, y, z, 0);
            break;
        }

        case sf::Mouse::Button::Right: {
            auto &stack = m_pPlayer->getHeldItems();
            auto &material = stack.getMaterial();

            if (material.id == Material::ID::Nothing || !material.isBlock) {
                return;
            }
            stack.remove();
            world.updateChunk(x, y, z);
            world.setBlock(x, y, z, material.toBlockID());
            // 如果放置的是熔炉，记录坐标
            if (material.id == Material::ID::Furnace) {
                m_pPlayer->m_furnacePos = glm::ivec3(x, y, z);
            }
            break;
        }
        default:
            break;
    }
}
