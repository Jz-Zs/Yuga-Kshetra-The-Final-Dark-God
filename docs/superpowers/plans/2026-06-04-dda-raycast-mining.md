# DDA 射线检测 + 挖掘目标跟随修复 实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 用 Amanatides-Woo DDA 体素遍历替代点采样射线检测，修复方块挖掘错位 bug；同时修复挖掘目标不跟随准星的问题。

**Architecture:** 重写 `Maths/Ray` 类，构造函数接受玩家位置和 rotation（{pitch,yaw,0}），内部用球坐标计算归一化方向向量，`advance()` 方法按 DDA 精确遍历体素。`Application.cpp` 中4处射线循环全部改造：实体攻击用内联点采样（不依赖 Ray），挖掘/放置用 DDA。

**Tech Stack:** C++23, GLM, SFML 3.0

---

## 文件结构

| 文件 | 角色 | 改动 |
|------|------|------|
| `Maths/Ray.h` | DDA 射线类声明 | 重写 |
| `Maths/Ray.cpp` | DDA 算法实现 | 重写 |
| `Application.cpp` | 实体攻击/挖掘/放置的射线循环 | 改造4处循环 + 挖掘目标每帧跟随 |

---

### Task 1: 重写 Ray.h — DDA API

**Files:**
- Modify: `MineCraft-One-Week-Challenge/Source/Maths/Ray.h`

- [ ] **Step 1: 写入新 Ray.h**

```cpp
#ifndef RAY_H_INCLUDED
#define RAY_H_INCLUDED

#include <cmath>
#include "glm.h"

/// @brief DDA (Amanatides-Woo) voxel traversal ray.
/// Construct with origin + player rotation (pitch, yaw, _).
/// Call advance() to step to the next voxel boundary.
class Ray {
  public:
    Ray(const glm::vec3 &origin, const glm::vec3 &rotation);

    /// Advance to next voxel along the ray. Returns false if stalled
    /// (direction component zero — should not happen with valid input).
    bool advance();

    /// Current voxel coordinates (integer block position).
    glm::ivec3 currentVoxel() const { return m_voxel; }

    /// Exact position on the ray at last boundary crossing.
    const glm::vec3 &getEnd() const { return m_end; }

    /// Total distance traveled along the ray so far.
    float getLength() const { return m_length; }

  private:
    glm::vec3 m_dir;          // normalized direction
    glm::vec3 m_origin;       // ray start
    glm::ivec3 m_voxel;       // current voxel coords
    glm::ivec3 m_step;        // +1 or -1 per axis
    glm::vec3 m_tMax;         // distance to next voxel boundary per axis
    glm::vec3 m_tDelta;       // distance to cross one voxel per axis
    glm::vec3 m_end;          // position at last boundary (starts at origin)
    float m_length = 0.0f;    // distance traveled
};

#endif // RAY_H_INCLUDED
```

---

### Task 2: 重写 Ray.cpp — DDA 实现

**Files:**
- Modify: `MineCraft-One-Week-Challenge/Source/Maths/Ray.cpp`

- [ ] **Step 1: 写入新 Ray.cpp**

```cpp
#include "Ray.h"
#include <algorithm>

static constexpr float kInf = std::numeric_limits<float>::infinity();

Ray::Ray(const glm::vec3 &origin, const glm::vec3 &rotation)
    : m_origin(origin)
    , m_end(origin)
{
    // Spherical → Cartesian direction (unit vector)
    float yr = glm::radians(rotation.y);
    float pr = glm::radians(rotation.x);
    float cp = glm::cos(pr);
    m_dir = glm::vec3(
         glm::sin(yr) * cp,
        -glm::sin(pr),
        -glm::cos(yr) * cp
    );

    // Starting voxel
    m_voxel = glm::ivec3(
        static_cast<int>(std::floor(origin.x)),
        static_cast<int>(std::floor(origin.y)),
        static_cast<int>(std::floor(origin.z))
    );

    // Step direction per axis
    m_step.x = (m_dir.x > 0.0f) ? 1 : ((m_dir.x < 0.0f) ? -1 : 0);
    m_step.y = (m_dir.y > 0.0f) ? 1 : ((m_dir.y < 0.0f) ? -1 : 0);
    m_step.z = (m_dir.z > 0.0f) ? 1 : ((m_dir.z < 0.0f) ? -1 : 0);

    // tMax & tDelta per axis
    for (int i = 0; i < 3; i++) {
        float d = m_dir[i];
        if (d != 0.0f) {
            float boundary = static_cast<float>(m_voxel[i] + (m_step[i] > 0 ? 1 : 0));
            m_tMax[i] = (boundary - origin[i]) / d;
            m_tDelta[i] = 1.0f / std::abs(d);
        } else {
            m_tMax[i] = kInf;
            m_tDelta[i] = kInf;
        }
    }
}

bool Ray::advance() {
    // Pick axis with smallest tMax (tie-break: X < Y < Z)
    if (m_tMax.x < m_tMax.y) {
        if (m_tMax.x < m_tMax.z) {
            m_length = m_tMax.x;
            m_end = m_origin + m_dir * m_tMax.x;
            m_voxel.x += m_step.x;
            m_tMax.x += m_tDelta.x;
        } else {
            m_length = m_tMax.z;
            m_end = m_origin + m_dir * m_tMax.z;
            m_voxel.z += m_step.z;
            m_tMax.z += m_tDelta.z;
        }
    } else {
        if (m_tMax.y < m_tMax.z) {
            m_length = m_tMax.y;
            m_end = m_origin + m_dir * m_tMax.y;
            m_voxel.y += m_step.y;
            m_tMax.y += m_tDelta.y;
        } else {
            m_length = m_tMax.z;
            m_end = m_origin + m_dir * m_tMax.z;
            m_voxel.z += m_step.z;
            m_tMax.z += m_tDelta.z;
        }
    }
    return !std::isinf(m_length);
}
```

