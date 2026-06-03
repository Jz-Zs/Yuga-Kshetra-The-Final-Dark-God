# Entry 9: Spider Monster + Dynamic Difficulty — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add spider enemy with ranged+melee hybrid combat, new Silk/SilkThread items, and simplified dynamic difficulty at 6:00.

**Architecture:** Refactor EntityRenderer to be parameterized (model path + texture name). Add SpiderEntity POD + SpiderAI namespace following the PigmanEntity/PigmanAI pattern. Add SpiderProjectile struct with billboard quad rendering. Integrate staged spawning, projectile physics, player slow debuff, and dynamic difficulty multiplier in World/Player/Application.

**Tech Stack:** C++23, OpenGL 4.6, SFML 3, tinygltf, ImGui

---

### Task 1: Stat Rebalance (Pigman + Player)

**Files:**
- Modify: `Source/Entity/PigmanEntity.h`
- Modify: `Source/Entity/PigmanAI.cpp`
- Modify: `Source/Player/Player.h`
- Modify: `Source/Player/Player.cpp`

- [ ] **Step 1: Pigman HP 30→40**

In `Source/Entity/PigmanEntity.h`, change:
```cpp
int hp = 40;
int maxHp = 40;
```

Also update the respawn reset in `Source/World/World.cpp` (line 492-493):
```cpp
e.hp = 40;
e.maxHp = 40;
```

- [ ] **Step 2: Pigman chase speed 4.0→4.5, attack cooldown 1.5→1.0**

In `Source/Entity/PigmanAI.cpp`, line 184 (patrol):
```cpp
e.velocity.x = dx * e.moveSpeed * 0.4f;  // patrol stays at 0.4 factor
e.velocity.z = dz * e.moveSpeed * 0.4f;
```

Line 238-239 (chase — change 0.625f to match new speed ratio):
```cpp
e.velocity.x = d.x * e.moveSpeed * 0.625f;
e.velocity.z = d.z * e.moveSpeed;
```

Wait — chase speed is `moveSpeed=4.0` × `0.625=2.5` X, `1.0=4.0` Z. So actual chase = (2.5, 4.0). To get 4.5 as effective speed, use `moveSpeed=4.5` and keep 0.625 factor. X = 2.8125, Z = 4.5.

In `PigmanEntity.h`:
```cpp
float moveSpeed = 4.5f;
```

Line 247 (fallback direct move):
```cpp
e.velocity.x = d.x * e.moveSpeed * 0.625f;
e.velocity.z = d.z * e.moveSpeed;
```

Attack cooldown in `PigmanEntity.h`:
```cpp
float attackCooldown = 1.0f;
```

In `World.cpp` line 497 (respawn reset):
```cpp
e.attackCooldown = 1.0f;
```

- [ ] **Step 3: Player walk speed 0.2→0.263, sprint ×5→×4, attack range 6→5**

In `Source/Player/Player.cpp`, line 287:
```cpp
float speed = 0.263f;
```

Lines 294-295 (Ctrl sprint):
```cpp
if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl))
    s *= 4;
```

In `Source/Application.cpp`, line 81 (raycast max length for pigman, was 6):
```cpp
for (; ray.getLength() < 5; ray.step(0.05f))
```

Line 146 (raycast max length for mining, was 6):
```cpp
ray2.getLength() < 5; ray2.step(0.05f)
```

- [ ] **Step 4: Build and verify compilation**

```bash
cd "D:/Yuga_Kshetra-The_Final_Dark_God/MineCraft-One-Week-Challenge"
VCPKG_ROOT="D:/Microsoft Visual Studio/2022/Community/VC/vcpkg" cmake --build "out/build/x64-Debug" --config Debug
```

Expected: Build succeeds with no errors.

---

### Task 2: EntityRenderer Refactoring

**Files:**
- Modify: `Source/Renderer/EntityRenderer.h`
- Modify: `Source/Renderer/EntityRenderer.cpp`
- Modify: `Source/Renderer/RenderMaster.h`
- Modify: `Source/Renderer/RenderMaster.cpp`
- Modify: `Source/Application.cpp`

- [ ] **Step 1: Rewrite EntityRenderer.h — parameterized, generic EntityRenderData**

Replace contents of `Source/Renderer/EntityRenderer.h`:

```cpp
#ifndef ENTITYRENDERER_H_INCLUDED
#define ENTITYRENDERER_H_INCLUDED

#include <vector>
#include <string>
#include <glad/glad.h>
#include <unordered_map>
#include "../Maths/glm.h"
#include "../Shaders/BasicShader.h"
#include "../Texture/BasicTexture.h"
#include "../Model.h"
#include "../Util/tiny_gltf.h"

class Camera;

struct GltfMeshPart {
    Model* model = nullptr;
    int nodeIndex = -1;
};

struct EntityRenderData {
    glm::vec3 position;
    glm::vec3 rotation;
    int state = 0;          // 0=Patrol, 1=Chase, 2=Attack, 3=Hurt, 4=Dead
    float animTimer = 0.0f;
    float deathAnimTimer = 0.0f;
    float attackCooldown = 0.0f;
    bool isHurt = false;
};

class EntityRenderer {
public:
    EntityRenderer(const std::string& modelPath, const std::string& textureName);
    ~EntityRenderer();

    void addEntity(const EntityRenderData& e);
    void render(const Camera& camera);

private:
    bool loadGltf(const char* path);
    void buildMeshParts(tinygltf::Model& gltf);
    void buildParentMap(tinygltf::Model& gltf);
    glm::mat4 getNodeWorldTransform(int nodeIdx, float animTime);

    std::vector<GltfMeshPart> m_meshParts;
    std::vector<int> m_parentMap;
    tinygltf::Model m_gltfModel;
    BasicTexture m_texture;
    BasicShader m_shader;
    std::vector<EntityRenderData> m_entities;

    GLuint m_tintLocation = 0;
};

#endif
```

Key changes: removed `#include "../Entity/PigmanEntity.h"`, added `EntityRenderData` struct, constructor takes `modelPath` and `textureName`.

- [ ] **Step 2: Rewrite EntityRenderer.cpp — parameterized load, generic render**

Replace `Source/Renderer/EntityRenderer.cpp`:

