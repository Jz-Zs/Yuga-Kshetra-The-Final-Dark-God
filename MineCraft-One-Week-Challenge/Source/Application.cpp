#include "Application.h"

#include <SFML/Window/Event.hpp>
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

                    if (id != BlockId::Air && id != BlockId::Water)
                    {
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
            m_player.m_miningProgress += delta / 0.3f;
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
                    if (id != BlockId::Air && id != BlockId::Water)
                    {
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

        // Right-click: place block
        if (rightPressed && m_rightClickTimer.getElapsedTime().asSeconds() > 0.2f)
        {
            for (Ray ray({m_player.position.x, m_player.position.y + 0.6f, m_player.position.z},
                         m_player.rotation);
                 ray.getLength() < 6; ray.step(0.05f))
            {
                int x = static_cast<int>(ray.getEnd().x);
                int y = static_cast<int>(ray.getEnd().y);
                int z = static_cast<int>(ray.getEnd().z);
                auto block = m_world.getBlock(x, y, z);
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

    // Player death handling
    if (m_player.m_isDead && m_player.m_hp <= 0) {
        m_player.m_hp = 100;
        m_player.m_isDead = false;
        m_player.position = {64.0f, 35.0f, 64.0f};
        m_player.velocity = {0, 0, 0};
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
