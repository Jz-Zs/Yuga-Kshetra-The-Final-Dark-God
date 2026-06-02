#ifndef BLOCKDATA_H_INCLUDED
#define BLOCKDATA_H_INCLUDED

#include "../../Util/NonCopyable.h"
#include "BlockId.h"
#include <SFML/Graphics.hpp>

/// @brief Allocates meshes to cubes and non-cube entities.
enum class BlockMeshType {
    Cube = 0,
    X = 1,
};

/// @brief Allocates shader behavior to groups of blocks.
enum class BlockShaderType {
    Chunk = 0,
    Liquid = 1,
    Flora = 2,
};

/// @brief Struct designed to hold geometric and tangibility data for each individual block.
struct BlockDataHolder : public NonCopyable {
    BlockId id;
    sf::Vector2i texTopCoord;
    sf::Vector2i texSideCoord;
    sf::Vector2i texBottomCoord;

    BlockMeshType meshType;
    BlockShaderType shaderType;

    bool isOpaque;
    bool isCollidable;

    float hardness = 1.0f;               // 挖掘时间倍率
    uint8_t requiredToolClass = 0;       // 0=None, 1=Pickaxe, 2=Axe
    uint8_t requiredToolLevel = 0;       // 0=任意, 1=木, 2=石, 3=铁, 4=钻石, 255=不可破坏
    BlockId dropBlockId = BlockId::Air;  // Air=掉落自身BlockId
    float dropProbability = 1.0f;        // 掉落概率，默认100%
};

class BlockData : public NonCopyable {
  public:
    BlockData(const std::string &fileName);

    const BlockDataHolder &getBlockData() const;

  private:
    BlockDataHolder m_data;
};

#endif // BLOCKDATA_H_INCLUDED