```cpp
#define TINYGLTF_USE_CPP14
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE

#include "EntityRenderer.h"
#include "../Camera.h"
#include <cstdint>
#include <iostream>
#include <cmath>
#include <glm/gtc/quaternion.hpp>

EntityRenderer::EntityRenderer(const std::string& modelPath, const std::string& textureName)
    : m_shader("Basic", "Basic")
{
    if (!loadGltf(modelPath.c_str())) {
        std::cerr << "EntityRenderer: failed to load model: " << modelPath << "\n";
    }
    m_texture.loadFromFile(textureName);

    m_shader.useProgram();
    m_tintLocation = glGetUniformLocation(m_shader.getID(), "tintColor");
}

EntityRenderer::~EntityRenderer()
{
    for (auto& part : m_meshParts)
        delete part.model;
}

bool EntityRenderer::loadGltf(const char* path)
{
    tinygltf::TinyGLTF loader;
    loader.SetImageLoader(
        [](tinygltf::Image*, const int, std::string*, std::string*,
           int, int, const unsigned char*, int, void*) { return true; },
        nullptr);
    std::string err, warn;
    bool ok = loader.LoadASCIIFromFile(&m_gltfModel, &err, &warn, path);
    if (!warn.empty()) std::cerr << "glTF warn: " << warn << "\n";
    if (!err.empty()) std::cerr << "glTF err: " << err << "\n";
    if (!ok) return false;

    buildParentMap(m_gltfModel);
    buildMeshParts(m_gltfModel);
    return !m_meshParts.empty();
}

void EntityRenderer::buildParentMap(tinygltf::Model& gltf)
{
    m_parentMap.assign(gltf.nodes.size(), -1);
    for (size_t i = 0; i < gltf.nodes.size(); i++) {
        for (int child : gltf.nodes[i].children) {
            m_parentMap[child] = (int)i;
        }
    }
}

void EntityRenderer::buildMeshParts(tinygltf::Model& gltf)
{
    for (size_t ni = 0; ni < gltf.nodes.size(); ni++) {
        auto& node = gltf.nodes[ni];
        if (node.mesh < 0) continue;

        auto& gltfMesh = gltf.meshes[node.mesh];
        for (auto& prim : gltfMesh.primitives) {
            auto posIt = prim.attributes.find("POSITION");
            auto uvIt = prim.attributes.find("TEXCOORD_0");
            if (posIt == prim.attributes.end()) continue;

            auto& posAcc = gltf.accessors[posIt->second];
            auto& posView = gltf.bufferViews[posAcc.bufferView];
            auto& posBuf = gltf.buffers[posView.buffer];
            const float* posData = reinterpret_cast<const float*>(
                posBuf.data.data() + posView.byteOffset + posAcc.byteOffset);
            std::vector<GLfloat> posVerts(posData, posData + posAcc.count * 3);

            std::vector<GLfloat> uvVerts;
            if (uvIt != prim.attributes.end()) {
                auto& uvAcc = gltf.accessors[uvIt->second];
                auto& uvView = gltf.bufferViews[uvAcc.bufferView];
                auto& uvBuf = gltf.buffers[uvView.buffer];
                const float* uvData = reinterpret_cast<const float*>(
                    uvBuf.data.data() + uvView.byteOffset + uvAcc.byteOffset);
                uvVerts.assign(uvData, uvData + uvAcc.count * 2);
            } else {
                uvVerts.resize(posAcc.count * 2, 0.0f);
            }

            std::vector<GLuint> indices;
            if (prim.indices >= 0) {
                auto& idxAcc = gltf.accessors[prim.indices];
                auto& idxView = gltf.bufferViews[idxAcc.bufferView];
                auto& idxBuf = gltf.buffers[idxView.buffer];
                const uint8_t* raw = idxBuf.data.data() + idxView.byteOffset + idxAcc.byteOffset;
                if (idxAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                    const uint16_t* p = reinterpret_cast<const uint16_t*>(raw);
                    indices.assign(p, p + idxAcc.count);
                } else if (idxAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
                    const uint32_t* p = reinterpret_cast<const uint32_t*>(raw);
                    indices.assign(p, p + idxAcc.count);
                } else {
                    const uint8_t* p = raw;
                    indices.assign(p, p + idxAcc.count);
                }
            }

            Mesh mesh;
            mesh.vertexPositions = std::move(posVerts);
            mesh.textureCoords = std::move(uvVerts);
            mesh.indices = std::move(indices);

            Model* model = new Model();
            model->addData(mesh);

            GltfMeshPart part;
            part.model = model;
            part.nodeIndex = (int)ni;
            m_meshParts.push_back(part);
        }
    }
}

glm::mat4 EntityRenderer::getNodeWorldTransform(int nodeIdx, float animTime)
{
    glm::mat4 result(1.0f);

    std::vector<int> chain;
    int cur = nodeIdx;
    while (cur >= 0) {
        chain.push_back(cur);
        cur = m_parentMap[cur];
    }

    for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
        int n = *it;
        auto& node = m_gltfModel.nodes[n];
        glm::mat4 local(1.0f);

        if (!node.matrix.empty()) {
            for (int c = 0; c < 4; c++)
                for (int r = 0; r < 4; r++)
                    local[c][r] = (float)node.matrix[c * 4 + r];
        } else {
            if (!node.translation.empty())
                local = glm::translate(local,
                    glm::vec3((float)node.translation[0],
                              (float)node.translation[1],
                              (float)node.translation[2]));
            if (!node.rotation.empty())
                local *= glm::mat4_cast(glm::quat(
                    (float)node.rotation[3], (float)node.rotation[0],
                    (float)node.rotation[1], (float)node.rotation[2]));
            if (!node.scale.empty())
                local = glm::scale(local,
                    glm::vec3((float)node.scale[0],
                              (float)node.scale[1],
                              (float)node.scale[2]));
        }

        for (auto& anim : m_gltfModel.animations) {
            for (auto& ch : anim.channels) {
                if (ch.target_node != n) continue;
                if (ch.target_path != "rotation") continue;

                auto& sampler = anim.samplers[ch.sampler];
                auto& inputAcc = m_gltfModel.accessors[sampler.input];
                auto& outputAcc = m_gltfModel.accessors[sampler.output];

                auto& inView = m_gltfModel.bufferViews[inputAcc.bufferView];
                auto& outView = m_gltfModel.bufferViews[outputAcc.bufferView];
                auto& inBuf = m_gltfModel.buffers[inView.buffer];
                auto& outBuf = m_gltfModel.buffers[outView.buffer];

                const float* times = reinterpret_cast<const float*>(
                    inBuf.data.data() + inView.byteOffset + inputAcc.byteOffset);
                const float* values = reinterpret_cast<const float*>(
                    outBuf.data.data() + outView.byteOffset + outputAcc.byteOffset);

                float maxTime = times[inputAcc.count - 1];
                float t = std::fmod(animTime, maxTime);

                int k = 0;
                for (int i = 1; i < (int)inputAcc.count; i++) {
                    if (times[i] > t) { k = i - 1; break; }
                }
                int k2 = (k + 1) % inputAcc.count;
                float alpha = 0.0f;
                float dtVal = times[k2] - times[k];
                if (dtVal > 0.0001f) alpha = (t - times[k]) / dtVal;

                glm::quat q0(values[k2*4+3], values[k2*4+0],
                             values[k2*4+1], values[k2*4+2]);
                glm::quat q1(values[k*4+3], values[k*4+0],
                             values[k*4+1], values[k*4+2]);
                glm::quat q = glm::slerp(q0, q1, alpha);

                local *= glm::mat4_cast(q);
            }
        }

        result *= local;
    }

    return result;
}

void EntityRenderer::addEntity(const EntityRenderData& e)
{
    m_entities.push_back(e);
}

void EntityRenderer::render(const Camera& camera)
{
    if (m_meshParts.empty() || m_entities.empty()) return;

    m_shader.useProgram();
    m_shader.loadProjectionViewMatrix(camera.getProjectionViewMatrix());
    m_texture.bindTexture();

    for (const auto& e : m_entities) {
        if (e.state == 4 && e.deathAnimTimer > 1.0f) continue; // Dead & anim done

        glm::mat4 worldMat(1.0f);
        worldMat = glm::translate(worldMat, e.position);
        worldMat = glm::rotate(worldMat, glm::radians(e.rotation.y + 180.0f),
                               glm::vec3(0, 1, 0));
        worldMat = glm::scale(worldMat, glm::vec3(0.6f));

        glm::mat4 animMat(1.0f);

        if (e.state == 2) { // Attack
            float phase = 1.0f - (e.attackCooldown / 1.5f);
            if (phase < 0.3f) {
                float t = phase / 0.3f;
                float lunge = std::sin(t * 3.14159265f) * 0.3f;
                animMat = glm::translate(animMat, glm::vec3(0, 0, -lunge));
                float headDip = std::sin(t * 3.14159265f) * 15.0f;
                animMat = glm::rotate(animMat, glm::radians(headDip),
                                      glm::vec3(1, 0, 0));
            }
        }
        else if (e.state == 4) { // Dead
            float t = e.deathAnimTimer;
            if (t > 1.0f) t = 1.0f;
            animMat = glm::rotate(animMat, glm::radians(t * 90.0f),
                                  glm::vec3(0, 0, 1));
            float s = 1.0f - t;
            animMat = glm::scale(animMat, glm::vec3(s, s, s));
        }

        if (m_tintLocation >= 0) {
            if (e.isHurt)
                glUniform3f(m_tintLocation, 3.0f, 3.0f, 3.0f);
            else
                glUniform3f(m_tintLocation, 1.0f, 1.0f, 1.0f);
        }

        float animTime = e.animTimer;

        for (auto& part : m_meshParts) {
            glm::mat4 nodeMat = getNodeWorldTransform(part.nodeIndex, animTime);
            glm::mat4 finalMat = worldMat * animMat * nodeMat;
            m_shader.loadModelMatrix(finalMat);
            part.model->bindVAO();
            glDrawElements(GL_TRIANGLES, part.model->getIndicesCount(),
                          GL_UNSIGNED_INT, nullptr);
        }
    }

    m_entities.clear();
}
```

