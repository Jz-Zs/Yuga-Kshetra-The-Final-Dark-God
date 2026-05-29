#include "Player.h"

#include <SFML/Graphics.hpp>

#include <iomanip>
#include <iostream>
#include <sstream>

#include "../Camera.h"
#include "../Input/Keyboard.h"
#include "../Renderer/RenderMaster.h"
#include "../World/World.h"
#include <imgui.h>
#include "../World/Block/BlockDatabase.h"

Player::Player()
    : Entity({64, 33, 64}, {0.f, 0.f, 0.f}, {0.3f, 1.f, 0.3f})
    , m_itemDown(sf::Keyboard::Key::Down)
    , m_itemUp(sf::Keyboard::Key::Up)
    , m_flyKey(sf::Keyboard::Key::F)
    , m_num1(sf::Keyboard::Key::Num1)
    , m_num2(sf::Keyboard::Key::Num2)
    , m_num3(sf::Keyboard::Key::Num3)
    , m_num4(sf::Keyboard::Key::Num4)
    , m_num5(sf::Keyboard::Key::Num5)
    , m_backpackKey(sf::Keyboard::Key::B)
    , m_slow(sf::Keyboard::Key::LShift)
    , m_acceleration(glm::vec3(0.f))
{

    for (int i = 0; i < 20; i++)
    {
        m_items.emplace_back(Material::NOTHING, 0);
    }
}

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

void Player::setDropItems(std::vector<ItemDropEntity>* drops)
{
    m_pDropItems = drops;
}

ItemStack& Player::getHeldItems()
{
    return m_items[m_heldItem];
}

void Player::handleInput(const sf::Window& window, const Keyboard& keyboard)
{
    keyboardInput(keyboard);
    mouseInput(window);

    if (m_itemDown.isKeyPressed())
    {
        m_heldItem++;
        if (m_heldItem == (int)m_items.size())
        {
            m_heldItem = 0;
        }
    }
    else if (m_itemUp.isKeyPressed())
    {
        m_heldItem--;
        if (m_heldItem == -1)
        {
            m_heldItem = m_items.size() - 1;
        }
    }

    if (m_flyKey.isKeyPressed())
    {
        m_isFlying = !m_isFlying;
    }

    if (m_num1.isKeyPressed())
    {
        m_heldItem = 0;
    }
    if (m_num2.isKeyPressed())
    {
        m_heldItem = 1;
    }
    if (m_num3.isKeyPressed())
    {
        m_heldItem = 2;
    }
    if (m_num4.isKeyPressed())
    {
        m_heldItem = 3;
    }
    if (m_num5.isKeyPressed())
    {
        m_heldItem = 4;
    }
    if (m_slow.isKeyPressed())
    {
        m_isSneak = !m_isSneak;
    }

    if (m_backpackKey.isKeyPressed())
    {
        m_backpackOpen = !m_backpackOpen;
    }
}

void Player::update(float dt, World& world)
{
    velocity += m_acceleration;
    m_acceleration = {0, 0, 0};

    if (!m_isFlying)
    {
        if (!m_isOnGround)
        {
            velocity.y -= 40 * dt;
        }
        m_isOnGround = false;
    }

    if (position.y <= 0 && !m_isFlying)
    {
        position.y = 300;
    }

    position.x += velocity.x * dt;
    collide(world, {velocity.x, 0, 0}, dt);

    position.y += velocity.y * dt;
    collide(world, {0, velocity.y, 0}, dt);

    position.z += velocity.z * dt;
    collide(world, {0, 0, velocity.z}, dt);

    box.update(position);
    velocity.x *= 0.95f;
    velocity.z *= 0.95f;
    if (m_isFlying)
    {
        velocity.y *= 0.95f;
    }
}

void Player::collide(World& world, const glm::vec3& vel, float dt)
{
    for (int x = position.x - box.dimensions.x; x < position.x + box.dimensions.x; x++)
        for (int y = position.y - box.dimensions.y; y < position.y + 0.7; y++)
            for (int z = position.z - box.dimensions.z; z < position.z + box.dimensions.z; z++)
            {
                auto block = world.getBlock(x, y, z);

                if (block != 0 && block.getData().isCollidable)
                {
                    if (vel.y > 0)
                    {
                        position.y = y - box.dimensions.y;
                        velocity.y = 0;
                    }
                    else if (vel.y < 0)
                    {
                        m_isOnGround = true;
                        position.y = y + box.dimensions.y + 1;
                        velocity.y = 0;
                    }

                    if (vel.x > 0)
                    {
                        position.x = x - box.dimensions.x;
                    }
                    else if (vel.x < 0)
                    {
                        position.x = x + box.dimensions.x + 1;
                    }

                    if (vel.z > 0)
                    {
                        position.z = z - box.dimensions.z;
                    }
                    else if (vel.z < 0)
                    {
                        position.z = z + box.dimensions.z + 1;
                    }
                }
            }
}

