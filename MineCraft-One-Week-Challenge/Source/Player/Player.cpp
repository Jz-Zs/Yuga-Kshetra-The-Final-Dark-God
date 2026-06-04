#include "Player.h"

#include <SFML/Graphics.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <glad/glad.h>
#include "../Camera.h"
#include "../Input/Keyboard.h"
#include "../Renderer/RenderMaster.h"
#include "../World/World.h"
#include <imgui.h>
#include "../World/Block/BlockDatabase.h"

namespace {
    const char* getMaterialCNName(Material::ID id) {
        switch (id) {
            case Material::ID::Nothing:       return "空";
            case Material::ID::Grass:         return "草方块";
            case Material::ID::Dirt:          return "泥土";
            case Material::ID::Stone:         return "石材";
            case Material::ID::OakBark:       return "木材";
            case Material::ID::OakLeaf:       return "树叶";
            case Material::ID::Sand:          return "沙子";
            case Material::ID::Cactus:        return "仙人掌";
            case Material::ID::Rose:          return "玫瑰";
            case Material::ID::TallGrass:     return "草";
            case Material::ID::DeadShrub:     return "枯木";
            case Material::ID::Stick:         return "木棍";
            case Material::ID::WoodenSword:   return "木剑";
            case Material::ID::RawMeat:       return "生肉";
            case Material::ID::GoldBlock:     return "金块";
            case Material::ID::Cobblestone:   return "圆石";
            case Material::ID::IronOre:       return "铁矿";
            case Material::ID::IronIngot:     return "铁锭";
            case Material::ID::WildFruit:     return "野果";
            case Material::ID::StoneArrow:    return "石箭矢";
            case Material::ID::WoodenPickaxe: return "木镐";
            case Material::ID::StonePickaxe:  return "石镐";
            case Material::ID::IronPickaxe:   return "铁镐";
            case Material::ID::WoodenAxe:     return "木斧";
            case Material::ID::StoneAxe:      return "石斧";
            case Material::ID::IronAxe:       return "铁斧";
            case Material::ID::IronSword:     return "铁剑";
            case Material::ID::CookedMeat:    return "熟肉";
            case Material::ID::Furnace:       return "熔炉";
            case Material::ID::Silk:          return "蛛丝";
            case Material::ID::SilkThread:    return "丝线";
            case Material::ID::Bow:           return "弓";
            case Material::ID::IronArrow:     return "铁箭";
            case Material::ID::SpiderSilkArrow: return "蛛丝箭";
            default: return "未知";
        }
    }
}

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
    , m_equipKey(sf::Keyboard::Key::R)
    , m_acceleration(glm::vec3(0.f))
{

    for (int i = 0; i < 20; i++)
    {
        m_items.emplace_back(Material::NOTHING, 0);
    }
    // 开局测试用：快捷栏放木剑、木镐、木斧各一把 + 熔炉 + 铁矿
    m_items[0] = ItemStack(Material::WOODEN_SWORD, 1);
    m_items[1] = ItemStack(Material::WOODEN_PICKAXE, 1);
    m_items[2] = ItemStack(Material::WOODEN_AXE, 1);
    m_items[3] = ItemStack(Material::FURNACE, 1);
    m_items[4] = ItemStack(Material::IRON_ORE_ITEM, 64);
    // Test items — bow/arrow testing
    m_items[5] = ItemStack(Material::STONE_BLOCK, 64);
    m_items[6] = ItemStack(Material::COBBLESTONE, 64);
    m_items[7] = ItemStack(Material::STICK, 99);
    m_items[8] = ItemStack(Material::IRON_INGOT, 64);
    m_items[9] = ItemStack(Material::SILK, 64);
    m_items[10] = ItemStack(Material::SILK_THREAD, 64);
    m_equipment[0] = ItemStack(Material::WOODEN_SWORD, 1);  // 主手装备木剑
    for (int i = 0; i < 9; i++)
    {
        m_craftGrid[i] = ItemStack(Material::NOTHING, 0);
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
            int leftover = m_items[i].add(1);
            if (leftover == 0) {
                if (material.id != Material::ID::Furnace) m_roundCollection[material.id]++;
                return true;
            }
            // Stack full, continue to next slot
        }
    }
    // Second pass: find first empty slot
    for (unsigned i = 0; i < m_items.size(); i++)
    {
        if (m_items[i].getMaterial().id == Material::ID::Nothing)
        {
            m_items[i] = {material, 1};
            m_roundCollection[material.id]++;
            return true;
        }
    }
    return false; // Inventory full
}

bool Player::hasFurnace() const
{
    // 检查是否有已放置的熔炉
    if (m_furnacePos != glm::ivec3{-1, -1, -1})
        return true;

    // 检查背包中是否有熔炉物品
    for (const auto& stack : m_items) {
        if (stack.getMaterial().id == Material::ID::Furnace)
            return true;
    }
    // 检查装备栏
    for (int i = 0; i < 2; i++) {
        if (m_equipment[i].getMaterial().id == Material::ID::Furnace)
            return true;
    }
    return false;
}

void Player::onFurnaceMined()
{
    // Drop furnace contents as individual items (one per stack count)
    if (m_pDropItems) {
        glm::vec3 pos(m_furnacePos.x + 0.5f, m_furnacePos.y + 0.5f, m_furnacePos.z + 0.5f);
        auto dropStack = [&](const ItemStack& s) {
            int n = s.getNumInStack();
            if (s.getMaterial().id == Material::ID::Nothing || n <= 0) return;
            for (int i = 0; i < n; i++) {
                ItemDropEntity d;
                d.position = pos;
                d.velocity = glm::vec3(0, 0, 0);
                d.material = &s.getMaterial();
                d.alive = true;
                m_pDropItems->push_back(d);
            }
        };
        dropStack(m_furnace.input);
        dropStack(m_furnace.fuel);
        dropStack(m_furnace.output);
    }
    m_furnacePos = {-1, -1, -1};
    m_furnaceUIOpen = false;
    m_furnace = FurnaceState{};
}

void Player::onWorldReset()
{
    m_furnacePos = {-1, -1, -1};
    m_furnaceUIOpen = false;
    m_furnace = FurnaceState{};
}

void Player::setDropItems(std::vector<ItemDropEntity>* drops)
{
    m_pDropItems = drops;
}

ItemStack& Player::getHeldItems()
{
    return m_items[m_heldItem];
}

void Player::handleInput(sf::Window& window, const Keyboard& keyboard)
{
    keyboardInput(keyboard);
    mouseInput(window);

    processRKey();

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
            m_heldItem = static_cast<int>(m_items.size()) - 1;
        }
    }

    if (m_flyKey.isKeyPressed())
    {
        m_isFlying = !m_isFlying;
    }

    if (m_num1.isKeyPressed()) { m_heldItem = 0; }
    if (m_num2.isKeyPressed()) { m_heldItem = 1; }
    if (m_num3.isKeyPressed()) { m_heldItem = 2; }
    if (m_num4.isKeyPressed()) { m_heldItem = 3; }
    if (m_num5.isKeyPressed()) { m_heldItem = 4; }
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

    if (m_slowTimer > 0.0f)
        m_slowTimer -= dt;

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

    // 熔炉冶炼计时（后台继续，不受UI开关影响）
    if (m_furnace.isSmelting) {
        const SmeltingRecipe* recipe = findSmeltingRecipe(
            m_furnace.input.getMaterial().id, m_furnace.input.getNumInStack(),
            m_furnace.fuel.getMaterial().id, m_furnace.fuel.getNumInStack());
        if (recipe) {
            m_furnace.progress += dt / 5.0f;  // 5秒冶炼时间
            if (m_furnace.progress >= 1.0f) {
                // 消耗输入和燃料
                for (int i = 0; i < recipe->inputCount; i++)
                    m_furnace.input.remove();
                for (int i = 0; i < recipe->fuelCount; i++)
                    m_furnace.fuel.remove();
                // 产出
                const Material& outMat = (recipe->output == Material::ID::IronIngot)
                    ? static_cast<const Material&>(Material::IRON_INGOT)
                    : static_cast<const Material&>(Material::COOKED_MEAT);
                if (m_furnace.output.getMaterial().id == Material::ID::Nothing) {
                    m_furnace.output = ItemStack(outMat, recipe->outputCount);
                } else {
                    m_furnace.output.add(recipe->outputCount);
                }
                m_furnace.progress = 0.f;
                // 检查是否可以继续下一轮
                const SmeltingRecipe* next = findSmeltingRecipe(
                    m_furnace.input.getMaterial().id, m_furnace.input.getNumInStack(),
                    m_furnace.fuel.getMaterial().id, m_furnace.fuel.getNumInStack());
                if (!next)
                    m_furnace.isSmelting = false;
            }
        } else {
            // 材料/燃料不足 → 停止冶炼，重置进度
            m_furnace.isSmelting = false;
            m_furnace.progress = 0.f;
        }
    }
}

void Player::collide(World& world, const glm::vec3& vel, float /*dt*/)
{
    for (int x = static_cast<int>(position.x - box.dimensions.x); x < position.x + box.dimensions.x; x++)
        for (int y = static_cast<int>(position.y - box.dimensions.y); y < position.y + 0.7; y++)
            for (int z = static_cast<int>(position.z - box.dimensions.z); z < position.z + box.dimensions.z; z++)
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
float speed = 0.263f;

void Player::keyboardInput(const Keyboard& keyboard)
{
    float currentSpeed = speed;
    if (m_slowTimer > 0.0f)
        currentSpeed *= m_slowFactor;

    if (keyboard.isKeyDown(sf::Keyboard::Key::W))
    {
        float s = currentSpeed;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl))
            s *= 4;
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift) ||
                 sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift))
            s *= 0.35f;
        m_acceleration.x += -glm::cos(glm::radians(rotation.y + 90)) * s;
        m_acceleration.z += -glm::sin(glm::radians(rotation.y + 90)) * s;
    }
    if (keyboard.isKeyDown(sf::Keyboard::Key::S))
    {
        m_acceleration.x += glm::cos(glm::radians(rotation.y + 90)) * currentSpeed;
        m_acceleration.z += glm::sin(glm::radians(rotation.y + 90)) * currentSpeed;
    }
    if (keyboard.isKeyDown(sf::Keyboard::Key::A))
    {
        m_acceleration.x += -glm::cos(glm::radians(rotation.y)) * currentSpeed;
        m_acceleration.z += -glm::sin(glm::radians(rotation.y)) * currentSpeed;
    }
    if (keyboard.isKeyDown(sf::Keyboard::Key::D))
    {
        m_acceleration.x += glm::cos(glm::radians(rotation.y)) * currentSpeed;
        m_acceleration.z += glm::sin(glm::radians(rotation.y)) * currentSpeed;
    }

    if (keyboard.isKeyDown(sf::Keyboard::Key::Space))
    {
        jump();
    }
    else if (keyboard.isKeyDown(sf::Keyboard::Key::LShift) && m_isFlying)
    {
        m_acceleration.y -= currentSpeed * 3;
    }
}