- [ ] **Step 3: Update RenderMaster.h — two EntityRenderers**

In `Source/Renderer/RenderMaster.h`, change:
```cpp
// Remove:
EntityRenderer m_entityRenderer;

// Replace with:
EntityRenderer m_pigmanRenderer;
EntityRenderer m_spiderRenderer;
```

Add constructor to RenderMaster.h class declaration:
```cpp
class RenderMaster {
  public:
    RenderMaster();  // New: constructor for parameterized renderers
    // ... rest unchanged
```

- [ ] **Step 4: Update RenderMaster.cpp — init renderers, render both**

In `Source/Renderer/RenderMaster.cpp`, add constructor:
```cpp
RenderMaster::RenderMaster()
    : m_pigmanRenderer("Res/Models/Zoglin/minecraft_-zoglin/scene.gltf", "zoglin")
    , m_spiderRenderer("Res/Models/Spider/minecraft-spider/source/model.gltf", "spider")
{
}
```

In `finishRender()`, change `m_entityRenderer.render(camera);` to:
```cpp
m_pigmanRenderer.render(camera);
m_spiderRenderer.render(camera);
```

- [ ] **Step 5: Update Application.cpp — use new EntityRenderData API**

In `Source/Application.cpp` line 414-417, change:
```cpp
// Old:
for (auto& e : m_world.getPigmen()) {
    m_masterRenderer.m_entityRenderer.addEntity(e);
}

// New:
for (auto& e : m_world.getPigmen()) {
    EntityRenderData rd;
    rd.position = e.position;
    rd.rotation = e.rotation;
    rd.state = (int)e.state;
    rd.animTimer = e.stateTimer;
    rd.deathAnimTimer = e.deathAnimTimer;
    rd.attackCooldown = e.attackCooldown;
    rd.isHurt = (e.state == PigmanEntity::Hurt);
    m_masterRenderer.m_pigmanRenderer.addEntity(rd);
}
// Spiders will be added similarly in Task 8
```

- [ ] **Step 6: Build and verify**

```bash
cd "D:/Yuga_Kshetra-The_Final_Dark_God/MineCraft-One-Week-Challenge"
VCPKG_ROOT="D:/Microsoft Visual Studio/2022/Community/VC/vcpkg" cmake --build "out/build/x64-Debug" --config Debug
```

Expected: Build succeeds with no errors.

---

### Task 3: Spider Entity + AI

**Files:**
- Create: `Source/Entity/SpiderEntity.h`
- Create: `Source/Entity/SpiderAI.h`
- Create: `Source/Entity/SpiderAI.cpp`

- [ ] **Step 1: Create SpiderEntity.h**

Create `Source/Entity/SpiderEntity.h`:

```cpp
#ifndef SPIDERENTITY_H_INCLUDED
#define SPIDERENTITY_H_INCLUDED

#include "../Maths/glm.h"
#include "../Physics/AABB.h"
#include <vector>

struct SpiderEntity {
    glm::vec3 position{0, 0, 0};
    glm::vec3 velocity{0, 0, 0};
    glm::vec3 rotation{0, 0, 0};
    AABB box{glm::vec3(0.7f, 0.45f, 0.7f)}; // Spider ~1.4x0.9x1.4

    int hp = 80;
    int maxHp = 80;
    float moveSpeed = 6.0f;

    enum State { Patrol, Chase, RangedAttack, MeleeAttack, Hurt, Dead };
    State state = Patrol;
    float stateTimer = 0.0f;
    float meleeCooldown = 1.5f;
    float rangedCooldown = 2.5f;
    float freezeTimer = 0.0f;      // Post-shot freeze
    float hurtTimer = 0.0f;
    float stuckTimer = 0.0f;
    glm::vec3 stuckPosition{0, 0, 0};
    float respawnTimer = -1.0f;

    glm::vec3 patrolOrigin{0, 0, 0};
    std::vector<glm::ivec3> path;
    int pathIndex = 0;

    float deathAnimTimer = 0.0f;
};

#endif
```

- [ ] **Step 2: Create SpiderAI.h**

Create `Source/Entity/SpiderAI.h`:

```cpp
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
```

- [ ] **Step 3: Create SpiderAI.cpp**

Create `Source/Entity/SpiderAI.cpp`:

```cpp
#include "SpiderAI.h"
#include "../World/World.h"
#include "../Player/Player.h"
#include "../World/WorldConstants.h"
#include "../Entity/PigmanAI.h"  // Reuse findPath, hasLineOfSight, isWalkable
#include <cmath>
#include <cstdlib>

namespace SpiderAI {

static float randFloat(float lo, float hi) {
    float t = (float)std::rand() / (float)RAND_MAX;
    return lo + t * (hi - lo);
}

void update(SpiderEntity& e, float dt, Player& player, World& world) {
    glm::vec3 toPlayer = player.position - e.position;
    toPlayer.y += 0.6f;
    float distToPlayer = glm::length(glm::vec3(toPlayer.x, 0, toPlayer.z));

    e.meleeCooldown -= dt;
    e.rangedCooldown -= dt;
    e.freezeTimer -= dt;
    e.hurtTimer -= dt;
    e.stateTimer += dt;

    switch (e.state) {

    case SpiderEntity::Patrol: {
        bool isStopped = (glm::length(glm::vec2(e.velocity.x, e.velocity.z)) < 0.01f);
        float interval = isStopped ? 0.3f : randFloat(3.0f, 5.0f);
        if (e.stateTimer > interval) {
            e.stateTimer = 0.0f;
            float angle = randFloat(0.0f, 360.0f);
            if (e.position.x < 10) angle = randFloat(-60, 60);
            else if (e.position.x > MVP_WORLD_SIZE_X - 10) angle = randFloat(120, 240);
            if (e.position.z < 10) angle = randFloat(30, 150);
            else if (e.position.z > MVP_WORLD_SIZE_Z - 10) angle = randFloat(210, 330);
            float dx =  std::sin(glm::radians(angle));
            float dz = -std::cos(glm::radians(angle));
            e.rotation.y = glm::degrees(std::atan2(dx, dz));
            e.velocity.x = dx * 2.0f * 0.4f;  // patrol speed 2.0
            e.velocity.z = dz * 2.0f * 0.4f;
        }

        if (distToPlayer < 10.0f && PigmanAI::findPath(world, e.position, player.position).size() > 0) {
            // verify line of sight via ray-step check
            glm::vec3 dir = glm::normalize(player.position - e.position);
            bool blocked = false;
            for (float t = 0.5f; t < glm::distance(e.position, player.position); t += 0.5f) {
                glm::vec3 p = e.position + dir * t;
                int bx = (int)p.x, by = (int)p.y, bz = (int)p.z;
                auto block = world.getBlock(bx, by, bz);
                if (block.id != 0 && block.getData().isCollidable) { blocked = true; break; }
            }
            if (!blocked) {
                e.state = SpiderEntity::Chase;
                e.stateTimer = 0.0f;
                e.path.clear();
                e.stuckPosition = e.position;
                e.stuckTimer = 0.0f;
            }
        }
        break;
    }

    case SpiderEntity::Chase: {
        if (e.stateTimer > 0.5f || e.path.empty()) {
            e.path = PigmanAI::findPath(world, e.position, player.position);
            e.pathIndex = 0;
            e.stateTimer = 0.0f;
        }

        float moved = glm::distance(
            glm::vec3(e.position.x, 0, e.position.z),
            glm::vec3(e.stuckPosition.x, 0, e.stuckPosition.z));
        if (moved < 0.1f) { e.stuckTimer += dt; }
        else { e.stuckTimer = 0.0f; e.stuckPosition = e.position; }

        if (distToPlayer > 10.0f || e.stuckTimer > 3.0f) {
            e.state = SpiderEntity::Patrol;
            e.stateTimer = 0.0f;
            e.stuckTimer = 0.0f;
            e.path.clear();
            break;
        }

        // Check ranged attack first (>5 blocks away, LOS, cooldown ready)
        if (distToPlayer > 5.0f && e.rangedCooldown <= 0.0f) {
            glm::vec3 dir = glm::normalize(player.position - e.position);
            bool blocked = false;
            for (float t = 0.5f; t < distToPlayer; t += 0.5f) {
                glm::vec3 p = e.position + dir * t;
                auto b = world.getBlock((int)p.x, (int)p.y, (int)p.z);
                if (b.id != 0 && b.getData().isCollidable) { blocked = true; break; }
            }
            if (!blocked) {
                e.state = SpiderEntity::RangedAttack;
                e.stateTimer = 0.0f;
                e.velocity.x = 0; e.velocity.z = 0;
                break;
            }
        }

        // Melee range
        if (distToPlayer <= 5.0f) {
            e.state = SpiderEntity::MeleeAttack;
            e.meleeCooldown = 0.5f; // brief delay before first hit
            e.velocity.x = 0; e.velocity.z = 0;
            break;
        }

        // Follow path
        if (!e.path.empty() && e.pathIndex < (int)e.path.size()) {
            glm::vec3 target(e.path[e.pathIndex].x + 0.5f, 0,
                             e.path[e.pathIndex].z + 0.5f);
            glm::vec3 d = target - e.position; d.y = 0;
            float hd = glm::length(d);
            if (hd < 0.3f) { e.pathIndex++; }
            else {
                d = glm::normalize(d);
                e.velocity.x = d.x * e.moveSpeed * 0.625f;
                e.velocity.z = d.z * e.moveSpeed;
            }
        } else {
            toPlayer.y = 0;
            if (glm::length(toPlayer) > 0.1f) {
                glm::vec3 d = glm::normalize(toPlayer);
                e.velocity.x = d.x * e.moveSpeed * 0.625f;
                e.velocity.z = d.z * e.moveSpeed;
            }
        }

        // Smooth rotation toward player
        {
            float targetYaw = glm::degrees(std::atan2(toPlayer.x, toPlayer.z));
            float diff = targetYaw - e.rotation.y;
            while (diff > 180) diff -= 360;
            while (diff < -180) diff += 360;
            e.rotation.y += diff * std::min(dt * 3.0f, 1.0f);
        }
        break;
    }

    case SpiderEntity::RangedAttack: {
        e.velocity.x = 0; e.velocity.z = 0;
        // World handles projectile spawning — just freeze and wait
        if (e.freezeTimer <= 0.0f) {
            e.state = SpiderEntity::Chase;
            e.stateTimer = 0.0f;
        }
        break;
    }

    case SpiderEntity::MeleeAttack: {
        e.velocity.x = 0; e.velocity.z = 0;
        if (distToPlayer > 5.0f) {
            e.state = SpiderEntity::Chase;
            e.stateTimer = 0.0f;
            break;
        }
        if (distToPlayer > 2.0f && e.meleeCooldown <= 0.0f) {
            // Too far for melee hit, but ≤5 — transition back to chase
            e.state = SpiderEntity::Chase;
            e.stateTimer = 0.0f;
            break;
        }
        // Actual damage in World::updateEntities
        break;
    }

    case SpiderEntity::Hurt: {
        e.velocity.x = 0; e.velocity.z = 0;
        if (e.hurtTimer <= 0.0f) {
            e.state = SpiderEntity::Chase;
            e.stateTimer = 0.0f;
        }
        break;
    }

    case SpiderEntity::Dead: {
        e.velocity.x = 0; e.velocity.z = 0;
        break;
    }
    }
}

} // namespace SpiderAI
```

