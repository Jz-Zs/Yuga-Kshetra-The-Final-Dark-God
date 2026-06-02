#include "Application.h"

#include <SFML/Window/Event.hpp>
#include <cmath>
#include <imgui.h>
#include <iostream>

#include "Entity/PigmanEntity.h"
#include "Maths/Ray.h"
#include "Renderer/RenderMaster.h"
#include "World/Block/BlockDatabase.h"
#include "World/Event/PlayerDigEvent.h"
#include "World/WorldConstants.h"
#include "Item/CraftingRecipe.h"
float g_timeElapsed = 0;

Application::Application(sf::Window& window, const Config& config)
    : m_window(window)
    , m_camera(config)
    , m_config(config)
    , m_world(m_camera, config, m_player)

{
    BlockDatabase::get();
    initCraftingRecipes();
    m_camera.hookEntity(m_player);

    if (!m_music.openFromFile("Res/Musics/Minecraft-C418.mp3"))
        std::cerr << "Failed to load music\n";
    m_music.setLooping(true);

    // Auto-start first round
    m_player.m_roundActive = true;
    m_player.m_roundTimeLeft = 600.0f;
}

void Application::on_event(const sf::Event& event)
{
}

void Application::on_update(const Keyboard& keyboard, sf::Time dt)
{
    // Music: start on first frame, toggle with M
    if (!m_musicStarted) {
        m_music.play();
        m_musicStarted = true;
    }
    if (m_musicKey.isKeyPressed()) {
        if (m_music.getStatus() == sf::Music::Status::Playing)
            m_music.pause();
        else
            m_music.play();
    }

    float delta = dt.asSeconds();
    m_player.handleInput(m_window, keyboard);
    glm::vec3 lastPosition;

    bool leftPressed = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
    bool leftClicked = !m_prevLeftPressed && leftPressed;
    m_prevLeftPressed = leftPressed;
    bool rightPressed = sf::Mouse::isButtonPressed(sf::Mouse::Button::Right);

    // Skip interaction when backpack is open
    if (!sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LAlt) && !m_player.isBackpackOpen())
    {
        // --- Left-click: swing animation for any click ---
        if (leftClicked && !m_player.isBackpackOpen())
            m_player.triggerSwing();

        // --- Left-click: pigman attack first, then mining ---
        if (leftClicked && !m_player.m_isDead)
        {
            bool hitPigman = false;

            // Raycast for pigman
            Ray ray({m_player.position.x, m_player.position.y + 0.6f, m_player.position.z},
                     m_player.rotation);
            for (; ray.getLength() < 6; ray.step(0.05f))
            {
                // Check pigman first
                for (auto& e : m_world.getPigmen())
                {
                    if (e.state == PigmanEntity::Dead) continue;

                    glm::vec3 eMin = e.box.position - e.box.dimensions;
                    glm::vec3 eMax = e.box.position + e.box.dimensions;
                    glm::vec3 rp = ray.getEnd();

                    if (rp.x >= eMin.x && rp.x <= eMax.x &&
                        rp.y >= eMin.y && rp.y <= eMax.y &&
                        rp.z >= eMin.z && rp.z <= eMax.z)
                    {
                        // Hit!
                        int dmg = m_player.getAttackPower();
                        e.hp -= dmg;

                        // Knockback
                        glm::vec3 kb = glm::normalize(e.position - m_player.position);
                        kb.y = 0;
                        if (glm::length(kb) < 0.01f) kb = glm::vec3(0, 0, -1);
                        e.velocity.x += kb.x * 6.0f;
                        e.velocity.z += kb.z * 6.0f;

                        if (e.hp <= 0) {
                            e.hp = 0;
                            e.state = PigmanEntity::Dead;
                            e.deathAnimTimer = 0.0f;
                            e.respawnTimer = 10.0f + (float)(std::rand() % 11);

                            // Drops: 5x independent rolls
                            glm::vec3 dropPos(e.position.x, e.position.y + 0.75f, e.position.z);
                            auto pushDrop = [&](const Material& mat) {
                                ItemDropEntity d;
                                d.position = dropPos;
                                d.velocity = glm::vec3(0.0f);
                                d.material = &mat;
                                d.alive = true;
                                m_world.getDropItems().push_back(d);
                            };
                            for (int r = 0; r < 5; r++) {
                                if (std::rand() % 100 < 40)
                                    pushDrop(Material::RAW_MEAT);
                                if (std::rand() % 100 < 30)
                                    pushDrop(Material::STICK);
                            }
                        } else {
                            e.state = PigmanEntity::Hurt;
                            e.hurtTimer = 0.3f;
                        }

                        hitPigman = true;
                        break;
                    }
                }
                if (hitPigman) break;
            }

            // If no pigman hit, try mining — start/reacquire target
            if (!hitPigman)
            {
                for (Ray ray2({m_player.position.x, m_player.position.y + 0.6f, m_player.position.z},
                              m_player.rotation);
                     ray2.getLength() < 6; ray2.step(0.05f))
                {
                    int x = static_cast<int>(ray2.getEnd().x);
                    int y = static_cast<int>(ray2.getEnd().y);
                    int z = static_cast<int>(ray2.getEnd().z);

                    auto block = m_world.getBlock(x, y, z);
                    auto id = (BlockId)block.id;
                    auto& data = block.getData();

                    // Unbreakable blocks stop the ray (barrier)
                    if (data.requiredToolLevel == 255) {
                        break;
                    }

                    if (id != BlockId::Air && id != BlockId::Water)
                    {
                        // Check tool tier requirement
                        const auto& eqM = m_player.m_equipment[m_player.m_equipSlot].getMaterial();
                        if (data.requiredToolLevel > 0 && eqM.toolTier < data.requiredToolLevel)
                        {
                            break; // Insufficient tool — ray blocked
                        }

                        if (!m_player.m_isMining
                            || m_player.m_miningTarget.x != x
                            || m_player.m_miningTarget.y != y
                            || m_player.m_miningTarget.z != z)
                        {
                            m_player.m_isMining = true;
                            m_player.m_miningProgress = 0.0f;
                            m_player.m_miningTarget = {x, y, z};
                        }
                        break;
                    }
                }
            }
        }

        // Mining progress — only while holding left button
        if (leftPressed && m_player.m_isMining && m_player.m_miningProgress < 1.0f)
        {
            // Get target block properties for mining speed
            auto targetBlock = m_world.getBlock(
                m_player.m_miningTarget.x,
                m_player.m_miningTarget.y,
                m_player.m_miningTarget.z);
            auto& targetData = targetBlock.getData();

            // Calculate mining speed based on tool match
            const auto& eqM = m_player.m_equipment[m_player.m_equipSlot].getMaterial();
            float multiplier = 1.0f;
            // Tool bonus only applies when block requires a specific tool class AND equipped tool matches
            if (targetData.requiredToolClass != 0
                && eqM.toolClass == targetData.requiredToolClass)
            {
                multiplier = eqM.miningMultiplier;
            }
            float digTime = targetData.hardness * 0.3f / multiplier;
            m_player.m_miningProgress += delta / digTime;


            if (m_player.m_miningProgress >= 1.0f)
            {
                // Dig current block
                m_world.addEvent<PlayerDigEvent>(sf::Mouse::Button::Left,
                    glm::vec3(m_player.m_miningTarget.x + 0.5f,
                              m_player.m_miningTarget.y + 0.5f,
                              m_player.m_miningTarget.z + 0.5f), m_player);
                // Continuous mining: raycast for next block, skip the one just dug
                glm::ivec3 prevTarget = m_player.m_miningTarget;
                m_player.m_miningProgress = 0.0f;
                m_player.m_isMining = false;
                m_player.m_miningTarget = {0, -999, 0};
                for (Ray ray({m_player.position.x, m_player.position.y + 0.6f, m_player.position.z},
                             m_player.rotation);
                     ray.getLength() < 6; ray.step(0.05f))
                {
                    int x = static_cast<int>(ray.getEnd().x);
                    int y = static_cast<int>(ray.getEnd().y);
                    int z = static_cast<int>(ray.getEnd().z);
                    if (x == prevTarget.x && y == prevTarget.y && z == prevTarget.z) continue;
                    auto block = m_world.getBlock(x, y, z);
                    auto id = (BlockId)block.id;
                    auto& data = block.getData();

                    // Unbreakable blocks stop the ray (barrier)
                    if (data.requiredToolLevel == 255) {
                        break;
                    }

                    if (id != BlockId::Air && id != BlockId::Water)
                    {
                        // Check tool tier requirement
                        const auto& eqM = m_player.m_equipment[m_player.m_equipSlot].getMaterial();
                        if (data.requiredToolLevel > 0 && eqM.toolTier < data.requiredToolLevel)
                        {
                            break; // Insufficient tool — ray blocked
                        }

                        m_player.m_isMining = true;
                        m_player.m_miningTarget = {x, y, z};
                        break;
                    }
                }
            }
        }
        else if (!leftPressed && m_player.m_isMining)
        {
            // Button released — cancel mining
            m_player.m_isMining = false;
            m_player.m_miningProgress = 0.0f;
            m_player.m_miningTarget = {0, -999, 0};
        }

        // Eating: update progress when holding right-click on food
        m_player.updateEating(delta, rightPressed);

        // Right-click: place block (skip if eating)
        if (rightPressed && m_rightClickTimer.getElapsedTime().asSeconds() > 0.2f && !m_player.isEating())
        {
            for (Ray ray({m_player.position.x, m_player.position.y + 0.6f, m_player.position.z},
                         m_player.rotation);
                 ray.getLength() < 6; ray.step(0.05f))
            {
                int x = static_cast<int>(ray.getEnd().x);
                int y = static_cast<int>(ray.getEnd().y);
                int z = static_cast<int>(ray.getEnd().z);
                auto block = m_world.getBlock(x, y, z);
                auto& data = block.getData();
                // Unbreakable blocks stop placement ray
                if (data.requiredToolLevel == 255) {
                    break;
                }
                if (block.id != 0 && block.id != (int)BlockId::Water)
                {
                    m_rightClickTimer.restart();
                    m_world.addEvent<PlayerDigEvent>(sf::Mouse::Button::Right, lastPosition, m_player);
                    break;
                }
                lastPosition = ray.getEnd();
            }
        }
    }

    m_camera.update();
    m_player.update(delta, m_world);

    // Update entities (pigman AI + physics)
    m_world.updateEntities(delta, m_player);

    m_world.update(m_camera, delta);

    // Extraction point detection
    if (m_player.m_roundActive && m_world.isExtractionActive()) {
        int px = (int)std::floor(m_player.position.x);
        int py = (int)std::floor(m_player.position.y - 0.01f); // block at feet
        int pz = (int)std::floor(m_player.position.z);

        // Check if standing on any GoldBlock in the 3x3 platform
        bool onGold = false;
        auto ec = m_world.getExtractionCenter();
        for (int dx = -1; dx <= 1; dx++) {
            for (int dz = -1; dz <= 1; dz++) {
                if (px == ec.x + dx && pz == ec.z + dz && py == ec.y + 1) {
                    auto footBlock = m_world.getBlock(px, py - 1, pz);
                    if (footBlock.id == (int)BlockId::GoldBlock) {
                        onGold = true;
                    }
                }
            }
        }

        if (onGold) {
            if (!m_player.m_isExtracting) {
                m_player.m_isExtracting = true;
                m_player.m_extractionProgress = 0.0f;
            }
            m_player.m_extractionProgress += delta / 5.0f;
            if (m_player.m_extractionProgress >= 1.0f) {
                // Extraction success
                m_player.m_roundActive = false;
                m_player.m_isExtracting = false;
                m_player.m_extractionProgress = 0.0f;
                m_player.m_roundNumber++;
            }
        } else {
            m_player.m_isExtracting = false;
            m_player.m_extractionProgress = 0.0f;
        }
    }

    // Round timer logic
    if (m_player.m_roundActive) {
        m_player.m_roundTimeLeft -= delta;
        if (m_player.m_roundTimeLeft <= 0.0f) {
            m_player.m_roundTimeLeft = 0.0f;
            m_player.m_roundActive = false;
            m_player.m_settlementOutcome = Player::SettlementOutcome::TimeUp;
            m_player.clearInventory();
        }
        // Spawn extraction point at round start (was 300s for 5-min delay)
        if (m_player.m_roundTimeLeft <= 600.0f && !m_extractionSpawned && !m_world.isExtractionActive()) {
            m_world.placeExtractionPoint();
            m_extractionSpawned = true;
        }
    }

    // Player death — trigger settlement
    if (m_player.m_isDead && m_player.m_hp <= 0) {
        m_player.m_roundActive = false;
        m_player.m_settlementOutcome = Player::SettlementOutcome::Death;
        m_player.clearInventory();
        m_player.m_hp = 0;
    }

    // Settlement "准备下一回合" button callback
    if (m_player.m_requestNewRound) {
        m_player.m_requestNewRound = false;
        m_player.m_isDead = false;
        m_player.m_hp = m_player.m_maxHp;
        m_player.m_extractionProgress = 0.0f;
        m_player.m_isExtracting = false;
        m_player.m_pigmanKills = 0;
        m_player.m_roundCollection.clear();
        m_player.m_roundTimeLeft = 600.0f;
        m_player.m_roundActive = true;
        m_extractionSpawned = false;
        m_world.resetWorld(m_camera, m_player);
    }

    // Auto-pickup
    {
        auto& drops = m_world.getDropItems();
        for (auto& drop : drops)
        {
            if (!drop.alive) continue;
            if (glm::distance(m_player.position, drop.position) < 2.0f)
            {
                if (m_player.addItem(*drop.material))
                {
                    drop.alive = false;
                }
            }
        }
    }

    // Clamp player to MVP world bounds
    if (m_player.position.x < 0) { m_player.position.x = 0; m_player.velocity.x = 0; }
    if (m_player.position.x >= MVP_WORLD_SIZE_X - 1) { m_player.position.x = static_cast<float>(MVP_WORLD_SIZE_X - 1); m_player.velocity.x = 0; }
    if (m_player.position.z < 0) { m_player.position.z = 0; m_player.velocity.z = 0; }
    if (m_player.position.z >= MVP_WORLD_SIZE_Z - 1) { m_player.position.z = static_cast<float>(MVP_WORLD_SIZE_Z - 1); m_player.velocity.z = 0; }
    if (m_player.position.y < 0) { m_player.position.y = 1; }
}

void Application::on_render(bool show_debug_info)
{
    m_player.setDropItems(&m_world.getDropItems());
    m_player.draw(m_masterRenderer, &m_camera);

    m_world.renderWorld(m_masterRenderer, m_camera);

    // Add entities to renderer
    for (auto& e : m_world.getPigmen()) {
        m_masterRenderer.m_entityRenderer.addEntity(e);
    }

    m_masterRenderer.finishRender(m_window, m_camera);

    m_player.renderWeapon();
}
