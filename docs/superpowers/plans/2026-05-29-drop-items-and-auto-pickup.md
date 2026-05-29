# Drop Items + Auto-Pickup Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** When a block is broken, spawn a physical drop entity that falls to ground and renders as an ImGui overlay icon. Player auto-picks up within 2 blocks. Inventory expanded from 5 to 20 slots with ImGui drag-and-drop backpack.

**Architecture:** One new struct (`ItemDropEntity`), no new shaders/renderers. Drops stored in `World::m_dropItems`, physics in `World::update()`, icons projected from 3D→2D in `Player::draw()` via ImGui. Backpack uses ImGui built-in drag-and-drop API (payload = slot index).

**Tech Stack:** C++23, glm, ImGui 1.92.8 + imgui-sfml v3, SFML 3, OpenGL 4.6

---

### Task 1: Create ItemDropEntity header

**Files:**
- Create: `MineCraft-One-Week-Challenge/Source/Entity/ItemDropEntity.h`

- [ ] **Step 1: Write ItemDropEntity.h**

```cpp
#ifndef ITEMDROPENTITY_H_INCLUDED
#define ITEMDROPENTITY_H_INCLUDED

#include "../Maths/glm.h"
#include "../Item/Material.h"

struct ItemDropEntity {
    glm::vec3 position;
    glm::vec3 velocity;
    const Material* material;
    float lifeTime = 0.0f;
    bool onGround = false;
    bool alive = true;
};

#endif // ITEMDROPENTITY_H_INCLUDED
```

- [ ] **Step 2: Verify the file compiles**

Build the project to confirm no syntax errors in the header:
```bash
cd MineCraft-One-Week-Challenge && cmake --build build --target YugaKshetra 2>&1 | tail -5
```

- [ ] **Step 3: Commit**

```bash
git add MineCraft-One-Week-Challenge/Source/Entity/ItemDropEntity.h
git commit -m "feat: add ItemDropEntity struct for drop items system"
```

---

### Task 2: Add drop storage, spawning, and physics to World

**Files:**
- Modify: `MineCraft-One-Week-Challenge/Source/World/World.h`
- Modify: `MineCraft-One-Week-Challenge/Source/World/World.cpp`

- [ ] **Step 1: Add includes, member, and method declarations to World.h**

In `World.h`, add the include after line 12 (`#include "Chunk/ChunkManager.h"`):
```cpp
#include "../Entity/ItemDropEntity.h"
```

After line 38 (`ChunkManager &getChunkManager();`), add:
```cpp
    void spawnDrop(const glm::ivec3& blockPos, BlockId blockId);
    void updateDrops(float dt);
    std::vector<ItemDropEntity>& getDropItems();
```

After line 57 (`std::vector<std::unique_ptr<IWorldEvent>> m_events;`), add:
```cpp
    std::vector<ItemDropEntity> m_dropItems;
```

- [ ] **Step 2: Add include to World.cpp**

In `World.cpp`, after line 6 (`#include "../Camera.h"`), add:
```cpp
#include "../Item/Material.h"
```

- [ ] **Step 3: Add spawnDrop() implementation to World.cpp**

Add after `setSpawnPoint()` (before the closing brace of the file, around line 241):

```cpp
void World::spawnDrop(const glm::ivec3& blockPos, BlockId blockId)
{
    const Material& mat = Material::toMaterial(blockId);
    if (mat.id == Material::ID::Nothing)
        return;

    ItemDropEntity drop;
    drop.position = glm::vec3(blockPos) + glm::vec3(0.5f, 0.75f, 0.5f);
    drop.velocity = glm::vec3(0.0f);
    drop.material = &mat;
    drop.lifeTime = 0.0f;
    drop.onGround = false;
    drop.alive = true;
    m_dropItems.push_back(drop);
}
```

- [ ] **Step 4: Add updateDrops() implementation to World.cpp**

Add after `spawnDrop()`:

```cpp
void World::updateDrops(float dt)
{
    for (auto& drop : m_dropItems) {
        if (!drop.alive)
            continue;
        if (!drop.onGround) {
            drop.velocity.y -= 40.0f * dt;
            drop.position += drop.velocity * dt;

            // Ground check: query block below drop
            int bx = static_cast<int>(drop.position.x);
            int by = static_cast<int>(drop.position.y - 0.125f);
            int bz = static_cast<int>(drop.position.z);
            ChunkBlock below = getBlock(bx, by, bz);
            if (below.id != 0 && below.getData().isCollidable) {
                drop.position.y = static_cast<float>(by) + 1.0f + 0.125f;
                drop.velocity = glm::vec3(0.0f);
                drop.onGround = true;
            }
        }
        drop.lifeTime += dt;
        if (drop.lifeTime > 90.0f)
            drop.alive = false;
    }

    // Remove dead drops
    m_dropItems.erase(
        std::remove_if(m_dropItems.begin(), m_dropItems.end(),
                       [](const ItemDropEntity& d) { return !d.alive; }),
        m_dropItems.end());
}
```

- [ ] **Step 5: Add getDropItems() implementation**

```cpp
std::vector<ItemDropEntity>& World::getDropItems()
{
    return m_dropItems;
}
```

- [ ] **Step 6: Add updateDrops() call in World::update()**

In `World::update()` (line 74), add after `updateChunks();`:
```cpp
    updateDrops(0.016f); // ~60fps fixed step
```

But we need real dt. Change `World::update(const Camera &camera)` signature to accept dt:

In `World.h` line 33, change:
```cpp
    void update(const Camera &camera);
```
to:
```cpp
    void update(const Camera &camera, float dt);
```

Then in the `update()` implementation, change line 74 `updateChunks();` to:
```cpp
    updateDrops(dt);
```

(Note: also need to update the call site in Application.cpp — we handle that in Task 8)

- [ ] **Step 7: Build and verify compilation**

```bash
cd MineCraft-One-Week-Challenge && cmake --build build --target YugaKshetra 2>&1 | tail -10
```

Expected: compilation passes (may have unused parameter warning until Task 8).

- [ ] **Step 8: Commit**

```bash
git add MineCraft-One-Week-Challenge/Source/World/World.h MineCraft-One-Week-Challenge/Source/World/World.cpp
git commit -m "feat: add drop storage, spawning, physics, and cleanup to World"
```

---

### Task 3: Modify PlayerDigEvent to spawn drops instead of direct inventory

**Files:**
- Modify: `MineCraft-One-Week-Challenge/Source/World/Event/PlayerDigEvent.cpp`

- [ ] **Step 1: Replace addItem with spawnDrop in PlayerDigEvent::dig()**

In `PlayerDigEvent.cpp` lines 32-51, replace the left-click case:

Old (lines 32-51):
```cpp
        case sf::Mouse::Button::Left: {
            auto block = world.getBlock(x, y, z);
            const auto &material = Material::toMaterial((BlockId)block.id);
            m_pPlayer->addItem(material);
            /*
                        auto r = 1;
                        for (int y = -r; y < r; y++)
                        for (int x = -r; x < r;x++)
                        for (int z = -r; z < r; z++)
                        {
                            int newX = m_digSpot.x + x;
                            int newY = m_digSpot.y + y;
                            int newZ = m_digSpot.z + z;
                            world.updateChunk   (newX, newY, newZ);
                            world.setBlock      (newX, newY, newZ, 0);
            */
            world.updateChunk(x, y, z);
            world.setBlock(x, y, z, 0);
            //}
            break;
        }
```

New:
```cpp
        case sf::Mouse::Button::Left: {
            auto block = world.getBlock(x, y, z);
            world.spawnDrop(glm::ivec3(x, y, z), (BlockId)block.id);
            world.updateChunk(x, y, z);
            world.setBlock(x, y, z, 0);
            break;
        }
```

This removes the commented-out area-mining code and the direct `addItem` call. The include of `Player.h` is still used by right-click (getHeldItems), so keep it.

- [ ] **Step 2: Build and verify**

```bash
cd MineCraft-One-Week-Challenge && cmake --build build --target YugaKshetra 2>&1 | tail -5
```

- [ ] **Step 3: Commit**

```bash
git add MineCraft-One-Week-Challenge/Source/World/Event/PlayerDigEvent.cpp
git commit -m "feat: spawn drop entity on block break instead of direct inventory add"
```

