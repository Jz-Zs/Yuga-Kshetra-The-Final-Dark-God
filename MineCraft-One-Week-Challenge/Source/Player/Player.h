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
#include "../Item/SmeltingRecipe.h"
#include "../Util/BitmapText.h"

class Keyboard;
class World;
class RenderMaster;

struct FurnaceState {
    ItemStack input;
    ItemStack fuel;
    ItemStack output;
    float progress = 0.f;    // 0.0 ~ 1.0
    bool isSmelting = false;
};

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

    // 物品栏 (Application 需要直接访问)
    std::vector<ItemStack> m_items;

    // 合成系统
    ItemStack m_craftGrid[9];
    const CraftingRecipe* m_currentRecipe = nullptr;

    // 战斗/挖掘 (Application 需要直接访问)
    float m_leftClickCooldown = 0.0f;
    float m_miningProgress = 0.0f;
    bool m_isMining = false;
    glm::ivec3 m_miningTarget{0, -999, 0};

    // Bow charge (equipment slot bow, Shift+LeftClick)
    float m_bowCharge = 0.0f;       // 0.0 ~ 1.0
    bool m_bowCharging = false;

    // 食用进度（长按右键吃野果）
    float m_eatProgress = 0.0f;
    bool m_isEating = false;

    // 丢弃确认（Shift+右键）
    int m_discardPending = -1; // slot index, -1 = none
    float m_discardMouseX = 0, m_discardMouseY = 0; // mouse position when triggered

    // 输入处理
    void processRKey();

    float getMiningProgress() const { return m_miningProgress; }
    bool isMining() const { return m_isMining; }
    float getEatProgress() const { return m_eatProgress; }
    bool isEating() const { return m_isEating; }
    float getBowCharge() const { return m_bowCharge; }
    bool isBowCharging() const { return m_bowCharging; }
    void updateEating(float dt, bool rightHeld);
    bool isBackpackOpen() const { return m_backpackOpen; }
    bool isUIOpen() const { return m_backpackOpen || m_furnaceUIOpen; }
    void onFurnaceMined();
    void onWorldReset();
    void drawFurnaceUI();
    bool hasFurnace() const;
    bool isMouseLockedForUI() const { return m_mouseLocked && !isUIOpen(); }
    void triggerSwing() { m_isSwinging = true; m_swingTimer = 0.0f; }

    // Combat
    int m_hp = 100;
    int m_maxHp = 100;
    int m_baseAttack = 2;
    // Slow debuff (spider web)
    float m_slowTimer = 0.0f;
    float m_slowFactor = 1.0f;  // 1.0=normal, 0.8=slowed
    int getAttackPower() const;
    void takeDamage(int amount, glm::vec3 knockbackDir);

    // Death
    bool m_isDead = false;
    float m_deathTimer = 0.0f;

    // Dynamic difficulty
    bool m_difficultyActive = false;

    // HUD data (Entry 5 renders, Entry 6 fills)
    int m_roundNumber = 1;
    float m_roundTimeLeft = 600.0f;
    int m_pigmanKills = 0;
    std::unordered_map<Material::ID, int> m_roundCollection;
    bool m_roundActive = false;

    // Extraction
    float m_extractionProgress = 0.0f;
    bool m_isExtracting = false;

    enum class SettlementOutcome { Success, TimeUp, Death };
    SettlementOutcome m_settlementOutcome = SettlementOutcome::Success;
    bool m_requestNewRound = false;

    // 熔炉系统
    FurnaceState m_furnace;
    bool m_furnaceUIOpen = false;
    glm::ivec3 m_furnacePos{-1, -1, -1};  // 已放置熔炉坐标，(-1,-1,-1)=未放置

    void clearInventory() {
        for (int i = 0; i < 17; i++) m_items[i] = ItemStack();
        m_equipment[0] = ItemStack();
        m_equipment[1] = ItemStack();
    }

  private:
    void jump();

    void keyboardInput(const Keyboard& keyboard);
    void mouseInput(sf::Window& window);
    bool m_isOnGround = false;
    bool m_isFlying = false;
    bool m_isSneak = false;

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
    BitmapText m_buttonText;       // Chinese text for settlement button
    BitmapText m_discardText;      // Chinese text for discard confirmation popup
    BitmapText m_furnaceText;      // Chinese text for furnace UI title
    BitmapText m_craftTitleText;   // Chinese text for crafting window title
    BitmapText m_bpTitleText;      // Chinese text for backpack window title
    BitmapText m_furnaceQtyText;   // Quantity overlay for furnace slots
    BitmapText m_furnaceCloseText; // Chinese text for furnace close button

    std::vector<ItemDropEntity>* m_pDropItems = nullptr;
};

#endif // PLAYER_H_INCLUDED
