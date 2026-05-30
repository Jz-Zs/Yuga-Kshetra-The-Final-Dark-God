# Crafting System + Equipment System Design

**Date:** 2026-05-30
**Status:** Spec complete
**Phase:** Entry Point 3

## Scope

- Add 2 new non-block Material types: Stick, WoodenSword
- Implement 2 crafting recipes: 3 OakBark → 1 Stick (left column), 3 Stick → 1 WoodenSword (left column)
- Implement 3×3 crafting grid UI (above inventory in backpack window)
- Implement main-hand / off-hand equipment slots (left of hotbar, independent window)
- Implement R-key equipment focus switching, 1-5 hotbar hotkeys
- Implement crosshair cursor with mining progress ring
- Differentiate attack (<0.5s left click) vs mining (>=0.5s hold)

## Data Model

### New Material::ID entries

Append to Material.h enum `ID`:
- `Stick` — craft intermediate, maxStack=99
- `WoodenSword` — weapon, maxStack=1

OakBark blocks directly use existing `Material::OAK_BARK_BLOCK` as crafting input (no separate WoodItem needed).

### CraftingRecipe (new file: Source/Item/CraftingRecipe.h)

```cpp
struct CraftingRecipe {
    const Material* pattern[9];  // 3×3 grid, nullptr = empty
    const Material* output;
    int outputCount;
};
```

Recipe registry: `std::vector<CraftingRecipe> g_recipes`, populated at init.

### Player additions (Player.h)

```cpp
ItemStack m_equipment[2]     = {Material::NOTHING, Material::NOTHING};
ItemStack m_craftGrid[9]     = {};  // all NOTHING
const CraftingRecipe* m_currentRecipe = nullptr;

enum class Focus { Hotbar, MainHand, OffHand };
Focus m_focus = Focus::Hotbar;
```

## UI Layout

```
B key opens (single ImGui window, no title, no move):
+-----------------+
| 3×3 Craft Grid  |  ← 9 slots (32px each)
|   [Output]      |  ← result slot, right side
+-----------------+
| 3×5 Inventory   |  ← existing 20-slot backpack
+-----------------+

Bottom of screen (independent windows):
[MainHand][OffHand]    [1][2][3][4][5]
 ↑ equipment window     ↑ hotbar window

Screen center:
  +           ← white crosshair
  ○           ← white ring (mining progress)
```

## Interaction Rules

### Crafting grid
- **Backpack → grid:** Drag from inventory slot to grid slot, exactly 1 item transferred
- **Grid → backpack:** Left-click grid slot OR drag back to inventory; exact slot placement
- **Matching:** On each grid change, scan `g_recipes` for exact 3×3 match
- **Result slot:** Left-click to claim 1 output, deduct 1 set of materials; if remaining materials still match, result persists for another claim
- **Close window (B key):** Return all grid contents to inventory (fill existing stacks first, then empty slots; if inventory full, items remain in grid)

### Equipment slots
- Independent ImGui window at screen bottom-left of hotbar
- Drag from hotbar/inventory → equipment slot: places exactly 1 item
- If slot already occupied, old item returns to inventory first
- Left-click equipment slot → return item to inventory (same as crafting grid: quick clear)
- Equipment slot also supports drag-out back to inventory
- Clicking an equipment slot also switches focus to that slot

### Focus & R key
```
Focus=Hotbar   --R--> MainHand
Focus=MainHand --R--> OffHand
Focus=OffHand  --R--> MainHand
```
- 1-5 keys: always switch focus to Hotbar, set m_heldItem accordingly
- Selecting equipment slot via click also switches focus

### Weapon rendering
- When focus = MainHand or OffHand AND equipped item is a weapon (isWeapon flag): render sprite at screen bottom-right (48×48, sampled from texture atlas)
- On left-click attack: weapon sprite swings (rotate ±30° around Z axis, 0.3s animation)
- Mining also triggers animation while holding

### Attack vs Mining
- Left-click duration <500ms on release → attack (instant)
- Left-click held >=500ms → mining begins (continuous, same as current break logic)
- Raycast and block interaction only during mining hold (not during attack)

### Crosshair & Mining ring
- When mouse is locked: render white crosshair at screen center (2 lines, 8px each)
- During mining hold: render white ring around crosshair (partial circle arc)
  - Ring fills clockwise 0°→360° proportional to mining progress
  - Mining progress source: existing block break timer (time held / break time required)
  - Radius: 16px, line width: 2px
  - When full → block breaks → ring resets
- When mouse is not locked (Alt/B key): hide crosshair

### Hotbar selection
- 1-5 keys: always switch to Hotbar focus, set m_heldItem
- Number keys set m_heldItem to the corresponding slot index (1→0, 2→1, ..., 5→4)

## Files to Create

| File | Purpose |
|------|---------|
| `Source/Item/CraftingRecipe.h` | Recipe struct + registry declaration |
| `Source/Item/CraftingRecipe.cpp` | Register 2 initial recipes |

## Files to Modify

| File | Changes |
|------|---------|
| `Source/Item/Material.h` | Add Stick, WoodenSword to ID enum |
| `Source/Item/Material.cpp` | Add static instances for Stick, WoodenSword |
| `Source/Player/Player.h` | Add equipment, craft grid, focus, current recipe |
| `Source/Player/Player.cpp` | Crafting UI, equipment window, weapon sprite, crosshair, mining ring, R key, attack/mining split, 1-5 hotkeys |
| `Source/Application.cpp` | Wire input events (R key, 1-5, left-click timing) |

## Recipes (Initial)

| Pattern (3×3) | Output |
|---------------|--------|
| OakBark ×3 (left column: [0,3,6]) | Stick ×1 |
| Stick ×3 (left column: [0,3,6]) | WoodenSword ×1 |

## Texture Atlas

Add two 16×16 pixel icons to atlas.png at unused tile positions:
- Stick: brown narrow vertical line
- WoodenSword: brown short sword (MC style)

WoodItem reuses OakBark tile.

## Constraints
- Y=vertical (gravity/height), X/Z=horizontal
- ImGui font broken for glyphs → BitmapText for numbers
- MSVC /utf-8 required
- All new UI uses ImGui (consistent with existing inventory UI)