---

### Task 3: Application.cpp — 实体攻击改为内联点采样

**Files:**
- Modify: `MineCraft-One-Week-Challenge/Source/Application.cpp:141-261`

- [ ] **Step 1: 前置 — 将 hitPigman/hitSpider 声明移到外部**

在原 L141-142 附近，`bool bowBlocksCombat = ...` 之后、`if (!bowBlocksCombat && leftClicked ...)` 之前，插入：

```cpp
            bool hitPigman = false;
            bool hitSpider = false;
```

同时，将原 L145-146 的 `bool hitPigman = false; bool hitSpider = false;` 删除（它们在 if 块内部，作用域不够）。

- [ ] **Step 2: 替换实体攻击射线循环**

这段代码在 `if (!bowBlocksCombat && leftClicked && !m_player.m_isDead)` 块内（约 L143-261），将 `Ray ray(...)` + `for(;ray.getLength()<5;ray.step(0.05f))` 替换为内联方向计算 + 手动步进。

替换 L148-261 的内容：

```cpp
            // Raycast for entities (inline point-sampling, no Ray dependency)
            glm::vec3 eyePos(m_player.position.x, m_player.position.y + 0.6f, m_player.position.z);
            float yr = glm::radians(m_player.rotation.y);
            float pr = glm::radians(m_player.rotation.x);
            float cp = glm::cos(pr);
            glm::vec3 dir(
                 glm::sin(yr) * cp,
                -glm::sin(pr),
                -glm::cos(yr) * cp
            );
            for (float dist = 0.0f; dist < 5.0f; dist += 0.05f)
            {
                glm::vec3 rp = eyePos + dir * dist;
                // Check pigman
                for (auto& e : m_world.getPigmen())
                {
                    if (e.state == PigmanEntity::Dead) continue;
                    glm::vec3 eMin = e.box.position - e.box.dimensions;
                    glm::vec3 eMax = e.box.position + e.box.dimensions;
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
                            e.state = PigmanEntity::Dead;
                            e.deathAnimTimer = 0.0f;
                            e.respawnTimer = 10.0f + (float)(std::rand() % 11);
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
                                if (std::rand() % 100 < 40) pushDrop(Material::RAW_MEAT);
                                if (std::rand() % 100 < 30) pushDrop(Material::STICK);
                            }
                        } else {
                            e.state = PigmanEntity::Hurt;
                            e.hurtTimer = 0.3f;
                        }
                        hitPigman = true;
                        break;
                    }
                }

                // Check spider
                for (auto& e : m_world.getSpiders())
                {
                    if (e.state == SpiderEntity::Dead) continue;
                    glm::vec3 eMin = e.box.position - e.box.dimensions;
                    glm::vec3 eMax = e.box.position + e.box.dimensions;
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
                            m_player.m_pigmanKills++;
                        } else {
                            e.state = SpiderEntity::Hurt;
                            e.hurtTimer = 0.3f;
                        }
                        hitSpider = true;
                        break;
                    }
                }
                if (hitPigman || hitSpider) break;
            }
```

注意：删除原来 `#include "Maths/Ray.h"` 和 `#include "World/Block/BlockDatabase.h"` 等可能的依赖 — 保留 `#include "Maths/Ray.h"` 因为后续步骤仍需使用（挖掘/放置）。

- [ ] **Step 3: 删除旧的挖掘目标获取代码块**

删除 L263-304（`if (!hitPigman && !hitSpider)` → 旧的点采样挖掘目标选择），后续步骤会用 DDA 版本替换。

