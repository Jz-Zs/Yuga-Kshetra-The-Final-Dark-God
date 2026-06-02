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
    m_items[0] = ItemStack(Material::WOODEN_SWORD, 1);  // 开局测试用
    m_equipment[0] = ItemStack(Material::WOODEN_SWORD, 1);  // 主手装备
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
                m_roundCollection[material.id]++;
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
            m_heldItem = m_items.size() - 1;
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
    GLuint texId = m_hudText.update(lines, texW, texH, true);

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

void Player::drawSettlement(const Camera* camera)
{
    // Trigger: round ended OR player dead
    bool shouldShow = (!m_roundActive && m_roundNumber > 0) || m_isDead;
    if (!shouldShow) return;

    auto displaySize = ImGui::GetIO().DisplaySize;

    // ================================================================
    // Chinese name mapping for Material IDs
    // ================================================================
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
            case Material::ID::Stick:      return "木棍";
            case Material::ID::WoodenSword:return "木剑";
            case Material::ID::RawMeat:    return "生肉";
            case Material::ID::GoldBlock:  return "金块";
            default: return "未知";
        }
    };

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
            snprintf(buf, sizeof(buf), "  %s  x%d", cnName(matId).c_str(), count);
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

void Player::draw(RenderMaster& master, const Camera* camera)
{
    // --- Chinese material names (kept for future text-based UI) ---
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

    // --- Backpack + Crafting (B key) ---
    if (m_backpackOpen)
    {
        const int craftRows = 3, craftCols = 3;
        float craftContentW = craftCols * (slotSize + padding) + padding + slotSize + padding + 56.0f;
        float craftContentH = craftRows * (slotSize + padding) + padding;
        const int bpRows = 3;
        float bpContentH = bpRows * (slotSize + padding) + padding;

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

            // 3×3 grid
            for (int row = 0; row < craftRows; row++)
                for (int col = 0; col < craftCols; col++)
                {
                    int slotIdx = row * craftCols + col;
                    float x = padding + col * (slotSize + padding);
                    float y = padding + row * (slotSize + padding);
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
                        int n = m_craftGrid[slotIdx].getNumInStack();
                        if (n > 1) {
                            char buf[8];
                            snprintf(buf, 8, "%d", n);
                            wdl->AddText(ImVec2(p1.x - 16, p1.y - 16), IM_COL32(255,255,255,255), buf);
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
                        if (ImGui::GetIO().KeyShift) {
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
                float ay = winPos.y + padding + 1 * (slotSize + padding) + slotSize * 0.5f;
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
                float ry = padding + 1 * (slotSize + padding);
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
                }
                if (m_currentRecipe && ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    const Material& outMat = *m_currentRecipe->output;
                    if (addItem(outMat)) {
                        for (int i = 0; i < 9; i++)
                            if (m_currentRecipe->pattern[i] != nullptr) m_craftGrid[i].remove();
                        m_currentRecipe = findMatchingRecipe(m_craftGrid);
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
            for (int bpRow = 0; bpRow < bpRows; bpRow++)
                for (int col = 0; col < cols; col++)
                {
                    int slotIndex = 5 + bpRow * cols + col;
                    float x = padding + col * (slotSize + padding);
                    float y = padding + bpRow * (slotSize + padding);
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
            ImGui::PopID();
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();

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
    }

    // --- Extraction Countdown HUD ---
    if (m_isExtracting) {
        ImVec2 center(displaySize.x * 0.5f, displaySize.y * 0.5f);
        auto* fg = ImGui::GetForegroundDrawList();
        int secLeft = 5 - (int)m_extractionProgress;
        if (secLeft < 0) secLeft = 0;
        char buf[32];
        snprintf(buf, sizeof(buf), "撤离中 %d 秒", secLeft + 1);
        float textY = center.y + 30.0f;
        ImVec2 textSize = ImGui::CalcTextSize(buf);
        fg->AddText(ImVec2(center.x - textSize.x * 0.5f, textY),
                    IM_COL32(255, 255, 100, 255), buf);
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
    if (m_isSwinging) {
        float t = m_swingTimer / 0.3f; if (t > 1.0f) t = 1.0f;
        float s = sinf(t * 3.14159265f);
        angle = s * 25.0f; ox = -s * 90.0f; oy = s * 50.0f;
    }
    float ws = 600.0f;
    float wx = displaySize.x - ws + 90.0f + ox;
    float wy = displaySize.y - ws + 70.0f + oy;

    float hsw = displaySize.x * 0.5f, hsh = displaySize.y * 0.5f;
    // Pivot at bottom-right (handle)
    float cx = (wx + ws) / hsw - 1.0f;
    float cy = 1.0f - (wy + ws) / hsh;
    float sx = ws * 0.5f / hsw, sy = ws * 0.5f / hsh;
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
    // Vertices: pos(x,y) + uv(x,y) interleaved, 6 verts
    float vd[] = {
        0,0, uv[2],uv[3],  0,2, uv[2],uv[5],  -2,0, uv[0],uv[3],
        0,2, uv[2],uv[5],  -2,2, uv[0],uv[5],  -2,0, uv[0],uv[3]};
    glBindVertexArray(wpVAO);
    glBindBuffer(GL_ARRAY_BUFFER, wpVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vd), vd);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(va); glUseProgram(pp); glBindTexture(GL_TEXTURE_2D, tx);
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}

int Player::getAttackPower() const
{
    int weaponAtk = 0;
    const auto& eqM = m_equipment[m_equipSlot].getMaterial();
    if (eqM.id == Material::ID::WoodenSword) {
        weaponAtk = 10;
    }
    return m_baseAttack + weaponAtk;
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
