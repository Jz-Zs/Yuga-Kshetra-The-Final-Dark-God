#include "World.h"
#include "../Entity/PigmanAI.h"
#include "../Entity/SpiderAI.h"

#include <algorithm>
#include <future>
#include <iostream>

#include "../Camera.h"
#include "../Item/Material.h"
#include "../Input/ToggleKey.h"
#include "../Maths/Vector2XZ.h"
#include "../Player/Player.h"
#include "../Renderer/RenderMaster.h"
#include "../Util/Random.h"
#include "WorldConstants.h"

World::World(const Camera &camera, const Config &config, Player &player)
    : m_chunkManager(*this)
    , m_renderDistance(config.renderDistance)
{
    setSpawnPoint();
    player.position = m_playerSpawnPoint;

    for (int i = 0; i < 1; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        m_chunkLoadThreads.emplace_back([&]() { loadChunks(camera); });
    }
}

World::~World()
{
    m_isRunning = false;
    for (auto &thread : m_chunkLoadThreads) {
        thread.join();
    }
}

void World::resetWorld(const Camera& /*camera*/, Player &player)
{
    // 1. Clear entities
    m_dropItems.clear();
    m_pigmen.clear();
    m_spiders.clear();
    m_projectiles.clear();
    m_arrows.clear();
    m_difficultyTriggered = false;
    m_difficultyMultiplier = 1.0f;
    m_events.clear();

    // 2. Delete GPU meshes and erase all chunks
    m_chunkManager.deleteMeshes();
    auto& chunks = m_chunkManager.getChunks();
    chunks.clear();

    // 3. Reset player position to spawn point
    setSpawnPoint();
    player.position = m_playerSpawnPoint;
    player.velocity = {0, 0, 0};

    // 清理熔炉状态
    player.onWorldReset();

    m_extractionActive = false;

    // 4. Spawn initial pigmen (max 8)
    m_pigmen.clear();
    for (int i = 0; i < 8; i++) {
        spawnPigman(player, 15.0f);
    }
}

// world coords into chunk column coords
ChunkBlock World::getBlock(int x, int y, int z)
{
    auto bp = getBlockXZ(x, z);
    auto chunkPosition = getChunkXZ(x, z);

    return m_chunkManager.getChunk(chunkPosition.x, chunkPosition.z)
        .getBlock(bp.x, y, bp.z);
}

void World::setBlock(int x, int y, int z, ChunkBlock block)
{
    if (y <= 0)
        return;

    auto bp = getBlockXZ(x, z);
    auto chunkPosition = getChunkXZ(x, z);

    m_chunkManager.getChunk(chunkPosition.x, chunkPosition.z)
        .setBlock(bp.x, y, bp.z, block);
}

// loads chunks
// make chunk meshes
void World::update(const Camera& /*camera*/, float dt)
{
    static ToggleKey key(sf::Keyboard::Key::C);

    if (key.isKeyPressed()) {
        std::unique_lock<std::mutex> lock(m_mainMutex);
        m_chunkManager.deleteMeshes();
        m_loadDistance = 2;
    }

    for (auto &event : m_events) {
        event->handle(*this);
    }
    m_events.clear();

    updateChunks();
    updateDrops(dt);
}

