# Entry 9: Spider Monster + Dynamic Difficulty — Design Spec

**Date:** 2026-06-03
**Status:** Approved

---

## 1. Overview

Add spider enemy with ranged+melee hybrid combat, new items (Silk/Silk Thread), a synthesis recipe, and a simplified dynamic difficulty system. Also rebalance pigman and player stats.

## 2. Stat Rebalance

### 2.1 Pigman Changes

| Property | Old | New |
|----------|-----|-----|
| HP | 30 | **40** |
| Chase Speed | 4.0 | **4.5** |
| Attack Cooldown | 1.5s | **1.0s** |

### 2.2 Player Changes

| Property | Old | New |
|----------|-----|-----|
| Walk Speed (speed constant) | 0.2 (~3.8/s) | **0.263 (~5.0/s)** |
| Sprint Multiplier | ×5 | **×4** |
| Attack Range | 6 | **5** |

### 2.3 Spider Stats

| Property | Value |
|----------|-------|
| HP | 80 |
| Max Count | 5 (staged: 0→1→2→3→4→5 per minute starting at 1:00) |
| Patrol Speed | 2.0 |
| Chase Speed | 6.0 |
| Detection Range | 10 (line of sight required) |
| Spawn Min Distance | 20 from player |
| Respawn Time | 30–50s after death |
| AABB | Derived from model bounds at load time |
| Model | `Res/Models/Spider/minecraft-spider/source/model.gltf` (ASCII glTF) |
| Texture | Spider model texture → copy to `Res/Textures/spider.png` |

### 2.4 Spider Combat

| Mode | Condition | ATK | Range | Cooldown | Notes |
|------|-----------|-----|-------|----------|-------|
| Ranged (web spit) | distance > 5 | 5 | 8 (max) | 2.5s | 0.5s freeze after shot, 20% slow for 1s on hit |
| Melee (bite) | distance ≤ 5 | 10 | 2 | 1.5s | Same pattern as pigman melee |

## 3. Spider AI State Machine

```
Patrol
  ├─ Random direction every 3-5s, speed 2.0
  ├─ World-edge avoidance (same as pigman)
  └─ → Chase {dist < 10 && lineOfSight}

Chase
  ├─ A* pathfind toward player (reuse PigmanAI::findPath)
  ├─ Speed 6.0, stuck detection 3s → back to Patrol
  ├─ → RangedAttack  {dist > 5 && lineOfSight && cooldown ≤ 0}
  ├─ → MeleeAttack   {dist ≤ 5}
  └─ → Patrol        {dist > 10 || stuck > 3s}

RangedAttack
  ├─ Spawn projectile toward player
  ├─ Set velocity = 0, freezeTimer = 0.5
  ├─ Set rangedCooldown = 2.5
  └─ → Chase {freezeTimer ≤ 0}

MeleeAttack
  ├─ Velocity = 0
  ├─ Deal 10 damage if dist ≤ 2 && cooldown ≤ 0
  ├─ Set meleeCooldown = 1.5
  └─ → Chase {dist > 2}

Hurt
  ├─ Velocity = 0
  └─ → Chase {hurtTimer ≤ 0}

Dead
  ├─ Death animation (rotation + shrink, same as pigman)
  └─ Respawn after 30-50s
```

## 4. Projectile System

**Visual:** Uses DefaultPack.png texture atlas, UV at **(13, 2)**. No Material/BlockId/.block file needed — projectile is a visual-only entity, not an inventory item.

### 4.1 SpiderProjectile Struct

```cpp
struct SpiderProjectile {
    glm::vec3 position;
    glm::vec3 velocity;  // horizontal only, speed 4.0 toward player at fire time
    int damage = 5;
    float lifetime = 0.0f;
    float maxLifetime = 2.0f;  // 8 range / 4 speed
    bool alive = true;
};
```

### 4.2 Lifecycle
- Spawned by SpiderAI at spider position + (0, 0.75, 0)
- Direction: normalized horizontal vector to player at fire time
- Flight: `position += velocity * dt` (straight line, no gravity)
- Destroyed on: collision with collidable block OR lifetime > maxLifetime
- Hit player: AABB check → 5 damage + apply slow (20%, 1s) → projectile destroyed

### 4.3 Slow Effect on Player
- New fields: `float m_slowTimer = 0.0f; float m_slowFactor = 1.0f;`
- On hit: `m_slowTimer = 1.0f; m_slowFactor = 0.8f;`
- In `keyboardInput()`: `speed *= m_slowFactor` when `m_slowTimer > 0`
- `update()` decrements `m_slowTimer -= dt`

