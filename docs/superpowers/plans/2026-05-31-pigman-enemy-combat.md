# Pigman Enemy + Combat System Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement pigman enemy with AI (patrol/chase/attack/hurt/dead), A* pathfinding, OBJ model rendering with programmatic animations, player HP/damage/death, and combat detection integrated into the game loop.

**Architecture:** Follows the existing `ItemDropEntity` pattern — `PigmanEntity` is a data struct, AI logic in free functions (`PigmanAI`), rendering via new `EntityRenderer` in `RenderMaster`, entity lifecycle managed by `World`. No external library dependencies.

**Tech Stack:** C++23, SFML 3, OpenGL 4.6 (glad), glm, existing Model/Mesh/BasicShader pipeline

---

### Task 1: Add RAW_MEAT Material

**Files:**
- Modify: `Source/Item/Material.h`
- Modify: `Source/Item/Material.cpp`

- [ ] **Step 1: Add RawMeat to the Material::ID enum**

In `Source/Item/Material.h:24-25`, add after `WoodenSword`:

```cpp
        Stick,
        WoodenSword,
        RawMeat
```

- [ ] **Step 2: Declare static instance**

In `Source/Item/Material.h:31`, add `RAW_MEAT`:

```cpp
    const static Material STICK, WOODEN_SWORD, RAW_MEAT;
```

- [ ] **Step 3: Define RAW_MEAT constant**

In `Source/Item/Material.cpp`, add after line 19 (`WOODEN_SWORD` definition):

```cpp
const Material Material::RAW_MEAT(ID::RawMeat, 30, false, "Raw Meat");
```

- [ ] **Step 4: Handle in toBlockID()**

In `Source/Item/Material.cpp:30-74`, add `RawMeat` case before `default:`:

```cpp
        case Stick:
            return BlockId::Stick;
        case WoodenSword:
            return BlockId::WoodenSword;
        case RawMeat:
            return BlockId::NUM_TYPES;
```

- [ ] **Step 5: Handle in toMaterial()**

In `Source/Item/Material.cpp:77-113`, add `Stick` and `WoodenSword` cases before `default:`:

```cpp
        case BlockId::Stick:
            return STICK;
        case BlockId::WoodenSword:
            return WOODEN_SWORD;
```

- [ ] **Step 6: Build**

```bash
cd "D:\Yuga_Kshetra-The_Final_Dark_God\MineCraft-One-Week-Challenge"
VCPKG_ROOT="D:\Microsoft Visual Studio\2022\Community\VC\vcpkg" cmake --build "out/build/x64-Debug" --config Debug
```
Expected: compiles clean.

---

### Task 2: PigmanEntity Data Structure

**Files:**
- Create: `Source/Entity/PigmanEntity.h`

- [ ] **Step 1: Create PigmanEntity.h**

```cpp
#ifndef PIGMANENTITY_H_INCLUDED
#define PIGMANENTITY_H_INCLUDED

#include "../Maths/glm.h"
#include "../Physics/AABB.h"
#include <vector>

class Model;

struct PigmanEntity {
    glm::vec3 position{0, 0, 0};
    glm::vec3 velocity{0, 0, 0};
    glm::vec3 rotation{0, 0, 0};
    AABB box{glm::vec3(0.4f, 1.0f, 0.4f)};

    int hp = 30;
    int maxHp = 30;
    float moveSpeed = 4.0f;

    enum State { Patrol, Chase, Attack, Hurt, Dead };
    State state = Patrol;
    float stateTimer = 0.0f;
    float attackCooldown = 1.5f;
    float hurtTimer = 0.0f;
    float stuckTimer = 0.0f;
    glm::vec3 stuckPosition{0, 0, 0};
    float respawnTimer = -1.0f;

    glm::vec3 patrolOrigin{0, 0, 0};
    glm::vec3 patrolTarget{0, 0, 0};
    std::vector<glm::ivec3> path;
    int pathIndex = 0;

    float deathAnimTimer = 0.0f;

    const Model* model = nullptr;
};

#endif
```

- [ ] **Step 2: Build to verify header**

```bash
cd "D:\Yuga_Kshetra-The_Final_Dark_God\MineCraft-One-Week-Challenge"
VCPKG_ROOT="D:\Microsoft Visual Studio\2022\Community\VC\vcpkg" cmake --build "out/build/x64-Debug" --config Debug
```
Expected: compiles.

---

### Task 3: PigmanAI — A* Pathfinding + State Machine

**Files:**
- Create: `Source/Entity/PigmanAI.h`
- Create: `Source/Entity/PigmanAI.cpp`

- [ ] **Step 1: Create PigmanAI.h**

```cpp
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
```

- [ ] **Step 2: Create PigmanAI.cpp**