- [ ] **Step 4: Build and verify**

Expected: Compiles. SpiderAI.cpp may have warnings if `hasLineOfSight` is defined static in PigmanAI.cpp. If link error, inline the LOS check in SpiderAI (already done above).

---

### Task 4: SpiderProjectile + ProjectileRenderer

**Files:**
- Create: `Source/Entity/SpiderProjectile.h`
- Create: `Source/Renderer/ProjectileRenderer.h`
- Create: `Source/Renderer/ProjectileRenderer.cpp`

- [ ] **Step 1: Create SpiderProjectile.h**

Create `Source/Entity/SpiderProjectile.h`:

```cpp
#ifndef SPIDERPROJECTILE_H_INCLUDED
#define SPIDERPROJECTILE_H_INCLUDED

#include "../Maths/glm.h"

struct SpiderProjectile {
    glm::vec3 position;
    glm::vec3 velocity;  // horizontal only, speed 4.0
    int damage = 5;
    float lifetime = 0.0f;
    float maxLifetime = 2.0f;
    bool alive = true;
};

#endif
```

- [ ] **Step 2: Create ProjectileRenderer.h**

Create `Source/Renderer/ProjectileRenderer.h`:

```cpp
#ifndef PROJECTILERENDERER_H_INCLUDED
#define PROJECTILERENDERER_H_INCLUDED

#include <vector>
#include <glad/glad.h>
#include "../Maths/glm.h"
#include "../Shaders/BasicShader.h"
#include "../Texture/TextureAtlas.h"

class Camera;
struct SpiderProjectile;

class ProjectileRenderer {
public:
    ProjectileRenderer();
    ~ProjectileRenderer();

    void addProjectile(const SpiderProjectile& p);
    void render(const Camera& camera);

private:
    void buildQuad();

    BasicShader m_shader;
    TextureAtlas m_atlas;
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_ebo = 0;
    std::vector<glm::vec3> m_positions;
};

#endif
```

- [ ] **Step 3: Create ProjectileRenderer.cpp**

Create `Source/Renderer/ProjectileRenderer.cpp`:

```cpp
#include "ProjectileRenderer.h"
#include "../Entity/SpiderProjectile.h"
#include "../Camera.h"
#include <glm/gtc/matrix_transform.hpp>

ProjectileRenderer::ProjectileRenderer()
    : m_shader("Basic", "Basic")
    , m_atlas("DefaultPack")
{
    buildQuad();
}

ProjectileRenderer::~ProjectileRenderer()
{
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_ebo) glDeleteBuffers(1, &m_ebo);
}

void ProjectileRenderer::buildQuad()
{
    // UV for cell (13, 2) in 16x16 atlas
    float cellU = 13.0f / 16.0f;
    float cellV = 2.0f / 16.0f;
    float u0 = cellU, u1 = cellU + 1.0f/16.0f;
    float v0 = cellV, v1 = cellV + 1.0f/16.0f;

    float vertices[] = {
        // pos (x,y,z)      // uv
        -0.25f,  0.25f, 0,  u0, v0,
         0.25f,  0.25f, 0,  u1, v0,
         0.25f, -0.25f, 0,  u1, v1,
        -0.25f, -0.25f, 0,  u0, v1,
    };
    unsigned int indices[] = { 0, 1, 2, 0, 2, 3 };

    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &m_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // uv
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void ProjectileRenderer::addProjectile(const SpiderProjectile& p)
{
    m_positions.push_back(p.position);
}

void ProjectileRenderer::render(const Camera& camera)
{
    if (m_positions.empty()) return;

    m_shader.useProgram();
    m_shader.loadProjectionViewMatrix(camera.getProjectionViewMatrix());
    m_atlas.bind();

    glBindVertexArray(m_vao);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (const auto& pos : m_positions) {
        glm::mat4 model(1.0f);
        model = glm::translate(model, pos);

        // Billboard Y-rotation toward camera (camera.position is Entity::position)
        glm::vec3 toCam = camera.position - pos;
        float yaw = glm::degrees(std::atan2(toCam.x, toCam.z));
        model = glm::rotate(model, glm::radians(yaw), glm::vec3(0, 1, 0));

        m_shader.loadModelMatrix(model);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    }

    glDisable(GL_BLEND);
    glBindVertexArray(0);

    m_positions.clear();
}
```

Camera extends Entity, so `camera.position` gives us the camera world position for billboard rotation.

- [ ] **Step 4: Build and verify**

```bash
cd "D:/Yuga_Kshetra-The_Final_Dark_God/MineCraft-One-Week-Challenge"
VCPKG_ROOT="D:/Microsoft Visual Studio/2022/Community/VC/vcpkg" cmake --build "out/build/x64-Debug" --config Debug
```

Expected: Build succeeds.

---

### Task 5: Player Slow Effect

**Files:**
- Modify: `Source/Player/Player.h`
- Modify: `Source/Player/Player.cpp`