## 5. Staged Spawning

```
Elapsed Time → maxSpiderCount:
  0:00–1:00    0
  1:00–2:00    1
  2:00–3:00    2
  3:00–4:00    3
  4:00–5:00    4
  5:00+         5
```

Implementation: `maxCount = std::min(5, std::max(0, (int)elapsedTime / 60))` when `elapsedTime >= 60`.

Spawn loop: if `m_spiders.size() < maxCount`, try spawn with 50 attempts, ≥20 blocks from player.

No spawning on trees. During spawn position search, if ground block is a leaf, skip and retry.

## 6. EntityRenderer Refactoring

**Problem:** `TINYGLTF_IMPLEMENTATION` is a one-definition-rule macro. Copying EntityRenderer would cause link errors.

**Solution:** Make EntityRenderer parameterized by model path and texture name.

```
EntityRenderer(modelPath, textureName)  ← new constructor signature
  ↓ internal
  m_gltfModel  → loaded from modelPath
  m_meshParts  → built from gltf nodes
  m_texture    → loaded from textureName
  m_entities   → vector of lightweight render data (pos, rot, state, animTime, tint)
```

**API change:**
```cpp
// Before
void addEntity(const PigmanEntity& e);

// After
struct EntityRenderData {
    glm::vec3 position;
    glm::vec3 rotation;
    int state;       // 0=Patrol, 1=Chase, 2=Attack, 3=Hurt, 4=Dead
    float animTimer; // stateTimer equivalent
    float deathAnimTimer;
    bool isHurt;
};
void addEntity(const EntityRenderData& rd);
```

World owns two renderers:
```cpp
EntityRenderer m_pigmanRenderer{"Res/Models/Zoglin/minecraft_-zoglin/scene.gltf", "zoglin"};
EntityRenderer m_spiderRenderer{"Res/Models/Spider/minecraft-spider/source/model.gltf", "spider"};
```

For the spider model: use the ASCII glTF at `minecraft-spider/source/model.gltf` (consistent with zoglin). The GLB at root is an alternative format.

**Animation mapping:** The spider gltf likely has animation data. If present, `getNodeWorldTransform` applies it automatically. Additional programmatic animations:
- Dead: rotate 90° around Z + shrink to 0 (same as pigman)
- Hurt: white tint (same as pigman)
- Melee: lunge forward + head dip (same as pigman attack)

## 7. New Items: Silk & Silk Thread

### 7.1 Material + BlockId Registration

| Item | Material::ID | BlockId | TexCoords in DefaultPack.png |
|------|-------------|---------|------------------------------|
| Silk (蛛丝) | Silk | Silk=30 | (10, 2) |
| Silk Thread (丝线) | SilkThread | SilkThread=31 | (11, 2) |

### 7.2 .block Files
- `Res/Blocks/Silk.block`: `TexAll` → `10 2`, meshType=none (non-block item)
- `Res/Blocks/SilkThread.block`: `TexAll` → `11 2`, meshType=none

### 7.3 Material Registration
```cpp
const Material Material::SILK(Material::ID::Silk, 64, false, "Silk");
const Material Material::SILK_THREAD(Material::ID::SilkThread, 64, false, "Silk Thread");
```

### 7.4 Recipe: 3 Silk → 1 Silk Thread
Pattern (vertical left column in 3×3 crafting grid):
```
S  -  -
S  -  -
S  -  -
```
Where S = Silk. Output: 1 Silk Thread.

## 8. Spider Drops (on death)

5 independent rolls per kill:
- 35% RawMeat
- 50% Silk
- 20% Stick
- 15% Silk Thread

## 9. Dynamic Difficulty (Simplified V1)

### 9.1 Trigger
At **6:00** elapsed time, one-time activation.

### 9.2 Effects
- All existing + new enemies: HP ×1.1, ATK ×1.1, speed ×1.1
- Max pigman count: 8 → 9
- Max spider count (at that time): e.g., 5 → 6
- Timer HUD text color: white → **orange** (`ImColor(255, 165, 0)`)

### 9.3 Implementation
- `World` stores `float m_difficultyMultiplier = 1.0f;`
- At 360s mark: `m_difficultyMultiplier = 1.1f;`
- `updateEntities()` applies multiplier when reading stats (hp, atk, speed)
- Pigman max count check: `m_pigmen.size() < (m_difficultyMultiplier > 1.05f ? 9 : 8)`
- Spider max count: `baseMax + 1` after difficulty spike
- Player stores `bool m_difficultyActive` for HUD color switching

