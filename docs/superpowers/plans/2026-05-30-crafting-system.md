# Crafting System + Equipment System Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement 3×3 crafting grid, 2 recipes (OakBark→Stick→Sword), equipment slots with R-key switching, crosshair + mining progress ring, and attack/mining differentiation.

**Architecture:** Expand Material/BlockId enums for Stick/WoodenSword. OakBark reused directly. CraftingRecipe uses 3×3 pattern matching. Equipment slots and crafting grid are ImGui windows sharing the existing inventory drag-drop system. Mining changes from instant-break to progressive-hold, tracked via left-click timer.

**Tech Stack:** C++23, SFML3, OpenGL4.6, ImGui, GLM

---

## File Structure

| Action | File | Responsibility |
|--------|------|----------------|
| Modify | `Source/World/Block/BlockId.h` | Add Stick=12, WoodenSword=13 item block IDs |
| Create | `Res/Blocks/Stick.block` | Block data: texture coords, non-collidable |
| Create | `Res/Blocks/WoodenSword.block` | Block data: texture coords, non-collidable |
| Modify | `Source/World/Block/BlockDatabase.cpp` | Register Stick, WoodenSword in database |
| Modify | `Source/Item/Material.h` | Add Stick, WoodenSword IDs |
| Modify | `Source/Item/Material.cpp` | Static instances, toBlockID() |
| Create | `Source/Item/CraftingRecipe.h` | Recipe struct, matching function |
| Create | `Source/Item/CraftingRecipe.cpp` | Recipe registry, 2 initial recipes (OakBark→Stick→Sword) |
| Modify | `Source/Player/Player.h` | Equipment slots, craft grid, focus, mining timer, swing state |
| Modify | `Source/Player/Player.cpp` | Crafting UI, equipment window, weapon sprite, crosshair, mining ring |
| Modify | `Source/Application.cpp` | Left-click timing, R-key routing, mining progress |

---

### Task 1: Extend BlockId and BlockDatabase for new items

**Files:**
- Modify: `MineCraft-One-Week-Challenge/Source/World/Block/BlockId.h`
- Create: `MineCraft-One-Week-Challenge/Res/Blocks/Stick.block`
- Create: `MineCraft-One-Week-Challenge/Res/Blocks/WoodenSword.block`
- Modify: `MineCraft-One-Week-Challenge/Source/World/Block/BlockDatabase.cpp`

- [ ] **Step 1: Add Stick and WoodenSword to BlockId enum**

In `BlockId.h`, before `NUM_TYPES`:
```cpp
Stick = 12,
WoodenSword = 13,
```

- [ ] **Step 2: Create Stick.block data file**

Create `Res/Blocks/Stick.block`:
```
Name
Stick

Id
12

TexAll
4 0

Opaque
0

MeshType
0

ShaderType
0

Collidable
0
```

(Note: TexAll (4,0) is OakBark side texture as placeholder. User should add proper pixel art at an unused tile and update coordinates.)

- [ ] **Step 3: Create WoodenSword.block data file**

Create `Res/Blocks/WoodenSword.block`:
```
Name
WoodenSword

Id
13

TexAll
4 0

Opaque
0

MeshType
0

ShaderType
0

Collidable
0
```

- [ ] **Step 4: Register new blocks in BlockDatabase**

In `BlockDatabase.cpp`, after the DeadShrub line:
```cpp
m_blocks[(int)BlockId::Stick] = std::make_unique<DefaultBlock>("Stick");
m_blocks[(int)BlockId::WoodenSword] = std::make_unique<DefaultBlock>("WoodenSword");
```

- [ ] **Step 5: Build and verify compilation**

```bash
cd MineCraft-One-Week-Challenge && cmake --build build --config Debug 2>&1 | tail -20
```
Expected: Build succeeds with no errors.

- [ ] **Step 6: Commit**

```bash
git add MineCraft-One-Week-Challenge/Source/World/Block/BlockId.h \
        MineCraft-One-Week-Challenge/Source/World/Block/BlockDatabase.cpp \
        MineCraft-One-Week-Challenge/Res/Blocks/Stick.block \
        MineCraft-One-Week-Challenge/Res/Blocks/WoodenSword.block
git commit -m "feat: add Stick and WoodenSword block IDs and data files"
```

---

### Task 2: Expand Material system (Stick + WoodenSword only)

OakBark is reused directly; no separate WoodItem needed.

**Files:**
- Modify: `MineCraft-One-Week-Challenge/Source/Item/Material.h`
- Modify: `MineCraft-One-Week-Challenge/Source/Item/Material.cpp`

- [ ] **Step 1: Add new Material::ID values**

In `Material.h`, add to the `ID` enum before the closing `};`:
```cpp
Stick,
WoodenSword
```

