# Bow & Arrow System — Design Spec

**Date:** 2026-06-04
**Status:** Approved

---

## 1. Overview

Add bow (ranged weapon) and iron arrows (consumable ammo) to the equipment/combat system. Bow uses Shift+LeftClick charge-to-fire mechanic with a progress ring. Stone arrows (existing item) gain combat use as basic ammo.

## 2. New Items

| Item | Material::ID | BlockId | TexCoords | MaxStack | isBlock |
|------|-------------|---------|-----------|----------|---------|
| Bow (弓) | Bow | Bow=32 | (14, 2) | 1 | false |
| IronArrow (铁箭矢) | IronArrow | IronArrow=33 | (15, 2) | 99 | false |

### 2.1 Bow Stats

| Property | Value |
|----------|-------|
| attackBonus | +1 (melee only, total 3 with baseAttack) |
| toolClass | 0 (not a tool) |
| toolTier | 0 |
| miningMultiplier | 1.0 |
| maxStackSize | 1 |

### 2.2 IronArrow Stats

| Property | Value |
|----------|-------|
| attackBonus | +15 (used as projectile damage, not melee) |
| maxStackSize | 99 |

### 2.3 SpiderSilkArrow (蛛丝箭矢)

| Property | Value |
|----------|-------|
| attackBonus | +10 |
| Special | Hit applies 20% slow for 1 second (same as spider web) |
| maxStackSize | 99 |
| TexCoords | (15, 3) |

### 2.4 StoneArrow Update

- Texture moved from (2, 2) → **(14, 3)**
- `attackBonus = 10` for projectile damage calculation

## 3. Crafting Recipes

### 3.1 Bow — 3 Stick + 3 SilkThread → 1 Bow

Pattern (sticks in "<" left, silk threads right vertical):
```
-  S  T
S  -  T     S = Stick, T = SilkThread
-  S  T
```

### 3.2 IronArrow — 1 IronIngot + 2 Stick → 4 IronArrow

Pattern (same as StoneArrow):
```
I  -  -
-  S  -     I = IronIngot, S = Stick
-  -  S
```

### 3.3 SpiderSilkArrow — 1 Silk + 2 Stick → 4 SpiderSilkArrow

Pattern (same as StoneArrow):
```
L  -  -
-  S  -     L = Silk, S = Stick
-  -  S
```

## 4. Combat Mechanics

### 4.1 Bow Usage (equipment slot only)

| Operation | Behavior |
|-----------|----------|
| Left-click (tap) | Melee attack: baseAttack(2) + Bow.attackBonus(1) = 3 dmg, 5-block range |
| Shift + LeftClick hold | Charge bow, progress ring displayed |
| Charge ≥ 1.0s → release | Fire arrow projectile |
| Charge < 1.0s → release | Cancel, no ammo consumed |

### 4.2 Arrow Consumption Priority

Search order: hotbar slots 0→4, then backpack slots 0→19 (left→right, top→bottom). First found arrow type (IronArrow or StoneArrow) is consumed.

### 4.3 Bow Charge

- Hold Shift+LeftClick: `m_bowCharge` accumulates `dt` each frame
- Progress ring drawn on ForegroundDrawList (same pattern as mining ring)
- At 1.0s: ring complete (green highlight)
- Release before 1.0s: cancel, `m_bowCharge` resets

### 4.4 Shift Key

Shift+LeftClick charges the bow AND applies the normal sneak slowness (×0.35). No special handling — the sneak behavior is preserved as-is.

### 4.5 Arrow Type Comparison

| Arrow | Damage | Special | Recipe | Texture |
|-------|--------|---------|--------|---------|
| Stone Arrow | 10 | — | 1 Cobblestone + 2 Stick → 4 | (14, 3) |
| Iron Arrow | 15 | — | 1 IronIngot + 2 Stick → 4 | (15, 2) |
| SpiderSilk Arrow | 10 | Slow 20% 1s | 1 Silk + 2 Stick → 4 | (15, 3) |

### 4.6 Enemy Aggro on Arrow Hit

When an enemy is hit by an arrow:
- `aggroTimer = 3.0f` set on the entity
- Immediately switch to Chase state
- While `aggroTimer > 0`: skip distance-based disengage (>10 blocks does NOT return to Patrol)
- Spider in Chase: normal behavior — close to 5-8 blocks → shoot web, ≤5 blocks → melee
- After aggroTimer expires: resume normal distance checks

New field: `float aggroTimer = 0.0f;` on both PigmanEntity and SpiderEntity.

## 5. Arrow Projectile

### 5.1 ArrowEntity Struct

```cpp
struct ArrowEntity {
    glm::vec3 position;
    glm::vec3 velocity;   // horizontal, speed 10.0
    int damage = 10;
    float lifetime = 0.0f;
    float maxLifetime = 1.2f;  // 12 range / 10 speed
    bool alive = true;
    const Material* arrowMaterial = nullptr; // for texture
};
```

### 5.2 Lifecycle

- Spawned at player eye position + direction * 10.0 speed
- Straight line (no gravity), horizontal direction = player look direction
- Destroyed on: collision with collidable block OR lifetime > maxLifetime OR hit enemy entity
- Hit enemy: apply damage, destroy projectile
- **Not pickable** — destroyed after any collision

### 5.3 Rendering