## 10. Rendering: Spider in World

### 10.1 Per-frame flow in World::updateEntities()
1. Check staged spawn condition → try spawn spider
2. For each live spider: run SpiderAI::update()
3. Apply gravity, velocity, block collision (same as pigman)
4. Check melee attack damage window
5. Check ranged attack cooldown → spawn projectile
6. Update projectiles: move, collide, hit player
7. For dead spiders: death animation timer → respawn
8. Register spider EntityRenderData to spiderRenderer

### 10.2 Render pass (World::renderWorld)
```cpp
m_pigmanRenderer.render(camera);
m_spiderRenderer.render(camera);
```

### 10.3 Projectile rendering
- Each projectile = one textured quad (billboard, always facing camera)
- Uses `BasicShader` with TextureAtlas (DefaultPack.png), UV at grid cell (13, 2)
- Quad built as 2-triangle VAO, reused for all projectiles
- Rendered between chunk rendering and entity rendering for correct 3D depth

## 11. Files Changed / Created

### New files
| File | Purpose |
|------|---------|
| `Source/Entity/SpiderEntity.h` | Spider POD struct |
| `Source/Entity/SpiderAI.h` | Spider AI namespace declaration |
| `Source/Entity/SpiderAI.cpp` | Spider AI state machine + A* wrapper |
| `Source/Entity/SpiderProjectile.h` | Projectile POD struct |
| `Source/Renderer/ProjectileRenderer.h` | Projectile textured quad VAO + shader |
| `Source/Renderer/ProjectileRenderer.cpp` | Projectile rendering impl |
| `Res/Blocks/Silk.block` | Silk item definition |
| `Res/Blocks/SilkThread.block` | Silk Thread item definition |

### Modified files
| File | Changes |
|------|---------|
| `Source/Entity/PigmanEntity.h` | HP 30→40 |
| `Source/Entity/PigmanAI.cpp` | Chase speed 4.0→4.5, attack cooldown 1.5→1.0 |
| `Source/Renderer/EntityRenderer.h` | Add parameterized constructor, EntityRenderData struct, remove PigmanEntity dependency |
| `Source/Renderer/EntityRenderer.cpp` | Remove hardcoded paths, accept model/texture in constructor, use EntityRenderData |
| `Source/World/World.h` | Add spider vector, spiderRenderer, projectile vector, difficulty fields |
| `Source/World/World.cpp` | Spider spawn/update/render logic, projectile update, difficulty trigger |
| `Source/Player/Player.h` | m_slowTimer, m_slowFactor, modify speed constant, m_difficultyActive |
| `Source/Player/Player.cpp` | Walk speed 0.2→0.263, sprint ×5→×4, attack range 6→5, slow application in keyboardInput, slow decrement in update |
| `Source/Item/Material.h` | Add Silk, SilkThread to ID enum |
| `Source/Item/Material.cpp` | Register SILK, SILK_THREAD materials |
| `Source/World/Block/BlockId.h` | Add Silk=30, SilkThread=31 |
| `Source/World/Block/BlockDatabase.cpp` | Register Silk, SilkThread blocks |
| `Source/Application.cpp` | Spider raycast hit check (left-click combat), attack range 6→5, difficulty HUD orange |
| `Source/Player/Player.cpp` (crafting) | Add 3-Silk→1-SilkThread recipe |

## 12. Testing Checklist
- [ ] Spider spawns after 1:00 (not before)
- [ ] Spider count increases at minute boundaries
- [ ] Spider attacks player (ranged at >5, melee at ≤5)
- [ ] Web projectile visible and flies straight
- [ ] Web hit slows player 20% for 1s
- [ ] Spider freeze 0.5s after ranged attack
- [ ] Spider drops Silk/Thread/RawMeat/Stick on death
- [ ] Spider respawns 30-50s after death
- [ ] 3 Silk crafts into 1 Silk Thread
- [ ] Spider + Pigman coexist and render correctly
- [ ] At 6:00: difficulty spike applied, timer turns orange
- [ ] Player speed 5.0, sprint speed 20.0
- [ ] Pigman HP 40, chase 4.5, attack cooldown 1.0s
- [ ] Attack range 5 blocks
- [ ] Silk/Silk Thread textures display correctly in inventory
