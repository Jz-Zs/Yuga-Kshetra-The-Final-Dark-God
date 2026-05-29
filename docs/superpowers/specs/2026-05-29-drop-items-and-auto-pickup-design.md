# Entry Point 2: Drop Items + Auto-Pickup — Design Spec

**Date:** 2026-05-29  
**Status:** Updated (2026-05-29) — Icon rendering + ImGui drag-and-drop

---

## 1. Requirements Summary

When a block is broken (left-click), instead of adding the item directly to the player's inventory, spawn a physical drop-item entity in the world. The entity falls under simple gravity to the ground, where it waits. Drop items are rendered as small icons via ImGui overlay (3D→2D screen projection). When the player walks within 2 blocks, the item is automatically picked up into inventory. Inventory expanded from 5 to 20 slots (5 hotbar + 15 backpack), with ImGui drag-and-drop for backpack item management.

---

## 2. Architecture

### 2.1 New Files

| File | Purpose | Est. Lines |
|------|---------|------------|
| `Source/Entity/ItemDropEntity.h` | Drop entity struct: pos, vel, Material*, lifetime, onGround | ~30 |

No new shader files. No new renderer classes. Drop icons are drawn via ImGui overlay using existing `BitmapText`/`ImGui::Image` rendering path.

### 2.2 Modified Files

| File | Change |
|------|--------|
| `World.h/.cpp` | Add `std::vector<ItemDropEntity> m_dropItems`, `spawnDrop()`, `updateDrops()`, expose getter |
| `World/Event/PlayerDigEvent.cpp` | Left-click: call `world.spawnDrop()` instead of `player.addItem()` |
| `Player/Player.h/.cpp` | 20-slot inventory, B-key backpack toggle, Alt-key mouse unlock, `addItem()` returns bool, ImGui drag-and-drop backpack, drop icon rendering in draw() |
| `Application.cpp` | Auto-pickup loop, Alt mouse locking |

---

## 3. ItemDropEntity

```cpp
struct ItemDropEntity {
    glm::vec3 position;
    glm::vec3 velocity;
    const Material* material;
    float lifeTime = 0.0f;
    bool onGround = false;
    bool alive = true;  // false → remove next frame
};
```

- **Spawn position**: destroyed block world pos + (0.5, 0.75, 0.5) — centered above broken block
- **Gravity**: `velocity.y -= 40 * dt` (same as player)
- **Ground stop**: query `world.getBlock(dropPos.x, dropPos.y - 0.125f, dropPos.z)`. If the block below is solid (collidable, not air/water), snap `position.y` to block top + 0.125, set `onGround = true`, zero velocity.
- **Despawn**: `lifeTime > 90s` → `alive = false`, removed next frame
- **Collision box**: 0.25×0.25×0.25 units

---

## 4. Drop Icon Rendering (ImGui Overlay)

Instead of OpenGL billboard shaders, drop items are drawn as ImGui overlay icons.

### 4.1 3D→2D Projection

For each drop entity, in `Player::draw()` after the existing hotbar rendering:

1. Compute clip-space position: `clip = projection * view * vec4(drop.position, 1.0)`
2. If `clip.w <= 0` → behind camera, skip
3. NDC: `ndc = clip.xyz / clip.w`
4. Screen position: `screenX = (ndc.x * 0.5 + 0.5) * windowWidth`, `screenY = (1.0 - (ndc.y * 0.5 + 0.5)) * windowHeight`
5. If screen coords outside viewport → skip

### 4.2 Icon Drawing

- Icon size: 24×24 pixels at 1-block distance, scale down linearly with distance
- Max visible distance: 32 blocks
- Distance fade: `alpha = 1.0 - clamp(distance / maxDistance, 0.0, 1.0)`, then `alpha = alpha * alpha` for smooth falloff
- Use `ImGui::GetBackgroundDrawList()` → `AddImage()` with computed screen rect
- Texture UV from `BlockDatabase` top-face texture (existing atlas)
- **No OpenGL shader work needed** — pure ImGui rendering

### 4.3 Performance

- 100 drops × 1 ImGui quad each = trivial (ImGui batches draw calls)
- No VAO/VBO management, no shader compilation

---

## 5. World Integration

### 5.1 Drop Spawning

```cpp
void World::spawnDrop(const glm::ivec3& blockPos, BlockId blockId);
```

Called from `PlayerDigEvent::dig()` on left-click. Converts `BlockId` → `Material` via existing `Material::toMaterial()`. If material is `Nothing`, skip spawn.

### 5.2 Drop Updates

In `World::update()`: iterate `m_dropItems`, apply physics (gravity + ground detection), increment `lifeTime`, remove dead drops.

### 5.3 Pickup Logic

