# Entry Point 5: Game HUD — Design Spec

**Date:** 2026-06-01
**Status:** Approved

## Architecture: Approach A — ImGui Texture-Based HUD

All HUD elements rendered via ImGui draw lists + texture atlas, following existing patterns (crosshair, hotbar, equipment). No new rendering pipeline.

## Visual Style

Minecraft-style minimal: semi-transparent black backgrounds, white text, pixel art heart icons. All backgrounds use `IM_COL32(0, 0, 0, alpha)`.

---

## Data Interface (Entry 5 renders, Entry 6 fills)

Added to `Player.h`:

```cpp
int m_roundNumber = 1;                               // current round
float m_roundTimeLeft = 600.0f;                      // remaining seconds (10 min)
int m_pigmanKills = 0;                               // kill count
std::unordered_map<Material::ID, int> m_roundCollection; // items collected by type
bool m_roundActive = false;                          // round in progress
```

Existing fields reused: `m_hp` (100), `m_maxHp` (100), `m_isDead`.

---

## Component 1: Health Bar (Hearts)

**Asset:** CC0 heart sprites from [Pixel Healthbars and Hearts by unbreaded](https://unbreaded.itch.io/pixel-health#comments) — 16×16 full/half/empty hearts added to `DefaultPack.png` at unused tile coordinates (e.g. (14,1)/(15,1)/(13,1)).

**Rendering:** `ImGui::GetForegroundDrawList()->AddImage()` (same pattern as crosshair).

**Layout:**
- Position: screen top-left, (16, 16)
- 10 hearts in a row, each 24×24 px, 2 px gap
- Each heart = 10 HP; half heart = 5 HP
- `m_hp=100` → 10 full; `m_hp=63` → 6 full + 1 half + 3 empty

**Implementation:** `Player::drawHealthBar()`, called at end of `Player::draw()`.

**Transparency:** DrawList AddImage has no alpha channel — hearts always full opacity. Future: `m_hurtCooldown` flash effect.

---

## Component 2: Countdown Timer + Round Counter

**Layout:** Top-center bar, centered horizontally.

```
┌──────────────────────────┐
│   第 3 回合   │  07:32   │
└──────────────────────────┘
```

**Implementation:** ImGui window with `NoMove|NoResize|NoCollapse|NoTitleBar` flags.

**Style:**
- Background: `IM_COL32(0, 0, 0, 100)` — semi-transparent black
- Text: white, font size ~20px via ImGui default font
- Timer format: `MM:SS` (from `m_roundTimeLeft` seconds)
- Red flash when `m_roundTimeLeft <= 30`: red when `sin(time) > 0`, white otherwise
- Hidden when `m_roundActive == false`

**Z-order:** Registered after backpack/crafting in `Player::draw()`, below backpack layer.

---

## Component 3: Settlement Screen

**Trigger:** `(m_roundActive == false && m_roundNumber > 0)` or `m_isDead == true`.

**Layout:**

```
┌──────────────────────────────────────┐
│         ═══ 第 3 回合 结束 ═══        │
│                                      │
│      存活时间         09:45          │
│      击杀猪人            12          │
│      收集物品      8 类 / 47 个      │
│                                      │
│  ▶ 收集明细                          │
│  ┌──────────────────────────────┐   │
│  │ 草方块              ×12      │   │
│  │ 木材                 ×8      │   │
│  │ 石材                 ×6      │   │
│  │ ...                          │   │
│  └──────────────────────────────┘   │
│                                      │
│       [ 准备下一回合 ] (grey/flash)   │
└──────────────────────────────────────┘
```

**Implementation:**
- Outer: full-screen ImGui window, `NoMove|NoResize|NoTitleBar`, background `IM_COL32(0,0,0,180)`
- Inner: centered `ImGui::BeginChild` panel
- Summary text: rendered via BitmapText (Chinese support)
- Detail: `ImGui::CollapsingHeader("收集明细")` with 2-column `ImGui::Table` (item Chinese name | count), using existing `cnName()` mapping
- Bottom button: placeholder only, callback implemented in Entry 6

**Z-order:** Last in `Player::draw()`, on top of all other UI.

---

## Z-Order Summary (bottom → top)

1. Drop item world overlays (BackgroundDrawList)
2. Hotbar + Equipment (ImGui windows)
3. Health bar (ForegroundDrawList)
4. Countdown timer (ImGui window)
5. Crosshair + mining ring (ForegroundDrawList)
6. Backpack + Crafting (ImGui windows, only when open)
7. Settlement screen (ImGui window, only when triggered)

---

## Modified Files

| File | Changes |
|------|---------|
| `Source/Player/Player.h` | Add HUD data fields (`m_roundNumber`, `m_roundTimeLeft`, `m_pigmanKills`, `m_roundCollection`, `m_roundActive`); declare `drawHealthBar()`, `drawTimer()`, `drawSettlement()` |
| `Source/Player/Player.cpp` | Implement `drawHealthBar()`, `drawTimer()`, `drawSettlement()`; call them from `draw()`; extend `cnName` mapping for new materials |
| `Res/Textures/DefaultPack.png` | Add 3 heart texture tiles (full/half/empty) at unused coordinates |

No new files. No external library dependencies.

---

## Open-Source Assets

- **Heart sprites:** [Pixel Healthbars and Hearts (CC0) by unbreaded](https://unbreaded.itch.io/pixel-health#comments) — CC0 license, no attribution required
- Heart tiles placed at unused coordinates on existing 256×256 DefaultPack.png atlas

---

## Estimated Effort

| Component | Lines | Complexity |
|-----------|-------|------------|
| Heart textures → atlas | ~0 (image edit) | Low |
| `drawHealthBar()` | ~40 lines | Low |
| `drawTimer()` | ~50 lines | Low |
| `drawSettlement()` | ~80 lines | Medium |
| Data fields + declarations | ~15 lines | Low |
| `cnName` extension | ~5 lines | Low |
| **Total** | ~190 lines | 2-3 hours |