Add new static const declarations:
```cpp
const static Material STICK, WOODEN_SWORD;
```

- [ ] **Step 2: Add static Material instances**

In `Material.cpp`, add after DeadShrub:
```cpp
const Material Material::STICK(ID::Stick, 99, false, "Stick");
const Material Material::WOODEN_SWORD(ID::WoodenSword, 1, false, "Wooden Sword");
```

- [ ] **Step 3: Update toBlockID()**

In `Material.cpp` `toBlockID()`, add to the switch:
```cpp
case Stick:
    return BlockId::Stick;
case WoodenSword:
    return BlockId::WoodenSword;
```

- [ ] **Step 4: Build and verify**

```bash
cd MineCraft-One-Week-Challenge && cmake --build build --config Debug 2>&1 | tail -20
```

- [ ] **Step 5: Commit**

```bash
git add MineCraft-One-Week-Challenge/Source/Item/Material.h \
        MineCraft-One-Week-Challenge/Source/Item/Material.cpp
git commit -m "feat: add Stick and WoodenSword materials"
```

---

### Task 3: Create CraftingRecipe system

**Files:**
- Create: `MineCraft-One-Week-Challenge/Source/Item/CraftingRecipe.h`
- Create: `MineCraft-One-Week-Challenge/Source/Item/CraftingRecipe.cpp`

- [ ] **Step 1: Create CraftingRecipe.h**

```cpp
#pragma once

#include "Material.h"
#include <vector>

struct CraftingRecipe {
    const Material* pattern[9];  // 3×3 grid, nullptr = empty
    const Material* output;
    int outputCount;
};

extern std::vector<CraftingRecipe> g_recipes;

/// Find the first recipe matching the given 3×3 grid.
/// Returns nullptr if no recipe matches.
const CraftingRecipe* findMatchingRecipe(const ItemStack grid[9]);

void initCraftingRecipes();
```

- [ ] **Step 2: Create CraftingRecipe.cpp**

```cpp
#include "CraftingRecipe.h"
#include "ItemStack.h"

std::vector<CraftingRecipe> g_recipes;

void initCraftingRecipes()
{
    // 3 OakBark (left column) → 1 Stick
    g_recipes.push_back({{
        &Material::OAK_BARK_BLOCK, nullptr, nullptr,
        &Material::OAK_BARK_BLOCK, nullptr, nullptr,
        &Material::OAK_BARK_BLOCK, nullptr, nullptr,
    }, &Material::STICK, 1});

    // 3 Stick (left column) → 1 WoodenSword
    g_recipes.push_back({{
        &Material::STICK, nullptr, nullptr,
        &Material::STICK, nullptr, nullptr,
        &Material::STICK, nullptr, nullptr,
    }, &Material::WOODEN_SWORD, 1});
}

const CraftingRecipe* findMatchingRecipe(const ItemStack grid[9])
{
    for (const auto& recipe : g_recipes) {
        bool match = true;
        for (int i = 0; i < 9; i++) {
            const Material* expected = recipe.pattern[i];
            const Material::ID actualID = grid[i].getMaterial().id;
            if (expected == nullptr) {
                if (actualID != Material::ID::Nothing) {
                    match = false;
                    break;
                }
            } else {
                if (actualID != expected->id) {
                    match = false;
                    break;
                }
            }
        }
        if (match) return &recipe;
    }
    return nullptr;
}
```

- [ ] **Step 3: Build and verify**

```bash
cd MineCraft-One-Week-Challenge && cmake --build build --config Debug 2>&1 | tail -20
```

- [ ] **Step 4: Commit**

```bash
git add MineCraft-One-Week-Challenge/Source/Item/CraftingRecipe.h \
        MineCraft-One-Week-Challenge/Source/Item/CraftingRecipe.cpp
git commit -m "feat: add CraftingRecipe system with 2 initial recipes"
```

---

### Task 4: Add crafting, equipment, and combat state to Player.h

**Files:**
- Modify: `MineCraft-One-Week-Challenge/Source/Player/Player.h`

- [ ] **Step 1: Add includes and new members to Player.h**

Add includes after existing ones:
```cpp
#include "../Item/CraftingRecipe.h"
```

Add new members in the **public:** section (Application.cpp needs direct access to mining state):
```cpp
// Equipment system
ItemStack m_equipment[2] = {ItemStack(Material::NOTHING, 0), ItemStack(Material::NOTHING, 0)};
enum class Focus { Hotbar, MainHand, OffHand };
Focus m_focus = Focus::Hotbar;

// Crafting system
ItemStack m_craftGrid[9];
const CraftingRecipe* m_currentRecipe = nullptr;

// Combat / mining (public for Application access)
float m_leftHoldTime = 0.0f;
bool m_leftHeld = false;
float m_miningProgress = 0.0f;
bool m_isMining = false;
glm::ivec3 m_miningTarget{0, -999, 0};
```