void Player::mouseInput(sf::Window& window)
{
    m_mouseLocked = !sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LAlt) && !m_backpackOpen;
    window.setMouseCursorGrabbed(m_mouseLocked && !m_backpackOpen);

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

void Player::drawHealthBar()
{
    auto& atlas = BlockDatabase::get().textureAtlas;
    GLuint atlasID = atlas.getID();

    auto fullUV  = atlas.getTexture(sf::Vector2i(13, 1));
    auto halfUV  = atlas.getTexture(sf::Vector2i(14, 1));
    auto emptyUV = atlas.getTexture(sf::Vector2i(15, 1));

    const float heartSize = 36.0f;
    const float gap = -1.0f;
    const int numHearts = 10;
    const float pad = 8.0f;
    float contentW = numHearts * (heartSize + gap) - gap + pad * 2;
    float contentH = heartSize + pad * 2;

    auto displaySize = ImGui::GetIO().DisplaySize;

    // Position: centered above hotbar, always
    const float slotSize = 48.0f;
    const float slotPad = 4.0f;
    float hotbarContentH = 1 * (slotSize + slotPad) + slotPad;
    float hotbarY = displaySize.y - hotbarContentH - 10.0f;
    float winY = hotbarY - contentH - 1.0f;

    float winX = (displaySize.x - contentW) * 0.5f - 55.0f;

    ImGui::SetNextWindowPos(ImVec2(winX, winY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(contentW, contentH), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

    int flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoBackground;

    if (ImGui::Begin("HealthBar", nullptr, flags)) {
        ImVec2 wp = ImGui::GetWindowPos();
        auto* dl = ImGui::GetWindowDrawList();

        for (int i = 0; i < numHearts; i++) {
            float x = wp.x + pad + i * (heartSize + gap);
            float y = wp.y + pad;
            int hpForHeart = m_hp - i * 10;

            std::array<GLfloat, 8>* uv;
            if (hpForHeart >= 10)      uv = &fullUV;
            else if (hpForHeart >= 5)  uv = &halfUV;
            else                       uv = &emptyUV;

            ImVec2 uv0((*uv)[2], (*uv)[5]);
            ImVec2 uv1((*uv)[0], (*uv)[3]);
            ImVec2 p0(x, y);
            ImVec2 p1(x + heartSize, y + heartSize);

            dl->AddImage((ImTextureID)(intptr_t)atlasID, p0, p1, uv0, uv1);
        }
    }
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

void Player::drawTimer()
{
    if (!m_roundActive) return;

    auto displaySize = ImGui::GetIO().DisplaySize;
    int minutes = (int)m_roundTimeLeft / 60;
    int seconds = (int)m_roundTimeLeft % 60;

    // Build two lines: round text (30px) + MM:SS (same size), via BitmapText
    m_hudText.setFontSize(30.0f);
    char roundBuf[32], timeBuf[16];
    snprintf(roundBuf, sizeof(roundBuf), "第 %d 回合", (int)m_roundNumber);
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", minutes, seconds);

    int roundW = m_hudText.measureTextWidth(roundBuf);
    int timeW  = m_hudText.measureTextWidth(timeBuf);
    int texW = std::max(roundW, timeW) + 12;
    int lineH = (int)(30.0f * 1.1f);
    int texH = 4 + lineH * 2 + 4;

    std::vector<std::string> lines = { roundBuf, timeBuf };
    unsigned char r = m_difficultyActive ? 255 : 255;
    unsigned char g = m_difficultyActive ? 165 : 255;
    unsigned char b = m_difficultyActive ? 0 : 255;
    GLuint texId = m_hudText.update(lines, texW, texH, true, r, g, b);

    const float pad = 4.0f;
    const float winW = (float)texW + pad * 2;
    const float winH = (float)texH + pad * 2;
    float winX = (displaySize.x - winW) * 0.5f;
    float winY = 6.0f;

    ImGui::SetNextWindowPos(ImVec2(winX, winY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(winW, winH), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.5f));

    int flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar;

    if (ImGui::Begin("Timer", nullptr, flags)) {
        ImVec2 wp = ImGui::GetWindowPos();
        auto* dl = ImGui::GetWindowDrawList();

        // Red flash background for MM:SS line when <= 30s
        bool redFlash = (m_roundTimeLeft <= 30.0f) && (sin(ImGui::GetTime() * 4.0f) > 0.0);
        if (redFlash) {
            float flashY = wp.y + pad + 4 + lineH;
            dl->AddRectFilled(ImVec2(wp.x + pad, flashY),
                              ImVec2(wp.x + pad + texW, flashY + lineH),
                              IM_COL32(200, 40, 40, 180));
        }

        if (texId)
            dl->AddImage((ImTextureID)(intptr_t)texId,
                         ImVec2(wp.x + pad, wp.y + pad),
                         ImVec2(wp.x + pad + texW, wp.y + pad + texH));
    }
    ImGui::End();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

void Player::drawFurnaceUI()
{
    if (!m_furnaceUIOpen) return;

    auto& atlas = BlockDatabase::get().textureAtlas;
    GLuint atlasID = atlas.getID();
    const float slotSize = 48.0f;
    const float padding = 4.0f;
    const float texPad = 4.0f;
    const float titleH = 22.0f;

    auto displaySize = ImGui::GetIO().DisplaySize;
    const int cols = 5;
    float hotbarW = cols * (slotSize + padding) + padding;
    float hotbarContentH = 1 * (slotSize + padding) + padding;
    float hotbarY = displaySize.y - hotbarContentH - 10.0f;
    float bpContentH = 3 * (slotSize + padding) + padding + 22.0f;
    float bpY = hotbarY - bpContentH - 6.0f;
    float craftContentH = 3 * (slotSize + padding) + padding + 22.0f;
    float craftY = bpY - craftContentH - 6.0f;

    float furnaceContentH = titleH + padding + slotSize + (padding + 8.0f) + slotSize + padding;
    float furnaceWinW = hotbarW;
    float furnaceX = (displaySize.x - furnaceWinW) * 0.5f;
    float furnaceY = craftY - furnaceContentH - 6.0f;

    const int winFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
                         ImGuiWindowFlags_NoBringToFrontOnFocus;

    // --- Prepare quantity BitmapText for furnace slots ---
    std::vector<std::string> qtyLines;
    int qtyMap[3] = {-1, -1, -1};
    auto pushQty = [&](int idx, const ItemStack& stack) {
        int n = stack.getNumInStack();
        if (n > 1) {
            qtyMap[idx] = (int)qtyLines.size();
            qtyLines.push_back(std::to_string(n));
        }
    };
    pushQty(0, m_furnace.input);
    pushQty(1, m_furnace.fuel);
    pushQty(2, m_furnace.output);
    int qtyTexW = 40, qtyTexH = 0;
    GLuint qtyTexId = 0;
    if (!qtyLines.empty()) {
        qtyTexH = 4 + (int)(qtyLines.size() * 26.4f) + 4;
        qtyTexId = m_furnaceQtyText.update(qtyLines, qtyTexW, qtyTexH);
    }
    auto drawFurnaceQty = [&](int idx, ImVec2 p1) {
        int lineIdx = qtyMap[idx];
        if (lineIdx < 0 || qtyTexId == 0) return;
        float lineY = 4.0f + lineIdx * 26.4f;
        float glyphH = 26.0f;
        float measuredW = (float)m_furnaceQtyText.measureTextWidth(qtyLines[lineIdx]);
        float glyphW = measuredW + 8.0f;
        ImVec2 qtyUV0(0.08f, lineY / qtyTexH);
        ImVec2 qtyUV1((4.0f + glyphW) / qtyTexW, (lineY + glyphH) / qtyTexH);
        float qtyH = 20.0f;
        float qtyW = glyphW * (qtyH / glyphH);
        ImGui::GetWindowDrawList()->AddImage(
            (ImTextureID)(intptr_t)qtyTexId,
            ImVec2(p1.x - qtyW - 1.0f, p1.y - qtyH),
            ImVec2(p1.x - 1.0f, p1.y),
            qtyUV0, qtyUV1);
    };

    // --- Title: "熔  炉" (uses m_furnaceText, separate from Timer's m_hudText) ---
    std::string titleStr = "熔  炉";
    m_furnaceText.setFontSize(20.0f);
    int titleTexW = m_furnaceText.measureTextWidth(titleStr) + 12;
    int titleTexH = (int)(20.0f * 1.1f) + 4;
    GLuint titleTexId = m_furnaceText.update({titleStr}, titleTexW, titleTexH, true);

    // --- Close button "X" (uses m_furnaceCloseText, independent instance) ---
    m_furnaceCloseText.setFontSize(18.0f);
    int closeTexW = m_furnaceCloseText.measureTextWidth("X") + 8;
    int closeTexH = (int)(18.0f * 1.1f) + 4;
    GLuint closeTexId = m_furnaceCloseText.update({"X"}, closeTexW, closeTexH, true);

    ImGui::SetNextWindowPos(ImVec2(furnaceX, furnaceY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(furnaceWinW, furnaceContentH), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    if (ImGui::Begin("Furnace", nullptr, winFlags)) {
        ImVec2 winPos = ImGui::GetWindowPos();
        auto* dl = ImGui::GetWindowDrawList();

        // --- Title ---
        if (titleTexId) {
            float tx = winPos.x + (furnaceWinW - titleTexW) * 0.5f;
            dl->AddImage((ImTextureID)(intptr_t)titleTexId,
                         ImVec2(tx, winPos.y + 2.0f),
                         ImVec2(tx + titleTexW, winPos.y + 2.0f + titleTexH));
        }

        // --- Close button [X] ---
        float closeX = winPos.x + furnaceWinW - closeTexW - 8.0f;
        float closeY = winPos.y + 2.0f;
        ImGui::SetCursorScreenPos(ImVec2(closeX, closeY));
        ImGui::PushID("furnaceClose");
        if (ImGui::InvisibleButton("##furnaceCloseBtn", ImVec2((float)closeTexW, (float)closeTexH)))
            m_furnaceUIOpen = false;
        ImGui::PopID();
        bool closeHovered = ImGui::IsItemHovered();
        dl->AddImage((ImTextureID)(intptr_t)closeTexId,
                     ImVec2(closeX, closeY), ImVec2(closeX + closeTexW, closeY + closeTexH),
                     ImVec2(0, 0), ImVec2(1, 1),
                     closeHovered ? IM_COL32(255, 80, 80, 255) : IM_COL32(200, 200, 200, 255));

        // Slots: centered functional area (input + arrow + output), fuel below
        float totalW = slotSize + 24.0f + 20.0f + 24.0f + slotSize;
        float areaX = (furnaceWinW - totalW) * 0.5f;
        float inputX = areaX;
        float inputY = titleH + padding;
        float outputX = areaX + slotSize + 24.0f + 20.0f + 24.0f;
        float outputY = titleH + padding;
        float fuelX = areaX;
        float fuelY = inputY + slotSize + padding + 8.0f;

        // Arrow region: between input and output
        float arrowX = areaX + slotSize + 24.0f;
        float arrowY = inputY + slotSize * 0.5f;
        float arrowLen = 20.0f;

        // ==================== INPUT SLOT ====================
        ImGui::SetCursorPos(ImVec2(inputX, inputY));
        ImGui::PushID(400);
        ImVec2 inP0 = ImGui::GetCursorScreenPos();
        ImGui::Dummy(ImVec2(slotSize, slotSize));
        ImVec2 inP1(inP0.x + slotSize, inP0.y + slotSize);
        dl->AddRectFilled(inP0, inP1, IM_COL32(60, 60, 60, 200));
        dl->AddRect(inP0, inP1, IM_COL32(180, 180, 180, 255));
        const auto& inMat = m_furnace.input.getMaterial();
        if (inMat.id != Material::ID::Nothing) {
            BlockId bId = inMat.toBlockID();
            const auto& bd = BlockDatabase::get().getData(bId);
            auto uv = atlas.getTexture(bd.getBlockData().texTopCoord);
            dl->AddImage((ImTextureID)(intptr_t)atlasID,
                ImVec2(inP0.x + texPad, inP0.y + texPad),
                ImVec2(inP1.x - texPad, inP1.y - texPad),
                ImVec2(uv[2], uv[5]), ImVec2(uv[0], uv[3]));
            drawFurnaceQty(0, inP1);
            // Drag source
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                int fakeSlot = 900;
                ImGui::SetDragDropPayload("INV_SLOT", &fakeSlot, sizeof(int));
                auto duv = atlas.getTexture(bd.getBlockData().texTopCoord);
                ImGui::Image((ImTextureID)(intptr_t)atlasID, ImVec2(slotSize * 0.7f, slotSize * 0.7f),
                             ImVec2(duv[2], duv[5]), ImVec2(duv[0], duv[3]));
                ImGui::EndDragDropSource();
            }
        }
        // Drag target
        if (ImGui::BeginDragDropTarget()) {
            const ImGuiPayload* pl = ImGui::AcceptDragDropPayload("INV_SLOT");
            if (pl) {
                int src = *(const int*)pl->Data;
                const auto& sm = m_items[src].getMaterial();
                if (sm.id != Material::ID::Nothing) {
                    const auto& curMat = m_furnace.input.getMaterial();
                    if (curMat.id == Material::ID::Nothing || curMat.id == sm.id) {
                        if (curMat.id == Material::ID::Nothing)
                            m_furnace.input = ItemStack(sm, 1);
                        else
                            m_furnace.input.add(1);
                        m_items[src].remove();
                        m_furnace.isSmelting = false; m_furnace.progress = 0.f;
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }
        // Shift+click quick-add
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)
            && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift)) {
            const auto& hm = m_items[m_heldItem].getMaterial();
            if (hm.id != Material::ID::Nothing) {
                const auto& curMat = m_furnace.input.getMaterial();
                if (curMat.id == Material::ID::Nothing || curMat.id == hm.id) {
                    if (curMat.id == Material::ID::Nothing)
                        m_furnace.input = ItemStack(hm, 1);
                    else
                        m_furnace.input.add(1);
                    m_items[m_heldItem].remove();
                    m_furnace.isSmelting = false; m_furnace.progress = 0.f;
                }
            }
        }
        // Left-click retrieve (without Shift)
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)
            && !sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift)) {
            if (inMat.id != Material::ID::Nothing && addItem(inMat)) {
                m_furnace.input.remove();
                m_furnace.isSmelting = false; m_furnace.progress = 0.f;
            }
        }
        ImGui::PopID();

        // ==================== OUTPUT SLOT ====================
        ImGui::SetCursorPos(ImVec2(outputX, outputY));
        ImGui::PushID(401);
        ImVec2 outP0 = ImGui::GetCursorScreenPos();
        ImGui::Dummy(ImVec2(slotSize, slotSize));
        ImVec2 outP1(outP0.x + slotSize, outP0.y + slotSize);
        dl->AddRectFilled(outP0, outP1, IM_COL32(50, 50, 50, 200));
        dl->AddRect(outP0, outP1, IM_COL32(200, 200, 100, 255));
        const auto& outMat = m_furnace.output.getMaterial();
        if (outMat.id != Material::ID::Nothing) {
            const auto& bd = BlockDatabase::get().getData(outMat.toBlockID());
            auto uv = atlas.getTexture(bd.getBlockData().texTopCoord);
            dl->AddImage((ImTextureID)(intptr_t)atlasID,
                ImVec2(outP0.x + texPad, outP0.y + texPad),
                ImVec2(outP1.x - texPad, outP1.y - texPad),
                ImVec2(uv[2], uv[5]), ImVec2(uv[0], uv[3]));
            drawFurnaceQty(2, outP1);
        }
        // Left-click to retrieve only (no drag-in)
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            if (outMat.id != Material::ID::Nothing && addItem(outMat))
                m_furnace.output.remove();
        }
        ImGui::PopID();

        // ==================== FUEL SLOT ====================
        ImGui::SetCursorPos(ImVec2(fuelX, fuelY));
        ImGui::PushID(402);
        ImVec2 fuelP0 = ImGui::GetCursorScreenPos();
        ImGui::Dummy(ImVec2(slotSize, slotSize));
        ImVec2 fuelP1(fuelP0.x + slotSize, fuelP0.y + slotSize);
        dl->AddRectFilled(fuelP0, fuelP1, IM_COL32(60, 60, 60, 200));
        dl->AddRect(fuelP0, fuelP1, IM_COL32(180, 180, 180, 255));
        const auto& fuelMat = m_furnace.fuel.getMaterial();
        if (fuelMat.id != Material::ID::Nothing) {
            BlockId bId = fuelMat.toBlockID();
            const auto& bd = BlockDatabase::get().getData(bId);
            auto uv = atlas.getTexture(bd.getBlockData().texTopCoord);
            dl->AddImage((ImTextureID)(intptr_t)atlasID,
                ImVec2(fuelP0.x + texPad, fuelP0.y + texPad),
                ImVec2(fuelP1.x - texPad, fuelP1.y - texPad),
                ImVec2(uv[2], uv[5]), ImVec2(uv[0], uv[3]));
            drawFurnaceQty(1, fuelP1);
            // Drag source
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                int fakeSlot = 901;
                ImGui::SetDragDropPayload("INV_SLOT", &fakeSlot, sizeof(int));
                auto duv = atlas.getTexture(bd.getBlockData().texTopCoord);
                ImGui::Image((ImTextureID)(intptr_t)atlasID, ImVec2(slotSize * 0.7f, slotSize * 0.7f),
                             ImVec2(duv[2], duv[5]), ImVec2(duv[0], duv[3]));
                ImGui::EndDragDropSource();
            }
        }
        // Drag target
        if (ImGui::BeginDragDropTarget()) {
            const ImGuiPayload* pl = ImGui::AcceptDragDropPayload("INV_SLOT");
            if (pl) {
                int src = *(const int*)pl->Data;
                const auto& sm = m_items[src].getMaterial();
                if (sm.id != Material::ID::Nothing) {
                    const auto& curMat = m_furnace.fuel.getMaterial();
                    if (curMat.id == Material::ID::Nothing || curMat.id == sm.id) {
                        if (curMat.id == Material::ID::Nothing)
                            m_furnace.fuel = ItemStack(sm, 1);
                        else
                            m_furnace.fuel.add(1);
                        m_items[src].remove();
                        m_furnace.isSmelting = false; m_furnace.progress = 0.f;
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }
        // Shift+click quick-add
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)
            && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift)) {
            const auto& hm = m_items[m_heldItem].getMaterial();
            if (hm.id != Material::ID::Nothing) {
                const auto& curMat = m_furnace.fuel.getMaterial();
                if (curMat.id == Material::ID::Nothing || curMat.id == hm.id) {
                    if (curMat.id == Material::ID::Nothing)
                        m_furnace.fuel = ItemStack(hm, 1);
                    else
                        m_furnace.fuel.add(1);
                    m_items[m_heldItem].remove();
                    m_furnace.isSmelting = false; m_furnace.progress = 0.f;
                }
            }
        }
        // Left-click retrieve (without Shift)
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)
            && !sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift)) {
            if (fuelMat.id != Material::ID::Nothing && addItem(fuelMat)) {
                m_furnace.fuel.remove();
                m_furnace.isSmelting = false; m_furnace.progress = 0.f;
            }
        }
        ImGui::PopID();

        // ==================== ARROW + PROGRESS RING ====================
        {
            ImU32 arrowColor = IM_COL32(255, 255, 255, 220);
            // Shaft
            dl->AddLine(ImVec2(winPos.x + arrowX, winPos.y + arrowY),
                        ImVec2(winPos.x + arrowX + arrowLen, winPos.y + arrowY),
                        arrowColor, 4.0f);
            // Head
            dl->AddLine(ImVec2(winPos.x + arrowX + arrowLen, winPos.y + arrowY),
                        ImVec2(winPos.x + arrowX + arrowLen - 9, winPos.y + arrowY - 8),
                        arrowColor, 4.0f);
            dl->AddLine(ImVec2(winPos.x + arrowX + arrowLen, winPos.y + arrowY),
                        ImVec2(winPos.x + arrowX + arrowLen - 9, winPos.y + arrowY + 8),
                        arrowColor, 4.0f);

            // Progress ring around arrow center
            if (m_furnace.isSmelting) {
                float cx = winPos.x + arrowX + arrowLen * 0.5f;
                float cy = winPos.y + arrowY;
                float r = 20.0f;
                float pi = 3.14159265f;
                int segs = 36;
                float full = m_furnace.progress * 2.0f * pi;
                float startAngle = -pi / 2.0f;
                ImVec2 prev(cx + r * cosf(startAngle), cy + r * sinf(startAngle));
                for (int i = 1; i <= segs; i++) {
                    float a = (float)i / (float)segs * full;
                    if (a > 2.0f * pi) a = 2.0f * pi;
                    ImVec2 pt(cx + r * cosf(startAngle + a), cy + r * sinf(startAngle + a));
                    dl->AddLine(prev, pt, IM_COL32(255, 180, 60, 240), 3.0f);
                    prev = pt;
                }
            }
        }

        // ==================== AUTO-START SMELTING ====================
        if (!m_furnace.isSmelting) {
            const SmeltingRecipe* recipe = findSmeltingRecipe(
                m_furnace.input.getMaterial().id, m_furnace.input.getNumInStack(),
                m_furnace.fuel.getMaterial().id, m_furnace.fuel.getNumInStack());
            if (recipe) {
                const auto& outM = m_furnace.output.getMaterial();
                const Material& recipeOut = (recipe->output == Material::ID::IronIngot)
                    ? static_cast<const Material&>(Material::IRON_INGOT)
                    : static_cast<const Material&>(Material::COOKED_MEAT);
                if (outM.id == Material::ID::Nothing ||
                    (outM.id == recipeOut.id && m_furnace.output.getNumInStack() < recipeOut.maxStackSize))
                    m_furnace.isSmelting = true;
            }
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

void Player::drawSettlement(const Camera* /*camera*/)
{
    // Trigger: round ended OR player dead
    bool shouldShow = (!m_roundActive && m_roundNumber > 0) || m_isDead;
    if (!shouldShow) return;

    auto displaySize = ImGui::GetIO().DisplaySize;

    // ================================================================
    // Prepare summary text
    // ================================================================
    int survTotalSec = (int)(600.0f - m_roundTimeLeft);
    int survMin = survTotalSec / 60;
    int survSec = survTotalSec % 60;

    const char* titleText = "撤离成功";
    switch (m_settlementOutcome) {
        case SettlementOutcome::Success: titleText = "撤离成功"; break;
        case SettlementOutcome::TimeUp:  titleText = "时间耗尽"; break;
        case SettlementOutcome::Death:   titleText = "你已死亡"; break;
    }
    char titleBuf[64];
    snprintf(titleBuf, sizeof(titleBuf), "═══ %s ═══", titleText);

    char survBuf[64];
    snprintf(survBuf, sizeof(survBuf), "存活时间   %02d:%02d", survMin, survSec);

    char killBuf[64];
    snprintf(killBuf, sizeof(killBuf), "击杀猪人   %d", m_pigmanKills);

    int totalItems = 0;
    int totalTypes = 0;
    for (auto& [matId, count] : m_roundCollection) {
        if (count > 0) { totalItems += count; totalTypes++; }
    }
    char collectBuf[64];
    snprintf(collectBuf, sizeof(collectBuf), "收集物品   %d 类 / %d 个", totalTypes, totalItems);

    // ================================================================
    // Build collection detail lines
    // ================================================================
    std::vector<std::string> detailLines;
    for (auto& [matId, count] : m_roundCollection) {
        if (count > 0) {
            char buf[64];
            snprintf(buf, sizeof(buf), "  %s  x%d", getMaterialCNName(matId), count);
            detailLines.push_back(buf);
        }
    }
    int detailCount = (int)detailLines.size();

    // ================================================================
    // Batch ALL text into one BitmapText texture
    // ================================================================
    std::vector<std::string> textLines;
    textLines.push_back(titleBuf);
    textLines.push_back("");
    textLines.push_back(survBuf);
    textLines.push_back(killBuf);
    textLines.push_back(collectBuf);
    if (detailCount > 0) {
        textLines.push_back("");
        textLines.push_back("收集明细:");
        for (auto& dl : detailLines)
            textLines.push_back(dl);
    }

    int texW = 320;
    int lineH = 22;
    int texH = 8 + (int)textLines.size() * lineH + 8;
    m_hudText.setFontSize(20.0f);
    GLuint hudTexId = m_hudText.update(textLines, texW, texH);

    // ================================================================
    // Full-screen overlay
    // ================================================================
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(displaySize, ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.7f));

    int flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoBringToFrontOnFocus;

    if (ImGui::Begin("Settlement", nullptr, flags)) {
        float panelW = (float)texW + 40.0f;
        float panelH = (float)texH + 60.0f;
        float panelX = (displaySize.x - panelW) * 0.5f;
        float panelY = (displaySize.y - panelH) * 0.5f;

        auto* dl = ImGui::GetWindowDrawList();

        // Panel background
        dl->AddRectFilled(ImVec2(panelX, panelY),
                          ImVec2(panelX + panelW, panelY + panelH),
                          IM_COL32(20, 20, 20, 220));
        dl->AddRect(ImVec2(panelX, panelY),
                    ImVec2(panelX + panelW, panelY + panelH),
                    IM_COL32(180, 180, 180, 255), 0.0f, 0, 2.0f);

        // BitmapText texture (all Chinese text in one image)
        dl->AddImage((ImTextureID)(intptr_t)hudTexId,
                     ImVec2(panelX + 20, panelY + 20),
                     ImVec2(panelX + 20 + texW, panelY + 20 + texH));

        // "准备下一回合" button — clickable
        float btnW = 160.0f, btnH = 36.0f;
        float btnX = panelX + (panelW - btnW) * 0.5f;
        float btnY = panelY + panelH - btnH - 16.0f;

        // Render button text via BitmapText (Chinese-capable, separate instance)
        m_buttonText.setFontSize(18.0f);
        std::vector<std::string> btnLines = { "准备下一回合" };
        int btnTexW = m_buttonText.measureTextWidth("准备下一回合") + 12;
        int btnTexH = (int)(18.0f * 1.1f) + 4;
        GLuint btnTexId = m_buttonText.update(btnLines, btnTexW, btnTexH, true);

        ImGui::SetCursorScreenPos(ImVec2(btnX, btnY));
        ImGui::PushID("nextRound");
        bool clicked = ImGui::InvisibleButton("##nextRoundBtn", ImVec2(btnW, btnH));
        ImGui::PopID();

        // Draw button visual
        bool hovered = ImGui::IsItemHovered();
        ImU32 btnBg = hovered ? IM_COL32(100, 100, 100, 220) : IM_COL32(60, 60, 60, 200);
        ImU32 btnBorder = hovered ? IM_COL32(220, 220, 220, 255) : IM_COL32(150, 150, 150, 255);
        dl->AddRectFilled(ImVec2(btnX, btnY), ImVec2(btnX + btnW, btnY + btnH), btnBg);
        dl->AddRect(ImVec2(btnX, btnY), ImVec2(btnX + btnW, btnY + btnH), btnBorder);

        // Button text centered in button area
        if (btnTexId) {
            float textX = btnX + (btnW - btnTexW) * 0.5f;
            float textY = btnY + (btnH - btnTexH) * 0.5f;
            dl->AddImage((ImTextureID)(intptr_t)btnTexId,
                         ImVec2(textX, textY),
                         ImVec2(textX + btnTexW, textY + btnTexH));
        }

        if (clicked) {
            m_requestNewRound = true;
        }
    }
    ImGui::End();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

void Player::draw(RenderMaster& /*master*/, const Camera* camera)
{

    auto& atlas = BlockDatabase::get().textureAtlas;
    GLuint atlasID = atlas.getID();
    const float slotSize = 48.0f;
    const float padding = 4.0f;
    const float texPad = 4.0f;
    const int cols = 5;

    // Window flags: fixed position, no user dragging
    const int winFlags = ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_NoCollapse |
                         ImGuiWindowFlags_NoTitleBar |
                         ImGuiWindowFlags_NoBringToFrontOnFocus;

    auto displaySize = ImGui::GetIO().DisplaySize;
    float hotbarW = cols * (slotSize + padding) + padding;
    float hotbarContentH = 1 * (slotSize + padding) + padding;

    // Hotbar: bottom-center of screen
    float hotbarX = (displaySize.x - hotbarW) * 0.5f;
    float hotbarY = displaySize.y - hotbarContentH - 10.0f;

    // --- Collect quantities for BitmapText rendering (batch all into one texture) ---
    std::vector<std::string> qtyLines;
    int qtySlotMap[20];
    int qtyTexW = 40, qtyTexH = 0;
    GLuint qtyTexId = 0;
    std::memset(qtySlotMap, -1, sizeof(qtySlotMap));
    for (int i = 0; i < 20; i++) {
        int n = m_items[i].getNumInStack();
        if (n > 1) {
            qtySlotMap[i] = (int)qtyLines.size();
            qtyLines.push_back(std::to_string(n));
        }
    }
    // Crafting grid + result quantities (same batch, shared m_bitmapText)
    int craftQtyMap[10]; // 0-8 = grid, 9 = result
    std::memset(craftQtyMap, -1, sizeof(craftQtyMap));
    if (m_backpackOpen) {
        for (int i = 0; i < 9; i++) {
            int n = m_craftGrid[i].getNumInStack();
            if (n > 1) {
                craftQtyMap[i] = (int)qtyLines.size();
                qtyLines.push_back(std::to_string(n));
            }
        }
        if (m_currentRecipe && m_currentRecipe->outputCount > 1) {
            craftQtyMap[9] = (int)qtyLines.size();
            qtyLines.push_back(std::to_string(m_currentRecipe->outputCount));
        }
    }
    if (!qtyLines.empty()) {
        qtyTexH = 4 + (int)(qtyLines.size() * 26.4f) + 4;
        qtyTexId = m_bitmapText.update(qtyLines, qtyTexW, qtyTexH);
    }

    // --- Draw a single inventory slot ---
    auto drawSlot = [&](int slotIndex, ImVec2 p0, ImVec2 p1, bool isHotbar) {
        const auto& stack = m_items[slotIndex];
        const auto& mat = stack.getMaterial();

        // Background
        ImU32 bgColor = (slotIndex == m_heldItem)
            ? IM_COL32(255, 215, 0, 80)
            : IM_COL32(60, 60, 60, 200);
        auto* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(p0, p1, bgColor);
        dl->AddRect(p0, p1, IM_COL32(180, 180, 180, 255));

        // Selected hotbar slot: yellow border
        if (isHotbar && slotIndex == m_heldItem)
        {
            dl->AddRect(p0, p1, IM_COL32(255, 255, 0, 255), 0.0f, 0, 2.0f);
        }

        // Item icon
        if (mat.id != Material::ID::Nothing)
        {
            BlockId bId = mat.toBlockID();
            const auto& blockData = BlockDatabase::get().getData(bId);
            auto uv = atlas.getTexture(blockData.getBlockData().texTopCoord);
            ImVec2 uv0(uv[2], uv[5]);
            ImVec2 uv1(uv[0], uv[3]);
            dl->AddImage((ImTextureID)(intptr_t)atlasID,
                         ImVec2(p0.x + texPad, p0.y + texPad),
                         ImVec2(p1.x - texPad, p1.y - texPad),
                         uv0, uv1);
        }
    };

    // --- Draw quantity overlay on a slot ---
    auto drawQuantity = [&](int slotIndex, ImVec2 p1) {
        int lineIdx = qtySlotMap[slotIndex];
        if (lineIdx < 0 || qtyTexId == 0) return;
        // BitmapText renders at fontSize=24, line starts at y=4+N*26.4
        float lineY = 4.0f + lineIdx * 26.4f;
        float glyphH = 26.0f;
        // Measure actual glyph width for right-alignment
        float measuredW = (float)m_bitmapText.measureTextWidth(qtyLines[lineIdx]);
        float glyphW = measuredW + 8.0f; // margin for glyph bearing
        // UV sub-rect: glyph at x=4, texW=40
        ImVec2 qtyUV0(0.08f, lineY / qtyTexH);
        ImVec2 qtyUV1((4.0f + glyphW) / qtyTexW, (lineY + glyphH) / qtyTexH);
        // Display size proportional to measured width
        float qtyH = 20.0f;
        float qtyW = glyphW * (qtyH / glyphH);
        // Right-aligned at slot bottom-right
        ImGui::GetWindowDrawList()->AddImage(
            (ImTextureID)(intptr_t)qtyTexId,
            ImVec2(p1.x - qtyW - 1.0f, p1.y - qtyH),
            ImVec2(p1.x - 1.0f, p1.y),
            qtyUV0, qtyUV1);
    };

    // --- Weapon animation tick ---
    if (m_isSwinging) {
        m_swingTimer += 0.016f;
        if (m_swingTimer >= 0.3f) { m_isSwinging = false; m_swingTimer = 0.0f; }
    }

    // --- HUD: Health Bar (above hotbar, hidden when backpack open) ---
    if (!m_isDead && !m_backpackOpen)
        drawHealthBar();


    // --- HUD: Timer + Round Counter (top-center) ---
    if (m_roundActive && !m_isDead)
        drawTimer();

    // --- Furnace UI ---
    drawFurnaceUI();

    // --- Backpack + Crafting (B key) ---
    if (m_backpackOpen)
    {
        const int craftRows = 3, craftCols = 3;
        float craftContentW = craftCols * (slotSize + padding) + padding + slotSize + padding + 56.0f;
        float craftContentH = craftRows * (slotSize + padding) + padding + 22.0f;
        const int bpRows = 3;
        float bpContentH = bpRows * (slotSize + padding) + padding + 22.0f;

        // Backpack: above hotbar
        float bpX = (displaySize.x - hotbarW) * 0.5f;
        float bpY = hotbarY - bpContentH - 6.0f;
        // Crafting: above backpack
        float craftWinX = (displaySize.x - craftContentW) * 0.5f;
        float craftWinY = bpY - craftContentH - 6.0f;

        // --- Crafting window (top) ---
        ImGui::SetNextWindowPos(ImVec2(craftWinX, craftWinY), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(craftContentW, craftContentH), ImGuiCond_Always);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        if (ImGui::Begin("Crafting", nullptr, winFlags))
        {
            ImVec2 winPos = ImGui::GetWindowPos();
            auto* wdl = ImGui::GetWindowDrawList();

            // 标题
            {
                m_craftTitleText.setFontSize(20.0f);
                std::string title = "工 作 台";
                int tw = m_craftTitleText.measureTextWidth(title) + 12;
                int th = (int)(20.0f * 1.1f) + 4;
                GLuint tid = m_craftTitleText.update({title}, tw, th, true);
                if (tid) {
                    float tx = winPos.x + (craftContentW - tw) * 0.5f;
                    float ty = winPos.y + 2.0f;
                    wdl->AddImage((ImTextureID)(intptr_t)tid,
                                   ImVec2(tx, ty), ImVec2(tx + tw, ty + th));
                }
            }

            // 3×3 grid
            for (int row = 0; row < craftRows; row++)
                for (int col = 0; col < craftCols; col++)
                {
                    int slotIdx = row * craftCols + col;
                    float x = padding + col * (slotSize + padding);
                    float y = 22.0f + padding + row * (slotSize + padding);
                    ImGui::SetCursorPos(ImVec2(x, y));
                    ImGui::PushID(100 + slotIdx);
                    ImVec2 p0 = ImGui::GetCursorScreenPos();
                    ImGui::Dummy(ImVec2(slotSize, slotSize));
                    ImVec2 p1(p0.x + slotSize, p0.y + slotSize);
                    wdl->AddRectFilled(p0, p1, IM_COL32(60, 60, 60, 200));
                    wdl->AddRect(p0, p1, IM_COL32(180, 180, 180, 255));

                    const auto& gmat = m_craftGrid[slotIdx].getMaterial();
                    if (gmat.id != Material::ID::Nothing)
                    {
                        BlockId bId = gmat.toBlockID();
                        const auto& bd = BlockDatabase::get().getData(bId);
                        auto uv = atlas.getTexture(bd.getBlockData().texTopCoord);
                        wdl->AddImage((ImTextureID)(intptr_t)atlasID,
                            ImVec2(p0.x + texPad, p0.y + texPad),
                            ImVec2(p1.x - texPad, p1.y - texPad),
                            ImVec2(uv[2], uv[5]), ImVec2(uv[0], uv[3]));
                        int lineIdx = craftQtyMap[slotIdx];
                        if (lineIdx >= 0 && qtyTexId) {
                            float lineY = 4.0f + lineIdx * 26.4f;
                            float glyphH = 26.0f;
                            float mw = (float)m_bitmapText.measureTextWidth(qtyLines[lineIdx]) + 8.0f;
                            ImVec2 uv0(0.08f, lineY / qtyTexH);
                            ImVec2 uv1((4.0f + mw) / qtyTexW, (lineY + glyphH) / qtyTexH);
                            float qH = 20.0f, qW = mw * (qH / glyphH);
                            wdl->AddImage((ImTextureID)(intptr_t)qtyTexId,
                                ImVec2(p1.x - qW - 1.0f, p1.y - qH),
                                ImVec2(p1.x - 1.0f, p1.y), uv0, uv1);
                        }
                    }

                    if (ImGui::BeginDragDropTarget())
                    {
                        const ImGuiPayload* pl = ImGui::AcceptDragDropPayload("INV_SLOT");
                        if (pl) {
                            int src = *(const int*)pl->Data;
                            const auto& sm = m_items[src].getMaterial();
                            if (sm.id != Material::ID::Nothing) {
                                if (m_craftGrid[slotIdx].getMaterial().id == Material::ID::Nothing)
                                    m_craftGrid[slotIdx] = ItemStack(sm, 1);
                                else m_craftGrid[slotIdx].add(1);
                                m_items[src].remove();
                                m_currentRecipe = findMatchingRecipe(m_craftGrid);
                            }
                        }
                        ImGui::EndDragDropTarget();
                    }
                    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift)) {
                            // Shift+Click: quick-add from hotbar to this grid slot
                            const auto& hm = m_items[m_heldItem].getMaterial();
                            if (hm.id != Material::ID::Nothing) {
                                if (m_craftGrid[slotIdx].getMaterial().id == Material::ID::Nothing)
                                    m_craftGrid[slotIdx] = ItemStack(hm, 1);
                                else if (m_craftGrid[slotIdx].getMaterial().id == hm.id)
                                    m_craftGrid[slotIdx].add(1);
                                m_items[m_heldItem].remove();
                                m_currentRecipe = findMatchingRecipe(m_craftGrid);
                            }
                        } else {
                            const auto& gm = m_craftGrid[slotIdx].getMaterial();
                            if (gm.id != Material::ID::Nothing && addItem(gm))
                            { m_craftGrid[slotIdx].remove(); m_currentRecipe = findMatchingRecipe(m_craftGrid); }
                        }
                    }
                    ImGui::PopID();
                }

            // Arrow →
            {
                float ax = winPos.x + padding + craftCols * (slotSize + padding) + 12.0f;
                float ay = winPos.y + 22.0f + padding + 1 * (slotSize + padding) + slotSize * 0.5f;
                float len = 20.0f;
                ImU32 ac = IM_COL32(255, 255, 255, 240);
                // shaft
                wdl->AddLine(ImVec2(ax, ay), ImVec2(ax + len, ay), ac, 4.0f);
                // head
                wdl->AddLine(ImVec2(ax + len, ay), ImVec2(ax + len - 9, ay - 8), ac, 4.0f);
                wdl->AddLine(ImVec2(ax + len, ay), ImVec2(ax + len - 9, ay + 8), ac, 4.0f);
            }

            // Result slot
            {
                float rx = padding + craftCols * (slotSize + padding) + 50.0f;
                float ry = 22.0f + padding + 1 * (slotSize + padding);
                ImGui::SetCursorPos(ImVec2(rx, ry));
                ImGui::PushID(200);
                ImVec2 rp0 = ImGui::GetCursorScreenPos();
                ImGui::Dummy(ImVec2(slotSize, slotSize));
                ImVec2 rp1(rp0.x + slotSize, rp0.y + slotSize);
                wdl->AddRectFilled(rp0, rp1, IM_COL32(50, 50, 50, 200));
                wdl->AddRect(rp0, rp1, IM_COL32(200, 200, 100, 255));

                if (m_currentRecipe) {
                    BlockId bId = m_currentRecipe->output->toBlockID();
                    const auto& bd = BlockDatabase::get().getData(bId);
                    auto uv = atlas.getTexture(bd.getBlockData().texTopCoord);
                    wdl->AddImage((ImTextureID)(intptr_t)atlasID,
                        ImVec2(rp0.x + texPad, rp0.y + texPad),
                        ImVec2(rp1.x - texPad, rp1.y - texPad),
                        ImVec2(uv[2], uv[5]), ImVec2(uv[0], uv[3]));
                    // Quantity overlay for result
                    int rLineIdx = craftQtyMap[9];
                    if (rLineIdx >= 0 && qtyTexId) {
                        float rLineY = 4.0f + rLineIdx * 26.4f;
                        float rGlyphH = 26.0f;
                        float rMw = (float)m_bitmapText.measureTextWidth(qtyLines[rLineIdx]) + 8.0f;
                        ImVec2 rUV0(0.08f, rLineY / qtyTexH);
                        ImVec2 rUV1((4.0f + rMw) / qtyTexW, (rLineY + rGlyphH) / qtyTexH);
                        float rqH = 20.0f, rqW = rMw * (rqH / rGlyphH);
                        wdl->AddImage((ImTextureID)(intptr_t)qtyTexId,
                            ImVec2(rp1.x - rqW - 1.0f, rp1.y - rqH),
                            ImVec2(rp1.x - 1.0f, rp1.y), rUV0, rUV1);
                    }
                }
                if (m_currentRecipe && ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    const Material& outMat = *m_currentRecipe->output;
                    // 熔炉唯一性检查：已有熔炉则静默拒绝合成
                    if (outMat.id == Material::ID::Furnace && hasFurnace()) {
                        // 已有熔炉，不消耗材料，不做任何事
                    } else {
                        // Add outputCount items, one at a time
                        bool allAdded = true;
                        for (int oc = 0; oc < m_currentRecipe->outputCount; oc++) {
                            if (!addItem(outMat)) { allAdded = false; break; }
                        }
                        if (allAdded) {
                            for (int i = 0; i < 9; i++)
                                if (m_currentRecipe->pattern[i] != nullptr) m_craftGrid[i].remove();
                        m_currentRecipe = findMatchingRecipe(m_craftGrid);
                    }
                    }
                }
                ImGui::PopID();
            }
        }
        ImGui::End();
        ImGui::PopStyleVar();

        // --- Backpack window (below crafting) ---
        ImGui::SetNextWindowPos(ImVec2(bpX, bpY), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(hotbarW, bpContentH), ImGuiCond_Always);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        if (ImGui::Begin("Backpack", nullptr, winFlags))
        {
            // 标题
            {
                m_bpTitleText.setFontSize(20.0f);
                std::string title = "背 包";
                int tw = m_bpTitleText.measureTextWidth(title) + 12;
                int th = (int)(20.0f * 1.1f) + 4;
                GLuint tid = m_bpTitleText.update({title}, tw, th, true);
                if (tid) {
                    ImVec2 winPos = ImGui::GetWindowPos();
                    float tx = winPos.x + (hotbarW - tw) * 0.5f;
                    float ty = winPos.y + 2.0f;
                    ImGui::GetWindowDrawList()->AddImage((ImTextureID)(intptr_t)tid,
                                       ImVec2(tx, ty), ImVec2(tx + tw, ty + th));
                }
            }

            for (int bpRow = 0; bpRow < bpRows; bpRow++)
                for (int col = 0; col < cols; col++)
                {
                    int slotIndex = 5 + bpRow * cols + col;
                    float x = padding + col * (slotSize + padding);
                    float y = 22.0f + padding + bpRow * (slotSize + padding);
                    ImGui::SetCursorPos(ImVec2(x, y));
                    ImGui::PushID(slotIndex);
                    ImVec2 p0 = ImGui::GetCursorScreenPos();
                    ImGui::Dummy(ImVec2(slotSize, slotSize));
                    ImVec2 p1(p0.x + slotSize, p0.y + slotSize);
                    drawSlot(slotIndex, p0, p1, false);
                    drawQuantity(slotIndex, p1);
                    // Yellow border for safe slots (17, 18, 19)
                    if (slotIndex >= 17)
                        ImGui::GetWindowDrawList()->AddRect(p0, p1, IM_COL32(255, 215, 0, 255), 0.0f, 0, 2.0f);


                    const auto& mat = m_items[slotIndex].getMaterial();
                    if (mat.id != Material::ID::Nothing && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                        ImGui::SetDragDropPayload("INV_SLOT", &slotIndex, sizeof(int));
                        BlockId bId = mat.toBlockID();
                        const auto& bd = BlockDatabase::get().getData(bId);
                        auto uv = atlas.getTexture(bd.getBlockData().texTopCoord);
                        ImGui::Image((ImTextureID)(intptr_t)atlasID, ImVec2(slotSize * 0.7f, slotSize * 0.7f),
                                     ImVec2(uv[2], uv[5]), ImVec2(uv[0], uv[3]));
                        ImGui::EndDragDropSource();
                    }
                    if (ImGui::BeginDragDropTarget()) {
                        const ImGuiPayload* pl = ImGui::AcceptDragDropPayload("INV_SLOT");
                        if (pl) { int src = *(const int*)pl->Data; std::swap(m_items[src], m_items[slotIndex]); }
                        ImGui::EndDragDropTarget();
                    }
                    // Shift+右键 → 丢弃确认（用坐标判定hover）
                    ImVec2 mp = ImGui::GetIO().MousePos;
                    if (mat.id != Material::ID::Nothing
                        && mp.x >= p0.x && mp.x <= p1.x && mp.y >= p0.y && mp.y <= p1.y
                        && ImGui::IsMouseClicked(ImGuiMouseButton_Right)
                        && (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift))) {
                        m_discardPending = slotIndex;
                        m_discardMouseX = mp.x; m_discardMouseY = mp.y;
                    }
                    ImGui::PopID();
                }
        }
        ImGui::End();
        ImGui::PopStyleVar();

    }

    // --- Hotbar window (1×5, slots 0-4, always visible) ---
    ImGui::SetNextWindowPos(ImVec2(hotbarX, hotbarY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(hotbarW, hotbarContentH), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    if (ImGui::Begin("Hotbar", nullptr, winFlags))
    {
        for (int col = 0; col < 5; col++)
        {
            int slotIndex = col;
            float x = padding + col * (slotSize + padding);
            float y = padding;
            ImGui::SetCursorPos(ImVec2(x, y));

            ImGui::PushID(slotIndex);
            ImVec2 p0 = ImGui::GetCursorScreenPos();
            ImGui::Dummy(ImVec2(slotSize, slotSize));
            ImVec2 p1(p0.x + slotSize, p0.y + slotSize);
            drawSlot(slotIndex, p0, p1, true);
            drawQuantity(slotIndex, p1);

            // Drag-and-drop on hotbar too
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
            // Shift+右键 → 丢弃确认（用坐标判定hover）
            ImVec2 mp2 = ImGui::GetIO().MousePos;
            if (mat.id != Material::ID::Nothing
                && mp2.x >= p0.x && mp2.x <= p1.x && mp2.y >= p0.y && mp2.y <= p1.y
                && ImGui::IsMouseClicked(ImGuiMouseButton_Right)
                && (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift))) {
                m_discardPending = slotIndex;
                m_discardMouseX = mp2.x; m_discardMouseY = mp2.y;
            }
            ImGui::PopID();
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();

    // --- 丢弃确认弹窗（Shift+右键触发）---
    if (m_discardPending >= 0 && m_discardPending < 20) {
        const auto& dm = m_items[m_discardPending].getMaterial();
        if (dm.id == Material::ID::Nothing) {
            m_discardPending = -1;
        } else {
            auto& io = ImGui::GetIO();

            char qbuf[128];
            snprintf(qbuf, sizeof(qbuf), "丢弃 %s x%d？",
                     getMaterialCNName(dm.id), m_items[m_discardPending].getNumInStack());

            // Measure text widths for precise button placement
            std::string btnLine = "[确认]        [取消]";
            m_discardText.setFontSize(18.0f);
            int tw = m_discardText.measureTextWidth(qbuf);
            int fullBtnW = m_discardText.measureTextWidth(btnLine);
            int yesW = m_discardText.measureTextWidth("[确认]");
            int noW  = m_discardText.measureTextWidth("[取消]");
            int maxW = std::max(tw, fullBtnW) + 12;
            maxW = std::max(maxW, 180);
            int lineH = (int)(18.0f * 1.1f);
            int texW = maxW;
            int texH = 8 + lineH * 2 + 4;

            std::vector<std::string> lines = { qbuf, btnLine };
            GLuint texId = m_discardText.update(lines, texW, texH, true);

            float popupW = (float)maxW + 16.0f;
            float popupH = (float)texH + 16.0f;
            float px = m_discardMouseX + 12.0f;
            float py = m_discardMouseY - popupH - 8.0f;
            if (px + popupW > io.DisplaySize.x) px = io.DisplaySize.x - popupW - 8.0f;
            if (py < 0.0f) py = m_discardMouseY + 16.0f;

            auto* fg = ImGui::GetForegroundDrawList();
            fg->AddRectFilled(ImVec2(px, py), ImVec2(px + popupW, py + popupH),
                              IM_COL32(20, 20, 20, 240));
            fg->AddRect(ImVec2(px, py), ImVec2(px + popupW, py + popupH),
                        IM_COL32(100, 100, 100, 255));
            if (texId)
                fg->AddImage((ImTextureID)(intptr_t)texId,
                             ImVec2(px + 8, py + 8),
                             ImVec2(px + 8 + texW, py + 8 + texH));

            // Button positions: text is centered in texW, line 2 at y = 4 + lineH within texture
            float line2X = px + 8.0f + (texW - fullBtnW) * 0.5f; // centered line start
            float line2Y = py + 8.0f + (float)lineH;
            float btnH = (float)lineH;

            float yesX = line2X;
            float noX  = line2X + (float)(fullBtnW - noW);

            ImVec2 mousePos = io.MousePos;
            bool hoverYes = mousePos.x >= yesX && mousePos.x <= yesX + yesW
                         && mousePos.y >= line2Y && mousePos.y <= line2Y + btnH;
            bool hoverNo  = mousePos.x >= noX && mousePos.x <= noX + noW
                         && mousePos.y >= line2Y && mousePos.y <= line2Y + btnH;

            if (hoverYes)
                fg->AddRectFilled(ImVec2(yesX, line2Y), ImVec2(yesX + yesW, line2Y + btnH),
                                  IM_COL32(80, 80, 80, 180));
            if (hoverNo)
                fg->AddRectFilled(ImVec2(noX, line2Y), ImVec2(noX + noW, line2Y + btnH),
                                  IM_COL32(80, 80, 80, 180));

            ImGui::SetNextWindowPos(ImVec2(px, py), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(popupW, popupH), ImGuiCond_Always);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
            int popupFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
                             ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground;
            if (ImGui::Begin("DiscardConfirm", nullptr, popupFlags)) {
                ImGui::SetCursorPos(ImVec2(yesX - px, line2Y - py));
                if (ImGui::InvisibleButton("DYes", ImVec2((float)yesW, btnH))) {
                    m_items[m_discardPending] = ItemStack(Material::NOTHING, 0);
                    m_discardPending = -1;
                }
                ImGui::SetCursorPos(ImVec2(noX - px, line2Y - py));
                if (ImGui::InvisibleButton("DNo", ImVec2((float)noW, btnH))) {
                    m_discardPending = -1;
                }
                if (ImGui::IsKeyPressed(ImGuiKey_Escape))
                    m_discardPending = -1;
            }
            ImGui::End();
            ImGui::PopStyleColor();

            // Cancel on left-click outside the popup
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                ImVec2 mp = io.MousePos;
                if (mp.x < px || mp.x > px + popupW || mp.y < py || mp.y > py + popupH)
                    m_discardPending = -1;
            }
        }
    }

    // --- Equipment window (2 slots, left of hotbar) ---
    {
        float eqSlots = 2;
        float eqContentW = eqSlots * (slotSize + padding) + padding;
        float eqContentH = 1 * (slotSize + padding) + padding;
        float eqY = hotbarY;
        float eqX = hotbarX - eqContentW - 6.0f;

        ImGui::SetNextWindowPos(ImVec2(eqX, eqY), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(eqContentW, eqContentH), ImGuiCond_Always);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        if (ImGui::Begin("Equipment", nullptr, winFlags))
        {
            auto* edl = ImGui::GetWindowDrawList();
            for (int i = 0; i < 2; i++)
            {
                float x = padding + i * (slotSize + padding);
                float y = padding;
                ImGui::SetCursorPos(ImVec2(x, y));

                ImGui::PushID(300 + i);
                ImVec2 p0 = ImGui::GetCursorScreenPos();
                ImGui::Dummy(ImVec2(slotSize, slotSize));
                ImVec2 p1(p0.x + slotSize, p0.y + slotSize);

                ImU32 bg = (m_equipSlot == i)
                    ? IM_COL32(255, 215, 0, 60) : IM_COL32(50, 50, 50, 200);
                edl->AddRectFilled(p0, p1, bg);
                edl->AddRect(p0, p1, IM_COL32(180, 180, 180, 255));

                const auto& eqMat = m_equipment[i].getMaterial();
                if (eqMat.id != Material::ID::Nothing)
                {
                    BlockId bId = eqMat.toBlockID();
                    const auto& bd = BlockDatabase::get().getData(bId);
                    auto uv = atlas.getTexture(bd.getBlockData().texTopCoord);
                    edl->AddImage((ImTextureID)(intptr_t)atlasID,
                        ImVec2(p0.x + texPad, p0.y + texPad),
                        ImVec2(p1.x - texPad, p1.y - texPad),
                        ImVec2(uv[2], uv[5]), ImVec2(uv[0], uv[3]));
                }

                // Drop target: drag from inventory
                if (ImGui::BeginDragDropTarget())
                {
                    const ImGuiPayload* pl = ImGui::AcceptDragDropPayload("INV_SLOT");
                    if (pl)
                    {
                        int src = *(const int*)pl->Data;
                        const auto& srcMat = m_items[src].getMaterial();
                        if (srcMat.id != Material::ID::Nothing)
                        {
                            const auto& oldMat = m_equipment[i].getMaterial();
                            if (oldMat.id != Material::ID::Nothing)
                                addItem(oldMat);
                            m_equipment[i] = ItemStack(srcMat, 1);
                            m_items[src].remove();
                        }
                    }
                    ImGui::EndDragDropTarget();
                }

                // Left-click → return to inventory, switch focus
                if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                {
                    const auto& eMat = m_equipment[i].getMaterial();
                    if (eMat.id != Material::ID::Nothing)
                    {
                        addItem(eMat);
                        m_equipment[i] = ItemStack(Material::NOTHING, 0);
                    }
                    m_equipSlot = i;
                }
                ImGui::PopID();
            }
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }

    // --- Crosshair + Mining Progress Ring ---
    if (m_mouseLocked && !m_backpackOpen)
    {
        ImVec2 center(displaySize.x * 0.5f, displaySize.y * 0.5f);
        auto* fg = ImGui::GetForegroundDrawList();

        fg->AddLine(ImVec2(center.x - 8, center.y), ImVec2(center.x + 8, center.y),
                    IM_COL32(255, 255, 255, 200), 1.5f);
        fg->AddLine(ImVec2(center.x, center.y - 8), ImVec2(center.x, center.y + 8),
                    IM_COL32(255, 255, 255, 200), 1.5f);

        if (m_isMining && m_miningProgress > 0.0f)
        {
            float r = 16.0f, pi = 3.14159265f;
            int segs = 36;
            float full = m_miningProgress * 2.0f * pi;
            float startAngle = -pi / 2.0f;
            ImVec2 prev(center.x + r * cosf(startAngle),
                        center.y + r * sinf(startAngle));
            for (int i = 1; i <= segs; i++)
            {
                float a = (float)i / (float)segs * full;
                ImVec2 pt(center.x + r * cosf(startAngle + a),
                          center.y + r * sinf(startAngle + a));
                fg->AddLine(prev, pt, IM_COL32(255, 255, 255, 220), 4.0f);
                prev = pt;
            }
        }

        // 食用进度环（长按右键吃野果）
        if (m_isEating && m_eatProgress > 0.0f)
        {
            float r = 16.0f, pi = 3.14159265f;
            int segs = 36;
            float full = m_eatProgress * 2.0f * pi;
            float startAngle = -pi / 2.0f;
            ImVec2 prev(center.x + r * cosf(startAngle),
                        center.y + r * sinf(startAngle));
            for (int i = 1; i <= segs; i++)
            {
                float a = (float)i / (float)segs * full;
                ImVec2 pt(center.x + r * cosf(startAngle + a),
                          center.y + r * sinf(startAngle + a));
                fg->AddLine(prev, pt, IM_COL32(100, 220, 100, 220), 4.0f);
                prev = pt;
            }
            // "正在食用" 提示词
            const char* eatText = "正在食用";
            m_buttonText.setFontSize(18.0f);
            int ew = m_buttonText.measureTextWidth(eatText) + 8;
            int eh = (int)(18.0f * 1.1f) + 4;
            GLuint etId = m_buttonText.update({eatText}, ew, eh, true);
            float textY = center.y + r + 6.0f;
            if (etId)
                fg->AddImage((ImTextureID)(intptr_t)etId,
                             ImVec2(center.x - ew * 0.5f, textY),
                             ImVec2(center.x + ew * 0.5f, textY + eh));
        }

        // Bow charge progress ring
        if (m_bowCharging && m_bowCharge > 0.0f) {
            float pi = 3.14159265f;
            int segs = 36;
            float full = m_bowCharge * 2.0f * pi;
            if (full > 2.0f * pi) full = 2.0f * pi;
            float startAngle = -pi / 2.0f;
            // Green when fully charged, yellow while charging
            ImU32 ringColor = (m_bowCharge >= 1.0f)
                ? IM_COL32(100, 255, 100, 240)
                : IM_COL32(255, 220, 100, 240);
            float r = 22.0f;
            ImVec2 prev(center.x + r * cosf(startAngle), center.y + r * sinf(startAngle));
            for (int i = 1; i <= segs; i++) {
                float a = (float)i / (float)segs * full;
                ImVec2 pt(center.x + r * cosf(startAngle + a), center.y + r * sinf(startAngle + a));
                fg->AddLine(prev, pt, ringColor, 3.0f);
                prev = pt;
            }
        }
    }

    // --- Extraction Countdown HUD ---
    if (m_isExtracting) {
        ImVec2 center(displaySize.x * 0.5f, displaySize.y * 0.5f);
        int secLeft = 5 - (int)std::floor(m_extractionProgress * 5.0f);
        if (secLeft < 1) secLeft = 1;
        char buf[32];
        snprintf(buf, sizeof(buf), "撤离中 %d 秒", secLeft);

        m_buttonText.setFontSize(24.0f);
        int cw = m_buttonText.measureTextWidth(buf) + 12;
        int ch = (int)(24.0f * 1.1f) + 4;
        GLuint ctId = m_buttonText.update({buf}, cw, ch, true);

        float textY = center.y + 30.0f;
        if (ctId)
            ImGui::GetForegroundDrawList()->AddImage(
                (ImTextureID)(intptr_t)ctId,
                ImVec2(center.x - cw * 0.5f, textY),
                ImVec2(center.x + cw * 0.5f, textY + ch));
    }

    // --- Drop item icon rendering via ImGui overlay ---
    if (m_pDropItems && !m_pDropItems->empty() && camera)
    {
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

            glm::vec4 clip = vp * glm::vec4(drop.position, 1.0f);
            if (clip.w <= 0.0f)
                continue;

            glm::vec3 ndc = glm::vec3(clip) / clip.w;
            if (ndc.x < -1.0f || ndc.x > 1.0f ||
                ndc.y < -1.0f || ndc.y > 1.0f)
                continue;

            float screenX = (ndc.x * 0.5f + 0.5f) * winW;
            float screenY = (1.0f - (ndc.y * 0.5f + 0.5f)) * winH;

            float dist = glm::distance(
                glm::vec3(camera->position), drop.position);
            float maxDist = 32.0f;
            if (dist > maxDist)
                continue;

            float scale = 1.0f - (dist / maxDist);
            float alpha = 0.4f + 0.6f * (scale * scale);
            float iconSize = 32.0f * (0.5f + 0.5f * scale);

            BlockId bId = drop.material->toBlockID();
            if (bId == BlockId::NUM_TYPES) continue; // skip non-block items (e.g. RawMeat)
            const auto& blockData = BlockDatabase::get().getData(bId);
            auto uv = atlas.getTexture(blockData.getBlockData().texTopCoord);

            ImVec2 uv0(uv[2], uv[5]);
            ImVec2 uv1(uv[0], uv[3]);
            ImVec2 p0(screenX - iconSize * 0.5f, screenY - iconSize * 0.5f);
            ImVec2 p1(screenX + iconSize * 0.5f, screenY + iconSize * 0.5f);

            ImU32 col = IM_COL32(
                (int)(255 * alpha), (int)(255 * alpha),
                (int)(255 * alpha), (int)(255 * alpha));

            float outlineOff = 2.0f;
            ImU32 outlineCol = IM_COL32(255, 255, 255, (int)(140 * alpha));
            ImVec2 os0(p0.x - outlineOff, p0.y - outlineOff);
            ImVec2 os1(p1.x + outlineOff, p1.y + outlineOff);
            ImGui::GetBackgroundDrawList()->AddImage(
                (ImTextureID)(intptr_t)atlasID, os0, os1, uv0, uv1, outlineCol);
            ImGui::GetBackgroundDrawList()->AddRect(os0, os1,
                IM_COL32(255, 255, 255, (int)(200 * alpha)), 0.0f, 0, 1.5f);

            ImGui::GetBackgroundDrawList()->AddImage(
                (ImTextureID)(intptr_t)atlasID, p0, p1, uv0, uv1, col);
        }
    }

    // --- HUD: Settlement Screen (full overlay, top-most) ---
    drawSettlement(camera);

    // --- Pause overlay ---
    if (m_paused) {
        auto* fg = ImGui::GetForegroundDrawList();
        auto sz = ImGui::GetIO().DisplaySize;
        fg->AddRectFilled(ImVec2(0, 0), ImVec2(sz.x, sz.y), IM_COL32(0, 0, 0, 120));
        std::string ps = "暂停中";
        m_pauseText.setFontSize(36.0f);
        int tw = m_pauseText.measureTextWidth(ps) + 16;
        int th = (int)(36.0f * 1.2f) + 4;
        GLuint tid = m_pauseText.update({ps}, tw, th, true, 255, 255, 255);
        if (tid)
            fg->AddImage((ImTextureID)(intptr_t)tid,
                ImVec2((sz.x - tw) * 0.5f, (sz.y - th) * 0.5f),
                ImVec2((sz.x + tw) * 0.5f, (sz.y + th) * 0.5f));
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

void Player::processRKey()
{
    if (!m_equipKey.isKeyPressed()) return;
    m_equipSlot = (m_equipSlot == 0) ? 1 : 0;
}

void Player::renderWeapon()
{
    const auto& eqM = m_equipment[m_equipSlot].getMaterial();
    if (eqM.id == Material::ID::Nothing) return;

    auto& atlas = BlockDatabase::get().textureAtlas;
    GLuint atlasID = atlas.getID();
    BlockId bId = eqM.toBlockID();
    auto uv = atlas.getTexture(BlockDatabase::get().getData(bId).getBlockData().texTopCoord);
    // 斧头：X轴翻转
    if (eqM.toolClass == 2) {
        std::swap(uv[0], uv[2]);
        std::swap(uv[4], uv[6]);
    }
    auto displaySize = ImGui::GetIO().DisplaySize;

    // Lazy init shader + VAO
    static GLuint wpProg = 0, wpVAO = 0, wpVBO = 0;
    if (wpProg == 0) {
        const char* vs = R"(#version 460 core
            layout(location=0) in vec2 aPos;
            layout(location=1) in vec2 aUV;
            uniform vec2 uC, uS, uR;
            out vec2 vUV;
            void main() {
                vec2 p = aPos * uS;
                vec2 r = vec2(p.x*uR.x - p.y*uR.y, p.x*uR.y + p.y*uR.x);
                gl_Position = vec4(uC + r, 0.0, 1.0);
                vUV = aUV;
            })";
        const char* fs = R"(#version 460 core
            in vec2 vUV;
            uniform sampler2D uTex;
            out vec4 oC;
            void main() { oC = texture(uTex, vUV); })";
        auto compileGL = [](GLenum t, const char* s) {
            GLuint id = glCreateShader(t); glShaderSource(id, 1, &s, 0); glCompileShader(id);
            GLint ok; glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
            if (!ok) { char buf[512]; glGetShaderInfoLog(id, 512, 0, buf);
                fprintf(stderr, "WP shader err: %s\n", buf); }
            return id;
        };
        GLuint sv = compileGL(GL_VERTEX_SHADER, vs);
        GLuint sf = compileGL(GL_FRAGMENT_SHADER, fs);
        wpProg = glCreateProgram(); glAttachShader(wpProg, sv); glAttachShader(wpProg, sf);
        glLinkProgram(wpProg);
        GLint ok; glGetProgramiv(wpProg, GL_LINK_STATUS, &ok);
        if (!ok) { char buf[512]; glGetProgramInfoLog(wpProg, 512, 0, buf);
            fprintf(stderr, "WP link err: %s\n", buf); }
        glDeleteShader(sv); glDeleteShader(sf);
        // Quad pivot at bottom-right: aPos range (x in [-2,0], y in [0,2])
        float q[] = {0,0, 0,0,  0,2, 0,1,  -2,0, 1,0,  0,2, 0,1,  -2,2, 1,1,  -2,0, 1,0};
        glGenVertexArrays(1, &wpVAO); glGenBuffers(1, &wpVBO);
        glBindVertexArray(wpVAO); glBindBuffer(GL_ARRAY_BUFFER, wpVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(q), q, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 16, (void*)0); glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 16, (void*)8); glEnableVertexAttribArray(1);
    }

    float angle = 0.0f, ox = 0.0f, oy = 0.0f;
    bool isBow = (eqM.id == Material::ID::Bow);
    if (m_isSwinging && !isBow) {
        float t = m_swingTimer / 0.3f; if (t > 1.0f) t = 1.0f;
        float s = sinf(t * 3.14159265f);
        angle = s * 25.0f; ox = -s * 90.0f; oy = s * 50.0f;
    }
    // Bow charge animation: pull toward bottom-right
    if (isBow && m_bowCharging) {
        float pull = m_bowCharge; if (pull > 1.0f) pull = 1.0f;
        ox =  pull * 40.0f;  // right
        oy =  pull * 30.0f;  // down
    }
    float ws = 600.0f;
    float wx = displaySize.x - ws + 90.0f + ox;
    float wy = displaySize.y - ws + 70.0f + oy;

    float hsw = displaySize.x * 0.5f, hsh = displaySize.y * 0.5f;
    // Pivot at bottom-right (handle)
    float cx = (wx + ws) / hsw - 1.0f;
    float cy = 1.0f - (wy + ws) / hsh;
    float sx = ws * 0.5f / hsw, sy = ws * 0.5f / hsh;
    // Bow charge shrink
    if (isBow && m_bowCharging) {
        float pull = m_bowCharge; if (pull > 1.0f) pull = 1.0f;
        sx *= (1.0f - pull * 0.15f);
        sy *= (1.0f - pull * 0.15f);
    }
    float rad = angle * 3.14159265f / 180.0f;

    GLint pp = 0, va = 0, tx = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &pp); glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &va);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &tx);

    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glUseProgram(wpProg);
    glUniform2f(glGetUniformLocation(wpProg, "uC"), cx, cy);
    glUniform2f(glGetUniformLocation(wpProg, "uS"), sx, sy);
    glUniform2f(glGetUniformLocation(wpProg, "uR"), cosf(rad), sinf(rad));
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, atlasID);
    glUniform1i(glGetUniformLocation(wpProg, "uTex"), 0);

    // UVs match tri order: pivot(BR), topR, botL, topR, topL, botL
    // X-flipped: bottom-right=(xMin,yMax), top-right=(xMin,yMin), bottom-left=(xMax,yMax), top-left=(xMax,yMin)
    // Vertices: pos(x,y) + uv(x,y) interleaved, 6 verts (24 floats)
    float vd[24];
    if (eqM.toolClass == 2 || isBow) {
        // 斧头/弓顺时针90°
        float a[] = {0,0, uv[0],uv[1], 0,2, uv[2],uv[3], -2,0, uv[6],uv[7],
                     0,2, uv[2],uv[3], -2,2, uv[4],uv[5], -2,0, uv[6],uv[7]};
        for (int i = 0; i < 24; ++i) vd[i] = a[i];
    } else {
        float a[] = {0,0, uv[2],uv[3], 0,2, uv[2],uv[5], -2,0, uv[0],uv[3],
                     0,2, uv[2],uv[5], -2,2, uv[0],uv[5], -2,0, uv[0],uv[3]};
        for (int i = 0; i < 24; ++i) vd[i] = a[i];
    }
    glBindVertexArray(wpVAO);
    glBindBuffer(GL_ARRAY_BUFFER, wpVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vd), vd);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(va); glUseProgram(pp); glBindTexture(GL_TEXTURE_2D, tx);
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}