```cpp
#include "PigmanAI.h"
#include "../World/World.h"
#include "../Player/Player.h"
#include "../World/WorldConstants.h"
#include <queue>
#include <unordered_map>
#include <cmath>
#include <cstdlib>

namespace PigmanAI {

// --- Utility: Random float ---
static float randFloat(float lo, float hi) {
    float t = (float)std::rand() / (float)RAND_MAX;
    return lo + t * (hi - lo);
}

// --- Hash for ivec3 ---
struct IVec3Hash {
    size_t operator()(const glm::ivec3& v) const {
        size_t h = 17;
        h = h * 31 + std::hash<int>()(v.x);
        h = h * 31 + std::hash<int>()(v.y);
        h = h * 31 + std::hash<int>()(v.z);
        return h;
    }
};

// --- Check if a world position is walkable (ground solid, body+head clear) ---
static bool isWalkable(World& world, int x, int y, int z) {
    if (x < 0 || x >= MVP_WORLD_SIZE_X) return false;
    if (z < 0 || z >= MVP_WORLD_SIZE_Z) return false;
    if (y <= 0 || y >= MVP_WORLD_HEIGHT) return false;

    auto ground = world.getBlock(x, y - 1, z);
    if (ground.id == 0 || !ground.getData().isCollidable) return false;

    auto body = world.getBlock(x, y, z);
    if (body.getData().isCollidable && body.id != 0) return false;

    auto head = world.getBlock(x, y + 1, z);
    if (head.getData().isCollidable && head.id != 0) return false;

    return true;
}

// --- Find ground Y for a given (x,z) ---
static int getGroundY(World& world, int x, int z) {
    for (int y = MVP_WORLD_HEIGHT - 1; y > 0; y--) {
        auto block = world.getBlock(x, y, z);
        if (block.id != 0 && block.getData().isCollidable) {
            return y + 1;
        }
    }
    return -1;
}

// --- Line of sight check ---
static bool hasLineOfSight(World& world, const glm::vec3& from, const glm::vec3& to) {
    glm::vec3 dir = glm::normalize(to - from);
    float dist = glm::distance(from, to);
    for (float t = 0.0f; t < dist; t += 0.5f) {
        glm::vec3 p = from + dir * t;
        int x = static_cast<int>(p.x);
        int y = static_cast<int>(p.y);
        int z = static_cast<int>(p.z);
        auto block = world.getBlock(x, y, z);
        if (block.id != 0 && block.getData().isCollidable)
            return false;
    }
    return true;
}

// --- A* Pathfinding ---
std::vector<glm::ivec3> findPath(World& world,
                                  const glm::vec3& from,
                                  const glm::vec3& to) {
    glm::ivec3 start(static_cast<int>(from.x),
                      static_cast<int>(from.y),
                      static_cast<int>(from.z));
    glm::ivec3 goal(static_cast<int>(to.x),
                     static_cast<int>(to.y),
                     static_cast<int>(to.z));

    int sy = getGroundY(world, start.x, start.z);
    int gy = getGroundY(world, goal.x, goal.z);
    if (sy < 0 || gy < 0) return {};
    start.y = sy;
    goal.y = gy;

    if (!isWalkable(world, start.x, start.y, start.z)) return {};

    struct Node { float g = 0, h = 0; glm::ivec3 parent{-1,-1,-1}; bool closed = false; };
    std::unordered_map<glm::ivec3, Node, IVec3Hash> nodes;
    nodes[start].g = 0;
    nodes[start].h = std::abs((float)(start.x - goal.x)) +
                     std::abs((float)(start.y - goal.y)) +
                     std::abs((float)(start.z - goal.z));
    nodes[start].parent = {-1, -1, -1};

    auto cmp = [&](const glm::ivec3& a, const glm::ivec3& b) {
        return nodes[a].g + nodes[a].h > nodes[b].g + nodes[b].h;
    };
    std::priority_queue<glm::ivec3, std::vector<glm::ivec3>, decltype(cmp)> open(cmp);
    open.push(start);

    int steps = 0;
    while (!open.empty() && steps < 256) {
        steps++;
        glm::ivec3 cur = open.top(); open.pop();
        if (nodes[cur].closed) continue;
        nodes[cur].closed = true;

        // Reached goal or adjacent
        int dx = std::abs(cur.x - goal.x);
        int dz = std::abs(cur.z - goal.z);
        if ((dx <= 1 && dz <= 1) || (cur == goal)) {
            std::vector<glm::ivec3> path;
            glm::ivec3 p = cur;
            while (p != glm::ivec3(-1, -1, -1)) {
                path.push_back(p);
                p = nodes[p].parent;
            }
            std::reverse(path.begin(), path.end());
            return path;
        }

        glm::ivec3 nb[8] = {
            {cur.x+1, cur.y, cur.z}, {cur.x-1, cur.y, cur.z},
            {cur.x, cur.y, cur.z+1}, {cur.x, cur.y, cur.z-1},
            {cur.x+1, cur.y+1, cur.z}, {cur.x-1, cur.y+1, cur.z},
            {cur.x, cur.y+1, cur.z+1}, {cur.x, cur.y+1, cur.z-1},
        };

        for (auto& n : nb) {
            int gy2 = getGroundY(world, n.x, n.z);
            if (gy2 < 0) continue;
            n.y = gy2;
            if (!isWalkable(world, n.x, n.y, n.z)) continue;

            float stepCost = (n.y != cur.y) ? 1.5f : 1.0f;
            float newG = nodes[cur].g + stepCost;
            auto it = nodes.find(n);
            if (it != nodes.end() && it->second.closed) continue;
            if (it != nodes.end() && newG >= it->second.g) continue;

            nodes[n].g = newG;
            nodes[n].h = std::abs((float)(n.x - goal.x)) +
                         std::abs((float)(n.y - goal.y)) +
                         std::abs((float)(n.z - goal.z));
            nodes[n].parent = cur;
            open.push(n);
        }
    }
    return {};
}

// --- AI State Machine ---
void update(PigmanEntity& e, float dt, Player& player, World& world) {
    glm::vec3 toPlayer = player.position - e.position;
    toPlayer.y += 0.6f; // aim at player upper body
    float distToPlayer = glm::length(glm::vec3(toPlayer.x, 0, toPlayer.z));
    float fullDist = glm::length(toPlayer);

    e.attackCooldown -= dt;
    e.hurtTimer -= dt;
    e.stateTimer += dt;

    switch (e.state) {

    case PigmanEntity::Patrol: {
        if (e.path.empty() && e.stateTimer > randFloat(2.0f, 4.0f)) {
            float rx = randFloat(-8.0f, 8.0f);
            float rz = randFloat(-8.0f, 8.0f);
            e.patrolTarget = glm::vec3(e.patrolOrigin.x + rx, e.patrolOrigin.y,
                                       e.patrolOrigin.z + rz);
            e.path = findPath(world, e.position, e.patrolTarget);
            e.pathIndex = 0;
            e.stateTimer = 0.0f;
        }

        if (distToPlayer < 12.0f && hasLineOfSight(world, e.position, player.position)) {
            e.state = PigmanEntity::Chase;
            e.stateTimer = 0.0f;
            e.path.clear();
            break;
        }

        if (!e.path.empty() && e.pathIndex < (int)e.path.size()) {
            glm::vec3 target(e.path[e.pathIndex].x + 0.5f, 0,
                             e.path[e.pathIndex].z + 0.5f);
            glm::vec3 d = target - e.position; d.y = 0;
            float hd = glm::length(d);
            if (hd < 0.3f) { e.pathIndex++; }
            else {
                d = glm::normalize(d);
                e.velocity.x = d.x * e.moveSpeed * 0.6f; // patrol slow
                e.velocity.z = d.z * e.moveSpeed * 0.6f;
                e.rotation.y = glm::degrees(std::atan2(d.x, d.z));
            }
        }
        break;
    }

    case PigmanEntity::Chase: {
        if (e.stateTimer > 0.5f || e.path.empty()) {
            e.path = findPath(world, e.position, player.position);
            e.pathIndex = 0;
            e.stateTimer = 0.0f;
        }

        // Stuck detection
        float moved = glm::distance(
            glm::vec3(e.position.x, 0, e.position.z),
            glm::vec3(e.stuckPosition.x, 0, e.stuckPosition.z));
        if (moved < 0.1f) { e.stuckTimer += dt; }
        else { e.stuckTimer = 0.0f; e.stuckPosition = e.position; }

        bool lostTarget = (distToPlayer > 12.0f && e.stateTimer > 5.0f);
        bool stuck = (e.stuckTimer > 3.0f);
        if (lostTarget || stuck) {
            e.state = PigmanEntity::Patrol;
            e.stateTimer = 0.0f;
            e.stuckTimer = 0.0f;
            e.path.clear();
            break;
        }

        if (distToPlayer < 2.0f) {
            e.state = PigmanEntity::Attack;
            e.attackCooldown = 0.0f;
            e.velocity.x = 0; e.velocity.z = 0;
            break;
        }

        if (!e.path.empty() && e.pathIndex < (int)e.path.size()) {
            glm::vec3 target(e.path[e.pathIndex].x + 0.5f, 0,
                             e.path[e.pathIndex].z + 0.5f);
            glm::vec3 d = target - e.position; d.y = 0;
            float hd = glm::length(d);
            if (hd < 0.3f) { e.pathIndex++; }
            else {
                d = glm::normalize(d);
                e.velocity.x = d.x * e.moveSpeed;
                e.velocity.z = d.z * e.moveSpeed;
            }
        } else {
            // No path — move directly toward player
            toPlayer.y = 0;
            if (glm::length(toPlayer) > 0.1f) {
                glm::vec3 d = glm::normalize(toPlayer);
                e.velocity.x = d.x * e.moveSpeed;
                e.velocity.z = d.z * e.moveSpeed;
            }
        }
        e.rotation.y = glm::degrees(std::atan2(toPlayer.x, toPlayer.z));
        break;
    }

    case PigmanEntity::Attack: {
        e.velocity.x = 0; e.velocity.z = 0;
        e.rotation.y = glm::degrees(std::atan2(toPlayer.x, toPlayer.z));

        if (distToPlayer > 2.0f) {
            e.state = PigmanEntity::Chase;
            e.stateTimer = 0.0f;
            break;
        }
        // Actual damage dealt in World::updateEntities when cooldown ≤ 0
        break;
    }

    case PigmanEntity::Hurt: {
        e.velocity.x = 0; e.velocity.z = 0;
        if (e.hurtTimer <= 0.0f) {
            e.state = PigmanEntity::Chase;
            e.stateTimer = 0.0f;
        }
        break;
    }

    case PigmanEntity::Dead: {
        e.velocity.x = 0; e.velocity.z = 0;
        e.deathAnimTimer += dt;
        // Respawn handled in World
        break;
    }
    }
}

} // namespace PigmanAI
```

