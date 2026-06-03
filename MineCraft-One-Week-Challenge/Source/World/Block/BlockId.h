#ifndef BLOCKID_H_INCLUDED
#define BLOCKID_H_INCLUDED

#include <cstdint>

using Block_t = uint8_t;

/// @brief Known block ID types used in game.
enum class BlockId : Block_t {
    Air = 0,
    Grass = 1,
    Dirt = 2,
    Stone = 3,
    OakBark = 4,
    OakLeaf = 5,
    Sand = 6,
    Water = 7,
    Cactus = 8,
    Rose = 9,
    TallGrass = 10,
    DeadShrub = 11,
    Stick = 12,
    WoodenSword = 13,
    RawMeat = 14,
    GoldBlock = 15,
    IronOre = 16,
    Cobblestone = 17,
    IronIngot = 18,
    WildFruit = 19,
    StoneArrow = 20,
    WoodenPickaxe = 21,
    StonePickaxe = 22,
    IronPickaxe = 23,
    WoodenAxe = 24,
    StoneAxe = 25,
    IronAxe = 26,
    IronSword = 27,
    Furnace = 28,
    CookedMeat_Item = 29,
    Silk = 30,
    SilkThread = 31,
    Bow = 32,
    IronArrow = 33,
    SpiderSilkArrow = 34,

    NUM_TYPES
};

#endif // BLOCKID_H_INCLUDED
