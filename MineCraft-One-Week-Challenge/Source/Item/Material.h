#ifndef MATERIAL_H_INCLUDED
#define MATERIAL_H_INCLUDED

#include "../World/Block/BlockId.h"
#include <cstdint>
#include <string>

#include "../Util/NonCopyable.h"

/// @brief Determines case-by-case properties and behaviors of known block types.
struct Material : public NonCopyable {
    enum ID {
        Nothing,
        Grass,
        Dirt,
        Stone,
        OakBark,
        OakLeaf,
        Sand,
        Cactus,
        Rose,
        TallGrass,
        DeadShrub,
        Stick,
        WoodenSword,
        RawMeat,
        GoldBlock,

        // 入口7 新增
        Cobblestone,
        IronOre,
        IronIngot,
        WildFruit,
        StoneArrow,
        WoodenPickaxe,
        StonePickaxe,
        IronPickaxe,
        WoodenAxe,
        StoneAxe,
        IronAxe,
        IronSword,
        CookedMeat,
        Furnace,
        Silk,
        SilkThread
    };

    const static Material NOTHING, GRASS_BLOCK, DIRT_BLOCK, STONE_BLOCK,
        OAK_BARK_BLOCK, OAK_LEAF_BLOCK, SAND_BLOCK, CACTUS_BLOCK, ROSE,
        TALL_GRASS, DEAD_SHRUB;

    const static Material STICK, WOODEN_SWORD, RAW_MEAT, GOLD_BLOCK;

    const static Material COBBLESTONE, IRON_ORE_ITEM, IRON_INGOT, WILD_FRUIT,
        STONE_ARROW, WOODEN_PICKAXE, STONE_PICKAXE, IRON_PICKAXE,
        WOODEN_AXE, STONE_AXE, IRON_AXE, IRON_SWORD;

    const static Material COOKED_MEAT, FURNACE, SILK, SILK_THREAD;

    Material(Material::ID id, int maxStack, bool isBlock, std::string &&name,
             uint8_t toolClass = 0, uint8_t toolTier = 0,
             float miningMultiplier = 1.0f, int attackBonus = 0);

    BlockId toBlockID() const;

    static const Material &toMaterial(BlockId id);

    const Material::ID id;
    const int maxStackSize;
    const bool isBlock;
    const std::string name;

    // 入口7: 工具/属性系统
    uint8_t toolClass = 0;        // 0=None, 1=Pickaxe, 2=Axe, 3=Sword
    uint8_t toolTier = 0;         // 0=非工具, 1=木, 2=石, 3=铁, 4=钻石
    float miningMultiplier = 1.0f; // 挖掘倍率
    int attackBonus = 0;           // 攻击加成
};

namespace std {
template <> struct hash<Material::ID> {
    size_t operator()(const Material::ID &id) const
    {
        return std::hash<int>()(static_cast<int>(id));
    }
};
} // namespace std

#endif // MATERIAL_H_INCLUDED
