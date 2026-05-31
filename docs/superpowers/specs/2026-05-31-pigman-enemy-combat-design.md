# Entry Point 4: Pigman Enemy + Combat System — Design Spec

**Date:** 2026-05-31
**Status:** Approved

## Architecture: Approach A — Light Entity + World Management

Follows the existing `ItemDropEntity` pattern: data in struct, logic in World.

## New Files

| File | Purpose |
|------|---------|
| `Source/Entity/PigmanEntity.h` | Data struct (position, HP, AI state, AABB) |
| `Source/Entity/PigmanAI.h/cpp` | AI state machine + A* pathfinding |
| `Source/Renderer/EntityRenderer.h/cpp` | OBJ model rendering with programmatic animations |

## Modified Files

| File | Changes |
|------|---------|
| `Source/World/World.h/cpp` | +`m_pigmen`, +`spawnPigman()`, +`updateEntities()` |
| `Source/Player/Player.h/cpp` | +`m_hp`, +`m_baseAttack`, +`takeDamage()`, death handling |
| `Source/Renderer/RenderMaster.h/cpp` | +`EntityRenderer` in finishRender pipeline |
| `Source/Application.h/cpp` | Combat detection: left-click → pigman → mining |
| `Source/Item/Material.h/cpp` | +`RAW_MEAT` material |

No external library dependencies. All self-written.

---

## PigmanEntity Data

```cpp
struct PigmanEntity {
    glm::vec3 position, velocity, rotation;
    AABB box;
    int hp = 30, maxHp = 30;
    float moveSpeed = 4.0f;

    enum State { Patrol, Chase, Attack, Hurt, Dead };
    State state = Patrol;
    float stateTimer, attackCooldown, hurtTimer, stuckTimer;
    float respawnTimer = -1.0f;

    glm::vec3 patrolOrigin, patrolTarget;
    std::vector<glm::ivec3> path;
    int pathIndex = 0;
    const Model* model = nullptr;
};
```

---

## Combat Values

| Source | Value |
|--------|-------|
| Player HP | 100 (max 200) |
| Player base attack | 2 (bare hands) |
| Wooden Sword attack | +10 (total 12) |
| Player invincibility | 0.5s after hit |
| Pigman HP | 30 |
| Pigman attack | 5 |
| Pigman move speed | 4.0 blocks/s |
| Pigman detection range | 12 blocks |
| Pigman attack range | 2 blocks |
| Pigman attack interval | 1.5s |
| Knockback | 1 block |
| Hurt stun | 0.3s |

## Left-Click Behavior

```
Left-click:
  Raycast →
    ├── Hit pigman (distance ≤ 5) → Instant attack (damage + swing anim), NO cooldown
    └── No pigman hit → Mining (0.3s progress bar, existing logic)
```

Attack and mining are separated: click = attack, hold 0.3s = mine.

---

## AI State Machine

```
Patrol: random walk within 8-block radius of spawn point, A* pathing
  → Player in range (12 blocks, line of sight) → Chase

Chase: A* recalc every 0.5s toward player
  → Distance < 2 blocks → Attack
  → Target lost 5s or stuck 3s → Patrol

Attack: deal 5 damage to player every 1.5s
  → Player moves > 2 blocks away → Chase
  → Takes damage → Hurt

Hurt: knockback 1 block, stun 0.3s, model red tint
  → HP > 0 → Chase
  → HP ≤ 0 → Dead

Dead: 5x independent drop rolls, 10-20s respawn timer, HP refill
```

### A* Pathfinding

- Grid: ground-level block coordinates via Chunk data
- Cost: Manhattan heuristic + movement distance
- Max 256 steps, recalc every 0.5s, abandon on timeout
- Self-written, ~150 lines

### Stuck Detection

Position unchanged for 3 seconds → abandon path → return to Patrol.

---

## Spawning & Respawn

- **Max concurrent:** 8 pigmen
- **Initial spawn:** 8 random positions, ≥15 blocks from player, on collidable ground with 2-block headroom
- **Respawn:** 10-20s after death, 20-30 blocks from player, same standing conditions

---

## Model & Rendering

### Static OBJ

- Source: `Res/Models/PigMan/PigMan.obj` + `.mtl` + `.png`
- 890 vertices, 1562 triangles
- Self-written OBJ parser (~80 lines)
- Loaded once, shared `Model*` across all pigmen

### EntityRenderer

- Uses existing `Model` + `BasicShader` + `BasicTexture` pipeline
- Runs in `RenderMaster::finishRender()`, after ChunkRenderer, before SkyboxRenderer
- Per-entity model matrix: `translate * rotateY(facing) * scale(0.06)`

### Programmatic Animations

| State | Transform | Duration |
|-------|-----------|----------|
| Chase | `translate(0, sin(t×10)×0.1, 0)` Y-axis bob | continuous |
| Attack | `rotate(z, sin(t/0.3)×25°)` forward lunge | 0.3s |
| Hurt | Red tint via shader uniform | 0.3s |
| Dead | `rotate(x, t×90°)` + scale to 0 | 1.0s |

BasicShader: add `vec3 tintColor` uniform (default (1,1,1) = no tint).

---

## Drops

| Item | Probability/roll | Qty/roll |
|------|------------------|----------|
| RAW_MEAT | 40% | 1 |
| Stick (木棍, existing) | 30% | 1 |

5 independent rolls on death. RAW_MEAT texture at (12,1) in DefaultPack.png.

---

## Player Death

- HP ≤ 0 → clear inventory (safe slots preserved) → "You Died" screen → world reset
- Full death/reset flow deferred to Entry Point 6

---

## Game Loop (Application::on_update)

```
1. player.handleInput()
2. Left-click: pigman attack OR mining
3. Right-click: block placement
4. camera.update()
5. player.update(dt, world)       — physics + collision
6. world.updateEntities(dt, player) — pigman AI + physics + attack player
7. world.update(camera, dt)       — chunks + drops + pickup
```