- [ ] **Step 3: Build**

```bash
cd "D:\Yuga_Kshetra-The_Final_Dark_God\MineCraft-One-Week-Challenge"
VCPKG_ROOT="D:\Microsoft Visual Studio\2022\Community\VC\vcpkg" cmake --build "out/build/x64-Debug" --config Debug
```
Expected: compiles clean.

---

### Task 4: OBJ Loader + EntityRenderer

**Files:**
- Create: `Source/Renderer/EntityRenderer.h`
- Create: `Source/Renderer/EntityRenderer.cpp`

- [ ] **Step 1: Create EntityRenderer.h**

```cpp
#ifndef ENTITYRENDERER_H_INCLUDED
#define ENTITYRENDERER_H_INCLUDED

#include <vector>
#include <glad/glad.h>
#include "../Maths/glm.h"
#include "../Shaders/BasicShader.h"
#include "../Texture/BasicTexture.h"
#include "../Entity/PigmanEntity.h"
#include "../Model.h"

class Camera;

class EntityRenderer {
public:
    EntityRenderer();
    ~EntityRenderer();

    void addEntity(const PigmanEntity& e);
    void render(const Camera& camera);

private:
    Model* loadOBJ(const char* path);
    Model* m_pigmanModel = nullptr;
    BasicTexture m_texture;
    BasicShader m_shader;
    std::vector<const PigmanEntity*> m_entities;
};

#endif
```

