#include "Material.h"

const Material Material::NOTHING(ID::Nothing, 0, false, "None");
const Material Material::GRASS_BLOCK(ID::Grass, 99, true, "Grass Block");
const Material Material::DIRT_BLOCK(ID::Dirt, 99, true, "Dirt Block");
const Material Material::STONE_BLOCK(ID::Stone, 64, true, "Stone Block");
const Material Material::OAK_BARK_BLOCK(ID::OakBark, 99, true,
                                        "Oak Bark Block");
const Material Material::OAK_LEAF_BLOCK(ID::OakLeaf, 99, true,
                                        "Oak Leaf Block");
const Material Material::SAND_BLOCK(ID::Sand, 99, true, "Sand Block");
const Material Material::CACTUS_BLOCK(ID::Cactus, 99, true, "Cactus Block");

const Material Material::ROSE(ID::Rose, 99, true, "Rose");
const Material Material::TALL_GRASS(ID::TallGrass, 99, true, "Tall Grass");
const Material Material::DEAD_SHRUB(ID::DeadShrub, 99, true, "Dead Shrub");

const Material Material::STICK(ID::Stick, 99, false, "Stick");
const Material Material::WOODEN_SWORD(ID::WoodenSword, 1, false, "Wooden Sword",
    3, 0, 1.0f, 10);  // Sword, +10 atk
const Material Material::RAW_MEAT(ID::RawMeat, 30, false, "Raw Meat");
const Material Material::GOLD_BLOCK(ID::GoldBlock, 99, true, "Gold Block");

const Material Material::COBBLESTONE(ID::Cobblestone, 64, true, "Cobblestone");
const Material Material::IRON_ORE_ITEM(ID::IronOre, 64, true, "Iron Ore");
const Material Material::IRON_INGOT(ID::IronIngot, 64, false, "Iron Ingot");
const Material Material::WILD_FRUIT(ID::WildFruit, 30, false, "Wild Fruit");
const Material Material::STONE_ARROW(ID::StoneArrow, 99, false, "Stone Arrow",
    0, 0, 1.0f, 10);

const Material Material::BOW(ID::Bow, 1, false, "Bow",
    0, 0, 1.0f, 1);
const Material Material::IRON_ARROW(ID::IronArrow, 99, false, "Iron Arrow",
    0, 0, 1.0f, 15);
const Material Material::SPIDER_SILK_ARROW(ID::SpiderSilkArrow, 99, false, "Spider Silk Arrow",
    0, 0, 1.0f, 10);

const Material Material::WOODEN_PICKAXE(ID::WoodenPickaxe, 1, false, "Wooden Pickaxe",
    1, 1, 2.0f, 1);   // Pickaxe, Wood, 2x, +1 atk
const Material Material::STONE_PICKAXE(ID::StonePickaxe, 1, false, "Stone Pickaxe",
    1, 2, 4.0f, 2);   // Pickaxe, Stone, 4x, +2 atk
const Material Material::IRON_PICKAXE(ID::IronPickaxe, 1, false, "Iron Pickaxe",
    1, 3, 6.0f, 3);   // Pickaxe, Iron, 6x, +3 atk

const Material Material::WOODEN_AXE(ID::WoodenAxe, 1, false, "Wooden Axe",
    2, 1, 2.0f, 2);   // Axe, Wood, 2x, +2 atk
const Material Material::STONE_AXE(ID::StoneAxe, 1, false, "Stone Axe",
    2, 2, 4.0f, 3);   // Axe, Stone, 4x, +3 atk
const Material Material::IRON_AXE(ID::IronAxe, 1, false, "Iron Axe",
    2, 3, 6.0f, 4);   // Axe, Iron, 6x, +4 atk

const Material Material::IRON_SWORD(ID::IronSword, 1, false, "Iron Sword",
    3, 0, 1.0f, 20);  // Sword, 非工具, +20 atk
const Material Material::COOKED_MEAT(Material::ID::CookedMeat, 30, false, "Cooked Meat");
const Material Material::FURNACE(Material::ID::Furnace, 1, true, "Furnace");
const Material Material::SILK(Material::ID::Silk, 64, false, "Silk");
const Material Material::SILK_THREAD(Material::ID::SilkThread, 64, false, "Silk Thread");

Material::Material(Material::ID id, int maxStack, bool isBlock,
                   std::string &&name,
                   uint8_t toolClass, uint8_t toolTier,
                   float miningMultiplier, int attackBonus)
    : id(id)
    , maxStackSize(maxStack)
    , isBlock(isBlock)
    , name(std::move(name))
    , toolClass(toolClass)
    , toolTier(toolTier)
    , miningMultiplier(miningMultiplier)
    , attackBonus(attackBonus)
{
}