Add new members in the **private:** section:
```cpp
// Weapon animation
float m_swingAngle = 0.0f;
float m_swingTimer = 0.0f;
bool m_isSwinging = false;

// Keys
ToggleKey m_equipKey;  // R

// Mouse locked state (for crosshair)
bool m_mouseLocked = true;
```

Add public declarations:
```cpp
// Input processing
void processLeftClick(bool pressed, bool justPressed, bool justReleased, float dt, World& world);
void processRKey();

// Getters for Application
float getMiningProgress() const { return m_miningProgress; }
bool isMining() const { return m_isMining; }
bool isMouseLockedForUI() const { return m_mouseLocked && !m_backpackOpen; }
```

- [ ] **Step 2: Initialize craft grid in constructor**

In `Player.cpp` constructor, after the m_items initialization loop:
```cpp
for (int i = 0; i < 9; i++) {
    m_craftGrid[i] = ItemStack(Material::NOTHING, 0);
}
```

- [ ] **Step 3: Add R key initialization to constructor initializer list**

Add to the initializer list:
```cpp
, m_equipKey(sf::Keyboard::Key::R)
```

- [ ] **Step 4: Build and verify**

```bash
cd MineCraft-One-Week-Challenge && cmake --build build --config Debug 2>&1 | tail -20
```
Expected: Compiles. May have unused member warnings (will be used in next tasks).

- [ ] **Step 5: Commit**

```bash
git add MineCraft-One-Week-Challenge/Source/Player/Player.h \
        MineCraft-One-Week-Challenge/Source/Player/Player.cpp
git commit -m "feat: add equipment, crafting, and combat state to Player"
```

---

### Task 5: Implement crafting grid UI in Player::draw()

**Files:**
- Modify: `MineCraft-One-Week-Challenge/Source/Player/Player.cpp`

- [ ] **Step 1: Modify Player::draw() — restructure backpack window to include crafting grid above it**

The backpack window now contains: crafting grid (top) + inventory (bottom). In `draw()`, change the backpack section:

After the `m_backpackOpen` check, replace the backpack window code. The new window is taller to fit the grid:

```cpp
if (m_backpackOpen)
{
    const int craftRows = 3;
    const int craftCols = 3;
    float craftContentH = craftRows * (slotSize + padding) + padding;
    // Extra space for result slot (to the right of grid row 1)
    const int bpRows = 3;
    float bpContentH = bpRows * (slotSize + padding) + padding;
    float separatorH = 12.0f; // gap between grid and inventory
    float totalH = craftContentH + separatorH + bpContentH;

    float winX = (displaySize.x - contentW) * 0.5f;
    float winY = hotbarY - totalH - 6.0f;

    ImGui::SetNextWindowPos(ImVec2(winX, winY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(contentW, totalH), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    if (ImGui::Begin("BackpackAndCraft", nullptr, winFlags))
    {
        // --- 3×3 Crafting Grid ---
        for (int row = 0; row < craftRows; row++)
        {
            for (int col = 0; col < craftCols; col++)
            {
                int slotIdx = row * craftCols + col;
                float x = padding + col * (slotSize + padding);
                float y = padding + row * (slotSize + padding);
                ImGui::SetCursorPos(ImVec2(x, y));

                ImGui::PushID(100 + slotIdx); // unique ID range for grid
                ImVec2 p0 = ImGui::GetCursorScreenPos();
                ImGui::Dummy(ImVec2(slotSize, slotSize));
                ImVec2 p1(p0.x + slotSize, p0.y + slotSize);

                // Draw slot background
                ImGui::GetWindowDrawList()->AddRectFilled(p0, p1, IM_COL32(60, 60, 60, 200));
                ImGui::GetWindowDrawList()->AddRect(p0, p1, IM_COL32(180, 180, 180, 255));

                // Draw item icon if present
                const auto& mat = m_craftGrid[slotIdx].getMaterial();
                if (mat.id != Material::ID::Nothing)
                {
                    BlockId bId = mat.toBlockID();
                    const auto& bd = BlockDatabase::get().getData(bId);
                    auto uv = atlas.getTexture(bd.getBlockData().texTopCoord);
                    ImGui::GetWindowDrawList()->AddImage(
                        (ImTextureID)(intptr_t)atlasID,
                        ImVec2(p0.x + texPad, p0.y + texPad),
                        ImVec2(p1.x - texPad, p1.y - texPad),
                        ImVec2(uv[2], uv[5]), ImVec2(uv[0], uv[3]));
                }

                // Drop target: accept drag from inventory (1 item)
                if (ImGui::BeginDragDropTarget())
                {
                    const ImGuiPayload* pl = ImGui::AcceptDragDropPayload("INV_SLOT");
                    if (pl)
                    {
                        int src = *(const int*)pl->Data;
                        const auto& srcMat = m_items[src].getMaterial();
                        if (srcMat.id != Material::ID::Nothing)
                        {
                            // Move 1 item from inventory to grid
                            if (m_craftGrid[slotIdx].getMaterial().id == Material::ID::Nothing)
                            {
                                m_craftGrid[slotIdx] = ItemStack(srcMat, 1);
                            }
                            else
                            {
                                m_craftGrid[slotIdx].add(1);
                            }
                            m_items[src].remove();
                            m_currentRecipe = findMatchingRecipe(m_craftGrid);
                        }
                    }
                    ImGui::EndDragDropTarget();
                }

                // Left-click grid slot: return item to inventory
                if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                {
                    const auto& gridMat = m_craftGrid[slotIdx].getMaterial();
                    if (gridMat.id != Material::ID::Nothing)
                    {
                        bool added = false;
                        // Try merge into existing stack
                        for (int i = 0; i < 20; i++)
                        {
                            if (m_items[i].getMaterial().id == gridMat.id)
                            {
                                int leftover = m_items[i].add(1);
                                if (leftover == 0) { added = true; break; }
                            }
                        }
                        // Try empty slot
                        if (!added)
                        {
                            for (int i = 0; i < 20; i++)
                            {
                                if (m_items[i].getMaterial().id == Material::ID::Nothing)
                                {
                                    m_items[i] = ItemStack(gridMat, 1);
                                    added = true;
                                    break;
                                }
                            }
                        }
                        if (added)
                        {
                            m_craftGrid[slotIdx].remove();
                            m_currentRecipe = findMatchingRecipe(m_craftGrid);
                        }
                    }
                }

                ImGui::PopID();
            }
        }

        // --- Result Slot (to the right of grid row 1) ---
        {
            float rx = padding + craftCols * (slotSize + padding) + 16.0f;
            float ry = padding + 1 * (slotSize + padding); // aligned with row 1
            ImGui::SetCursorPos(ImVec2(rx, ry));
            ImGui::PushID(200);
            ImVec2 rp0 = ImGui::GetCursorScreenPos();
            ImGui::Dummy(ImVec2(slotSize, slotSize));
            ImVec2 rp1(rp0.x + slotSize, rp0.y + slotSize);

            ImGui::GetWindowDrawList()->AddRectFilled(rp0, rp1, IM_COL32(50, 50, 50, 200));
            ImGui::GetWindowDrawList()->AddRect(rp0, rp1, IM_COL32(200, 200, 100, 255));

            if (m_currentRecipe != nullptr)
            {
                BlockId bId = m_currentRecipe->output->toBlockID();
                const auto& bd = BlockDatabase::get().getData(bId);
                auto uv = atlas.getTexture(bd.getBlockData().texTopCoord);
                ImGui::GetWindowDrawList()->AddImage(
                    (ImTextureID)(intptr_t)atlasID,
                    ImVec2(rp0.x + texPad, rp0.y + texPad),
                    ImVec2(rp1.x - texPad, rp1.y - texPad),
                    ImVec2(uv[2], uv[5]), ImVec2(uv[0], uv[3]));
            }

            // Click result slot: claim output
            if (m_currentRecipe && ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                const Material& outMat = *m_currentRecipe->output;
                if (addItem(outMat))
                {
                    // Deduct one set of materials
                    for (int i = 0; i < 9; i++)
                    {
                        if (m_currentRecipe->pattern[i] != nullptr)
                        {
                            m_craftGrid[i].remove();
                        }
                    }
                    m_currentRecipe = findMatchingRecipe(m_craftGrid);
                }
            }
            ImGui::PopID();
        }

        // --- Separator ---
        ImGui::SetCursorPosY(craftContentH + 4.0f);
        ImGui::Separator();

        // --- 3×5 Inventory (below separator) ---
        ImGui::SetCursorPosY(craftContentH + separatorH);
        for (int bpRow = 0; bpRow < bpRows; bpRow++)
        {
            for (int col = 0; col < 5; col++)
            {
                int slotIndex = 5 + bpRow * 5 + col;
                float x = padding + col * (slotSize + padding);
                float y = craftContentH + separatorH + padding + bpRow * (slotSize + padding);
                ImGui::SetCursorPos(ImVec2(x, y));

                ImGui::PushID(slotIndex);
                ImVec2 p0 = ImGui::GetCursorScreenPos();
                ImGui::Dummy(ImVec2(slotSize, slotSize));
                ImVec2 p1(p0.x + slotSize, p0.y + slotSize);
                drawSlot(slotIndex, p0, p1, false);
                drawQuantity(slotIndex, p1);

                const auto& mat = m_items[slotIndex].getMaterial();
                if (mat.id != Material::ID::Nothing &&
                    ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
                {
                    ImGui::SetDragDropPayload("INV_SLOT", &slotIndex, sizeof(int));
                    BlockId bId = mat.toBlockID();
                    const auto& bd = BlockDatabase::get().getData(bId);
                    auto uv = atlas.getTexture(bd.getBlockData().texTopCoord);
                    ImGui::Image((ImTextureID)(intptr_t)atlasID,
                                 ImVec2(slotSize * 0.7f, slotSize * 0.7f),
                                 ImVec2(uv[2], uv[5]), ImVec2(uv[0], uv[3]));
                    ImGui::EndDragDropSource();
                }
                if (ImGui::BeginDragDropTarget())
                {
                    const ImGuiPayload* pl = ImGui::AcceptDragDropPayload("INV_SLOT");
                    if (pl)
                    {
                        int src = *(const int*)pl->Data;
                        std::swap(m_items[src], m_items[slotIndex]);
                    }
                    ImGui::EndDragDropTarget();
                }
                ImGui::PopID();
            }
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
}
```

