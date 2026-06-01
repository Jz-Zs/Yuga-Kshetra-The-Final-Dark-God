# Entry Point 6: Extraction Point + World Reset — Design Spec

**Date:** 2026-06-01
**Status:** Approved

## Architecture Overview

Game loop logic lives in `Application::on_update()` — round timer, extraction detection, death handling. World reset via `World::resetWorld()`. Extraction point is a 3×3 GoldBlock platform placed on the terrain surface. Interaction uses existing event system (`IWorldEvent`).

---

## New Block: GoldBlock

| Property | Value |
|----------|-------|
| BlockId | `GoldBlock = 15` |
| Texture | `DefaultPack.png` tile (0, 1), `TexAll` |
| Material | `GOLD_BLOCK`, maxStack=99, isBlock=true, isCollidable=true |
| ShaderType | Chunk |

File: `Res/Blocks/GoldBlock.block`

---

## Extraction Point

- **Structure**: 3×3 GoldBlock platform placed on the surface (replaces existing surface blocks)
- **Position**: Completely random within world bounds
  - Center (x,z) ∈ [1, 126] to keep full 3×3 inside 0-127
  - Height = surfaceY + 1 at that position
  - Rejection: if any block in the 3×3 area at surface level is a tree (OakBark/OakLeaf), water, or cactus, re-roll
- **Timing**: Appears at t=5:00 (300s into the 600s round). Hidden before that.
- **Generation**: Per-round, during world reset or at t=5:00

---

## Interaction

- Player stands on any GoldBlock → 5-second extraction countdown displayed on HUD
- Player leaves GoldBlock area → countdown cancels
- Countdown reaches 0 → extraction success → settlement screen

---

## Three Settlement Outcomes

| Trigger | Items | Round # | Title |
|---------|-------|---------|-------|
| Extraction success | Keep all | +1 | "撤离成功" |
| Timer expires (0s) | Clear (safe slots preserved) | Unchanged | "时间耗尽" |
| Player death | Clear (safe slots preserved) | Unchanged | "你已死亡" |

Safe slots: inventory slots 17-19 (last 3). Equipment slots also cleared.

---

## Round Cycle

```
Round N start → World reset → Countdown 600s
  ├── 0-300s: Explore/collect, no extraction point
  ├── @300s: Extraction point spawns at random position
  ├── 300-600s: Player can extract
  ├── [Extraction success] → Settlement → [准备下一回合] → Round N+1
  ├── [Timer expires] → Settlement → [准备下一回合] → Round N (retry)
  └── [Death] → Settlement → [准备下一回合] → Round N (retry)
```

**"准备下一回合" button** on settlement screen calls `World::resetWorld()` + resets player state + starts new round timer.

---

## World Reset (`World::resetWorld()`)

1. Clear `m_dropItems`
2. Clear `m_pigmen`
3. Clear `m_events`
4. Iterate all chunks in `m_chunkManager`, call deleteMeshes(), then erase from map
5. Re-load terrain: `m_chunkManager` regenerates chunks via `FlatPlainGenerator`
6. Place 3×3 GoldBlock platform at extraction point position (if after 5min mark)
7. Re-spawn pigmen (max 8)

Player position reset to spawn point during reset.

---

## Round Data (adjusting Entry 5 stubs)

`Player.h` fields used by Entry 6:

```cpp
int m_roundNumber = 1;          // only increments on extraction
float m_roundTimeLeft = 600.0f; // countdown, managed in Application
int m_pigmanKills = 0;          // per-round kill count, reset on new round
std::unordered_map<Material::ID, int> m_roundCollection; // per-round, reset on new round
bool m_roundActive = false;     // round running
bool m_isDead = false;          // death state
float m_extractionProgress = 0.0f;  // 0→1 over 5s when on gold block
bool m_isExtracting = false;
// Removed: m_deathTimer (settlement stays open until button click)
```

---

## Collection Tracking

`Player::addItem()` already exists. Add one line:

```cpp
m_roundCollection[material.id]++;
```

---

## HUD Additions

- **Extraction countdown**: When standing on gold block, show "撤离中 X 秒" below crosshair (same draw list pattern)
- **Timer red flash**: When `m_roundTimeLeft <= 30`, timer text flashes red (revisit BitmapText limitation)
- **Timer real data**: Remove hardcoded "第 3 回合", use `m_roundNumber` and `m_roundTimeLeft`

---

## Modified Files

| File | Changes |
|------|---------|
| `Source/World/Block/BlockId.h` | Add `GoldBlock = 15` |
| `Source/Item/Material.h` | Add `GoldBlock` to enum, declare `GOLD_BLOCK` |
| `Source/Item/Material.cpp` | Define `GOLD_BLOCK`, wire toBlockID/toMaterial |
| `Source/World/Block/BlockDatabase.cpp` | Register GoldBlock default block |
| `Source/World/World.h` | Declare `resetWorld()`, extraction fields |
| `Source/World/World.cpp` | Implement `resetWorld()`, extraction generation |
| `Source/Application.h` | Round state fields if needed |
| `Source/Application.cpp` | Round timer logic, extraction detection, settlement button callback, remove test data |
| `Source/Player/Player.h` | Add `m_extractionProgress`, `m_isExtracting`, remove `m_deathTimer` |
| `Source/Player/Player.cpp` | Extraction HUD, settlement "准备下一回合" callback, collection tracking |
| `Res/Blocks/GoldBlock.block` | New file |
| `Res/Textures/DefaultPack.png` | Gold block texture at (0, 1) — user already added |