BlockId Material::toBlockID() const
{
    switch (id) {
        case Nothing:
            return BlockId::Air;

        case Grass:
            return BlockId::Grass;

        case Dirt:
            return BlockId::Dirt;

        case Stone:
            return BlockId::Stone;

        case OakBark:
            return BlockId::OakBark;

        case OakLeaf:
            return BlockId::OakLeaf;

        case Sand:
            return BlockId::Sand;

        case Cactus:
            return BlockId::Cactus;

        case TallGrass:
            return BlockId::TallGrass;

        case Rose:
            return BlockId::Rose;

        case DeadShrub:
            return BlockId::DeadShrub;

        case Stick:
            return BlockId::Stick;

        case WoodenSword:
            return BlockId::WoodenSword;

        case RawMeat:
            return BlockId::RawMeat;

        case GoldBlock:
            return BlockId::GoldBlock;

        case Cobblestone:    return BlockId::Cobblestone;
        case IronOre:        return BlockId::IronOre;
        case IronIngot:      return BlockId::IronIngot;
        case WildFruit:      return BlockId::WildFruit;
        case StoneArrow:     return BlockId::StoneArrow;
        case WoodenPickaxe:  return BlockId::WoodenPickaxe;
        case StonePickaxe:   return BlockId::StonePickaxe;
        case IronPickaxe:    return BlockId::IronPickaxe;
        case WoodenAxe:      return BlockId::WoodenAxe;
        case StoneAxe:       return BlockId::StoneAxe;
        case IronAxe:        return BlockId::IronAxe;
        case IronSword:      return BlockId::IronSword;
        case CookedMeat:  return BlockId::CookedMeat_Item;
        case Furnace:     return BlockId::Furnace;
        case Silk:        return BlockId::Silk;
        case SilkThread:       return BlockId::SilkThread;
        case Bow:              return BlockId::Bow;
        case IronArrow:        return BlockId::IronArrow;
        case SpiderSilkArrow:  return BlockId::SpiderSilkArrow;

        default:
            return BlockId::NUM_TYPES;
    }
}

const Material &Material::toMaterial(BlockId id)
{
    switch (id) {
        case BlockId::Grass:
            return GRASS_BLOCK;

        case BlockId::Dirt:
            return DIRT_BLOCK;

        case BlockId::Stone:
            return STONE_BLOCK;

        case BlockId::OakBark:
            return OAK_BARK_BLOCK;

        case BlockId::OakLeaf:
            return OAK_LEAF_BLOCK;

        case BlockId::Sand:
            return SAND_BLOCK;

        case BlockId::Cactus:
            return CACTUS_BLOCK;

        case BlockId::Rose:
            return ROSE;

        case BlockId::TallGrass:
            return TALL_GRASS;

        case BlockId::DeadShrub:
            return DEAD_SHRUB;

        case BlockId::Stick:
            return STICK;
        case BlockId::WoodenSword:
            return WOODEN_SWORD;
        case BlockId::RawMeat:
            return RAW_MEAT;

        case BlockId::GoldBlock:
            return GOLD_BLOCK;

        case BlockId::Cobblestone:    return COBBLESTONE;
        case BlockId::IronOre:        return IRON_ORE_ITEM;
        case BlockId::IronIngot:      return IRON_INGOT;
        case BlockId::WildFruit:      return WILD_FRUIT;
        case BlockId::StoneArrow:     return STONE_ARROW;
        case BlockId::WoodenPickaxe:  return WOODEN_PICKAXE;
        case BlockId::StonePickaxe:   return STONE_PICKAXE;
        case BlockId::IronPickaxe:    return IRON_PICKAXE;
        case BlockId::WoodenAxe:      return WOODEN_AXE;
        case BlockId::StoneAxe:       return STONE_AXE;
        case BlockId::IronAxe:        return IRON_AXE;
        case BlockId::IronSword:      return IRON_SWORD;
        case BlockId::Furnace: return FURNACE;
        case BlockId::Silk:       return SILK;
        case BlockId::SilkThread:      return SILK_THREAD;
        case BlockId::Bow:              return BOW;
        case BlockId::IronArrow:        return IRON_ARROW;
        case BlockId::SpiderSilkArrow:  return SPIDER_SILK_ARROW;

        default:
            return NOTHING;
    }
}