- [ ] **Step 2: Create EntityRenderer.cpp**

```cpp
#include "EntityRenderer.h"
#include "../Camera.h"
#include "../Mesh.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>

EntityRenderer::EntityRenderer()
    : m_shader("Basic", "Basic")
{
    m_pigmanModel = loadOBJ("Res/Models/PigMan/PigMan.obj");
    if (m_pigmanModel) {
        m_texture.loadFromFile("Res/Models/PigMan/PigMan.png");
    }
}

EntityRenderer::~EntityRenderer()
{
    delete m_pigmanModel;
}

Model* EntityRenderer::loadOBJ(const char* path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "EntityRenderer: cannot open " << path << std::endl;
        return nullptr;
    }

    std::vector<glm::vec3> rawPos;
    std::vector<glm::vec2> rawUV;

    std::vector<GLfloat> outPos, outUV;
    std::vector<GLuint> outIdx;
    std::unordered_map<std::string, GLuint> indexMap;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "v") {
            glm::vec3 v; iss >> v.x >> v.y >> v.z;
            rawPos.push_back(v);
        } else if (type == "vt") {
            glm::vec2 vt; iss >> vt.x >> vt.y;
            rawUV.push_back(vt);
        } else if (type == "f") {
            // Handle 3 or 4 vertices per face
            std::string tokens[4];
            int count = 0;
            while (iss >> tokens[count] && count < 4) count++;

            int triVerts[6] = {0, 1, 2, 0, 2, 3}; // triangulate quad
            int triCount = (count == 4) ? 6 : 3;

            for (int ti = 0; ti < triCount; ti++) {
                int vi = triVerts[ti];
                if (vi >= count) continue;
                std::string& tok = tokens[vi];

                int vIdx = 0, tIdx = 0;
                size_t s1 = tok.find('/');
                if (s1 != std::string::npos) {
                    vIdx = std::stoi(tok.substr(0, s1));
                    size_t s2 = tok.find('/', s1 + 1);
                    if (s2 != std::string::npos && s2 > s1 + 1) {
                        tIdx = std::stoi(tok.substr(s1 + 1, s2 - s1 - 1));
                    }
                } else {
                    vIdx = std::stoi(tok);
                }

                std::string key = std::to_string(vIdx) + "/" + std::to_string(tIdx);
                auto it = indexMap.find(key);
                if (it != indexMap.end()) {
                    outIdx.push_back(it->second);
                } else {
                    GLuint idx = (GLuint)(outPos.size() / 3);
                    indexMap[key] = idx;
                    outIdx.push_back(idx);

                    const auto& p = rawPos[vIdx - 1];
                    outPos.push_back(p.x); outPos.push_back(p.y); outPos.push_back(p.z);

                    if (tIdx > 0 && tIdx <= (int)rawUV.size()) {
                        outUV.push_back(rawUV[tIdx - 1].x);
                        outUV.push_back(rawUV[tIdx - 1].y);
                    } else {
                        outUV.push_back(0.0f); outUV.push_back(0.0f);
                    }
                }
            }
        }
    }

    if (outPos.empty()) {
        std::cerr << "EntityRenderer: no vertices in " << path << std::endl;
        return nullptr;
    }

    std::cout << "EntityRenderer: loaded " << path << " ("
              << rawPos.size() << " verts, " << outIdx.size() << " indices)" << std::endl;

    Mesh mesh;
    mesh.vertexPositions = std::move(outPos);
    mesh.textureCoords = std::move(outUV);
    mesh.indices = std::move(outIdx);

    Model* model = new Model();
    model->addData(mesh);
    model->genVAO();
    return model;
}

void EntityRenderer::addEntity(const PigmanEntity& e)
{
    m_entities.push_back(&e);
}

void EntityRenderer::render(const Camera& camera)
{
    if (!m_pigmanModel || m_entities.empty()) return;

    m_shader.useProgram();
    m_shader.loadProjectionViewMatrix(camera.getProjViewMatrix());
    m_texture.bindTexture();

    for (const auto* e : m_entities) {
        if (e->state == PigmanEntity::Dead && e->deathAnimTimer > 1.0f) continue;

        glm::mat4 model(1.0f);
        // Pigman Y is at feet; model pivot at center → offset down
        model = glm::translate(model,
            glm::vec3(e->position.x, e->position.y - 0.5f, e->position.z));
        model = glm::rotate(model, glm::radians(e->rotation.y),
                            glm::vec3(0, 1, 0));
        model = glm::scale(model, glm::vec3(0.06f));

        // Programmatic animations
        if (e->state == PigmanEntity::Chase) {
            float bob = std::sin(e->stateTimer * 12.0f) * 0.08f;
            model = glm::translate(model, glm::vec3(0, bob, 0));
        }
        else if (e->state == PigmanEntity::Attack) {
            // attackCooldown counts from 1.5→0; animation in first 0.3s
            float phase = 1.0f - (e->attackCooldown / 1.5f);
            if (phase < 0.3f) {
                float t = phase / 0.3f;
                float angle = std::sin(t * 3.14159265f) * 25.0f;
                model = glm::rotate(model, glm::radians(angle), glm::vec3(0, 0, 1));
            }
        }
        else if (e->state == PigmanEntity::Dead) {
            float t = e->deathAnimTimer;
            if (t > 1.0f) t = 1.0f;
            float angle = t * 90.0f;
            model = glm::rotate(model, glm::radians(angle), glm::vec3(1, 0, 0));
            float s = 1.0f - t;
            model = glm::scale(model, glm::vec3(s, s, s));
        }
        // Hurt: just knockback, visible via position change (no tint for now)

        m_shader.loadModelMatrix(model);
        m_pigmanModel->bindVAO();
        glDrawElements(GL_TRIANGLES, m_pigmanModel->getIndicesCount(),
                       GL_UNSIGNED_INT, nullptr);
    }

    m_entities.clear();
}
```