- [ ] **Step 2: Build and verify**

```bash
cd MineCraft-One-Week-Challenge && cmake --build build --config Debug 2>&1 | tail -30
```
Expected: Build succeeds. Check that `findMatchingRecipe` is linked correctly.

- [ ] **Step 3: Run and visually test crafting grid**

Launch the game, press B, verify:
- 3×3 grid appears above inventory
- Result slot appears to the right
- Items can be dragged from inventory to grid
- Clicking grid slots returns items

- [ ] **Step 4: Commit**

```bash
git add MineCraft-One-Week-Challenge/Source/Player/Player.cpp
git commit -m "feat: implement crafting grid UI with drag-drop and result slot"
```

---

### Task 6: Implement equipment slots window

**Files:**
- Modify: `MineCraft-One-Week-Challenge/Source/Player/Player.cpp`

- [ ] **Step 1: Add equipment window rendering in Player::draw()**

After the hotbar window code and before the drop item code, add:

```cpp
// --- Equipment window (2 slots, left of hotbar) ---
{
    float eqSlots = 2;
    float eqContentW = eqSlots * (slotSize + padding) + padding;
    float eqContentH = 1 * (slotSize + padding) + padding;
    float eqY = hotbarY; // align with hotbar vertically
    float eqX = hotbarX - eqContentW - 6.0f; // left of hotbar with gap

    ImGui::SetNextWindowPos(ImVec2(eqX, eqY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(eqContentW, eqContentH), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    if (ImGui::Begin("Equipment", nullptr, winFlags))
    {
        for (int i = 0; i < 2; i++)
        {
            float x = padding + i * (slotSize + padding);
            float y = padding;
            ImGui::SetCursorPos(ImVec2(x, y));

            ImGui::PushID(300 + i);
            ImVec2 p0 = ImGui::GetCursorScreenPos();
            ImGui::Dummy(ImVec2(slotSize, slotSize));
            ImVec2 p1(p0.x + slotSize, p0.y + slotSize);

            // Highlight if focused
            ImU32 bgColor = (m_focus == (i == 0 ? Focus::MainHand : Focus::OffHand))
                ? IM_COL32(255, 215, 0, 60)
                : IM_COL32(50, 50, 50, 200);
            ImGui::GetWindowDrawList()->AddRectFilled(p0, p1, bgColor);
            ImGui::GetWindowDrawList()->AddRect(p0, p1, IM_COL32(180, 180, 180, 255));

            const auto& mat = m_equipment[i].getMaterial();
            if (mat.id != Material::ID::Nothing)
            {
                BlockId bId = mat.toBlockID();
                const auto& bd = BlockDatabase::get().getData(bId);
                auto uv = atlas.getTexture(bd.getBlockData().texTopCoord);
                ImGui::GetWindowDrawList()->AddImage(
                    (ImTextureID)(intptr_t)atlasID,
                    ImVec2(p0.x + texPad, p0.y + texPad),
                    ImVec2(p1.x - texPad, p1.y - texPad),
                    ImVec2(uv[2], uv[5]), ImVec2(uv[0], uv[3]));
            }

            // Drop target: accept drag from inventory (1 item)
            if (ImGui::BeginDragDropTarget())
            {
                const ImGuiPayload* pl = ImGui::AcceptDragDropPayload("INV_SLOT");
                if (pl)
                {
                    int src = *(const int*)pl->Data;
                    const auto& srcMat = m_items[src].getMaterial();
                    if (srcMat.id != Material::ID::Nothing)
                    {
                        // Return existing equipment to inventory
                        const auto& oldMat = m_equipment[i].getMaterial();
                        if (oldMat.id != Material::ID::Nothing)
                        {
                            addItem(oldMat);
                        }
                        // Place 1 from source
                        m_equipment[i] = ItemStack(srcMat, 1);
                        m_items[src].remove();
                    }
                }
                ImGui::EndDragDropTarget();
            }

            // Left-click equipment slot: return to inventory, switch focus
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                const auto& eqMat = m_equipment[i].getMaterial();
                if (eqMat.id != Material::ID::Nothing)
                {
                    addItem(eqMat);
                    m_equipment[i] = ItemStack(Material::NOTHING, 0);
                }
                m_focus = (i == 0) ? Focus::MainHand : Focus::OffHand;
            }

            ImGui::PopID();
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
}
```

