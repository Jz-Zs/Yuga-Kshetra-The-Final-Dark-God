#ifndef PLAYER_H_INCLUDED
#define PLAYER_H_INCLUDED

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <memory>
#include <sstream>
#include <vector>

#include "../Entity.h"

class Camera;

#include "../Input/ToggleKey.h"
#include "../Item/ItemStack.h"
#include "../Entity/ItemDropEntity.h"
#include "../Util/BitmapText.h"

class Keyboard;
class World;
class RenderMaster;

class Player : public Entity {
  public:
    Player();

    void handleInput(const sf::Window& window, const Keyboard& keyboard);

    void update(float dt, World &wolrd);
    void collide(World &world, const glm::vec3 &vel, float dt);

    bool addItem(const Material &material);
    void setDropItems(std::vector<ItemDropEntity>* drops);

    void draw(RenderMaster &master, const Camera* camera = nullptr);

    ItemStack &getHeldItems();

  private:
    void jump();

    void keyboardInput(const Keyboard& keyboard);
    void mouseInput(const sf::Window &window);
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

    // Software bitmap text renderer
    BitmapText m_bitmapText;

    std::vector<ItemDropEntity>* m_pDropItems = nullptr;
};

#endif // PLAYER_H_INCLUDED