---

### Task 4: Application.cpp — 挖掘目标 DDA + 准星跟随

**Files:**
- Modify: `MineCraft-One-Week-Challenge/Source/Application.cpp:263-381`（替换原 L263-381 区域）

- [ ] **Step 1: 写入新的挖掘目标选择 + 进度累积代码**

在已删除的旧代码位置（原 `if (!hitPigman && !hitSpider)` 之后），插入以下内容：

```cpp
            // --- Mining target selection: DDA raycast every frame while leftPressed ---
            // (skip if we just hit an entity on this click frame)
            bool entityBlocked = leftClicked && (hitPigman || hitSpider);
            if (leftPressed && !entityBlocked && !m_player.m_isDead)
            {
                glm::vec3 eyePos(m_player.position.x, m_player.position.y + 0.6f, m_player.position.z);
                Ray ray(eyePos, m_player.rotation);
                glm::ivec3 foundTarget{0, -999, 0};
                bool found = false;
                while (ray.getLength() < 5.0f) {
                    if (!ray.advance()) break;
                    auto voxel = ray.currentVoxel();
                    auto block = m_world.getBlock(voxel.x, voxel.y, voxel.z);
                    auto& data = block.getData();

                    if (data.requiredToolLevel == 255)
                        break; // unbreakable barrier

                    if (block.id != 0 && block.id != (int)BlockId::Water) {
                        const auto& eqMBlock = m_player.m_equipment[m_player.m_equipSlot].getMaterial();
                        if (data.requiredToolLevel > 0 && eqMBlock.toolTier < data.requiredToolLevel)
                            break; // insufficient tool
                        foundTarget = voxel;
                        found = true;
                        break;
                    }
                }

                if (found) {
                    if (!m_player.m_isMining
                        || m_player.m_miningTarget.x != foundTarget.x
                        || m_player.m_miningTarget.y != foundTarget.y
                        || m_player.m_miningTarget.z != foundTarget.z)
                    {
                        // Crosshair moved to a different block → reset progress
                        m_player.m_miningProgress = 0.0f;
                        m_player.m_miningTarget = foundTarget;
                    }
                    m_player.m_isMining = true;
                } else {
                    // No block under crosshair → cancel mining
                    m_player.m_isMining = false;
                    m_player.m_miningProgress = 0.0f;
                    m_player.m_miningTarget = {0, -999, 0};
                }
            }
            else if (!leftPressed && m_player.m_isMining)
            {
                // Button released — cancel mining
                m_player.m_isMining = false;
                m_player.m_miningProgress = 0.0f;
                m_player.m_miningTarget = {0, -999, 0};
            }

            // --- Mining progress accumulation ---
            if (leftPressed && m_player.m_isMining && m_player.m_miningProgress < 1.0f)
            {
                auto targetBlock = m_world.getBlock(
                    m_player.m_miningTarget.x,
                    m_player.m_miningTarget.y,
                    m_player.m_miningTarget.z);
                auto& targetData = targetBlock.getData();

                const auto& eqM = m_player.m_equipment[m_player.m_equipSlot].getMaterial();
                float multiplier = 1.0f;
                if (targetData.requiredToolClass != 0
                    && eqM.toolClass == targetData.requiredToolClass)
                {
                    multiplier = eqM.miningMultiplier;
                }
                float digTime = targetData.hardness * 2.0f / multiplier;
                m_player.m_miningProgress += delta / digTime;

                if (m_player.m_miningProgress >= 1.0f)
                {
                    m_world.addEvent<PlayerDigEvent>(sf::Mouse::Button::Left,
                        glm::vec3(m_player.m_miningTarget.x + 0.5f,
                                  m_player.m_miningTarget.y + 0.5f,
                                  m_player.m_miningTarget.z + 0.5f), m_player);

                    // Continuous mining: DDA raycast to find next block behind the dug one
                    glm::ivec3 prevTarget = m_player.m_miningTarget;
                    m_player.m_miningProgress = 0.0f;
                    m_player.m_isMining = false;
                    m_player.m_miningTarget = {0, -999, 0};

                    Ray ray(eyePos, m_player.rotation);
                    while (ray.getLength() < 5.0f) {
                        if (!ray.advance()) break;
                        auto voxel = ray.currentVoxel();
                        if (voxel.x == prevTarget.x && voxel.y == prevTarget.y && voxel.z == prevTarget.z)
                            continue;
                        auto block = m_world.getBlock(voxel.x, voxel.y, voxel.z);
                        auto& data = block.getData();

                        if (data.requiredToolLevel == 255)
                            break;

                        if (block.id != 0 && block.id != (int)BlockId::Water) {
                            const auto& eqMBlock = m_player.m_equipment[m_player.m_equipSlot].getMaterial();
                            if (data.requiredToolLevel > 0 && eqMBlock.toolTier < data.requiredToolLevel)
                                break;
                            m_player.m_isMining = true;
                            m_player.m_miningTarget = voxel;
                            break;
                        }
                    }
                }
            }
```