- [ ] **Step 2: Build and verify**

```bash
cd MineCraft-One-Week-Challenge && cmake --build build --config Debug 2>&1 | tail -20
```

- [ ] **Step 3: Run and test equipment window**

Launch game, verify equipment window appears left of hotbar. Drag items in, click to remove.

- [ ] **Step 4: Commit**

```bash
git add MineCraft-One-Week-Challenge/Source/Player/Player.cpp
git commit -m "feat: add equipment slots window with drag-drop interaction"
```

---

### Task 7: Wire input logic (R key, attack/mining, 1-5 hotkeys)

**Files:**
- Modify: `MineCraft-One-Week-Challenge/Source/Player/Player.cpp`
- Modify: `MineCraft-One-Week-Challenge/Source/Application.cpp`

- [ ] **Step 1: Add processRKey() and update 1-5 key handlers**

In `Player.cpp`, add `processRKey()`:
```cpp
void Player::processRKey()
{
    if (!m_equipKey.isKeyPressed()) return;

    switch (m_focus) {
        case Focus::Hotbar:
            m_focus = Focus::MainHand;
            break;
        case Focus::MainHand:
            m_focus = Focus::OffHand;
            break;
        case Focus::OffHand:
            m_focus = Focus::MainHand;
            break;
    }
}
```

In `handleInput()`, call `processRKey()` before the existing `m_itemDown` check:
```cpp
processRKey();
```

- [ ] **Step 2: Update mouseInput() to track m_mouseLocked**

In `Player::mouseInput()`, add at the top of the function:
```cpp
m_mouseLocked = !sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LAlt) && !m_backpackOpen;
```

- [ ] **Step 4: Update 1-5 keys to switch focus to Hotbar**

In `handleInput()`, modify each num key handler to also set focus:
```cpp
if (m_num1.isKeyPressed()) { m_heldItem = 0; m_focus = Focus::Hotbar; }
if (m_num2.isKeyPressed()) { m_heldItem = 1; m_focus = Focus::Hotbar; }
// ... same for 3,4,5
```

- [ ] **Step 5: Add processLeftClick() to Player.cpp**

```cpp
void Player::processLeftClick(bool pressed, bool justPressed, bool justReleased,
                               float dt, World& world)
{
    if (pressed) {
        m_leftHoldTime += dt;
    }

    if (justReleased && m_leftHeld) {
        if (m_leftHoldTime < 0.5f) {
            // Attack! Trigger swing animation
            m_isSwinging = true;
            m_swingTimer = 0.0f;
        }
        // Reset mining state
        m_isMining = false;
        m_miningProgress = 0.0f;
        m_leftHoldTime = 0.0f;
        m_leftHeld = false;
        m_miningTarget = {0, -999, 0};
    }

    if (justPressed) {
        m_leftHeld = true;
        m_leftHoldTime = 0.0f;
    }
}
```

- [ ] **Step 6: Modify Application::on_update()**

Replace the current left-click handling in `on_update()` (lines 53-68 in the ray loop):