---

### Task 4: Expand inventory from 5 to 20 slots

**Files:**
- Modify: `MineCraft-One-Week-Challenge/Source/Player/Player.cpp`

- [ ] **Step 1: Change loop bound and make addItem return bool**

In `Player.cpp` line 28, change:
```cpp
    for (int i = 0; i < 5; i++)
```
to:
```cpp
    for (int i = 0; i < 20; i++)
```

In `Player.cpp`, replace the `addItem` function (lines 34-51):

Old:
```cpp
void Player::addItem(const Material& material)
{
    Material::ID id = material.id;

    for (unsigned i = 0; i < m_items.size(); i++)
    {
        if (m_items[i].getMaterial().id == id)
        {
            m_items[i].add(1);
            return;
        }
        else if (m_items[i].getMaterial().id == Material::ID::Nothing)
        {
            m_items[i] = {material, 1};
            return;
        }
    }
}
```

New:
```cpp
bool Player::addItem(const Material& material)
{
    Material::ID id = material.id;

    // First pass: try to add to existing stack of same type
    for (unsigned i = 0; i < m_items.size(); i++)
    {
        if (m_items[i].getMaterial().id == id)
        {
            m_items[i].add(1);
            return true;
        }
    }
    // Second pass: find first empty slot
    for (unsigned i = 0; i < m_items.size(); i++)
    {
        if (m_items[i].getMaterial().id == Material::ID::Nothing)
        {
            m_items[i] = {material, 1};
            return true;
        }
    }
    return false; // Inventory full
}
```

- [ ] **Step 2: Update Player.h signature**

In `Player.h` line 28, change:
```cpp
    void addItem(const Material &material);
```
to:
```cpp
    bool addItem(const Material &material);
```

- [ ] **Step 3: Build and verify**

```bash
cd MineCraft-One-Week-Challenge && cmake --build build --target YugaKshetra 2>&1 | tail -5
```

- [ ] **Step 4: Commit**

```bash
git add MineCraft-One-Week-Challenge/Source/Player/Player.h MineCraft-One-Week-Challenge/Source/Player/Player.cpp
git commit -m "feat: expand inventory to 20 slots, addItem returns bool"
```

---

### Task 5: Add backpack toggle (B key) and Alt mouse unlock

**Files:**
- Modify: `MineCraft-One-Week-Challenge/Source/Player/Player.h`
- Modify: `MineCraft-One-Week-Challenge/Source/Player/Player.cpp`

- [ ] **Step 1: Add members to Player.h**

After line 50 (`ToggleKey m_num5;`), add:
```cpp
    ToggleKey m_backpackKey;
    bool m_backpackOpen = false;
```

After line 58 (`BitmapText m_bitmapText;`), add:
```cpp
    // Drop icon rendering state
    std::vector<ItemDropEntity>* m_pDropItems = nullptr;
```

Also add the include after line 12 (`#include "../Item/ItemStack.h"`):
```cpp
#include "../Entity/ItemDropEntity.h"
```

- [ ] **Step 2: Initialize m_backpackKey in Player constructor**

In `Player.cpp`, in the constructor initializer list, after `m_num5(sf::Keyboard::Key::Num5)` (line 22), add:
```cpp
    , m_backpackKey(sf::Keyboard::Key::B)
```

- [ ] **Step 3: Modify handleInput for B key**

In `Player::handleInput()` (Player.cpp line 58), after the `m_slow` check (line 105), add:
```cpp
    if (m_backpackKey.isKeyPressed())
    {
        m_backpackOpen = !m_backpackOpen;
    }
```

- [ ] **Step 4: Modify mouseInput for Alt key**

In `Player::mouseInput()` (Player.cpp line 233), modify the function to check Alt key. Replace the current logic at line 243-249:

Old:
```cpp
    if (!useMouse)
    {
        return;
    }
```

New:
```cpp
    // If Alt is held, release mouse for UI interaction
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LAlt))
    {
        return;
    }

    if (!useMouse)
    {
        return;
    }
```

This early-returns before `sf::Mouse::setPosition()` when Alt is pressed, freeing the cursor.