///@TODO Move this
float speed = 0.2f;

void Player::keyboardInput(const Keyboard& keyboard)
{
    if (keyboard.isKeyDown(sf::Keyboard::Key::W))
    {
        float s = speed;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl))
            s *= 5;
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift) ||
                 sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift))
            s *= 0.35f;
        m_acceleration.x += -glm::cos(glm::radians(rotation.y + 90)) * s;
        m_acceleration.z += -glm::sin(glm::radians(rotation.y + 90)) * s;
    }
    if (keyboard.isKeyDown(sf::Keyboard::Key::S))
    {
        m_acceleration.x += glm::cos(glm::radians(rotation.y + 90)) * speed;
        m_acceleration.z += glm::sin(glm::radians(rotation.y + 90)) * speed;
    }
    if (keyboard.isKeyDown(sf::Keyboard::Key::A))
    {
        m_acceleration.x += -glm::cos(glm::radians(rotation.y)) * speed;
        m_acceleration.z += -glm::sin(glm::radians(rotation.y)) * speed;
    }
    if (keyboard.isKeyDown(sf::Keyboard::Key::D))
    {
        m_acceleration.x += glm::cos(glm::radians(rotation.y)) * speed;
        m_acceleration.z += glm::sin(glm::radians(rotation.y)) * speed;
    }

    if (keyboard.isKeyDown(sf::Keyboard::Key::Space))
    {
        jump();
    }
    else if (keyboard.isKeyDown(sf::Keyboard::Key::LShift) && m_isFlying)
    {
        m_acceleration.y -= speed * 3;
    }
}

void Player::mouseInput(const sf::Window& window)
{
    static bool useMouse = true;
    static ToggleKey useMouseKey(sf::Keyboard::Key::L);

    if (useMouseKey.isKeyPressed())
    {
        useMouse = !useMouse;
    }

    // If Alt is held, release mouse for UI interaction
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LAlt))
    {
        return;
    }

    if (!useMouse)
    {
        return;
    }

    static float const BOUND = 89.f;
    static auto lastMousePosition = sf::Mouse::getPosition(window);
    auto change = sf::Mouse::getPosition() - lastMousePosition;

    rotation.y += change.x * 0.05f;
    rotation.x += change.y * 0.05f;

    if (rotation.x > BOUND)
        rotation.x = BOUND;
    else if (rotation.x < -BOUND)
        rotation.x = -BOUND;

    if (rotation.y > 360)
        rotation.y = 0;
    else if (rotation.y < 0)
        rotation.y = 360;

    auto cx = static_cast<int>(window.getSize().x / 2);
    auto cy = static_cast<int>(window.getSize().y / 2);

    sf::Mouse::setPosition({cx, cy}, window);

    lastMousePosition = sf::Mouse::getPosition();
}