int Player::getAttackPower() const
{
    const auto& eqM = m_equipment[m_equipSlot].getMaterial();
    return m_baseAttack + eqM.attackBonus;
}

void Player::updateEating(float dt, bool rightHeld)
{
    const auto& heldMat = m_items[m_heldItem].getMaterial();
    bool isFood = (heldMat.id == Material::ID::WildFruit || heldMat.id == Material::ID::CookedMeat);
    if (!isFood) {
        m_eatProgress = 0.0f;
        m_isEating = false;
        return;
    }
    float eatTime = (heldMat.id == Material::ID::CookedMeat) ? 1.5f : 1.0f;
    int healAmt  = (heldMat.id == Material::ID::CookedMeat) ? 5 : 1;
    if (rightHeld) {
        m_isEating = true;
        m_eatProgress += dt;
        if (m_eatProgress >= eatTime) {
            m_hp = std::min(m_hp + healAmt, m_maxHp);
            m_items[m_heldItem].remove();
            m_eatProgress = 0.0f;
            m_isEating = false;
        }
    } else {
        m_eatProgress = 0.0f;
        m_isEating = false;
    }
}

void Player::takeDamage(int amount, glm::vec3 knockbackDir)
{
    if (m_isDead) return;

    m_hp -= amount;
    if (m_hp < 0) m_hp = 0;

    // Knockback
    velocity.x += knockbackDir.x * 6.0f;
    velocity.z += knockbackDir.z * 6.0f;
    if (!m_isFlying) velocity.y += 4.0f;

    if (m_hp <= 0) {
        m_isDead = true;
        // Clear all except last 3 backpack slots (17, 18, 19)
        for (int i = 0; i < 17; i++) {
            m_items[i] = ItemStack(Material::NOTHING, 0);
        }
        for (int i = 0; i < 2; i++) {
            m_equipment[i] = ItemStack(Material::NOTHING, 0);
        }
        std::cout << "Player died! HP=0\n";
    }
}