- [ ] **Step 5: Add setDropItems method to Player**

In `Player.h` after line 28 (`bool addItem(const Material &material);`), add:
```cpp
    void setDropItems(std::vector<ItemDropEntity>* drops);
```

In `Player.cpp`, add:
```cpp
void Player::setDropItems(std::vector<ItemDropEntity>* drops)
{
    m_pDropItems = drops;
}
```

- [ ] **Step 6: Build and verify**

```bash
cd MineCraft-One-Week-Challenge && cmake --build build --target YugaKshetra 2>&1 | tail -5
```

- [ ] **Step 7: Commit**

```bash
git add MineCraft-One-Week-Challenge/Source/Player/Player.h MineCraft-One-Week-Challenge/Source/Player/Player.cpp
git commit -m "feat: add B-key backpack toggle, Alt mouse unlock, drop item pointer"
```

---

### Task 6: Backpack window with ImGui drag-and-drop

**Files:**
- Modify: `MineCraft-One-Week-Challenge/Source/Texture/BasicTexture.h`
- Modify: `MineCraft-One-Week-Challenge/Source/Player/Player.cpp`

This task adds `getID()` to BasicTexture (needed for ImGui texture rendering) and rewrites the `Player::draw()` method.

- [ ] **Step 1: Add getID() accessor to BasicTexture.h**

In `BasicTexture.h` line 22 (after `void bindTexture() const;`), add:
```cpp
    GLuint getID() const { return m_id; }
```

- [ ] **Step 2: Read BlockDatabase for texture UV of each item**

We need to get the top-face texture coordinates for each item. In `Player::draw()`, we'll use:
```cpp
const auto& atlas = BlockDatabase::get().textureAtlas;
// For a given blockId, the top texture coord is:
// BlockDatabase::get().getData(blockId).texTopCoord
```

- [ ] **Step 3: Rewrite Player::draw()**

Replace the entire `Player::draw()` function (lines 273-329 in Player.cpp) with:

```cpp
void Player::draw(RenderMaster& master)
{
    // --- Chinese material names (same as before) ---
    auto cnName = [](Material::ID id) -> std::string {
        switch (id) {
            case Material::ID::Nothing:    return u8"空";
            case Material::ID::Grass:      return u8"草方块";
            case Material::ID::Dirt:       return u8"泥土";
            case Material::ID::Stone:      return u8"石材";
            case Material::ID::OakBark:    return u8"木材";
            case Material::ID::OakLeaf:    return u8"树叶";
            case Material::ID::Sand:       return u8"沙子";
            case Material::ID::Cactus:     return u8"仙人掌";
            case Material::ID::Rose:       return u8"玫瑰";
            case Material::ID::TallGrass:  return u8"草";
            case Material::ID::DeadShrub:  return u8"枯木";
            default: return u8"未知";
        }
    };

    // --- Hotbar text lines (always visible) ---
    std::vector<std::string> lines;
    lines.push_back(u8"=== 快捷栏 ===");
    std::ostringstream ss;
    for (int i = 0; i < 5; i++)
    {
        ss.str("");
        ss << "[ " << (i + 1) << " ] ";
        if (m_items[i].getMaterial().id == Material::ID::Nothing)
            ss << u8"空";
        else
            ss << cnName(m_items[i].getMaterial().id)
               << " x" << m_items[i].getNumInStack();
        if (i == m_heldItem) ss << " <";
        lines.push_back(ss.str());
    }
    lines.push_back(u8"[B] 打开/关闭背包");

    // Render hotbar text
    int texW = 420, texH = 120;
    GLuint texId = m_bitmapText.update(lines, texW, texH);
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2((float)(texW + 16), (float)(texH + 16)),
                             ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Hotbar", nullptr,
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoTitleBar))
    {
        ImGui::Image((ImTextureID)(intptr_t)texId,
                     ImVec2((float)texW, (float)texH));
    }
    ImGui::End();

    // --- Backpack window (toggled with B) ---
    if (m_backpackOpen)
    {
        const auto& atlas = BlockDatabase::get().textureAtlas;
        GLuint atlasID = atlas.getID();
        const float slotSize = 48.0f;
        const int cols = 5;
        const int rows = 4; // row 0 = hotbar, rows 1-3 = backpack
        const float padding = 4.0f;
        const float texPad = 4.0f; // padding inside slot for texture icon

        float winW = cols * (slotSize + padding) + padding + 16.0f;
        float winH = rows * (slotSize + padding) + padding + 16.0f;

        ImGui::SetNextWindowPos(ImVec2(10, texH + 30), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(winW, winH), ImGuiCond_FirstUseEver);
        if (ImGui::Begin(u8"背包", nullptr,
                         ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
        {
            for (int row = 0; row < rows; row++)
            {
                for (int col = 0; col < cols; col++)
                {
                    int slotIndex = row * cols + col; // 0-19
                    const auto& stack = m_items[slotIndex];
                    const auto& mat = stack.getMaterial();

                    // Slot button position
                    float x = padding + col * (slotSize + padding);
                    float y = padding + row * (slotSize + padding);
                    ImGui::SetCursorPos(ImVec2(x, y));

                    // Unique ID per slot
                    ImGui::PushID(slotIndex);

                    // Draw slot background + item texture
                    ImVec2 p0 = ImGui::GetCursorScreenPos();
                    ImVec2 p1(p0.x + slotSize, p0.y + slotSize);

                    // Colored background: highlighted if selected
                    ImU32 bgColor = (slotIndex == m_heldItem)
                        ? IM_COL32(255, 215, 0, 80)
                        : IM_COL32(60, 60, 60, 200);
                    ImGui::GetWindowDrawList()->AddRectFilled(p0, p1, bgColor);
                    ImGui::GetWindowDrawList()->AddRect(p0, p1, IM_COL32(180, 180, 180, 255));

                    // Draw item icon if slot is not empty
                    if (mat.id != Material::ID::Nothing)
                    {
                        BlockId bId = mat.toBlockID();
                        const auto& blockData = BlockDatabase::get().getData(bId);
                        auto uv = atlas.getTexture(blockData.texTopCoord);
                        // uv: {xMax, yMax, xMin, yMax, xMin, yMin, xMax, yMin}
                        ImVec2 uv0(uv[2], uv[5]); // (xMin, yMin)
                        ImVec2 uv1(uv[0], uv[3]); // (xMax, yMax)
                        ImGui::GetWindowDrawList()->AddImage(
                            (ImTextureID)(intptr_t)atlasID,
                            ImVec2(p0.x + texPad, p0.y + texPad),
                            ImVec2(p1.x - texPad, p1.y - texPad),
                            uv0, uv1);

                        // Quantity text (white, bottom-right)
                        std::ostringstream qty;
                        qty << stack.getNumInStack();
                        ImGui::GetWindowDrawList()->AddText(
                            ImVec2(p1.x - 20, p1.y - 18),
                            IM_COL32(255, 255, 255, 255),
                            qty.str().c_str());
                    }

                    // Hotbar row label (row 0) and selected marker
                    if (row == 0 && col == m_heldItem)
                    {
                        ImGui::GetWindowDrawList()->AddRect(
                            p0, p1, IM_COL32(255, 255, 0, 255), 0.0f, 0, 2.0f);
                    }

                    // --- Drag and Drop ---
                    // Drag source
                    if (mat.id != Material::ID::Nothing &&
                        ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
                    {
                        ImGui::SetDragDropPayload("INV_SLOT", &slotIndex, sizeof(int));
                        // Drag preview: small version of the item icon
                        BlockId bId = mat.toBlockID();
                        const auto& blockData = BlockDatabase::get().getData(bId);
                        auto uv = atlas.getTexture(blockData.texTopCoord);
                        ImVec2 uv0(uv[2], uv[5]);
                        ImVec2 uv1(uv[0], uv[3]);
                        ImGui::Image((ImTextureID)(intptr_t)atlasID,
                                     ImVec2(slotSize * 0.7f, slotSize * 0.7f),
                                     uv0, uv1);
                        ImGui::EndDragDropSource();
                    }

                    // Drop target
                    if (ImGui::BeginDragDropTarget())
                    {
                        const ImGuiPayload* payload =
                            ImGui::AcceptDragDropPayload("INV_SLOT");
                        if (payload)
                        {
                            int srcSlot = *(const int*)payload->Data;
                            // Swap items
                            std::swap(m_items[srcSlot], m_items[slotIndex]);
                        }
                        ImGui::EndDragDropTarget();
                    }

                    ImGui::PopID();
                }
            }
        }
        ImGui::End();
    }

    // --- Drop item icon rendering (placeholder for Task 7) ---
    // Will be added in Task 7
}
```