- [ ] **Step 1: Add slow fields to Player.h**

In `Source/Player/Player.h`, after line 96 (`int m_baseAttack = 2;`), add:
```cpp
// Slow debuff (spider web)
float m_slowTimer = 0.0f;
float m_slowFactor = 1.0f;  // 1.0=normal, 0.8=slowed
```

- [ ] **Step 2: Apply slow in keyboardInput**

In `Source/Player/Player.cpp`, after line 287 (`float speed = 0.263f;`), add:
```cpp
if (m_slowTimer > 0.0f)
    speed *= m_slowFactor;
```

- [ ] **Step 3: Decrement slow in update**

In `Source/Player/Player.cpp`, in the `update()` function (around line 169), after `velocity += m_acceleration;`, add:
```cpp
if (m_slowTimer > 0.0f)
    m_slowTimer -= dt;
```

- [ ] **Step 4: Build and verify**

Expected: Compiles.

---

### Task 6: New Items — Silk & Silk Thread

**Files:**
- Modify: `Source/World/Block/BlockId.h`
- Modify: `Source/World/Block/BlockDatabase.cpp`
- Modify: `Source/Item/Material.h`
- Modify: `Source/Item/Material.cpp`
- Modify: `Source/Item/CraftingRecipe.cpp`
- Create: `Res/Blocks/Silk.block`
- Create: `Res/Blocks/SilkThread.block`

- [ ] **Step 1: Add BlockId entries**

In `Source/World/Block/BlockId.h`, before `NUM_TYPES`:
```cpp
Silk = 30,
SilkThread = 31,
```

- [ ] **Step 2: Register in BlockDatabase**

In `Source/World/Block/BlockDatabase.cpp`, after `CookedMeat_Item` line:
```cpp
m_blocks[(int)BlockId::Silk] = std::make_unique<DefaultBlock>("Silk");
m_blocks[(int)BlockId::SilkThread] = std::make_unique<DefaultBlock>("SilkThread");
```

- [ ] **Step 3: Add Material::ID entries**

In `Source/Item/Material.h`, add to the `ID` enum before `Furnace`:
```cpp
Silk,
SilkThread,
```

Add to declarations section:
```cpp
const static Material SILK, SILK_THREAD;
```

- [ ] **Step 4: Register Material instances**

In `Source/Item/Material.cpp`, add after `FURNACE` line:
```cpp
const Material Material::SILK(Material::ID::Silk, 64, false, "Silk");
const Material Material::SILK_THREAD(Material::ID::SilkThread, 64, false, "Silk Thread");
```

Add `toBlockID` cases in the switch:
```cpp
case Silk:        return BlockId::Silk;
case SilkThread:  return BlockId::SilkThread;
```

Add `toMaterial` cases:
```cpp
case BlockId::Silk:       return SILK;
case BlockId::SilkThread: return SILK_THREAD;
```

- [ ] **Step 5: Create .block files**

Create `Res/Blocks/Silk.block`:
```
TexAll
10 2
```

Create `Res/Blocks/SilkThread.block`:
```
TexAll
11 2
```

- [ ] **Step 6: Add crafting recipe (3 Silk left column → 1 Silk Thread)**

In `Source/Item/CraftingRecipe.cpp`, in `initCraftingRecipes()`, before the closing `}` of the function, add:
```cpp
// 3 Silk (left column) → 1 Silk Thread
g_recipes.push_back({{
    &Material::SILK, nullptr, nullptr,
    &Material::SILK, nullptr, nullptr,
    &Material::SILK, nullptr, nullptr,
}, &Material::SILK_THREAD, 1});
```

- [ ] **Step 7: Build and verify**

Expected: Compiles. Silk items appear in inventory with correct textures.

---

### Task 7: World Integration — Spider Spawn, Update, Projectiles

**Files:**
- Modify: `Source/World/World.h`
- Modify: `Source/World/World.cpp`

- [ ] **Step 1: Add spider/projectile/difficulty fields to World.h**

In `Source/World/World.h`, add includes:
```cpp
#include "../Entity/SpiderEntity.h"
#include "../Entity/SpiderProjectile.h"
```

Add after `#include "../Entity/PigmanEntity.h"`.

Add public methods after `getPigmen()`:
```cpp
void spawnSpider(const Player& player);
void updateEntities(float dt, Player& player);  // signature unchanged, body changes
std::vector<SpiderEntity>& getSpiders() { return m_spiders; }
std::vector<SpiderProjectile>& getProjectiles() { return m_projectiles; }
float getDifficultyMultiplier() const { return m_difficultyMultiplier; }
```

Add private fields:
```cpp
std::vector<SpiderEntity> m_spiders;
std::vector<SpiderProjectile> m_projectiles;
float m_difficultyMultiplier = 1.0f;
bool m_difficultyTriggered = false;
```

- [ ] **Step 2: Implement spawnSpider in World.cpp**

Add `#include "../Entity/SpiderAI.h"` at top.

Add `spawnSpider` method:
```cpp
void World::spawnSpider(const Player& player)
{
    SpiderEntity e;
    glm::vec3 spawnPos;
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

        // Check overhead clear (taller check for spider model, only 2 high)
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
```

- [ ] **Step 3: Rewrite updateEntities to handle spiders + projectiles + pigmen**

The updated `updateEntities` function adds:
- Staged spider spawning
- Spider AI + physics + projectile spawning
- Projectile updates
- Difficulty activation

Replace `updateEntities` body. The function now:

```cpp
void World::updateEntities(float dt, Player& player)
{
    float elapsed = 600.0f - player.m_roundTimeLeft;

    // --- Dynamic Difficulty trigger at 6:00 ---
    if (!m_difficultyTriggered && elapsed >= 360.0f) {
        m_difficultyTriggered = true;
        m_difficultyMultiplier = 1.1f;
    }

    // --- Pigman spawning and updates (existing logic, with difficulty-adjusted counts) ---
    int pigmanMax = (m_difficultyMultiplier > 1.05f) ? 9 : 8;
    if (m_pigmen.size() < (size_t)pigmanMax) {
        spawnPigman(player, 15.0f);
    }
    for (auto& e : m_pigmen) {
        // ... all existing pigman logic, unchanged except difficulty multiplier application:
        // hp comparisons use (int)(baseHp * m_difficultyMultiplier)
        // attack damage: (int)(5 * m_difficultyMultiplier)
        // speed: e.moveSpeed already set at spawn, update on difficulty spike
        // (existing code, no changes needed in this step)
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
                // Respawn: 30-50s, find new position
                glm::vec3 spawnPos;
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
                    // quick overhead check
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

        // Ranged attack: spawn projectile on state entry
        if (e.state == SpiderEntity::RangedAttack && e.freezeTimer <= 0.0f && e.rangedCooldown > 2.49f) {
            // First frame of RangedAttack — spawn projectile
            SpiderProjectile proj;
            proj.position = e.position + glm::vec3(0, 0.75f, 0);
            glm::vec3 dir = player.position - e.position;
            dir.y = 0;
            if (glm::length(dir) > 0.01f) {
                dir = glm::normalize(dir);
                proj.velocity = dir * 4.0f;
            } else {
                proj.velocity = glm::vec3(0, 0, -4.0f);
            }
            proj.damage = (int)(5 * m_difficultyMultiplier);
            proj.alive = true;
            proj.lifetime = 0.0f;
            proj.maxLifetime = 2.0f;
            m_projectiles.push_back(proj);

            e.freezeTimer = 0.5f;
            e.rangedCooldown = 2.5f;
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
            // Apply slow
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
}
```