Run in `Application::on_update()` after `world.update()`. Iterate `world.getDropItems()`, for each drop: if `glm::distance(player.position, drop.position) < 2.0f`, call `player.addItem(drop.material)`. On success, `drop.alive = false`. On inventory full, drop stays in world.

### 5.4 Icon Rendering Call

In `Player::draw()`: receive drop item list from World, project each to screen, draw icons via ImGui.

---

## 6. Inventory Expansion

### 6.1 20-Slot System

- Constructor: `for (int i = 0; i < 20; i++)` instead of 5
- Hotbar: slots 0-4 (always visible, keys 1-5 with ToggleKey)
- Backpack: slots 5-19 (visible only when B toggled)

### 6.2 addItem Modification

```cpp
bool Player::addItem(const Material& material);
// Returns true if item was successfully added, false if inventory is full
```

### 6.3 Backpack Window

- **Toggle**: Press B → `m_backpackOpen = !m_backpackOpen`
- **Layout**: 5 columns × 4 rows (row 1 = hotbar, rows 2-4 = backpack slots 5-19)
- **Rendering**: `ImGui::Image` buttons for each slot, showing block texture icon from atlas + quantity text via `BitmapText`
- **Drag-and-drop**: ImGui built-in API
  - Each slot image button is wrapped with `ImGui::BeginDragDropSource()` + `ImGui::BeginDragDropTarget()`
  - Drag source payload: slot index (`int`)
  - Drop target: accept payload, perform swap with target slot index
  - If dropped outside any slot → item returns to source slot
  - Payload type name: `"INV_SLOT"` (string constant)
  - Drag preview: `ImGui::Image()` of the dragged item's texture, following cursor

**Why ImGui drag-and-drop works here**: the API operates on ImGui IDs and mouse events, not font rendering. Item icons use `ImGui::Image()` (proven reliable), drag preview is pure image, and the payload is an integer. Zero font dependency.

### 6.4 Mouse Control

- **Normal (Alt not held)**: `sf::Mouse::setPosition(screenCenter)` each frame — FPS camera control
- **Alt held**: Mouse position not reset — cursor freely moves for UI interaction (backpack drag-and-drop, debug panels)
- Check `sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LAlt)` in the input/update loop
- When backpack is open (`m_backpackOpen`) and Alt is held, suppress FPS camera rotation

---

## 7. Data Flow: Block Break to Inventory

```
Left-click on block
  → Application::on_update() raycast hits block
  → World::addEvent<PlayerDigEvent>(Left, endPos, player)
  → World::update() processes event
    → PlayerDigEvent::dig()
      → material = Material::toMaterial(world.getBlock(pos))
      → world.setBlock(pos, Air)            // Remove block
      → world.spawnDrop(pos, blockId)        // Spawn drop entity (NEW)
      // No longer calls player.addItem() here

Application::on_update()
  → world.update(dt)                         // Physics + lifetime + cleanup
  → for each drop in world.getDropItems():   // Auto-pickup
      if distance(player, drop) < 2.0f:
        if player.addItem(drop.material):    // Try insert
          drop.alive = false
  → world.removeDeadDrops()                   // Remove picked/expired

Player::draw()
  → Render hotbar (slots 0-4, always visible)
  → if m_backpackOpen: render backpack (slots 5-19) with drag-and-drop
  → Project world drops to screen, draw icons via ImGui overlay
```

---

## 8. Risk Assessment

| Risk | Likelihood | Mitigation |
|------|-----------|------------|
| 3D→2D projection wrong (icons misaligned) | Medium | Use existing Camera view/projection matrices, test with known positions |
| TextureAtlas UV lookup wrong for some block types | Medium | Use block top-face texture UV from BlockDatabase |
| ImGui drag-and-drop API not compatible with imgui-sfml v3 | Low | Drag-and-drop is core ImGui (v1.60+), imgui-sfml only handles input forwarding. Payload system is ID-based, no rendering dependency. |
| Alt key conflicting with OS shortcuts (Alt+Tab, etc.) | Low | When SFML window is focused, key events won't reach OS. Alt behavior differs from Alt+Tab (Tab key triggers OS switch). |
| Inventory full → pickup loop infinite | None | `addItem` returns bool, drop stays in world if full |
| Many drops → ImGui overlay perf | None | 100 ImGui quads is trivial; ImGui batches draw calls internally |

---

## 9. Testing Plan

1. Break a grass block → verify drop icon appears at block position on screen
2. Drop icon falls to ground and stops (physics works)
3. Walk near drop (within ~2 blocks) → item auto-picked into hotbar
4. Press B → backpack window appears above hotbar (rows 2-4)
5. Hold Alt → mouse releases, drag item from slot 5 to slot 0 → items swap
6. Drag item outside backpack area → item stays in original slot
7. Drop older than 90 seconds → icon disappears
8. Inventory full → drop icon remains in world
9. Multiple drops nearby → all icons visible, no stacking artifacts
