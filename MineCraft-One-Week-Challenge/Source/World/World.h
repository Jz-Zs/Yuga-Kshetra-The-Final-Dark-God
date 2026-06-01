#ifndef WORLD_H_INCLUDED
#define WORLD_H_INCLUDED

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "../Util/NonCopyable.h"
#include "Chunk/Chunk.h"
#include "Chunk/ChunkManager.h"

#include "../Entity/ItemDropEntity.h"
#include "../Entity/PigmanEntity.h"

#include "Event/IWorldEvent.h"

#include "../Config.h"

class RenderMaster;
class Camera;
class Player;

struct Entity;

/// @brief Massive class designed to hold multiple chunks, the player, and most game aspects.
class World : public NonCopyable {
  public:
    World(const Camera &camera, const Config &config, Player &player);
    ~World();

    ChunkBlock getBlock(int x, int y, int z);
    void setBlock(int x, int y, int z, ChunkBlock block);

    void update(const Camera &camera, float dt);
    void updateChunk(int blockX, int blockY, int blockZ);

    void renderWorld(RenderMaster &master, const Camera &camera);

    ChunkManager &getChunkManager();

    void spawnDrop(const glm::ivec3& blockPos, BlockId blockId);
    void updateDrops(float dt);
    std::vector<ItemDropEntity>& getDropItems();

    void spawnPigman(const Player& player, float minDist);
    void updateEntities(float dt, Player& player);
    std::vector<PigmanEntity>& getPigmen() { return m_pigmen; }

    bool isExtractionActive() const { return m_extractionActive; }
    const glm::ivec3& getExtractionCenter() const { return m_extractionCenter; }

    void resetWorld(const Camera &camera, Player &player);

    void placeExtractionPoint();

    static VectorXZ getBlockXZ(int x, int z);
    static VectorXZ getChunkXZ(int x, int z);

    // void collisionTest(Entity &entity);

    template <typename T, typename... Args> void addEvent(Args &&... args)
    {
        m_events.push_back(std::make_unique<T>(std::forward<Args>(args)...));
    }

  private:
    void loadChunks(const Camera &camera);
    void updateChunks();
    void setSpawnPoint();

    ChunkManager m_chunkManager;

    std::vector<std::unique_ptr<IWorldEvent>> m_events;
    std::vector<ItemDropEntity> m_dropItems;
    std::vector<PigmanEntity> m_pigmen;
    std::unordered_map<sf::Vector3i, ChunkSection *> m_chunkUpdates;

    std::atomic<bool> m_isRunning{true};
    std::vector<std::thread> m_chunkLoadThreads;

    // Mutex classes invoked to protect data from shared threads

    std::mutex m_mainMutex;
    std::mutex m_genMutex;

    int m_loadDistance = 2;
    const int m_renderDistance;

    glm::vec3 m_playerSpawnPoint;

    glm::ivec3 m_extractionCenter{0, 0, 0};
    bool m_extractionActive = false;
};

#endif // WORLD_H_INCLUDED