- Billboard quad, 0.2×0.2 size
- Uses TextureAtlas UV from the arrow material's BlockData
- Reuses ProjectileRenderer with per-projectile texture support (or separate ArrowRenderer)

## 6. Player State

### 6.1 New Fields (Player.h)

```cpp
float m_bowCharge = 0.0f;       // 0.0 ~ 1.0, charge progress
bool m_bowCharging = false;     // Shift+LeftClick held with bow equipped
```

### 6.2 Input Flow (Application.cpp on_update)

```
if (装备栏是弓):
    if (Shift held && LeftClick pressed):
        m_bowCharging = true
        m_bowCharge += dt
        if m_bowCharge >= 1.0f:  // fully charged
            (mark as ready)
    elif (m_bowCharging && LeftClick released):
        if m_bowCharge >= 1.0f:
            fire arrow (spawn ArrowEntity)
        m_bowCharge = 0
        m_bowCharging = false
```

## 7. Enemy Hit Integration

### 7.1 Spider Web Projectile vs Arrow

Both use billboard rendering but different textures. The existing ProjectileRenderer renders all spider projectiles with a single texture (DefaultPack 13,2). Arrows need per-arrow-type textures.

**Option:** Extend ProjectileRenderer to accept per-projectile UV coordinates, or create a lightweight ArrowRenderer.

**Decision:** Create a separate `ArrowRenderer` that takes position + material → renders billboard with correct texture UV. Simpler than modifying ProjectileRenderer to be per-instance-textured.

### 7.2 Arrow Hit Detection (World.cpp updateEntities or Application.cpp on_update)

Arrow entities are stored in World (`std::vector<ArrowEntity> m_arrows`). Each frame:
1. Move: `position += velocity * dt`
2. Block collision: if block at position is solid → destroy
3. Lifetime check: if lifetime > maxLifetime → destroy
4. Entity hit: check AABB intersection with pigmen and spiders → deal damage → destroy
5. Cleanup dead arrows

## 8. Files Changed / Created

### New files
| File | Purpose |
|------|---------|
| `Source/Entity/ArrowEntity.h` | Arrow POD struct |
| `Source/Renderer/ArrowRenderer.h` | Arrow billboard rendering |
| `Source/Renderer/ArrowRenderer.cpp` | Arrow renderer impl |
| `Res/Blocks/Bow.block` | Bow item definition |
| `Res/Blocks/IronArrow.block` | IronArrow item definition |
| `Res/Blocks/SpiderSilkArrow.block` | SpiderSilkArrow item definition |

### Modified files
| File | Changes |
|------|---------|
| `Source/Item/Material.h` | Add Bow, IronArrow, SpiderSilkArrow to ID enum; update StoneArrow attackBonus to 10 |
| `Source/Item/Material.cpp` | Register BOW, IRON_ARROW, SPIDER_SILK_ARROW materials; update StoneArrow construction |
| `Source/World/Block/BlockId.h` | Add Bow=32, IronArrow=33, SpiderSilkArrow=34 |
| `Source/World/Block/BlockDatabase.cpp` | Register Bow, IronArrow, SpiderSilkArrow blocks |
| `Res/Blocks/StoneArrow.block` | Update TexAll from 2 2 → 14 3 |
| `Source/World/Block/BlockDatabase.cpp` | Register Bow, IronArrow blocks |
| `Source/Item/CraftingRecipe.cpp` | Add Bow and IronArrow recipes |
| `Source/Player/Player.h` | Add m_bowCharge, m_bowCharging, bowCharge progress accessors |
| `Source/Player/Player.cpp` | keyboardInput: nullify Shift slow when bow charging; draw bow charge ring |
| `Source/Application.cpp` | Bow input handling (Shift+Click charge/fire), arrow entity spawning, enemy hit detection for arrows |
| `Source/World/World.h` | Add m_arrows vector + accessor |
| `Source/World/World.cpp` | Arrow update loop (move/collision/cleanup), arrow enemy hit |
| `Source/Renderer/RenderMaster.h` | Add ArrowRenderer |
| `Source/Renderer/RenderMaster.cpp` | Render arrows in finishRender |

## 9. Testing Checklist
- [ ] Bow crafts from 3 Stick + 3 SilkThread
- [ ] IronArrow crafts from 1 IronIngot + 2 Stick → 4 arrows
- [ ] Bow placed in equipment slot, melee attack = 3 dmg, 5 blocks
- [ ] Shift+LeftClick charge: progress ring visible
- [ ] Release < 1s: cancel, no arrow consumed
- [ ] Release ≥ 1s: arrow fires, arrow consumed from inventory
- [ ] Arrow flies straight at speed 10, max range 12
- [ ] Arrow hits pigman/spider: deals damage (10 stone / 15 iron), arrow destroyed
- [ ] Arrow hits block: destroyed
- [ ] Arrow consumption priority: hotbar L→R then backpack L→R top→bottom
- [ ] Shift key does not slow player while bow charging
- [ ] Bow/arrow textures display correctly in inventory
- [ ] SpiderSilkArrow crafts from 1 Silk + 2 Stick → 4
- [ ] SpiderSilkArrow hit slows target 20% for 1 second
- [ ] Enemy hit by arrow: aggroTimer=3s, ignores distance disengage
- [ ] Spider hit by arrow: enters Chase, closes to 5-8 blocks → shoots web
- [ ] StoneArrow texture at (14, 3), IronArrow at (15, 2), SpiderSilkArrow at (15, 3)