- [ ] **Step 3: Build**

```bash
cd "D:\Yuga_Kshetra-The_Final_Dark_God\MineCraft-One-Week-Challenge"
VCPKG_ROOT="D:\Microsoft Visual Studio\2022\Community\VC\vcpkg" cmake --build "out/build/x64-Debug" --config Debug
```
Expected: compiles. May need to add `#include "../Model.h"` to EntityRenderer.h (already included).

---

### Task 5: World — Entity Management

**Files:**
- Modify: `Source/World/World.h`
- Modify: `Source/World/World.cpp`

- [ ] **Step 1: Add to World.h**

Add includes at top:
```cpp
#include "../Entity/PigmanEntity.h"
```

Add public methods after `getDropItems()`:
```cpp
    void updateEntities(float dt, Player& player);
    std::vector<PigmanEntity>& getPigmen() { return m_pigmen; }
    const Model* getPigmanModel() const { return m_pigmanModel.get(); }
```

Add private members after `m_dropItems`:
```cpp
    std::vector<PigmanEntity> m_pigmen;
    std::unique_ptr<Model> m_pigmanModel;
```

- [ ] **Step 2: Add to World.cpp — Constructor changes**

After `setSpawnPoint()` in the constructor body (line 20-21), add:
```cpp
    // Load shared pigman model
    m_pigmanModel = std::make_unique<Model>();
    // Model will be loaded via loadOBJ — actually load in EntityRenderer and pass pointer
    // For now, pigman model loaded separately via EntityRenderer.
    // Remove this unique_ptr; instead use raw pointer from EntityRenderer.

    // Initial spawn
    for (int i = 0; i < 8; i++) {
        spawnPigman(player, 15.0f);
    }
```

Wait — the model loading should happen once. Let's redesign: `EntityRenderer` owns the Model, and `World` stores pigmen that reference it. At spawn time, World creates PigmanEntity with `model` set to nullptr. World gets a `setPigmanModel(const Model* m)` call from Application.

Actually, simplest approach: `EntityRenderer` owns the Model. On each frame, World passes its pigmen list to EntityRenderer for rendering. The `model` pointer in PigmanEntity isn't needed (EntityRenderer only uses position/rotation/state).

Let me revise the PigmanEntity to drop the `model` pointer and store a simple pointer on the render side.

Revised PigmanEntity.h (remove model line):
```cpp
    // Remove: const Model* model = nullptr;
```

Revised approach:
- EntityRenderer owns m_pigmanModel
- World::updateEntities() handles AI + physics for each pigman
- Application calls entityRenderer.addEntity() for each pigman before rendering

Let me rewrite these task steps clearly:

- [ ] **Step 1: Add to World.h**

```cpp
// Add include at top:
#include "../Entity/PigmanEntity.h"

// Add public methods:
    void spawnPigman(const Player& player, float minDist);
    void updateEntities(float dt, Player& player);
    std::vector<PigmanEntity>& getPigmen() { return m_pigmen; }

// Add private member:
    std::vector<PigmanEntity> m_pigmen;
```

- [ ] **Step 2: Implement spawnPigman and updateEntities in World.cpp**

Add include at top:
```cpp
#include "../Entity/PigmanAI.h"
```

Add after `getDropItems()` implementation:

```cpp
void World::spawnPigman(const Player& player, float minDist)
{
    PigmanEntity e;
    glm::vec3 spawnPos;
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
            if (block.id != 0 && block.getData().isCollidable) {
                gy = y + 1;
                break;
            }
        }
        if (gy < 0) continue;

        // Check body space
        auto body = getBlock(gx, gy, gz);
        auto head = getBlock(gx, gy + 1, gz);
        if (body.getData().isCollidable || head.getData().isCollidable) continue;

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

void World::updateEntities(float dt, Player& player)
{
    for (auto& e : m_pigmen) {
        if (e.state == PigmanEntity::Dead) {
            e.respawnTimer -= dt;
            if (e.respawnTimer <= 0.0f) {
                // Respawn: find new position far from player
                glm::vec3 oldPos = e.position;
                *this; // dummy to capture world ref
                // Re-init the entity
                glm::vec3 spawnPos;
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
                    auto body = getBlock((int)x, gy, (int)z);
                    auto head = getBlock((int)x, gy + 1, (int)z);
                    if (body.getData().isCollidable || head.getData().isCollidable) continue;
                    spawnPos = glm::vec3(x + 0.5f, (float)gy, z + 0.5f);
                    found = true;
                    break;
                }
                if (found) {
                    e = PigmanEntity{};
                    e.position = spawnPos;
                    e.patrolOrigin = spawnPos;
                    e.stuckPosition = spawnPos;
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
        bool onGround = false;
        int bx = (int)e.position.x;
        int by = (int)(e.position.y - 0.125f);
        int bz = (int)e.position.z;
        auto below = getBlock(bx, by, bz);
        if (below.id != 0 && below.getData().isCollidable) {
            e.position.y = (float)by + 1.0f + 0.125f;
            e.velocity.y = 0;
            onGround = true;
        } else {
            e.velocity.y -= 40.0f * dt;
        }

        // Apply velocity + simple block collision
        e.position.x += e.velocity.x * dt;
        e.position.y += e.velocity.y * dt;
        e.position.z += e.velocity.z * dt;

        // Block collision (per-axis, same pattern as Player::collide)
        for (int dx = -1; dx <= 1; dx++)
        for (int dy = -1; dy <= 2; dy++)
        for (int dz = -1; dz <= 1; dz++) {
            int cx = (int)e.position.x + dx;
            int cy = (int)e.position.y + dy;
            int cz = (int)e.position.z + dz;
            auto block = getBlock(cx, cy, cz);
            if (block.id != 0 && block.getData().isCollidable) {
                // Simple push-out: stop velocity in blocked direction
                if (e.velocity.x > 0 && dx > 0) { e.position.x = cx - e.box.dimensions.x; e.velocity.x = 0; }
                if (e.velocity.x < 0 && dx < 0) { e.position.x = cx + 1.0f + e.box.dimensions.x; e.velocity.x = 0; }
                if (e.velocity.z > 0 && dz > 0) { e.position.z = cz - e.box.dimensions.z; e.velocity.z = 0; }
                if (e.velocity.z < 0 && dz < 0) { e.position.z = cz + 1.0f + e.box.dimensions.z; e.velocity.z = 0; }
            }
        }

        e.box.update(e.position);

        // Damping
        e.velocity.x *= 0.9f;
        e.velocity.z *= 0.9f;

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
                player.takeDamage(5, knockDir);
                e.attackCooldown = 1.5f;
            }
        }

        // Clamp to world bounds
        if (e.position.x < 0) e.position.x = 0;
        if (e.position.x >= MVP_WORLD_SIZE_X) e.position.x = MVP_WORLD_SIZE_X - 1;
        if (e.position.z < 0) e.position.z = 0;
        if (e.position.z >= MVP_WORLD_SIZE_Z) e.position.z = MVP_WORLD_SIZE_Z - 1;
        if (e.position.y < 0) e.position.y = 1;
    }
}
```

- [ ] **Step 3: Build**

```bash
cd "D:\Yuga_Kshetra-The_Final_Dark_God\MineCraft-One-Week-Challenge"
VCPKG_ROOT="D:\Microsoft Visual Studio\2022\Community\VC\vcpkg" cmake --build "out/build/x64-Debug" --config Debug
```
Expected: fails — `Player::takeDamage()` doesn't exist yet. That's Task 6.

---

### Task 6: Player — HP, Attack, Damage, Death

**Files:**
- Modify: `Source/Player/Player.h`
- Modify: `Source/Player/Player.cpp`

- [ ] **Step 1: Add to Player.h**

Add public members after `triggerSwing()`:
```cpp
    // Combat
    int m_hp = 100;
    int m_maxHp = 100;
    int m_baseAttack = 2;
    int getAttackPower() const;
    void takeDamage(int amount, glm::vec3 knockbackDir);

    // Death
    bool m_isDead = false;
```

- [ ] **Step 2: Implement in Player.cpp**

Add after `renderWeapon()` (at end of file):

```cpp
int Player::getAttackPower() const
{
    int weaponAtk = 0;
    const auto& eqM = m_equipment[m_equipSlot].getMaterial();
    if (eqM.id == Material::ID::WoodenSword) {
        weaponAtk = 10;
    }
    return m_baseAttack + weaponAtk;
}

void Player::takeDamage(int amount, glm::vec3 knockbackDir)
{
    if (m_isDead) return;

    m_hp -= amount;
    if (m_hp < 0) m_hp = 0;

    // Knockback
    velocity.x += knockbackDir.x * 6.0f;
    velocity.z += knockbackDir.z * 6.0f;
    if (!m_isFlying) velocity.y += 4.0f;

    if (m_hp <= 0) {
        m_isDead = true;
        // Clear inventory (except safe slots, which are first 3)
        // For MVP, clear all non-safe slots
        for (int i = 3; i < 20; i++) {
            m_items[i] = ItemStack(Material::NOTHING, 0);
        }
        for (int i = 0; i < 2; i++) {
            m_equipment[i] = ItemStack(Material::NOTHING, 0);
        }
        std::cout << "Player died! HP=0\n";
    }
}
```

- [ ] **Step 3: Build**

```bash
cd "D:\Yuga_Kshetra-The_Final_Dark_God\MineCraft-One-Week-Challenge"
VCPKG_ROOT="D:\Microsoft Visual Studio\2022\Community\VC\vcpkg" cmake --build "out/build/x64-Debug" --config Debug
```
Expected: compiles.

---

### Task 7: RenderMaster — Add EntityRenderer

**Files:**
- Modify: `Source/Renderer/RenderMaster.h`
- Modify: `Source/Renderer/RenderMaster.cpp`