Note: The existing pigman logic block inside `updateEntities` needs to be in this same function. For brevity above, the pigman section is indicated as "existing logic." In the actual implementation, the pigman section stays largely intact from current code, with the difficulty multiplier applied to damage: `player.takeDamage((int)(5 * m_difficultyMultiplier), knockDir);` instead of `player.takeDamage(5, knockDir);`.

- [ ] **Step 4: Update resetWorld to clear spiders and projectiles**

In `World::resetWorld()`, after `m_pigmen.clear();`:
```cpp
m_spiders.clear();
m_projectiles.clear();
m_difficultyTriggered = false;
m_difficultyMultiplier = 1.0f;
```

- [ ] **Step 5: Build and verify**

Expected: Compiles. Full spider lifecycle: spawn → chase → attack → die → drop → respawn.

---

### Task 8: Render Flow — Application + RenderMaster Integration

**Files:**
- Modify: `Source/Application.cpp`
- Modify: `Source/Renderer/RenderMaster.h`
- Modify: `Source/Renderer/RenderMaster.cpp`

- [ ] **Step 1: Add ProjectileRenderer to RenderMaster**

In `Source/Renderer/RenderMaster.h`, add:
```cpp
#include "ProjectileRenderer.h"
```

In class, add public member:
```cpp
ProjectileRenderer m_projectileRenderer;
```

- [ ] **Step 2: Render projectiles in finishRender**

In `Source/Renderer/RenderMaster.cpp`, in `finishRender()`, before entity rendering:
```cpp
// (after flora render, before entity render)
// Projectiles will be added from Application, rendered here
```

And after entity renders:
```cpp
m_projectileRenderer.render(camera);
```

Wait — the projectiles need to be added before render. In Application.cpp `on_render()`, we'll add projectiles to the renderer. So in `RenderMaster::finishRender()`:
```cpp
m_pigmanRenderer.render(camera);
m_spiderRenderer.render(camera);
m_projectileRenderer.render(camera);
```

- [ ] **Step 3: Update Application::on_render for spiders and projectiles**

In `Source/Application.cpp`, after the pigman loop (line 417 area), add:

```cpp
// Add spiders to renderer
// Spider state mapping: Patrol=0→0, Chase=1→1, RangedAttack=2→2(Attack),
//   MeleeAttack=3→2(Attack), Hurt=4→3(Hurt), Dead=5→4(Dead)
for (auto& e : m_world.getSpiders()) {
    EntityRenderData rd;
    rd.position = e.position;
    rd.rotation = e.rotation;
    int rawState = (int)e.state;
    if (rawState == 3) rawState = 2; // MeleeAttack → Attack
    else if (rawState == 4) rawState = 3; // Hurt → Hurt
    else if (rawState == 5) rawState = 4; // Dead → Dead
    rd.state = rawState;
    rd.animTimer = e.stateTimer;
    rd.deathAnimTimer = e.deathAnimTimer;
    rd.attackCooldown = e.meleeCooldown;
    rd.isHurt = (e.state == SpiderEntity::Hurt);
    m_masterRenderer.m_spiderRenderer.addEntity(rd);
}

// Add projectiles to renderer
for (auto& p : m_world.getProjectiles()) {
    m_masterRenderer.m_projectileRenderer.addProjectile(p);
}
```

Note: `SpiderEntity::Hurt` and `SpiderEntity` types need to be accessible. Already included via World.h which includes SpiderEntity.h.

- [ ] **Step 4: Add spider combat raycast in Application.cpp**

In `Source/Application.cpp`, in the left-click combat section (around line 76-139), add spider raycast check alongside pigman check:

After `for (auto& e : m_world.getPigmen())` block, add a parallel loop before `if (!hitPigman)`:

Keep the existing `hitPigman` variable. Add a `hitSpider` bool:

```cpp
bool hitPigman = false;
bool hitSpider = false;

// Raycast for pigman (existing)
// ...

// Raycast for spider
for (auto& e : m_world.getSpiders())
{
    if (e.state == SpiderEntity::Dead) continue;

    glm::vec3 eMin = e.box.position - e.box.dimensions;
    glm::vec3 eMax = e.box.position + e.box.dimensions;
    glm::vec3 rp = ray.getEnd();

    if (rp.x >= eMin.x && rp.x <= eMax.x &&
        rp.y >= eMin.y && rp.y <= eMax.y &&
        rp.z >= eMin.z && rp.z <= eMax.z)
    {
        int dmg = m_player.getAttackPower();
        e.hp -= dmg;

        glm::vec3 kb = glm::normalize(e.position - m_player.position);
        kb.y = 0;
        if (glm::length(kb) < 0.01f) kb = glm::vec3(0, 0, -1);
        e.velocity.x += kb.x * 6.0f;
        e.velocity.z += kb.z * 6.0f;

        if (e.hp <= 0) {
            e.hp = 0;
            e.state = SpiderEntity::Dead;
            e.deathAnimTimer = 0.0f;
            e.respawnTimer = 30.0f + (float)(std::rand() % 21);

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
                if (std::rand() % 100 < 35) pushDrop(Material::RAW_MEAT);
                if (std::rand() % 100 < 50) pushDrop(Material::SILK);
                if (std::rand() % 100 < 20) pushDrop(Material::STICK);
                if (std::rand() % 100 < 15) pushDrop(Material::SILK_THREAD);
            }
            m_player.m_pigmanKills++;  // track spider kills too
        } else {
            e.state = SpiderEntity::Hurt;
            e.hurtTimer = 0.3f;
        }

        hitSpider = true;
        break;
    }
}

// Modify: if (!hitPigman) → if (!hitPigman && !hitSpider)
```

