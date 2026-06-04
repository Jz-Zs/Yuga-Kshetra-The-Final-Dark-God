#include "Application.h"

#include <SFML/Window/Event.hpp>
#include <cmath>
#include <imgui.h>
#include <iostream>

#include "Entity/PigmanEntity.h"
#include "Entity/SpiderEntity.h"
#include "Entity/ArrowEntity.h"
#include "Maths/Ray.h"
#include "Renderer/RenderMaster.h"
#include "World/Block/BlockDatabase.h"
#include "World/Event/PlayerDigEvent.h"
#include "World/WorldConstants.h"
#include "Item/CraftingRecipe.h"
#include "Item/SmeltingRecipe.h"
float g_timeElapsed = 0;

Application::Application(sf::Window& window, const Config& config)
    : m_window(window)
    , m_camera(config)
    , m_config(config)
    , m_world(m_camera, config, m_player)

{
    BlockDatabase::get();
    initCraftingRecipes();
    initSmeltingRecipes();
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
    if (const auto* resized = event.getIf<sf::Event::Resized>()) {
        m_camera.updateProjection(resized->size.x, resized->size.y);
    }
}

void Application::on_update(const Keyboard& keyboard, sf::Time dt)
{
    float delta = dt.asSeconds();

    // Pause toggle
    if (m_pauseKey.isKeyPressed()) {
        m_paused = !m_paused;
        if (m_paused) {
            m_music.pause();
            m_window.setMouseCursorGrabbed(false);
        } else {
            if (m_music.getStatus() == sf::Music::Status::Paused)
                m_music.play();
        }
    }
    if (m_paused) return;

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

    m_player.handleInput(m_window, keyboard);
    bool leftPressed = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
    bool leftClicked = !m_prevLeftPressed && leftPressed;
    m_prevLeftPressed = leftPressed;
    bool rightPressed = sf::Mouse::isButtonPressed(sf::Mouse::Button::Right);

    // Skip interaction when backpack is open
    if (!sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LAlt) && !m_player.isUIOpen())
    {
        // --- Left-click: swing animation for any click ---
        if (leftClicked && !m_player.isUIOpen())
            m_player.triggerSwing();

        // --- Bow input (LeftClick hold to charge, release to fire) ---
        const auto& bowEq = m_player.m_equipment[m_player.m_equipSlot].getMaterial();
        bool bowEquipped = (bowEq.id == Material::ID::Bow);

        if (bowEquipped) {
            if (leftPressed && !m_player.m_bowCharging) {
                m_player.m_bowCharging = true;
                m_player.m_bowCharge = 0.0f;
            }
            if (m_player.m_bowCharging && leftPressed) {
                m_player.m_bowCharge += delta;
                if (m_player.m_bowCharge >= 1.0f)
                    m_player.m_bowCharge = 1.0f;
            }
            if (m_player.m_bowCharging && !leftPressed) {
                if (m_player.m_bowCharge >= 1.0f) {
                    // Find arrow: hotbar 0→4, then backpack 0→19
                    int arrowSlot = -1;
                    const Material* arrowMat = nullptr;
                    for (int i = 0; i < 20; i++) {
                        auto id = m_player.m_items[i].getMaterial().id;
                        if (id == Material::ID::IronArrow || id == Material::ID::StoneArrow || id == Material::ID::SpiderSilkArrow) {
                            arrowSlot = i;
                            arrowMat = &m_player.m_items[i].getMaterial();
                            break;
                        }
                    }
                    if (arrowSlot >= 0 && arrowMat) {
                        ArrowEntity arrow;
                        arrow.position = glm::vec3(m_player.position.x, m_player.position.y + 0.6f, m_player.position.z);
                        float yaw = glm::radians(m_player.rotation.y + 90);
                        float pitch = glm::radians(m_player.rotation.x);
                        float dx = -glm::cos(yaw);
                        float dy = -glm::tan(pitch);
                        float dz = -glm::sin(yaw);
                        float len = std::sqrt(dx*dx + dy*dy + dz*dz);
                        arrow.velocity.x = dx / len * 10.0f;
                        arrow.velocity.y = dy / len * 10.0f;
                        arrow.velocity.z = dz / len * 10.0f;
                        arrow.damage = arrowMat->attackBonus;
                        arrow.isSilkArrow = (arrowMat->id == Material::ID::SpiderSilkArrow);
                        m_world.getArrows().push_back(arrow);
                        m_player.m_items[arrowSlot].remove();
                    }
                }
                m_player.m_bowCharge = 0.0f;
                m_player.m_bowCharging = false;
            }
        }

        // --- Left-click: pigman attack first, then mining ---
        bool bowBlocksCombat = (bowEquipped && m_player.m_bowCharging);
        bool hitPigman = false;
        bool hitSpider = false;

        if (!bowBlocksCombat && leftClicked && !m_player.m_isDead)
        {

            // Raycast for entities (inline point-sampling, no Ray dependency)
            glm::vec3 eyePos(m_player.position.x, m_player.position.y + 0.6f, m_player.position.z);
            float yr = glm::radians(m_player.rotation.y);
            float pr = glm::radians(m_player.rotation.x);
            float cp = glm::cos(pr);
            glm::vec3 dir(
                 glm::sin(yr) * cp,
                -glm::sin(pr),
                -glm::cos(yr) * cp
            );
            for (float dist = 0.0f; dist < 5.0f; dist += 0.05f)
            {
                glm::vec3 rp = eyePos + dir * dist;
                // Check pigman
                for (auto& e : m_world.getPigmen())
                {
                    if (e.state == PigmanEntity::Dead) continue;

                    glm::vec3 eMin = e.box.position - e.box.dimensions;
                    glm::vec3 eMax = e.box.position + e.box.dimensions;
                    if (rp.x >= eMin.x && rp.x <= eMax.x &&
                        rp.y >= eMin.y && rp.y <= eMax.y &&
                        rp.z >= eMin.z && rp.z <= eMax.z)
                    {
                        int dmg = m_player.getAttackPower();
                        e.hp -= dmg;

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

                // Check spider
                for (auto& e : m_world.getSpiders())
                {
                    if (e.state == SpiderEntity::Dead) continue;

                    glm::vec3 eMin = e.box.position - e.box.dimensions;
                    glm::vec3 eMax = e.box.position + e.box.dimensions;
                    if (rp.x >= eMin.x && rp.x <= eMax.x &&
                        rp.y >= eMin.y && rp.y <= eMax.y &&
                        rp.z >= eMin.z && rp.z <= eMax.z)
                    {
                        int dmg = m_player.getAttackPower();
                        e.hp -= dmg;

                        glm::vec3 kb = glm::normalize(e.position - m_player.position);
                        kb.y = 0;
                        if (glm::length(kb) < 0.01f) kb = glm::vec3(0, 0, -1);
                        e.velocity.x += kb.x * 6.0f;
                        e.velocity.z += kb.z * 6.0f;

                        if (e.hp <= 0) {
                            e.hp = 0;
                            e.state = SpiderEntity::Dead;
                            e.deathAnimTimer = 0.0f;
                            e.respawnTimer = 30.0f + (float)(std::rand() % 21);

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
                                if (std::rand() % 100 < 35) pushDrop(Material::RAW_MEAT);
                                if (std::rand() % 100 < 50) pushDrop(Material::SILK);
                                if (std::rand() % 100 < 20) pushDrop(Material::STICK);
                                if (std::rand() % 100 < 15) pushDrop(Material::SILK_THREAD);
                            }
                            m_player.m_pigmanKills++;
                        } else {
                            e.state = SpiderEntity::Hurt;
                            e.hurtTimer = 0.3f;
                        }

                        hitSpider = true;
                        break;
                    }
                }

                if (hitPigman || hitSpider) break;
            }

        }

        // --- Mining target selection: DDA raycast every frame while leftPressed ---
        // (skip if we just hit an entity on this click frame)
        bool entityBlocked = leftClicked && (hitPigman || hitSpider);
        glm::vec3 eyePos(m_player.position.x, m_player.position.y + 0.6f, m_player.position.z);
        if (leftPressed && !entityBlocked && !m_player.m_isDead)
        {
            Ray ray(eyePos, m_player.rotation);
            glm::ivec3 foundTarget{0, -999, 0};
            bool found = false;
            while (ray.getLength() < 5.0f) {
                if (!ray.advance()) break;
                auto voxel = ray.currentVoxel();
                auto block = m_world.getBlock(voxel.x, voxel.y, voxel.z);
                auto& data = block.getData();

                if (data.requiredToolLevel == 255)
                    break; // unbreakable barrier

                if (block.id != 0 && block.id != (int)BlockId::Water) {
                    const auto& eqMBlock = m_player.m_equipment[m_player.m_equipSlot].getMaterial();
                    if (data.requiredToolLevel > 0 && eqMBlock.toolTier < data.requiredToolLevel)
                        break; // insufficient tool
                    foundTarget = voxel;
                    found = true;
                    break;
                }
            }

            if (found) {
                if (!m_player.m_isMining
                    || m_player.m_miningTarget.x != foundTarget.x
                    || m_player.m_miningTarget.y != foundTarget.y
                    || m_player.m_miningTarget.z != foundTarget.z)
                {
                    // Crosshair moved to a different block — reset progress
                    m_player.m_miningProgress = 0.0f;
                    m_player.m_miningTarget = foundTarget;
                }
                m_player.m_isMining = true;
            } else {
                // No block under crosshair — cancel mining
                m_player.m_isMining = false;
                m_player.m_miningProgress = 0.0f;
                m_player.m_miningTarget = {0, -999, 0};
            }
        }
        else if (!leftPressed && m_player.m_isMining)
        {
            // Button released — cancel mining
            m_player.m_isMining = false;
            m_player.m_miningProgress = 0.0f;
            m_player.m_miningTarget = {0, -999, 0};
        }

        // --- Mining progress accumulation ---
        if (leftPressed && m_player.m_isMining && m_player.m_miningProgress < 1.0f)
        {
            auto targetBlock = m_world.getBlock(
                m_player.m_miningTarget.x,
                m_player.m_miningTarget.y,
                m_player.m_miningTarget.z);
            auto& targetData = targetBlock.getData();

            const auto& eqM = m_player.m_equipment[m_player.m_equipSlot].getMaterial();
            float multiplier = 1.0f;
            if (targetData.requiredToolClass != 0
                && eqM.toolClass == targetData.requiredToolClass)
            {
                multiplier = eqM.miningMultiplier;
            }
            float digTime = targetData.hardness * 2.0f / multiplier;
            m_player.m_miningProgress += delta / digTime;

            if (m_player.m_miningProgress >= 1.0f)
            {
                m_world.addEvent<PlayerDigEvent>(sf::Mouse::Button::Left,
                    glm::vec3(m_player.m_miningTarget.x + 0.5f,
                              m_player.m_miningTarget.y + 0.5f,
                              m_player.m_miningTarget.z + 0.5f), m_player);

                // Continuous mining: DDA raycast to find next block behind the dug one
                glm::ivec3 prevTarget = m_player.m_miningTarget;
                m_player.m_miningProgress = 0.0f;
                m_player.m_isMining = false;
                m_player.m_miningTarget = {0, -999, 0};

                Ray ray(eyePos, m_player.rotation);
                while (ray.getLength() < 5.0f) {
                    if (!ray.advance()) break;
                    auto voxel = ray.currentVoxel();
                    if (voxel.x == prevTarget.x && voxel.y == prevTarget.y && voxel.z == prevTarget.z)
                        continue;
                    auto block = m_world.getBlock(voxel.x, voxel.y, voxel.z);
                    auto& data = block.getData();

                    if (data.requiredToolLevel == 255)
                        break;

                    if (block.id != 0 && block.id != (int)BlockId::Water) {
                        const auto& eqMBlock = m_player.m_equipment[m_player.m_equipSlot].getMaterial();
                        if (data.requiredToolLevel > 0 && eqMBlock.toolTier < data.requiredToolLevel)
                            break;
                        m_player.m_isMining = true;
                        m_player.m_miningTarget = voxel;
                        break;
                    }
                }
            }
        }

        // Eating: update progress when holding right-click on food
        m_player.updateEating(delta, rightPressed);

        // Right-click: place block (skip if eating)
        if (rightPressed && m_rightClickTimer.getElapsedTime().asSeconds() > 0.2f && !m_player.isEating())
        {
            Ray ray(eyePos, m_player.rotation);
            glm::ivec3 prevVoxel = ray.currentVoxel();
            while (ray.getLength() < 6.0f) {
                if (!ray.advance()) break;
                auto voxel = ray.currentVoxel();
                auto block = m_world.getBlock(voxel.x, voxel.y, voxel.z);
                auto& data = block.getData();

                if (data.requiredToolLevel == 255)
                    break;

                if (block.id != 0 && block.id != (int)BlockId::Water)
                {
                    if (block.id == (int)BlockId::Furnace && !m_player.isUIOpen()) {
                        m_player.m_furnaceUIOpen = true;
                        m_rightClickTimer.restart();
                        break;
                    }
                    m_rightClickTimer.restart();
                    glm::vec3 placePos(
                        prevVoxel.x + 0.5f,
                        prevVoxel.y + 0.5f,
                        prevVoxel.z + 0.5f);
                    m_world.addEvent<PlayerDigEvent>(sf::Mouse::Button::Right, placePos, m_player);
                    break;
                }
                prevVoxel = voxel;
            }
        }
    }

    m_camera.update();
    m_player.update(delta, m_world);

    // Update entities (pigman AI + physics)
    m_world.updateEntities(delta, m_player);

    // Sync dynamic difficulty state to player for timer color
    m_player.m_difficultyActive = m_world.isDifficultyActive();

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
        // Spawn extraction point 5 minutes into the round
        if (m_player.m_roundTimeLeft <= 300.0f && !m_extractionSpawned && !m_world.isExtractionActive()) {
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

void Application::on_render()
{
    m_player.setDropItems(&m_world.getDropItems());
    m_player.m_paused = m_paused;
    m_player.draw(m_masterRenderer, &m_camera);

    m_world.renderWorld(m_masterRenderer, m_camera);

    // Add pigman entities to renderer
    for (auto& e : m_world.getPigmen()) {
        EntityRenderData rd;
        rd.position = e.position;
        rd.rotation = e.rotation;
        rd.state = (int)e.state;
        rd.animTimer = e.stateTimer;
        rd.deathAnimTimer = e.deathAnimTimer;
        rd.attackCooldown = e.attackCooldown;
        rd.isHurt = (e.state == PigmanEntity::Hurt);
        m_masterRenderer.m_pigmanRenderer.addEntity(rd);
    }

    // Add spiders to renderer
    // Spider state mapping: Patrol=0→0, Chase=1→1, MeleeAttack=2→2(Attack),
    //   Hurt=3→3(Hurt), Dead=4→4(Dead)
    for (auto& e : m_world.getSpiders()) {
        EntityRenderData rd;
        rd.position = e.position;
        rd.rotation = e.rotation;
        int rawState = (int)e.state;
        if (rawState == 2) rawState = 2; // MeleeAttack → Attack (already 2)
        else if (rawState == 3) rawState = 3; // Hurt
        else if (rawState == 4) rawState = 4; // Dead
        rd.state = rawState;
        rd.animTimer = e.stateTimer;
        rd.deathAnimTimer = e.deathAnimTimer;
        rd.attackCooldown = e.meleeCooldown;
        rd.isHurt = (e.state == SpiderEntity::Hurt);
        rd.scale = 0.1f;
        rd.rotationYOffset = 90.0f;
        m_masterRenderer.m_spiderRenderer.addEntity(rd);
    }

    // Add projectiles to renderer
    for (auto& p : m_world.getProjectiles()) {
        m_masterRenderer.m_projectileRenderer.addProjectile(p);
    }

    // Add arrows to renderer (shrink as they fly)
    for (auto& a : m_world.getArrows()) {
        if (a.alive) {
            float shrink = 1.0f - a.lifetime / a.maxLifetime;
            if (shrink < 0.0f) shrink = 0.0f;
            m_masterRenderer.m_arrowRenderer.addPosition(a.position, 1.0f - shrink * 0.7f);
        }
    }

    m_masterRenderer.finishRender(m_window, m_camera);

    m_player.renderWeapon();
}