- [ ] **Step 1: Add to RenderMaster.h**

Add include and public method:
```cpp
#include "EntityRenderer.h"

// In public section:
    EntityRenderer m_entityRenderer;
```

- [ ] **Step 2: Add entity render pass in RenderMaster.cpp**

In `finishRender()`, after flora renderer and before skybox:
```cpp
void RenderMaster::finishRender(sf::Window &window, const Camera &camera)
{
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    m_chunkRenderer.render(camera);
    m_waterRenderer.render(camera);
    m_floraRenderer.render(camera);
    m_entityRenderer.render(camera);   // NEW

    if (m_drawBox) {
        glDisable(GL_CULL_FACE);
        m_skyboxRenderer.render(camera);
        m_drawBox = false;
    }
}
```

- [ ] **Step 3: Build**

```bash
cd "D:\Yuga_Kshetra-The_Final_Dark_God\MineCraft-One-Week-Challenge"
VCPKG_ROOT="D:\Microsoft Visual Studio\2022\Community\VC\vcpkg" cmake --build "out/build/x64-Debug" --config Debug
```
Expected: compiles.

---

### Task 8: Application — Combat Detection + Game Loop Wiring

**Files:**
- Modify: `Source/Application.h`
- Modify: `Source/Application.cpp`

- [ ] **Step 1: Revise Application.cpp — on_update combat detection**

Replace the left-click handling block (currently lines 38-105) with:

```cpp
void Application::on_update(const Keyboard& keyboard, sf::Time dt)
{
    float delta = dt.asSeconds();
    m_player.handleInput(m_window, keyboard);
    glm::vec3 lastPosition;

    bool leftPressed = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
    bool leftClicked = !m_prevLeftPressed && leftPressed;
    m_prevLeftPressed = leftPressed;
    bool rightPressed = sf::Mouse::isButtonPressed(sf::Mouse::Button::Right);

    // Skip interaction when backpack is open
    if (!sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LAlt) && !m_player.isBackpackOpen())
    {
        // --- Left-click: pigman attack first, then mining ---
        if (leftClicked && !m_player.m_isDead)
        {
            bool hitPigman = false;

            // Raycast for pigman
            Ray ray({m_player.position.x, m_player.position.y + 0.6f, m_player.position.z},
                     m_player.rotation);
            for (; ray.getLength() < 6; ray.step(0.05f))
            {
                // Check pigman first
                for (auto& e : m_world.getPigmen())
                {
                    if (e.state == PigmanEntity::Dead) continue;

                    glm::vec3 eMin = e.box.position - e.box.dimensions;
                    glm::vec3 eMax = e.box.position + e.box.dimensions;
                    glm::vec3 rp = ray.getEnd();

                    if (rp.x >= eMin.x && rp.x <= eMax.x &&
                        rp.y >= eMin.y && rp.y <= eMax.y &&
                        rp.z >= eMin.z && rp.z <= eMax.z)
                    {
                        // Hit!
                        int dmg = m_player.getAttackPower();
                        e.hp -= dmg;
                        m_player.triggerSwing();

                        // Knockback
                        glm::vec3 kb = glm::normalize(e.position - m_player.position);
                        kb.y = 0;
                        if (glm::length(kb) < 0.01f) kb = glm::vec3(0, 0, -1);
                        e.velocity.x += kb.x * 6.0f;
                        e.velocity.z += kb.z * 6.0f;

                        if (e.hp <= 0) {
                            e.hp = 0;
                            e.state = PigmanEntity::Dead;
                            e.deathAnimTimer = 0.0f;
                            e.respawnTimer = 10.0f + (float)(std::rand() % 11);

                            // Drops: 5x independent rolls
                            glm::vec3 dropPos(e.position.x, e.position.y + 0.75f, e.position.z);
                            auto pushDrop = [&](const Material& mat) {
                                ItemDropEntity d;
                                d.position = dropPos;
                                d.velocity = glm::vec3(0.0f);
                                d.material = &mat;
                                d.alive = true;
                                m_world.getDropItems().push_back(d);
                            };
                            for (int r = 0; r < 5; r++) {
                                if (std::rand() % 100 < 40)
                                    pushDrop(Material::RAW_MEAT);
                                if (std::rand() % 100 < 30)
                                    pushDrop(Material::STICK);
                            }
                        } else {
                            e.state = PigmanEntity::Hurt;
                            e.hurtTimer = 0.3f;
                        }

                        hitPigman = true;
                        break;
                    }
                }
                if (hitPigman) break;
            }

            // If no pigman hit, try mining
            if (!hitPigman)
            {
                for (Ray ray2({m_player.position.x, m_player.position.y + 0.6f, m_player.position.z},
                              m_player.rotation);
                     ray2.getLength() < 6; ray2.step(0.05f))
                {
                    int x = static_cast<int>(ray2.getEnd().x);
                    int y = static_cast<int>(ray2.getEnd().y);
                    int z = static_cast<int>(ray2.getEnd().z);

                    auto block = m_world.getBlock(x, y, z);
                    auto id = (BlockId)block.id;

                    if (id != BlockId::Air && id != BlockId::Water)
                    {
                        m_player.m_isMining = true;
                        m_player.m_miningProgress = 0.0f;
                        m_player.m_miningTarget = {x, y, z};
                        break;
                    }
                }
            }
        }

        // Mining progress
        if (m_player.m_isMining && m_player.m_miningProgress < 1.0f)
        {
            m_player.m_miningProgress += delta / 0.3f;
            if (m_player.m_miningProgress >= 1.0f)
            {
                m_player.m_miningProgress = 1.0f;
                m_player.m_isMining = false;
                if (m_player.m_miningTarget.y > -999)
                {
                    m_world.addEvent<PlayerDigEvent>(sf::Mouse::Button::Left,
                        glm::vec3(m_player.m_miningTarget.x + 0.5f,
                                  m_player.m_miningTarget.y + 0.5f,
                                  m_player.m_miningTarget.z + 0.5f), m_player);
                    m_player.m_miningTarget = {0, -999, 0};
                }
            }
        }

        // Right-click: place block
        if (rightPressed && m_rightClickTimer.getElapsedTime().asSeconds() > 0.2f)
        {
            // Raycast for placement
            for (Ray ray({m_player.position.x, m_player.position.y + 0.6f, m_player.position.z},
                         m_player.rotation);
                 ray.getLength() < 6; ray.step(0.05f))
            {
                int x = static_cast<int>(ray.getEnd().x);
                int y = static_cast<int>(ray.getEnd().y);
                int z = static_cast<int>(ray.getEnd().z);
                auto block = m_world.getBlock(x, y, z);
                if (block.id != 0 && block.id != (int)BlockId::Water)
                {
                    m_rightClickTimer.restart();
                    m_world.addEvent<PlayerDigEvent>(sf::Mouse::Button::Right, lastPosition, m_player);
                    break;
                }
                lastPosition = ray.getEnd();
            }
        }
    }

    m_camera.update();
    m_player.update(delta, m_world);

    // Update entities (pigman AI + physics)
    m_world.updateEntities(delta, m_player);

    m_world.update(m_camera, delta);

    // Player death handling
    if (m_player.m_isDead && m_player.m_hp <= 0) {
        // Respawn player at world center
        m_player.m_hp = 100;
        m_player.m_isDead = false;
        m_player.position = {64.0f, 35.0f, 64.0f};
        m_player.velocity = {0, 0, 0};
    }

    // Auto-pickup
    {
        auto& drops = m_world.getDropItems();
        for (auto& drop : drops)
        {
            if (!drop.alive) continue;
            if (glm::distance(m_player.position, drop.position) < 2.0f)
            {
                if (m_player.addItem(*drop.material))
                {
                    drop.alive = false;
                }
            }
        }
    }

    // Clamp player to MVP world bounds
    if (m_player.position.x < 0) { m_player.position.x = 0; m_player.velocity.x = 0; }
    if (m_player.position.x >= MVP_WORLD_SIZE_X - 1) { m_player.position.x = static_cast<float>(MVP_WORLD_SIZE_X - 1); m_player.velocity.x = 0; }
    if (m_player.position.z < 0) { m_player.position.z = 0; m_player.velocity.z = 0; }
    if (m_player.position.z >= MVP_WORLD_SIZE_Z - 1) { m_player.position.z = static_cast<float>(MVP_WORLD_SIZE_Z - 1); m_player.velocity.z = 0; }
    if (m_player.position.y < 0) { m_player.position.y = 1; }
}
```