Add required include at the top of Player.cpp after line 12 (`#include <imgui.h>`):
```cpp
#include "../World/Block/BlockDatabase.h"
```

- [ ] **Step 4: Build and verify compilation**

```bash
cd MineCraft-One-Week-Challenge && cmake --build build --target YugaKshetra 2>&1 | tail -10
```

Expected: compilation passes. Backpack shows 20-slot grid when B is pressed.

- [ ] **Step 5: Commit**

```bash
git add MineCraft-One-Week-Challenge/Source/Texture/BasicTexture.h MineCraft-One-Week-Challenge/Source/Player/Player.cpp
git commit -m "feat: add backpack window with ImGui drag-and-drop item swapping"
```

---

### Task 7: Drop icon rendering (3D→2D projection)

**Files:**
- Modify: `MineCraft-One-Week-Challenge/Source/Player/Player.h`
- Modify: `MineCraft-One-Week-Challenge/Source/Player/Player.cpp`

- [ ] **Step 1: Modify Player::draw() signature to accept Camera**

In `Player.h` line 30, change:
```cpp
    void draw(RenderMaster &master);
```
to:
```cpp
    void draw(RenderMaster &master, const Camera* camera = nullptr);
```

Add forward declaration after line 10 (`#include "../Entity.h"`):
```cpp
class Camera;
```

Update the `draw()` function signature in Player.cpp (line 273) to match:
```cpp
void Player::draw(RenderMaster& master, const Camera* camera)
```

- [ ] **Step 2: Add drop icon rendering code**

In `Player::draw()`, replace the placeholder comment `// --- Drop item icon rendering (placeholder for Task 7) ---` and the line `// Will be added in Task 7` with:

```cpp
    // --- Drop item icon rendering via ImGui overlay ---
    if (m_pDropItems && !m_pDropItems->empty() && camera)
    {
        const auto& atlas = BlockDatabase::get().textureAtlas;
        GLuint atlasID = atlas.getID();

        auto windowSize = ImGui::GetIO().DisplaySize;
        float winW = windowSize.x;
        float winH = windowSize.y;

        const auto& viewMat = camera->getViewMatrix();
        const auto& projMat = camera->getProjMatrix();
        glm::mat4 vp = projMat * viewMat;

        for (const auto& drop : *m_pDropItems)
        {
            if (!drop.alive)
                continue;

            // 3D → clip space
            glm::vec4 clip = vp * glm::vec4(drop.position, 1.0f);
            if (clip.w <= 0.0f)
                continue;

            // NDC
            glm::vec3 ndc = glm::vec3(clip) / clip.w;
            if (ndc.x < -1.0f || ndc.x > 1.0f ||
                ndc.y < -1.0f || ndc.y > 1.0f)
                continue;

            // Screen coords
            float screenX = (ndc.x * 0.5f + 0.5f) * winW;
            float screenY = (1.0f - (ndc.y * 0.5f + 0.5f)) * winH;

            // Distance-based icon size and fade
            float dist = glm::distance(
                glm::vec3(camera->position), drop.position);
            float maxDist = 32.0f;
            if (dist > maxDist)
                continue;

            float scale = 1.0f - (dist / maxDist);
            float alpha = scale * scale;
            float iconSize = 24.0f * (0.5f + 0.5f * scale);

            // Get texture UV for this item
            BlockId bId = drop.material->toBlockID();
            const auto& blockData = BlockDatabase::get().getData(bId);
            auto uv = atlas.getTexture(blockData.texTopCoord);

            ImVec2 uv0(uv[2], uv[5]); // (xMin, yMin)
            ImVec2 uv1(uv[0], uv[3]); // (xMax, yMax)
            ImVec2 p0(screenX - iconSize * 0.5f, screenY - iconSize * 0.5f);
            ImVec2 p1(screenX + iconSize * 0.5f, screenY + iconSize * 0.5f);

            // Faded white with alpha
            ImU32 col = IM_COL32(
                (int)(255 * alpha), (int)(255 * alpha),
                (int)(255 * alpha), (int)(255 * alpha));

            ImGui::GetBackgroundDrawList()->AddImage(
                (ImTextureID)(intptr_t)atlasID, p0, p1, uv0, uv1, col);
        }
    }
```