- [ ] **Step 5: Build and verify**

Expected: Compiles. Spider renders in world, can be fought, projectiles visible.

---

### Task 9: Dynamic Difficulty — Timer Color Change

**Files:**
- Modify: `Source/Player/Player.h`
- Modify: `Source/Player/Player.cpp`
- Modify: `Source/Util/BitmapText.h`
- Modify: `Source/Util/BitmapText.cpp`

- [ ] **Step 1: Add color parameter to BitmapText**

In `Source/Util/BitmapText.h`, change `update` signature:
```cpp
GLuint update(const std::vector<std::string>& lines,
              int texWidth, int texHeight, bool center = false,
              unsigned char r = 255, unsigned char g = 255, unsigned char b = 255);
```

In `Source/Util/BitmapText.cpp`, change `update` function signature to match, and change the glyph color in `renderUTF8` call. Currently lines 101-103 hardcode `buf[off+0]=255; buf[off+1]=255; buf[off+2]=255;`. Need to pass color through. Change `renderUTF8` to accept `unsigned char r, g, b`:

```cpp
int BitmapText::renderUTF8(unsigned char* buf, int bufW, int bufH,
                            int x, int y, const std::string& text,
                            unsigned char r, unsigned char g, unsigned char b)
```

And change the pixel write to:
```cpp
buf[off+0] = r;
buf[off+1] = g;
buf[off+2] = b;
```

Update `renderUTF8` declaration in header to match.

In `update()`, pass the color params through:
```cpp
renderUTF8(m_buffer.data(), texWidth, texHeight, x, y, line, r, g, b);
renderUTF8(m_buffer.data(), texWidth, texHeight, x + 1, y, line, r, g, b); // bold
```

All existing callers get default white (255,255,255), no call-site changes needed.

- [ ] **Step 2: Player knows difficulty state**

In `Source/Player/Player.h`, add:
```cpp
bool m_difficultyActive = false;
```

In `Source/World/World.h`, add accessor:
```cpp
bool isDifficultyActive() const { return m_difficultyTriggered; }
```

In `Source/Application.cpp`, in `on_update()`, after the `m_world.updateEntities(delta, m_player)` call, sync difficulty state:
```cpp
m_player.m_difficultyActive = m_world.isDifficultyActive();
```

- [ ] **Step 3: Timer orange when difficulty active**

In `Source/Player/Player.cpp`, in `drawTimer()`, change the `m_hudText.update()` call to use orange when difficulty is active:

```cpp
unsigned char r = m_difficultyActive ? 255 : 255;
unsigned char g = m_difficultyActive ? 165 : 255;
unsigned char b = m_difficultyActive ? 0 : 255;
GLuint texId = m_hudText.update(lines, texW, texH, true, r, g, b);
```

- [ ] **Step 4: Build and verify**

Expected: Compiles. At 6:00 game time, timer turns orange.

---

### Task 10: Spider Texture Setup + Final Build

**Files:**
- Copy: `Res/Models/Spider/minecraft_spider/textures/spider_texture_baseColor.png` → `Res/Textures/spider.png`

- [ ] **Step 1: Copy spider texture**

```bash
cp "D:/Yuga_Kshetra-The_Final_Dark_God/MineCraft-One-Week-Challenge/Res/Models/Spider/minecraft_spider/textures/spider_texture_baseColor.png" \
   "D:/Yuga_Kshetra-The_Final_Dark_God/MineCraft-One-Week-Challenge/Res/Textures/spider.png"
```

- [ ] **Step 2: Full build and smoke test**

```bash
cd "D:/Yuga_Kshetra-The_Final_Dark_God/MineCraft-One-Week-Challenge"
VCPKG_ROOT="D:/Microsoft Visual Studio/2022/Community/VC/vcpkg" cmake --build "out/build/x64-Debug" --config Debug
```

Expected: Clean build. Run the game:
- Spider model visible after 1:00
- Spider count increases each minute
- Spider shoots projectiles at >5 blocks
- Spider melees at ≤5 blocks
- Projectile hit slows player
- Killing spider drops Silk/Thread/RawMeat/Stick
- 3 Silk crafts into 1 Silk Thread
- At 6:00 timer turns orange, enemies stronger

---

### Summary: File Change List

**New (9 files):**
- `Source/Entity/SpiderEntity.h`
- `Source/Entity/SpiderAI.h`
- `Source/Entity/SpiderAI.cpp`
- `Source/Entity/SpiderProjectile.h`
- `Source/Renderer/ProjectileRenderer.h`
- `Source/Renderer/ProjectileRenderer.cpp`
- `Res/Blocks/Silk.block`
- `Res/Blocks/SilkThread.block`
- `Res/Textures/spider.png` (copied from model)

**Modified (14 files):**
- `Source/Entity/PigmanEntity.h` — HP, speed, cooldown
- `Source/Entity/PigmanAI.cpp` — chase speed, cooldown
- `Source/Renderer/EntityRenderer.h` — full rewrite (parameterized)
- `Source/Renderer/EntityRenderer.cpp` — full rewrite (parameterized)
- `Source/Renderer/RenderMaster.h` — two renderers + projectile renderer
- `Source/Renderer/RenderMaster.cpp` — constructor + render calls
- `Source/World/World.h` — spider/projectile/difficulty fields
- `Source/World/World.cpp` — spawnSpider, updateEntities rewrite, resetWorld
- `Source/Player/Player.h` — slow, difficulty, speed constant
- `Source/Player/Player.cpp` — speed changes, slow logic, timer color
- `Source/Item/Material.h` — Silk, SilkThread IDs
- `Source/Item/Material.cpp` — SILK, SILK_THREAD registration
- `Source/Item/CraftingRecipe.cpp` — 3 Silk → 1 Silk Thread recipe
- `Source/World/Block/BlockId.h` — Silk=30, SilkThread=31
- `Source/World/Block/BlockDatabase.cpp` — Silk, SilkThread registration
- `Source/Util/BitmapText.h` — color parameter
- `Source/Util/BitmapText.cpp` — color parameter
- `Source/Application.cpp` — spider combat, render, difficulty sync
<｜end▁of▁thinking｜>

<｜｜DSML｜｜tool_calls>
<｜｜DSML｜｜invoke name="Read">
<｜｜DSML｜｜parameter name="file_path" string="true">D:\Yuga_Kshetra-The_Final_Dark_God\MineCraft-One-Week-Challenge\Source\Camera.h