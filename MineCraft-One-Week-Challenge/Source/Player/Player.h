#ifndef PLAYER_H_INCLUDED
#define PLAYER_H_INCLUDED

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <memory>
#include <sstream>
#include <vector>
#include <unordered_map>

#include "../Entity.h"

class Camera;

#include "../Input/ToggleKey.h"
#include "../Item/ItemStack.h"
#include "../Item/CraftingRecipe.h"
#include "../Entity/ItemDropEntity.h"
#include "../Util/BitmapText.h"

class Keyboard;
class World;
class RenderMaster;

class Player : public Entity {
  public:
    Player();

    void handleInput(sf::Window& window, const Keyboard& keyboard);

    void update(float dt, World &wolrd);
    void collide(World &world, const glm::vec3 &vel, float dt);

    bool addItem(const Material &material);
    void setDropItems(std::vector<ItemDropEntity>* drops);

    void draw(RenderMaster &master, const Camera* camera = nullptr);
    void drawHealthBar();
    void drawTimer();
    void drawSettlement(const Camera* camera);
    void renderWeapon();

    ItemStack &getHeldItems();

    // 装备系统 (独立于快捷栏，左键攻击用装备，右键放置用快捷栏)
    ItemStack m_equipment[2] = {ItemStack(Material::NOTHING, 0), ItemStack(Material::NOTHING, 0)};
    int m_equipSlot = 0; // 0=主手, 1=副手

    // 合成系统
    ItemStack m_craftGrid[9];
    const CraftingRecipe* m_currentRecipe = nullptr;

    // 战斗/挖掘 (Application 需要直接访问)
    float m_leftClickCooldown = 0.0f;
    float m_miningProgress = 0.0f;
    bool m_isMining = false;
    glm::ivec3 m_miningTarget{0, -999, 0};

    // 输入处理
    void processRKey();

    float getMiningProgress() const { return m_miningProgress; }
    bool isMining() const { return m_isMining; }
    bool isBackpackOpen() const { return m_backpackOpen; }
    bool isMouseLockedForUI() const { return m_mouseLocked && !m_backpackOpen; }
    void triggerSwing() { m_isSwinging = true; m_swingTimer = 0.0f; }

    // Combat
    int m_hp = 100;
    int m_maxHp = 100;
    int m_baseAttack = 2;
    int getAttackPower() const;
    void takeDamage(int amount, glm::vec3 knockbackDir);

    // Death
    bool m_isDead = false;

    // HUD data (Entry 5 renders, Entry 6 fills)
    int m_roundNumber = 1;
    float m_roundTimeLeft = 600.0f;
    int m_pigmanKills = 0;
    std::unordered_map<Material::ID, int> m_roundCollection;
    bool m_roundActive = false;

  private:
    void jump();

    void keyboardInput(const Keyboard& keyboard);
    void mouseInput(sf::Window& window);
    bool m_isOnGround = false;
    bool m_isFlying = false;
    bool m_isSneak = false;

    std::vector<ItemStack> m_items;
    int m_heldItem = 0;

    ToggleKey m_itemDown;
    ToggleKey m_itemUp;
    ToggleKey m_flyKey;

    ToggleKey m_num1;
    ToggleKey m_num2;
    ToggleKey m_num3;
    ToggleKey m_num4;
    ToggleKey m_num5;

    ToggleKey m_backpackKey;
    bool m_backpackOpen = false;

    ToggleKey m_slow;

    glm::vec3 m_acceleration;

    // 武器动画
    float m_swingAngle = 0.0f;
    float m_swingTimer = 0.0f;
    bool m_isSwinging = false;

    // 装备切换键
    ToggleKey m_equipKey;

    // 鼠标锁定状态 (准星显隐)
    bool m_mouseLocked = true;

    // Software bitmap text renderer
    BitmapText m_bitmapText;
    BitmapText m_hudText;          // Chinese text for timer + settlement screen

    std::vector<ItemDropEntity>* m_pDropItems = nullptr;
};

#endif // PLAYER_H_INCLUDED