Note: `camera->position` access — Camera inherits from Entity which has `position` as a public field, so this works.

- [ ] **Step 3: Add Camera include in Player.cpp**

After the existing includes at the top of Player.cpp, add:
```cpp
#include "../Camera.h"
```

- [ ] **Step 4: Build and verify**

```bash
cd MineCraft-One-Week-Challenge && cmake --build build --target YugaKshetra 2>&1 | tail -10
```

- [ ] **Step 5: Commit**

```bash
git add MineCraft-One-Week-Challenge/Source/Player/Player.h MineCraft-One-Week-Challenge/Source/Player/Player.cpp
git commit -m "feat: add 3D→2D drop icon rendering via ImGui overlay"
```

---

### Task 8: Wire auto-pickup and pass dt/camera through Application

**Files:**
- Modify: `MineCraft-One-Week-Challenge/Source/Application.cpp`

- [ ] **Step 1: Update World::update() call in Application::on_update()**

In `Application.cpp` line 75:
```cpp
    m_world.update(m_camera);
```
Change to:
```cpp
    m_world.update(m_camera, dt.asSeconds());
```

- [ ] **Step 2: Add auto-pickup logic after world.update()**

After line 75 (`m_world.update(m_camera, dt.asSeconds());`), add:

```cpp
    // Auto-pickup: check player proximity to each drop
    {
        auto& drops = m_world.getDropItems();
        for (auto& drop : drops)
        {
            if (!drop.alive)
                continue;
            if (glm::distance(m_player.position, drop.position) < 2.0f)
            {
                if (m_player.addItem(*drop.material))
                {
                    drop.alive = false;
                }
            }
        }
    }
```

- [ ] **Step 3: Pass camera pointer to Player::draw()**

In `Application::on_render()` line 101:
```cpp
    m_player.draw(m_masterRenderer);
```
Change to:
```cpp
    m_player.setDropItems(&m_world.getDropItems());
    m_player.draw(m_masterRenderer, &m_camera);
```

- [ ] **Step 4: Build and verify**

```bash
cd MineCraft-One-Week-Challenge && cmake --build build --target YugaKshetra 2>&1 | tail -10
```

- [ ] **Step 5: Commit**

```bash
git add MineCraft-One-Week-Challenge/Source/Application.cpp
git commit -m "feat: wire auto-pickup, dt passing, and drop icon rendering through Application"
```

---

### Task 9: Build, run, and verify end-to-end

**Files:**
- Modify: None (verification only)

- [ ] **Step 1: Clean build**

```bash
cd MineCraft-One-Week-Challenge && cmake --build build --target YugaKshetra 2>&1 | tail -15
```

Expected: BUILD SUCCEEDED with zero errors.

- [ ] **Step 2: Run and verify checklist**

Launch the game:
```bash
cd MineCraft-One-Week-Challenge && ./build/Debug/YugaKshetra.exe
```

Manual verification:
1. Break a grass block (left-click) → drop icon appears at block position on screen ✓
2. Drop falls to ground (visible icon moves down) and stops ✓
3. Walk near drop → item auto-picked into hotbar ✓
4. Press B → backpack window appears above hotbar with 20 slots in 5×4 grid ✓
5. Hold Alt → mouse cursor appears, drag item from slot 5 to slot 0 → items swap ✓
6. Press B again → backpack window closes ✓
7. Wait 90s near a drop → icon disappears ✓
8. Fill inventory completely → new drops remain in world ✓

- [ ] **Step 3: Commit (if any final fixes needed)**

```bash
git add -A
git commit -m "fix: final adjustments for drop items and auto-pickup"
```