- [ ] **Step 2: Update on_render to pass entities to renderer**

In `Application::on_render()`:

```cpp
void Application::on_render(bool show_debug_info)
{
    m_player.setDropItems(&m_world.getDropItems());
    m_player.draw(m_masterRenderer, &m_camera);

    m_world.renderWorld(m_masterRenderer, m_camera);

    // Add entities to renderer
    for (auto& e : m_world.getPigmen()) {
        m_masterRenderer.m_entityRenderer.addEntity(e);
    }

    m_masterRenderer.finishRender(m_window, m_camera);

    m_player.renderWeapon();
}
```

- [ ] **Step 3: Add includes to Application.cpp**

Add at top of Application.cpp:
```cpp
#include "Entity/PigmanEntity.h"
```

- [ ] **Step 4: Build and test**

```bash
cd "D:\Yuga_Kshetra-The_Final_Dark_God\MineCraft-One-Week-Challenge"
VCPKG_ROOT="D:\Microsoft Visual Studio\2022\Community\VC\vcpkg" cmake --build "out/build/x64-Debug" --config Debug
```

Expected: compiles.

- [ ] **Step 5: Run and test**

```bash
cd "D:\Yuga_Kshetra-The_Final_Dark_God\MineCraft-One-Week-Challenge"
./out/build/x64-Debug/Debug/minecraft-one-week-challenge.exe
```

Verify:
- [ ] 8 pigmen visible in world, rendered as 3D OBJ models
- [ ] Pigmen patrol (random walk) when player is far
- [ ] Pigmen chase player when within 12 blocks
- [ ] Pigmen attack (deal damage) when within 2 blocks
- [ ] Player left-click attacks pigman within 5 blocks (instant)
- [ ] Player left-click mines blocks when no pigman hit
- [ ] Pigman hurt → knockback + stun
- [ ] Pigman death → drop items + respawn after 10-20s
- [ ] Player death (HP ≤ 0) → inventory clear + respawn
- [ ] No crash, no regression on existing features

---

### Task 9: Bugfix pass + polish

After initial playtest, fix issues. Common areas:
- A* pathfinding performance on 128×128 grid (pre-compute walkable surface)
- Pigman clipping through blocks (tune collision radii)
- OBJ coordinate system mismatch (swap Y/Z, flip normals)
- Drop items from pigman not appearing (check `toBlockID()` for `RawMeat`/`Stick`)
- Animation timing feels off (tune bob/attack/death curves)
