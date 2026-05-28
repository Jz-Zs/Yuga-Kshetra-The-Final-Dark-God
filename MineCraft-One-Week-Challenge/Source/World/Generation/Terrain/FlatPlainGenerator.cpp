#include "FlatPlainGenerator.h"

#include <algorithm>
#include <vector>

#include "../../../Maths/GeneralMaths.h"
#include "../../../Util/Random.h"
#include "../../Chunk/Chunk.h"
#include "../../WorldConstants.h"
#include "../Structures/TreeGenerator.h"

namespace {
const int seed = RandomSingleton::get().intInRange(424, 325322);
}

FlatPlainGenerator::FlatPlainGenerator()
    : m_heightNoise(seed)
{
    NoiseParameters params;
    params.octaves = 3;
    params.amplitude = 2;
    params.smoothness = 500;
    params.heightOffset = 0;
    params.roughness = 0.5;

    m_heightNoise.setParameters(params);
}

void FlatPlainGenerator::generateTerrainFor(Chunk &chunk)
{
    auto location = chunk.getLocation();
    int chunkX = location.x;
    int chunkZ = location.y;

    m_random.setSeed((chunkX ^ chunkZ) << 2 | seed);

    std::vector<sf::Vector3i> treeCandidates;

    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            int worldX = chunkX * CHUNK_SIZE + x;
            int worldZ = chunkZ * CHUNK_SIZE + z;

            if (worldX >= MVP_WORLD_SIZE_X || worldZ >= MVP_WORLD_SIZE_Z) {
                continue;
            }

            double noiseVal =
                m_heightNoise.getHeight(x, z, chunkX, chunkZ);
            int surfaceY = 32 + static_cast<int>(noiseVal * 4.0);
            surfaceY = std::clamp(surfaceY, 20, 60);

            int stoneTop = std::max(0, surfaceY - 4);
            for (int y = 0; y < stoneTop; y++) {
                chunk.setBlock(x, y, z, BlockId::Stone);
            }

            for (int y = stoneTop; y < surfaceY; y++) {
                chunk.setBlock(x, y, z, BlockId::Dirt);
            }

            chunk.setBlock(x, surfaceY, z, BlockId::Grass);

            if (surfaceY < MVP_WATER_LEVEL) {
                for (int y = surfaceY + 1; y <= MVP_WATER_LEVEL; y++) {
                    chunk.setBlock(x, y, z, BlockId::Water);
                }
            }
            else {
                treeCandidates.emplace_back(x, surfaceY + 1, z);
            }
        }
    }

    if (!treeCandidates.empty()) {
        int treeCount = m_random.intInRange(5, 8);
        for (int i = 0; i < treeCount; i++) {
            int idx = m_random.intInRange(0,
                static_cast<int>(treeCandidates.size()) - 1);
            auto &pos = treeCandidates[idx];
            makeOakTree(chunk, m_random, pos.x, pos.y, pos.z);
        }
    }
}

int FlatPlainGenerator::getMinimumSpawnHeight() const noexcept
{
    return 32;
}