///@TODO
/// Optimize for chunkPositionU usage :thinking:
void World::loadChunks(const Camera &camera)
{
    // Load all MVP world chunks progressively
    while (m_isRunning) {
        bool allDone = true;
        for (int x = 0; x < MVP_CHUNK_COUNT; x++) {
            for (int z = 0; z < MVP_CHUNK_COUNT; z++) {
                std::unique_lock<std::mutex> lock(m_mainMutex);
                if (m_chunkManager.makeMesh(x, z, camera)) {
                    allDone = false;
                }
            }
        }
        if (allDone) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

void World::updateChunk(int blockX, int blockY, int blockZ)
{
    std::unique_lock<std::mutex> lock(m_mainMutex);

    auto addChunkToUpdateBatch = [&](const sf::Vector3i &key,
                                     ChunkSection &section) {
        m_chunkUpdates.emplace(key, &section);
    };

    auto chunkPosition = getChunkXZ(blockX, blockZ);
    auto chunkSectionY = blockY / CHUNK_SIZE;

    sf::Vector3i key(chunkPosition.x, chunkSectionY, chunkPosition.z);
    addChunkToUpdateBatch(
        key, m_chunkManager.getChunk(chunkPosition.x, chunkPosition.z)
                 .getSection(chunkSectionY));

    auto sectionBlockXZ = getBlockXZ(blockX, blockZ);
    auto sectionBlockY = blockY % CHUNK_SIZE;

    if (sectionBlockXZ.x == 0) {
        sf::Vector3i newKey(chunkPosition.x - 1, chunkSectionY,
                            chunkPosition.z);
        addChunkToUpdateBatch(
            newKey,
            m_chunkManager.getChunk(newKey.x, newKey.z).getSection(newKey.y));
    }
    else if (sectionBlockXZ.x == CHUNK_SIZE - 1) {
        sf::Vector3i newKey(chunkPosition.x + 1, chunkSectionY,
                            chunkPosition.z);
        addChunkToUpdateBatch(
            newKey,
            m_chunkManager.getChunk(newKey.x, newKey.z).getSection(newKey.y));
    }

    if (sectionBlockY == 0) {
        sf::Vector3i newKey(chunkPosition.x, chunkSectionY - 1,
                            chunkPosition.z);
        addChunkToUpdateBatch(
            newKey,
            m_chunkManager.getChunk(newKey.x, newKey.z).getSection(newKey.y));
    }
    else if (sectionBlockY == CHUNK_SIZE - 1) {
        sf::Vector3i newKey(chunkPosition.x, chunkSectionY + 1,
                            chunkPosition.z);
        addChunkToUpdateBatch(
            newKey,
            m_chunkManager.getChunk(newKey.x, newKey.z).getSection(newKey.y));
    }

    if (sectionBlockXZ.z == 0) {
        sf::Vector3i newKey(chunkPosition.x, chunkSectionY,
                            chunkPosition.z - 1);
        addChunkToUpdateBatch(
            newKey,
            m_chunkManager.getChunk(newKey.x, newKey.z).getSection(newKey.y));
    }
    else if (sectionBlockXZ.z == CHUNK_SIZE - 1) {
        sf::Vector3i newKey(chunkPosition.x, chunkSectionY,
                            chunkPosition.z + 1);
        addChunkToUpdateBatch(
            newKey,
            m_chunkManager.getChunk(newKey.x, newKey.z).getSection(newKey.y));
    }
}

void World::renderWorld(RenderMaster &renderer, const Camera &camera)
{
    std::unique_lock<std::mutex> lock(m_mainMutex);
    renderer.drawSky();

    auto &chunkMap = m_chunkManager.getChunks();
    for (auto itr = chunkMap.begin(); itr != chunkMap.end();) {
        Chunk &chunk = itr->second;

        int cameraX = static_cast<int>(camera.position.x);
        int cameraZ = static_cast<int>(camera.position.z);

        int minX = (cameraX / CHUNK_SIZE) - m_renderDistance;
        int minZ = (cameraZ / CHUNK_SIZE) - m_renderDistance;
        int maxX = (cameraX / CHUNK_SIZE) + m_renderDistance;
        int maxZ = (cameraZ / CHUNK_SIZE) + m_renderDistance;

        auto location = chunk.getLocation();

        if (minX > location.x || minZ > location.y || maxZ < location.y ||
            maxX < location.x) {
            itr = chunkMap.erase(itr);
            continue;
        }
        else {
            chunk.drawChunks(renderer, camera);
            itr++;
        }
    }
}

ChunkManager &World::getChunkManager()
{
    return m_chunkManager;
}

VectorXZ World::getBlockXZ(int x, int z)
{
    return {x % CHUNK_SIZE, z % CHUNK_SIZE};
}

VectorXZ World::getChunkXZ(int x, int z)
{
    return {x / CHUNK_SIZE, z / CHUNK_SIZE};
}

void World::updateChunks()
{
    std::unique_lock<std::mutex> lock(m_mainMutex);
    for (auto &c : m_chunkUpdates) {
        ChunkSection &s = *c.second;
        s.makeMesh();
    }
    m_chunkUpdates.clear();
}

void World::placeExtractionPoint()
{
    // Random position, entire 3x3 within [0,127]
    glm::ivec3 center{64, 35, 64};
    bool valid = false;

    for (int tries = 0; tries < 200; tries++) {
        int cx = RandomSingleton::get().intInRange(1, 126);
        int cz = RandomSingleton::get().intInRange(1, 126);

        // Load chunk and get surface height
        int chunkX = cx / CHUNK_SIZE, chunkZ = cz / CHUNK_SIZE;
        m_chunkManager.loadChunk(chunkX, chunkZ);
        Chunk& chunk = m_chunkManager.getChunk(chunkX, chunkZ);
        int surfaceY = chunk.getHeightAt(cx & 15, cz & 15);

        // Reject if surface is too low (water level or below)
        if (surfaceY < MVP_WATER_LEVEL) continue;

        // Reject: tree blocks, water, cactus in 3x3 area at surface level
        bool rejected = false;
        for (int dx = -1; dx <= 1 && !rejected; dx++) {
            for (int dz = -1; dz <= 1 && !rejected; dz++) {
                int bx = cx + dx, bz = cz + dz;
                int ckx = bx / CHUNK_SIZE, ckz = bz / CHUNK_SIZE;
                m_chunkManager.loadChunk(ckx, ckz);
                Chunk& ck = m_chunkManager.getChunk(ckx, ckz);
                int sy = ck.getHeightAt(bx & 15, bz & 15);
                auto block = ck.getBlock(bx & 15, sy, bz & 15);
                if (block.id == (int)BlockId::OakBark || block.id == (int)BlockId::OakLeaf ||
                    block.id == (int)BlockId::Water   || block.id == (int)BlockId::Cactus) {
                    rejected = true;
                }
            }
        }

        if (!rejected) {
            center = {cx, surfaceY + 1, cz};
            valid = true;
            break;
        }
    }

    if (!valid) {
        // Fallback: center of world at reasonable height
        center = {64, 35, 64};
    }

    // Place 3x3 GoldBlock platform
    for (int dx = -1; dx <= 1; dx++) {
        for (int dz = -1; dz <= 1; dz++) {
            int bx = center.x + dx, bz = center.z + dz;
            setBlock(bx, center.y, bz, BlockId::GoldBlock);
            updateChunk(bx, center.y, bz);
        }
    }

    m_extractionCenter = center;
    m_extractionActive = true;
}

void World::setSpawnPoint()
{
    // Random spawn within 128x128 world, 8-block margin from edges
    constexpr int margin = 8;
    constexpr int maxRetries = 20;
    int worldX = 64, worldZ = 64; // fallback: center

    for (int retry = 0; retry < maxRetries; retry++) {
        int rx = (int)RandomSingleton::get().intInRange(margin, MVP_WORLD_SIZE_X - 1 - margin);
        int rz = (int)RandomSingleton::get().intInRange(margin, MVP_WORLD_SIZE_Z - 1 - margin);
        int cx = rx / CHUNK_SIZE, cz = rz / CHUNK_SIZE;
        int lx = rx % CHUNK_SIZE, lz = rz % CHUNK_SIZE;

        m_chunkManager.loadChunk(cx, cz);
        int sy = m_chunkManager.getChunk(cx, cz).getHeightAt(lx, lz);
        if (sy >= MVP_WATER_LEVEL) {
            worldX = rx; worldZ = rz;
            m_playerSpawnPoint = {(float)worldX + 0.5f, (float)(sy + 1), (float)worldZ + 0.5f};
            return;
        }
    }
    // Fallback: center of world
    int cx = 64 / CHUNK_SIZE, cz = 64 / CHUNK_SIZE;
    m_chunkManager.loadChunk(cx, cz);
    int sy = m_chunkManager.getChunk(cx, cz).getHeightAt(0, 0);
    m_playerSpawnPoint = {64.5f, (float)(sy + 1), 64.5f};
}

void World::spawnDrop(const glm::ivec3& blockPos, BlockId blockId)
{
    const Material& mat = Material::toMaterial(blockId);
    if (mat.id == Material::ID::Nothing)
        return;

    ItemDropEntity drop;
    drop.position = glm::vec3(blockPos) + glm::vec3(0.5f, 0.75f, 0.5f);
    drop.velocity = glm::vec3(0.0f);
    drop.material = &mat;
    drop.lifeTime = 0.0f;
    drop.onGround = false;
    drop.alive = true;
    m_dropItems.push_back(drop);
}

void World::updateDrops(float dt)
{
    for (auto& drop : m_dropItems) {
        if (!drop.alive)
            continue;
        if (!drop.onGround) {
            drop.velocity.y -= 40.0f * dt;
            drop.position += drop.velocity * dt;

            // Ground check: query block below drop
            int bx = static_cast<int>(drop.position.x);
            int by = static_cast<int>(drop.position.y - 0.125f);
            int bz = static_cast<int>(drop.position.z);
            ChunkBlock below = getBlock(bx, by, bz);
            if (below.id != 0 && below.getData().isCollidable) {
                drop.position.y = static_cast<float>(by) + 1.0f + 0.125f;
                drop.velocity = glm::vec3(0.0f);
                drop.onGround = true;
            }
        }
        drop.lifeTime += dt;
        if (drop.lifeTime > 90.0f)
            drop.alive = false;
    }

    // Remove dead drops
    m_dropItems.erase(
        std::remove_if(m_dropItems.begin(), m_dropItems.end(),
                       [](const ItemDropEntity& d) { return !d.alive; }),
        m_dropItems.end());
}

std::vector<ItemDropEntity>& World::getDropItems()
{
    return m_dropItems;
}

void World::spawnPigman(const Player& player, float minDist)
{
    PigmanEntity e;
    glm::vec3 spawnPos{};
    bool found = false;

    for (int tries = 0; tries < 50; tries++) {
        float x = (float)(std::rand() % (MVP_WORLD_SIZE_X - 10) + 5);
        float z = (float)(std::rand() % (MVP_WORLD_SIZE_Z - 10) + 5);

        if (glm::distance(glm::vec2(x, z),
                          glm::vec2(player.position.x, player.position.z)) < minDist)
            continue;

        // Find ground
        int gx = (int)x, gz = (int)z;
        int gy = -1;
        for (int y = MVP_WORLD_HEIGHT - 1; y > 0; y--) {
            auto block = getBlock(gx, y, gz);
            // Skip leaves/flora — only solid ground counts
            bool isLeaf = (block.id == (int)BlockId::OakLeaf);
            if (block.id != 0 && block.getData().isCollidable && !isLeaf) {
                gy = y + 1;
                break;
            }
        }
        if (gy < 0) continue;

        // Check 3x3 overhead column (avoid trees/leaves)
        bool blocked = false;
        for (int dy = 0; dy <= 7 && !blocked; dy++)
            for (int dx = -1; dx <= 1 && !blocked; dx++)
                for (int dz = -1; dz <= 1 && !blocked; dz++) {
                    auto b = getBlock(gx + dx, gy + dy, gz + dz);
                    if (b.id != 0 && b.getData().isCollidable) blocked = true;
                }
        if (blocked) continue;

        spawnPos = glm::vec3(x + 0.5f, (float)gy, z + 0.5f);
        found = true;
        break;
    }

    if (!found) return;

    e.position = spawnPos;
    e.patrolOrigin = spawnPos;
    e.stuckPosition = spawnPos;
    e.box.update(e.position);
    m_pigmen.push_back(e);
}

void World::spawnSpider(const Player& player)
{
    SpiderEntity e;
    glm::vec3 spawnPos{};
    bool found = false;

    for (int tries = 0; tries < 50; tries++) {
        float x = (float)(std::rand() % (MVP_WORLD_SIZE_X - 20) + 10);
        float z = (float)(std::rand() % (MVP_WORLD_SIZE_Z - 20) + 10);

        if (glm::distance(glm::vec2(x, z),
                          glm::vec2(player.position.x, player.position.z)) < 20.0f)
            continue;

        int gx = (int)x, gz = (int)z;
        int gy = -1;
        for (int y = MVP_WORLD_HEIGHT - 1; y > 0; y--) {
            auto block = getBlock(gx, y, gz);
            bool isLeaf = (block.id == (int)BlockId::OakLeaf);
            if (block.id != 0 && block.getData().isCollidable && !isLeaf) {
                gy = y + 1;
                break;
            }
        }
        if (gy < 0) continue;

        bool blocked = false;
        for (int dy = 0; dy <= 2 && !blocked; dy++)
            for (int dx = -1; dx <= 1 && !blocked; dx++)
                for (int dz = -1; dz <= 1 && !blocked; dz++) {
                    auto b = getBlock(gx + dx, gy + dy, gz + dz);
                    if (b.id != 0 && b.getData().isCollidable) blocked = true;
                }
        if (blocked) continue;

        spawnPos = glm::vec3(x + 0.5f, (float)gy, z + 0.5f);
        found = true;
        break;
    }

    if (!found) return;

    e.position = spawnPos;
    e.patrolOrigin = spawnPos;
    e.stuckPosition = spawnPos;
    e.box.update(e.position);
    m_spiders.push_back(e);
}

void World::updateEntities(float dt, Player& player)
{
    float elapsed = 600.0f - player.m_roundTimeLeft;

    // Dynamic Difficulty trigger at 6:00
    if (!m_difficultyTriggered && elapsed >= 360.0f) {
        m_difficultyTriggered = true;
        m_difficultyMultiplier = 1.1f;
    }

    // Lazy initial spawn — fill pigmen as chunks become available
    int pigmanMax = (m_difficultyMultiplier > 1.05f) ? 9 : 8;
    if (m_pigmen.size() < (size_t)pigmanMax) {
        spawnPigman(player, 15.0f);
    }
    for (auto& e : m_pigmen) {
        if (e.state == PigmanEntity::Dead) {
            e.deathAnimTimer += dt; // drive death animation
            e.respawnTimer -= dt;
            if (e.respawnTimer <= 0.0f) {
                // Respawn: find new position far from player
                glm::vec3 spawnPos{};
                bool found = false;
                for (int tries = 0; tries < 50; tries++) {
                    float x = (float)(std::rand() % (MVP_WORLD_SIZE_X - 20) + 10);
                    float z = (float)(std::rand() % (MVP_WORLD_SIZE_Z - 20) + 10);
                    if (glm::distance(glm::vec2(x, z),
                        glm::vec2(player.position.x, player.position.z)) < 20.0f)
                        continue;
                    int gy = -1;
                    for (int y = MVP_WORLD_HEIGHT - 1; y > 0; y--) {
                        auto block = getBlock((int)x, y, (int)z);
                        if (block.id != 0 && block.getData().isCollidable)
                            { gy = y + 1; break; }
                    }
                    if (gy < 0) continue;
                    bool blocked = false;
                    for (int dy = 0; dy <= 7; dy++) {
                        auto b = getBlock((int)x, gy + dy, (int)z);
                        if (b.id != 0 && b.getData().isCollidable) { blocked = true; break; }
                    }
                    if (blocked) continue;
                    spawnPos = glm::vec3(x + 0.5f, (float)gy, z + 0.5f);
                    found = true;
                    break;
                }
                if (found) {
                    // Manual reset since AABB has const dimensions (no copy assignment)
                    e.position = spawnPos;
                    e.velocity = glm::vec3(0, 0, 0);
                    e.rotation = glm::vec3(0, 0, 0);
                    e.hp = 40;
                    e.maxHp = 40;
                    e.moveSpeed = 4.5f;
                    e.state = PigmanEntity::Patrol;
                    e.stateTimer = 0.0f;
                    e.attackCooldown = 1.0f;
                    e.hurtTimer = 0.0f;
                    e.stuckTimer = 0.0f;
                    e.stuckPosition = spawnPos;
                    e.respawnTimer = -1.0f;
                    e.patrolOrigin = spawnPos;
                    e.patrolTarget = glm::vec3(0, 0, 0);
                    e.path.clear();
                    e.pathIndex = 0;
                    e.deathAnimTimer = 0.0f;
                    e.box.update(e.position);
                } else {
                    e.respawnTimer = 5.0f; // retry later
                }
            }
            continue;
        }

        // Run AI
        PigmanAI::update(e, dt, player, *this);

        // Gravity
        int bx = (int)e.position.x;
        int by = (int)(e.position.y - 0.125f);
        int bz = (int)e.position.z;
        auto below = getBlock(bx, by, bz);
        bool onSolidGround = (below.id != 0 && below.getData().isCollidable
                              && below.id != (int)BlockId::OakLeaf);
        if (onSolidGround) {
            float newY = (float)by + 1.0f + 0.125f;
            // Cap climb + verify body/head space clear (avoid climbing pit walls)
            int newBy = (int)newY;
            auto atBody = getBlock(bx, newBy, bz);
            auto atHead = getBlock(bx, newBy + 1, bz);
            bool bodyBlocked = (atBody.id != 0 && atBody.getData().isCollidable);
            bool headBlocked = (atHead.id != 0 && atHead.getData().isCollidable);
            if (newY > e.position.y + 1.0f || bodyBlocked || headBlocked)
                newY = e.position.y; // reject climb
            e.position.y = newY;
            e.velocity.y = 0;
        } else {
            e.velocity.y -= 40.0f * dt;
        }

        // Apply velocity
        e.position.x += e.velocity.x * dt;
        e.position.y += e.velocity.y * dt;
        e.position.z += e.velocity.z * dt;

        // Block collision
        for (int cx = (int)(e.position.x - e.box.dimensions.x);
             cx <= (int)(e.position.x + e.box.dimensions.x); cx++)
        for (int cy = (int)(e.position.y - e.box.dimensions.y);
             cy <= (int)(e.position.y + e.box.dimensions.y); cy++)
        for (int cz = (int)(e.position.z - e.box.dimensions.z);
             cz <= (int)(e.position.z + e.box.dimensions.z); cz++) {
            auto block = getBlock(cx, cy, cz);
            if (block.id == 0 || !block.getData().isCollidable) continue;
            bool isGround = (cy <= (int)(e.position.y - e.box.dimensions.y + 0.01f));
            if (e.velocity.y < -1.0f) continue;
            if (!isGround) {
            float rEdge = e.position.x + e.box.dimensions.x;
            float lEdge = e.position.x - e.box.dimensions.x;
            if (e.velocity.x > 0 && rEdge > (float)cx && lEdge < (float)(cx + 1)) {
                e.position.x = (float)cx - e.box.dimensions.x;
                e.velocity.x = 0;
            }
            if (e.velocity.x < 0 && lEdge < (float)(cx + 1) && rEdge > (float)cx) {
                e.position.x = (float)(cx + 1) + e.box.dimensions.x;
                e.velocity.x = 0;
            }
            float fEdge = e.position.z + e.box.dimensions.z;
            float bEdge = e.position.z - e.box.dimensions.z;
            if (e.velocity.z > 0 && fEdge > (float)cz && bEdge < (float)(cz + 1)) {
                e.position.z = (float)cz - e.box.dimensions.z;
                e.velocity.z = 0;
            }
            if (e.velocity.z < 0 && bEdge < (float)(cz + 1) && fEdge > (float)cz) {
                e.position.z = (float)(cz + 1) + e.box.dimensions.z;
                e.velocity.z = 0;
            }
            } // !isGround
        }

        e.box.update(e.position);

        // No damping — AI sets velocity directly each frame

        // Pigman attacks player
        if (e.state == PigmanEntity::Attack && e.attackCooldown <= 0.0f) {
            float d = glm::distance(
                glm::vec3(e.position.x, 0, e.position.z),
                glm::vec3(player.position.x, 0, player.position.z));
            if (d < 2.0f) {
                glm::vec3 knockDir = glm::normalize(
                    player.position - e.position);
                knockDir.y = 0;
                if (glm::length(knockDir) < 0.01f)
                    knockDir = glm::vec3(0, 0, -1);
                player.takeDamage((int)(5 * m_difficultyMultiplier), knockDir);
                e.attackCooldown = 1.0f;
            }
        }

        // Clamp to world bounds
        if (e.position.x < 0) e.position.x = 0;
        if (e.position.x >= MVP_WORLD_SIZE_X) e.position.x = MVP_WORLD_SIZE_X - 1;
        if (e.position.z < 0) e.position.z = 0;
        if (e.position.z >= MVP_WORLD_SIZE_Z) e.position.z = MVP_WORLD_SIZE_Z - 1;
        if (e.position.y < 0) e.position.y = 1;
    }

    // --- Staged spider spawning ---
    int baseSpiderMax = 0;
    if (elapsed >= 60.0f) {
        baseSpiderMax = std::min(5, (int)(elapsed / 60.0f));
    }
    int spiderMax = baseSpiderMax + (m_difficultyMultiplier > 1.05f ? 1 : 0);
    if ((int)m_spiders.size() < spiderMax) {
        spawnSpider(player);
    }

    // --- Spider updates ---
    for (auto& e : m_spiders) {
        if (e.state == SpiderEntity::Dead) {
            e.deathAnimTimer += dt;
            e.respawnTimer -= dt;
            if (e.respawnTimer <= 0.0f) {
                glm::vec3 spawnPos{};
                bool found = false;
                for (int tries = 0; tries < 50; tries++) {
                    float x = (float)(std::rand() % (MVP_WORLD_SIZE_X - 30) + 15);
                    float z = (float)(std::rand() % (MVP_WORLD_SIZE_Z - 30) + 15);
                    if (glm::distance(glm::vec2(x, z),
                        glm::vec2(player.position.x, player.position.z)) < 20.0f) continue;
                    int gy = -1;
                    for (int y = MVP_WORLD_HEIGHT - 1; y > 0; y--) {
                        auto b = getBlock((int)x, y, (int)z);
                        if (b.id != 0 && b.getData().isCollidable) { gy = y + 1; break; }
                    }
                    if (gy < 0) continue;
                    bool blocked = false;
                    for (int dy = 0; dy <= 2; dy++) {
                        auto b = getBlock((int)x, gy + dy, (int)z);
                        if (b.id != 0 && b.getData().isCollidable) { blocked = true; break; }
                    }
                    if (blocked) continue;
                    spawnPos = glm::vec3(x + 0.5f, (float)gy, z + 0.5f);
                    found = true; break;
                }
                if (found) {
                    e.position = spawnPos;
                    e.velocity = glm::vec3(0, 0, 0);
                    e.rotation = glm::vec3(0, 0, 0);
                    e.hp = 80;
                    e.state = SpiderEntity::Patrol;
                    e.stateTimer = 0.0f;
                    e.meleeCooldown = 1.5f;
                    e.rangedCooldown = 2.5f;
                    e.freezeTimer = 0.0f;
                    e.hurtTimer = 0.0f;
                    e.stuckTimer = 0.0f;
                    e.stuckPosition = spawnPos;
                    e.patrolOrigin = spawnPos;
                    e.respawnTimer = -1.0f;
                    e.path.clear();
                    e.pathIndex = 0;
                    e.deathAnimTimer = 0.0f;
                    e.box.update(e.position);
                } else {
                    e.respawnTimer = 5.0f;
                }
            }
            continue;
        }

        // Run AI
        SpiderAI::update(e, dt, player, *this);

        // Ranged attack: fire projectile in Chase state when cooldown just set
        if (e.state == SpiderEntity::Chase && e.rangedCooldown > 1.99f && e.rangedCooldown <= 2.0f) {
            SpiderProjectile proj;
            proj.position = e.position + glm::vec3(0, 0.75f, 0);
            glm::vec3 dir = player.position - e.position;
            dir.y = 0;
            if (glm::length(dir) > 0.01f) {
                dir = glm::normalize(dir);
                proj.velocity = dir * 5.0f;
            } else {
                proj.velocity = glm::vec3(0, 0, -4.0f);
            }
            proj.damage = (int)(5 * m_difficultyMultiplier);
            proj.alive = true;
            proj.lifetime = 0.0f;
            proj.maxLifetime = 2.0f;
            m_projectiles.push_back(proj);
        }

        // Gravity
        int bx = (int)e.position.x;
        int by = (int)(e.position.y - 0.125f);
        int bz = (int)e.position.z;
        auto below = getBlock(bx, by, bz);
        bool onSolidGround = (below.id != 0 && below.getData().isCollidable
                              && below.id != (int)BlockId::OakLeaf);
        if (onSolidGround) {
            float newY = (float)by + 1.0f + 0.125f;
            int newBy = (int)newY;
            auto atBody = getBlock(bx, newBy, bz);
            auto atHead = getBlock(bx, newBy + 1, bz);
            bool bodyBlocked = (atBody.id != 0 && atBody.getData().isCollidable);
            bool headBlocked = (atHead.id != 0 && atHead.getData().isCollidable);
            if (newY > e.position.y + 1.0f || bodyBlocked || headBlocked)
                newY = e.position.y;
            e.position.y = newY;
            e.velocity.y = 0;
        } else {
            e.velocity.y -= 40.0f * dt;
        }

        // Apply velocity
        e.position.x += e.velocity.x * dt;
        e.position.y += e.velocity.y * dt;
        e.position.z += e.velocity.z * dt;

        // Block collision (same pattern as pigman)
        for (int cx = (int)(e.position.x - e.box.dimensions.x);
             cx <= (int)(e.position.x + e.box.dimensions.x); cx++)
        for (int cy = (int)(e.position.y - e.box.dimensions.y);
             cy <= (int)(e.position.y + e.box.dimensions.y); cy++)
        for (int cz = (int)(e.position.z - e.box.dimensions.z);
             cz <= (int)(e.position.z + e.box.dimensions.z); cz++) {
            auto block = getBlock(cx, cy, cz);
            if (block.id == 0 || !block.getData().isCollidable) continue;
            bool isGround = (cy <= (int)(e.position.y - e.box.dimensions.y + 0.01f));
            if (e.velocity.y < -1.0f) continue;
            if (!isGround) {
                float rEdge = e.position.x + e.box.dimensions.x;
                float lEdge = e.position.x - e.box.dimensions.x;
                if (e.velocity.x > 0 && rEdge > (float)cx && lEdge < (float)(cx + 1))
                    { e.position.x = (float)cx - e.box.dimensions.x; e.velocity.x = 0; }
                if (e.velocity.x < 0 && lEdge < (float)(cx + 1) && rEdge > (float)cx)
                    { e.position.x = (float)(cx + 1) + e.box.dimensions.x; e.velocity.x = 0; }
                float fEdge = e.position.z + e.box.dimensions.z;
                float bEdge = e.position.z - e.box.dimensions.z;
                if (e.velocity.z > 0 && fEdge > (float)cz && bEdge < (float)(cz + 1))
                    { e.position.z = (float)cz - e.box.dimensions.z; e.velocity.z = 0; }
                if (e.velocity.z < 0 && bEdge < (float)(cz + 1) && fEdge > (float)cz)
                    { e.position.z = (float)(cz + 1) + e.box.dimensions.z; e.velocity.z = 0; }
            }
        }

        e.box.update(e.position);

        // Spider melee attack
        if (e.state == SpiderEntity::MeleeAttack && e.meleeCooldown <= 0.0f) {
            float d = glm::distance(
                glm::vec3(e.position.x, 0, e.position.z),
                glm::vec3(player.position.x, 0, player.position.z));
            if (d < 2.0f) {
                glm::vec3 knockDir = glm::normalize(player.position - e.position);
                knockDir.y = 0;
                if (glm::length(knockDir) < 0.01f) knockDir = glm::vec3(0, 0, -1);
                player.takeDamage((int)(10 * m_difficultyMultiplier), knockDir);
                e.meleeCooldown = 1.5f;
            }
        }

        // Clamp to world bounds
        if (e.position.x < 0) e.position.x = 0;
        if (e.position.x >= MVP_WORLD_SIZE_X) e.position.x = MVP_WORLD_SIZE_X - 1;
        if (e.position.z < 0) e.position.z = 0;
        if (e.position.z >= MVP_WORLD_SIZE_Z) e.position.z = MVP_WORLD_SIZE_Z - 1;
        if (e.position.y < 0) e.position.y = 1;
    }

    // --- Projectile updates ---
    for (auto& p : m_projectiles) {
        if (!p.alive) continue;
        p.lifetime += dt;
        if (p.lifetime > p.maxLifetime) {
            p.alive = false;
            continue;
        }
        p.position.x += p.velocity.x * dt;
        p.position.y += p.velocity.y * dt;
        p.position.z += p.velocity.z * dt;

        // Block collision
        int px = (int)p.position.x, py = (int)p.position.y, pz = (int)p.position.z;
        auto block = getBlock(px, py, pz);
        if (block.id != 0 && block.getData().isCollidable) {
            p.alive = false;
            continue;
        }

        // Player hit
        glm::vec3 pMin = p.position - glm::vec3(0.25f);
        glm::vec3 pMax = p.position + glm::vec3(0.25f);
        glm::vec3 plMin = player.position - player.box.dimensions;
        glm::vec3 plMax = player.position + player.box.dimensions;
        if (pMin.x < plMax.x && pMax.x > plMin.x &&
            pMin.y < plMax.y && pMax.y > plMin.y &&
            pMin.z < plMax.z && pMax.z > plMin.z) {
            glm::vec3 kb(0, 0, 0);
            player.takeDamage(p.damage, kb);
            player.m_slowTimer = 1.0f;
            player.m_slowFactor = 0.8f;
            p.alive = false;
        }
    }

    // Cleanup dead projectiles
    m_projectiles.erase(
        std::remove_if(m_projectiles.begin(), m_projectiles.end(),
            [](const SpiderProjectile& p) { return !p.alive; }),
        m_projectiles.end());

    // --- Arrow updates ---
    for (auto& a : m_arrows) {
        if (!a.alive) continue;
        a.lifetime += dt;
        if (a.lifetime > a.maxLifetime) {
            a.alive = false;
            continue;
        }
        a.position.x += a.velocity.x * dt;
        a.position.y += a.velocity.y * dt;
        a.position.z += a.velocity.z * dt;

        // Block collision
        int ax = (int)a.position.x, ay = (int)a.position.y, az = (int)a.position.z;
        auto block = getBlock(ax, ay, az);
        if (block.id != 0 && block.getData().isCollidable) {
            a.alive = false;
            continue;
        }

        // Drop helper for arrow kills
        auto arrowDrop = [&](const Material& mat) {
            ItemDropEntity d;
            d.position = glm::vec3(a.position.x, a.position.y - 0.3f, a.position.z);
            d.velocity = glm::vec3(0, 0, 0);
            d.material = &mat;
            d.alive = true;
            m_dropItems.push_back(d);
        };

        // Sphere-vs-AABB helper: arrow hit radius 0.4 blocks
        auto arrowHitsAABB = [&](const glm::vec3& eMin, const glm::vec3& eMax) -> bool {
            float r = 0.4f;
            float cx = glm::clamp(a.position.x, eMin.x, eMax.x);
            float cy = glm::clamp(a.position.y, eMin.y, eMax.y);
            float cz = glm::clamp(a.position.z, eMin.z, eMax.z);
            float dx = a.position.x - cx, dy = a.position.y - cy, dz = a.position.z - cz;
            return (dx*dx + dy*dy + dz*dz) < (r * r);
        };

        // Pigman hit
        for (auto& e : m_pigmen) {
            if (e.state == PigmanEntity::Dead) continue;
            glm::vec3 eMin = e.box.position - e.box.dimensions;
            glm::vec3 eMax = e.box.position + e.box.dimensions;
            if (arrowHitsAABB(eMin, eMax))
            {
                e.hp -= a.damage;
                e.aggroTimer = 3.0f;
                e.state = PigmanEntity::Chase;
                e.stateTimer = 0.0f;
                e.stuckPosition = e.position;
                e.stuckTimer = 0.0f;
                e.path.clear();
                if (a.isSilkArrow) {
                    e.m_slowTimer = 1.0f;
                    e.m_slowFactor = 0.8f;
                }
                if (e.hp <= 0) {
                    e.hp = 0;
                    for (int r = 0; r < 5; r++) {
                        if (std::rand() % 100 < 40) arrowDrop(Material::RAW_MEAT);
                        if (std::rand() % 100 < 30) arrowDrop(Material::STICK);
                    }
                    e.state = PigmanEntity::Dead;
                    e.deathAnimTimer = 0.0f;
                    e.respawnTimer = 10.0f + (float)(std::rand() % 11);
                } else {
                    e.state = PigmanEntity::Hurt;
                    e.hurtTimer = 0.3f;
                }
                a.alive = false;
                break;
            }
        }
        if (!a.alive) continue;

        // Spider hit
        for (auto& e : m_spiders) {
            if (e.state == SpiderEntity::Dead) continue;
            glm::vec3 eMin = e.box.position - e.box.dimensions;
            glm::vec3 eMax = e.box.position + e.box.dimensions;
            if (arrowHitsAABB(eMin, eMax))
            {
                e.hp -= a.damage;
                e.aggroTimer = 3.0f;
                e.state = SpiderEntity::Chase;
                e.stateTimer = 0.0f;
                e.stuckPosition = e.position;
                e.stuckTimer = 0.0f;
                e.path.clear();
                if (a.isSilkArrow) {
                    e.m_slowTimer = 1.0f;
                    e.m_slowFactor = 0.8f;
                }
                if (e.hp <= 0) {
                    e.hp = 0;
                    for (int r = 0; r < 5; r++) {
                        if (std::rand() % 100 < 35) arrowDrop(Material::RAW_MEAT);
                        if (std::rand() % 100 < 50) arrowDrop(Material::SILK);
                        if (std::rand() % 100 < 20) arrowDrop(Material::STICK);
                        if (std::rand() % 100 < 15) arrowDrop(Material::SILK_THREAD);
                    }
                    e.state = SpiderEntity::Dead;
                    e.deathAnimTimer = 0.0f;
                    e.respawnTimer = 30.0f + (float)(std::rand() % 21);
                } else {
                    e.state = SpiderEntity::Hurt;
                    e.hurtTimer = 0.3f;
                }
                a.alive = false;
                break;
            }
        }
    }

    // Cleanup dead arrows
    m_arrows.erase(
        std::remove_if(m_arrows.begin(), m_arrows.end(),
            [](const ArrowEntity& a) { return !a.alive; }),
        m_arrows.end());
}