void Player::draw(RenderMaster& master, const Camera* camera)
{
    // --- Chinese material names ---
    auto cnName = [](Material::ID id) -> std::string {
        switch (id) {
            case Material::ID::Nothing:    return "空";
            case Material::ID::Grass:      return "草方块";
            case Material::ID::Dirt:       return "泥土";
            case Material::ID::Stone:      return "石材";
            case Material::ID::OakBark:    return "木材";
            case Material::ID::OakLeaf:    return "树叶";
            case Material::ID::Sand:       return "沙子";
            case Material::ID::Cactus:     return "仙人掌";
            case Material::ID::Rose:       return "玫瑰";
            case Material::ID::TallGrass:  return "草";
            case Material::ID::DeadShrub:  return "枯木";
            default: return "未知";
        }
    };

    // --- Hotbar text lines (always visible) ---
    std::vector<std::string> lines;
    lines.push_back("=== 快捷栏 ===");
    std::ostringstream ss;
    for (int i = 0; i < 5; i++)
    {
        ss.str("");
        ss << "[ " << (i + 1) << " ] ";
        if (m_items[i].getMaterial().id == Material::ID::Nothing)
            ss << "空";
        else
            ss << cnName(m_items[i].getMaterial().id)
               << " x" << m_items[i].getNumInStack();
        if (i == m_heldItem) ss << " <";
        lines.push_back(ss.str());
    }
    lines.push_back("[B] 打开/关闭背包");

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
        auto& atlas = BlockDatabase::get().textureAtlas;
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
        if (ImGui::Begin("背包", nullptr,
                         ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
        {
            for (int row = 0; row < rows; row++)
            {
                for (int col = 0; col < cols; col++)
                {
                    int slotIndex = row * cols + col; // 0-19
                    const auto& stack = m_items[slotIndex];
                    const auto& mat = stack.getMaterial();

                    // Slot position
                    float x = padding + col * (slotSize + padding);
                    float y = padding + row * (slotSize + padding);
                    ImGui::SetCursorPos(ImVec2(x, y));

                    // Dummy widget to tell ImGui content extends here (prevents assertion)
                    ImGui::Dummy(ImVec2(slotSize, slotSize));

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
                        auto uv = atlas.getTexture(blockData.getBlockData().texTopCoord);
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

                    // Yellow border on selected hotbar slot
                    if (row == 0 && col == m_heldItem)
                    {
                        ImGui::GetWindowDrawList()->AddRect(
                            p0, p1, IM_COL32(255, 255, 0, 255), 0.0f, 0, 2.0f);
                    }

                    // --- Drag and Drop ---
                    if (mat.id != Material::ID::Nothing &&
                        ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
                    {
                        ImGui::SetDragDropPayload("INV_SLOT", &slotIndex, sizeof(int));
                        // Drag preview: small icon
                        BlockId bId = mat.toBlockID();
                        const auto& blockData = BlockDatabase::get().getData(bId);
                        auto uv = atlas.getTexture(blockData.getBlockData().texTopCoord);
                        ImVec2 uv0(uv[2], uv[5]);
                        ImVec2 uv1(uv[0], uv[3]);
                        ImGui::Image((ImTextureID)(intptr_t)atlasID,
                                     ImVec2(slotSize * 0.7f, slotSize * 0.7f),
                                     uv0, uv1);
                        ImGui::EndDragDropSource();
                    }

                    if (ImGui::BeginDragDropTarget())
                    {
                        const ImGuiPayload* payload =
                            ImGui::AcceptDragDropPayload("INV_SLOT");
                        if (payload)
                        {
                            int srcSlot = *(const int*)payload->Data;
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

    // --- Drop item icon rendering via ImGui overlay ---
    if (m_pDropItems && !m_pDropItems->empty() && camera)
    {
        auto& atlas = BlockDatabase::get().textureAtlas;
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

            // 3D world position → clip space
            glm::vec4 clip = vp * glm::vec4(drop.position, 1.0f);
            if (clip.w <= 0.0f)
                continue;

            // Clip → NDC
            glm::vec3 ndc = glm::vec3(clip) / clip.w;
            if (ndc.x < -1.0f || ndc.x > 1.0f ||
                ndc.y < -1.0f || ndc.y > 1.0f)
                continue;

            // NDC → screen coordinates
            float screenX = (ndc.x * 0.5f + 0.5f) * winW;
            float screenY = (1.0f - (ndc.y * 0.5f + 0.5f)) * winH;

            // Distance-based icon size and fade
            float dist = glm::distance(
                glm::vec3(camera->position), drop.position);
            float maxDist = 32.0f;
            if (dist > maxDist)
                continue;

            float scale = 1.0f - (dist / maxDist);
            float alpha = 0.3f + 0.7f * (scale * scale);
            float iconSize = 28.0f * (0.5f + 0.5f * scale);

            // Get texture UV for this item's top face
            BlockId bId = drop.material->toBlockID();
            const auto& blockData = BlockDatabase::get().getData(bId);
            auto uv = atlas.getTexture(blockData.getBlockData().texTopCoord);

            ImVec2 uv0(uv[2], uv[5]); // (xMin, yMin)
            ImVec2 uv1(uv[0], uv[3]); // (xMax, yMax)
            ImVec2 p0(screenX - iconSize * 0.5f, screenY - iconSize * 0.5f);
            ImVec2 p1(screenX + iconSize * 0.5f, screenY + iconSize * 0.5f);

            ImU32 col = IM_COL32(
                (int)(255 * alpha), (int)(255 * alpha),
                (int)(255 * alpha), (int)(255 * alpha));

            // Dark outline shadow for visibility
            float outlineOff = 1.5f;
            ImU32 outlineCol = IM_COL32(0, 0, 0, (int)(180 * alpha));
            ImVec2 os0(p0.x - outlineOff, p0.y - outlineOff);
            ImVec2 os1(p1.x + outlineOff, p1.y + outlineOff);
            ImGui::GetBackgroundDrawList()->AddImage(
                (ImTextureID)(intptr_t)atlasID, os0, os1, uv0, uv1, outlineCol);
            // Dark border rect
            ImGui::GetBackgroundDrawList()->AddRect(os0, os1,
                IM_COL32(0, 0, 0, (int)(220 * alpha)), 0.0f, 0, 1.5f);

            // Main icon on top
            ImGui::GetBackgroundDrawList()->AddImage(
                (ImTextureID)(intptr_t)atlasID, p0, p1, uv0, uv1, col);
        }
    }
}

void Player::jump()
{
    if (!m_isFlying)
    {
        if (m_isOnGround)
        {

            m_isOnGround = false;
            m_acceleration.y += speed * 50;
        }
    }
    else
    {
        m_acceleration.y += speed * 3;
    }
}