```cpp
// Check left/right mouse state for attack/mining differentiation
bool leftPressed = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
bool leftJustPressed = !m_prevLeftPressed && leftPressed;
bool leftJustReleased = m_prevLeftPressed && !leftPressed;
m_prevLeftPressed = leftPressed;

bool rightPressed = sf::Mouse::isButtonPressed(sf::Mouse::Button::Right);

m_player.processLeftClick(leftPressed, leftJustPressed, leftJustReleased,
                          dt.asSeconds(), m_world);

if (!sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LAlt))
{
    for (Ray ray({m_player.position.x, m_player.position.y + 0.6f, m_player.position.z},
                 m_player.rotation);
         ray.getLength() < 6; ray.step(0.05f))
    {
        int x = static_cast<int>(ray.getEnd().x);
        int y = static_cast<int>(ray.getEnd().y);
        int z = static_cast<int>(ray.getEnd().z);

        auto block = m_world.getBlock(x, y, z);
        auto id = (BlockId)block.id;

        if (id != BlockId::Air && id != BlockId::Water)
        {
            // Mining: left held >= 0.5s → progressive block breaking
            if (leftPressed && m_player.m_leftHoldTime >= 0.5f)
            {
                if (!m_player.m_isMining)
                {
                    m_player.m_isMining = true;
                    m_player.m_miningTarget = {x, y, z};
                    m_player.m_miningProgress = 0.0f;
                }
                else if (m_player.m_miningTarget == glm::ivec3(x, y, z))
                {
                    // TODO: vary break time per block type
                    float breakTime = 1.0f;
                    m_player.m_miningProgress += dt.asSeconds() / breakTime;
                    if (m_player.m_miningProgress >= 1.0f)
                    {
                        m_player.m_miningProgress = 0.0f;
                        m_player.m_isMining = false;
                        m_world.addEvent<PlayerDigEvent>(sf::Mouse::Button::Left,
                                                         ray.getEnd(), m_player);
                        m_player.m_miningTarget = {0, -999, 0};
                    }
                }
                else
                {
                    // Target changed, reset
                    m_player.m_isMining = true;
                    m_player.m_miningTarget = {x, y, z};
                    m_player.m_miningProgress = 0.0f;
                }
                break;
            }

            // Right-click: instant placement (unchanged)
            if (rightPressed && m_rightClickTimer.getElapsedTime().asSeconds() > 0.2f)
            {
                m_rightClickTimer.restart();
                m_world.addEvent<PlayerDigEvent>(sf::Mouse::Button::Right, lastPosition,
                                                 m_player);
                break;
            }
            lastPosition = ray.getEnd();
        }
        lastPosition = ray.getEnd();
    }
}
```

In `Application.h`, add:
```cpp
bool m_prevLeftPressed = false;
sf::Clock m_rightClickTimer;
```

- [ ] **Step 7: Build and verify**

```bash
cd MineCraft-One-Week-Challenge && cmake --build build --config Debug 2>&1 | tail -30
```

- [ ] **Step 8: Commit**

```bash
git add MineCraft-One-Week-Challenge/Source/Player/Player.cpp \
        MineCraft-One-Week-Challenge/Source/Application.cpp \
        MineCraft-One-Week-Challenge/Source/Application.h
git commit -m "feat: implement R-key switching, attack/mining differentiation, 1-5 hotkeys"
```

---

### Task 8: Implement crosshair, mining ring, and weapon sprite

**Files:**
- Modify: `MineCraft-One-Week-Challenge/Source/Player/Player.cpp`

- [ ] **Step 1: Add crosshair and mining ring in Player::draw()**

Before the drop item rendering code, add:

```cpp
// --- Crosshair + Mining Progress Ring ---
if (m_mouseLocked && !m_backpackOpen)
{
    ImVec2 center(displaySize.x * 0.5f, displaySize.y * 0.5f);
    auto* fgDraw = ImGui::GetForegroundDrawList();

    // White crosshair
    fgDraw->AddLine(
        ImVec2(center.x - 8, center.y),
        ImVec2(center.x + 8, center.y),
        IM_COL32(255, 255, 255, 200), 1.5f);
    fgDraw->AddLine(
        ImVec2(center.x, center.y - 8),
        ImVec2(center.x, center.y + 8),
        IM_COL32(255, 255, 255, 200), 1.5f);

    // Mining progress ring
    if (m_isMining && m_miningProgress > 0.0f)
    {
        float ringRadius = 16.0f;
        int numSegments = 32;
        float fullAngle = m_miningProgress * 2.0f * 3.14159265f;
        ImVec2 prevPt(center.x + ringRadius, center.y);
        for (int i = 1; i <= numSegments; i++)
        {
            float t = (float)i / (float)numSegments;
            float angle = t * fullAngle;
            if (angle > fullAngle) break;
            ImVec2 pt(center.x + ringRadius * cosf(angle - 3.14159265f / 2.0f),
                      center.y + ringRadius * sinf(angle - 3.14159265f / 2.0f));
            fgDraw->AddLine(prevPt, pt, IM_COL32(255, 255, 255, 220), 2.0f);
            prevPt = pt;
        }
    }
}
```

- [ ] **Step 2: Add weapon sprite rendering + swing animation**

After the equipment window code and before the drop item code, add:

```cpp
// --- Weapon sprite (bottom-right of screen) ---
if (!m_backpackOpen && m_focus != Focus::Hotbar)
{
    int eqIdx = (m_focus == Focus::MainHand) ? 0 : 1;
    const auto& eqMat = m_equipment[eqIdx].getMaterial();
    if (eqMat.id != Material::ID::Nothing)
    {
        // Update swing animation
        if (m_isSwinging)
        {
            m_swingTimer += 0.016f; // approximate frame time
            float t = m_swingTimer / 0.3f;
            if (t >= 1.0f)
            {
                t = 1.0f;
                m_isSwinging = false;
                m_swingTimer = 0.0f;
            }
            m_swingAngle = sinf(t * 3.14159265f) * 30.0f;
        }
        else
        {
            m_swingAngle = 0.0f;
        }

        BlockId bId = eqMat.toBlockID();
        const auto& bd = BlockDatabase::get().getData(bId);
        auto uv = atlas.getTexture(bd.getBlockData().texTopCoord);

        float weaponSize = 64.0f;
        float wx = displaySize.x - weaponSize - 32.0f;
        float wy = displaySize.y - weaponSize - 16.0f;

        ImVec2 center(wx + weaponSize * 0.5f, wy + weaponSize * 0.5f);
        float rad = m_swingAngle * 3.14159265f / 180.0f;

        // 4 corners rotated around center
        auto rotPt = [&](float ox, float oy) -> ImVec2 {
            float dx = ox - center.x;
            float dy = oy - center.y;
            float cr = cosf(rad), sr = sinf(rad);
            return ImVec2(center.x + dx * cr - dy * sr,
                          center.y + dx * sr + dy * cr);
        };

        ImVec2 c0 = rotPt(wx, wy);
        ImVec2 c1 = rotPt(wx + weaponSize, wy);
        ImVec2 c2 = rotPt(wx + weaponSize, wy + weaponSize);
        ImVec2 c3 = rotPt(wx, wy + weaponSize);

        ImGui::GetForegroundDrawList()->AddImageQuad(
            (ImTextureID)(intptr_t)atlasID,
            ImVec2(uv[2], uv[5]),  // top-left UV
            ImVec2(uv[0], uv[5]),  // top-right UV (swapped for correct orientation)
            ImVec2(uv[0], uv[3]),  // bottom-right UV
            ImVec2(uv[2], uv[3]),  // bottom-left UV
            c0, c1, c2, c3,
            IM_COL32(255, 255, 255, 255));
    }
}
```

- [ ] **Step 3: Build and verify**

```bash
cd MineCraft-One-Week-Challenge && cmake --build build --config Debug 2>&1 | tail -30
```

- [ ] **Step 4: Run and visually verify**

Launch game, verify:
- Crosshair visible when mouse locked, hidden when B or Alt pressed
- Mining progress ring fills when holding left click ≥0.5s
- Weapon sprite appears bottom-right when equipment slot selected
- Weapon swings on attack (left click <0.5s)

- [ ] **Step 5: Commit**

```bash
git add MineCraft-One-Week-Challenge/Source/Player/Player.cpp
git commit -m "feat: add crosshair, mining progress ring, and weapon sprite rendering"
```

---

### Task 9: Integration testing and fixes

**Files:**
- Modify: `MineCraft-One-Week-Challenge/Source/Main.cpp` (only if recipe init needed)

- [ ] **Step 1: Add recipe initialization call**

In `Main.cpp`, after `BlockDatabase::get();` in the game initialization (or in the Application constructor after BlockDatabase::get()), add:
```cpp
initCraftingRecipes();
```

Check if there's a call to `BlockDatabase::get()` in `Application.cpp` constructor — yes, line 21. In `Application.cpp` constructor, after `BlockDatabase::get();`:
```cpp
#include "../Item/CraftingRecipe.h"
// in constructor:
initCraftingRecipes();
```

- [ ] **Step 2: Full build**

```bash
cd MineCraft-One-Week-Challenge && cmake --build build --config Debug 2>&1
```
Expected: Zero errors.

- [ ] **Step 3: Integration test checklist**

Run the game and verify:
1. Press B → backpack window shows crafting grid (3×3) + inventory (3×5)
2. Break tree → OakBark drops OAK_BARK_BLOCK (unchanged behavior)
3. Drag OakBark to crafting grid left column (3 slots) → Stick appears in result slot
4. Click result slot → Stick added to inventory, WoodItem consumed
5. Drag 3 Sticks to crafting grid left column → WoodenSword appears in result slot
6. Click result slot → WoodenSword added to inventory
7. Drag WoodenSword to main-hand equipment slot → weapon sprite appears bottom-right
8. Press R → focus switches to main-hand, then off-hand, then main-hand
9. Press 1-5 → focus returns to hotbar
10. Left click (tap <0.5s) → weapon swing animation triggers
11. Left click (hold ≥0.5s on block) → mining progress ring fills, block breaks when full
12. Right click → still places blocks
13. Crosshair is visible when mouse locked
14. Press B or Alt → crosshair hidden
15. Close backpack (B) → crafting grid contents auto-return to inventory

- [ ] **Step 4: Fix any issues found**

- [ ] **Step 5: Final commit**

```bash
git add MineCraft-One-Week-Challenge/Source/Application.cpp
git commit -m "feat: wire recipe initialization and integration fixes"
```
