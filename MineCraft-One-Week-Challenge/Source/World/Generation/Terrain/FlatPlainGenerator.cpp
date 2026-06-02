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

            if (worldX < 0 || worldX >= MVP_WORLD_SIZE_X ||
                worldZ < 0 || worldZ >= MVP_WORLD_SIZE_Z) {
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

    // 入口7: 铁矿脉生成 — Z ∈ [5, 20), 每区块 2~4 个矿脉
    int veinCount = m_random.intInRange(2, 5); // [2, 4]
    for (int v = 0; v < veinCount; v++) {
        int cx = m_random.intInRange(0, CHUNK_SIZE - 1);
        int cy = m_random.intInRange(5, 20);
        int cz = m_random.intInRange(0, CHUNK_SIZE - 1);
        int veinSize = m_random.intInRange(6, 11); // [6, 10]
        int axis = m_random.intInRange(0, 3); // 0=X, 1=Y, 2=Z
        int worldX = chunkX * CHUNK_SIZE + cx;
        int worldZ = chunkZ * CHUNK_SIZE + cz;
        if (worldX < 0 || worldX >= MVP_WORLD_SIZE_X ||
            worldZ < 0 || worldZ >= MVP_WORLD_SIZE_Z) continue;

        for (int step = 0; step < veinSize; step++) {
            // 每步在 3×3×3 邻域内随机放 1 个铁矿，仅替换石头
            int dx = m_random.intInRange(-1, 1);
            int dy = m_random.intInRange(-1, 1);
            int dz = m_random.intInRange(-1, 1);
            int px = cx + dx, py = cy + dy, pz = cz + dz;
            if (px >= 0 && px < CHUNK_SIZE &&
                py >= 5 && py < 20 &&
                pz >= 0 && pz < CHUNK_SIZE) {
                if (chunk.getBlock(px, py, pz).id == (int)BlockId::Stone) {
                    chunk.setBlock(px, py, pz, BlockId::IronOre);
                }
            }
            int dir = (m_random.intInRange(0, 5) < 3) ? 1 : -1;
            switch (axis) { case 0: cx += dir; break; case 1: cy += dir; break; case 2: cz += dir; break; }
            cx = std::clamp(cx, 0, CHUNK_SIZE - 1);
            cy = std::clamp(cy, 5, 19);
            cz = std::clamp(cz, 0, CHUNK_SIZE - 1);
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
