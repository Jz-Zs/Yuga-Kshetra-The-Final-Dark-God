#include "BlockDatabase.h"

// Block Database initializes to first pack, not the second.
BlockDatabase::BlockDatabase()
    : textureAtlas("DefaultPack")
{
    m_blocks[(int)BlockId::Air] = std::make_unique<DefaultBlock>("Air");
    m_blocks[(int)BlockId::Grass] = std::make_unique<DefaultBlock>("Grass");
    m_blocks[(int)BlockId::Dirt] = std::make_unique<DefaultBlock>("Dirt");
    m_blocks[(int)BlockId::Stone] = std::make_unique<DefaultBlock>("Stone");
    m_blocks[(int)BlockId::OakBark] = std::make_unique<DefaultBlock>("OakBark");
    m_blocks[(int)BlockId::OakLeaf] = std::make_unique<DefaultBlock>("OakLeaf");
    m_blocks[(int)BlockId::Sand] = std::make_unique<DefaultBlock>("Sand");
    m_blocks[(int)BlockId::Water] = std::make_unique<DefaultBlock>("Water");
    m_blocks[(int)BlockId::Cactus] = std::make_unique<DefaultBlock>("Cactus");
    m_blocks[(int)BlockId::TallGrass] =
        std::make_unique<DefaultBlock>("TallGrass");
    m_blocks[(int)BlockId::Rose] = std::make_unique<DefaultBlock>("Rose");
    m_blocks[(int)BlockId::DeadShrub] =
        std::make_unique<DefaultBlock>("DeadShrub");
    m_blocks[(int)BlockId::Stick] = std::make_unique<DefaultBlock>("Stick");
    m_blocks[(int)BlockId::WoodenSword] =
        std::make_unique<DefaultBlock>("WoodenSword");
    m_blocks[(int)BlockId::RawMeat] =
        std::make_unique<DefaultBlock>("RawMeat");
    m_blocks[(int)BlockId::GoldBlock] =
        std::make_unique<DefaultBlock>("GoldBlock");
    m_blocks[(int)BlockId::IronOre] = std::make_unique<DefaultBlock>("IronOre");
    m_blocks[(int)BlockId::Cobblestone] = std::make_unique<DefaultBlock>("Cobblestone");
    m_blocks[(int)BlockId::IronIngot] = std::make_unique<DefaultBlock>("IronIngot");
    m_blocks[(int)BlockId::WildFruit] = std::make_unique<DefaultBlock>("WildFruit");
    m_blocks[(int)BlockId::StoneArrow] = std::make_unique<DefaultBlock>("StoneArrow");
    m_blocks[(int)BlockId::WoodenPickaxe] = std::make_unique<DefaultBlock>("WoodenPickaxe");
    m_blocks[(int)BlockId::StonePickaxe] = std::make_unique<DefaultBlock>("StonePickaxe");
    m_blocks[(int)BlockId::IronPickaxe] = std::make_unique<DefaultBlock>("IronPickaxe");
    m_blocks[(int)BlockId::WoodenAxe] = std::make_unique<DefaultBlock>("WoodenAxe");
    m_blocks[(int)BlockId::StoneAxe] = std::make_unique<DefaultBlock>("StoneAxe");
    m_blocks[(int)BlockId::IronAxe] = std::make_unique<DefaultBlock>("IronAxe");
    m_blocks[(int)BlockId::IronSword] = std::make_unique<DefaultBlock>("IronSword");
    m_blocks[(int)BlockId::Furnace] = std::make_unique<DefaultBlock>("Furnace");
    m_blocks[(int)BlockId::CookedMeat_Item] = std::make_unique<DefaultBlock>("CookedMeat_Item");
    m_blocks[(int)BlockId::Silk] = std::make_unique<DefaultBlock>("Silk");
    m_blocks[(int)BlockId::SilkThread] = std::make_unique<DefaultBlock>("SilkThread");
    m_blocks[(int)BlockId::Bow] = std::make_unique<DefaultBlock>("Bow");
    m_blocks[(int)BlockId::IronArrow] = std::make_unique<DefaultBlock>("IronArrow");
    m_blocks[(int)BlockId::SpiderSilkArrow] = std::make_unique<DefaultBlock>("SpiderSilkArrow");
}

BlockDatabase &BlockDatabase::get()
{
    static BlockDatabase d;
    return d;
}

const BlockType &BlockDatabase::getBlock(BlockId id) const
{
    return *m_blocks[(int)id];
}

const BlockData &BlockDatabase::getData(BlockId id) const
{
    int idx = (int)id;
    if (idx < 0 || idx >= (int)m_blocks.size()) idx = 0; // fallback to Air
    return m_blocks[idx]->getData();
}