- [ ] **Step 2: 验证完整性**

确认以下内容已正确衔接：
- Task 3 Step 1 已将 `hitPigman`/`hitSpider` 声明移至外部作用域
- Task 3 Step 3 已删除旧的挖掘目标获取代码块（原 L263-304）
- Task 4 的新代码插入在正确位置（原 L263-381 区域）
- `entityBlocked` 变量引用的 `hitPigman`/`hitSpider`/`leftClicked` 均在作用域内

---

### Task 5: Application.cpp — 方块放置 DDA

**Files:**
- Modify: `MineCraft-One-Week-Challenge/Source/Application.cpp:386-416`

- [ ] **Step 1: 替换方块放置射线循环**

将 L387-416 的 `for (Ray ray(...); ray.getLength() < 6; ray.step(0.05f))` 循环替换为 DDA：

```cpp
        // Right-click: place block (skip if eating)
        if (rightPressed && m_rightClickTimer.getElapsedTime().asSeconds() > 0.2f && !m_player.isEating())
        {
            glm::vec3 eyePos(m_player.position.x, m_player.position.y + 0.6f, m_player.position.z);
            Ray ray(eyePos, m_player.rotation);
            glm::ivec3 prevVoxel = ray.currentVoxel(); // air voxel (player's eye position)
            glm::vec3 lastAirPos = eyePos;               // fallback for placement position
            while (ray.getLength() < 6.0f) {
                if (!ray.advance()) break;
                auto voxel = ray.currentVoxel();
                auto block = m_world.getBlock(voxel.x, voxel.y, voxel.z);
                auto& data = block.getData();

                if (data.requiredToolLevel == 255)
                    break;

                if (block.id != 0 && block.id != (int)BlockId::Water)
                {
                    if (block.id == (int)BlockId::Furnace && !m_player.isUIOpen()) {
                        m_player.m_furnaceUIOpen = true;
                        m_rightClickTimer.restart();
                        break;
                    }
                    m_rightClickTimer.restart();
                    // Place at previous (air) voxel center
                    glm::vec3 placePos(
                        prevVoxel.x + 0.5f,
                        prevVoxel.y + 0.5f,
                        prevVoxel.z + 0.5f);
                    m_world.addEvent<PlayerDigEvent>(sf::Mouse::Button::Right, placePos, m_player);
                    break;
                }
                prevVoxel = voxel;
                lastAirPos = ray.getEnd();
            }
        }
```

注意：删除了原来外层的 `lastPosition` 变量声明（原 L77 附近 `glm::vec3 lastPosition;`），因为 DDA 循环内部使用了 `prevVoxel` 代替。检查 `lastPosition` 是否在其他地方被使用 —— 在原代码中只在 L411 和 L414 使用，均在放置循环内。

---

### Task 6: 编译验证

**Files:**
- 无新文件

- [ ] **Step 1: 运行 CMake 配置 + 编译**

```bash
cd "D:\Yuga_Kshetra-The_Final_Dark_God"
sh scripts/build.sh release
```

预期：编译通过，0 错误，0 警告。

- [ ] **Step 2: 检查编译输出**

如果有编译错误，根据错误信息修复：
- `glm::vec3` 的 `operator[]` — 如果报错，改用 `.x/.y/.z` 显式访问
- `std::isinf` — 需要 `#include <cmath>`
- `std::numeric_limits<float>::infinity()` — 需要 `#include <limits>`

- [ ] **Step 3: 功能测试**

运行游戏并验证：
1. 贴近方块挖掘 → 准星上方块被挖掘（不是下方）
2. 方块接缝处挖掘 → 无错位
3. 按住左键移动准星 → 目标切换，进度重置，新方块开始挖掘
4. 连续挖掘（挖穿一排方块）→ 正常运行
5. 攻击猪人/蜘蛛 → 命中判定正常
6. 右键放置方块 → 位置正确（相邻面）
7. 俯视/仰视极端角度 → 无崩溃

- [ ] **Step 4: 提交**

```bash
git add MineCraft-One-Week-Challenge/Source/Maths/Ray.h \
        MineCraft-One-Week-Challenge/Source/Maths/Ray.cpp \
        MineCraft-One-Week-Challenge/Source/Application.cpp
git commit -m "修复：DDA体素遍历替代点采样，解决挖掘方块错位；挖掘目标跟随准星"
```
