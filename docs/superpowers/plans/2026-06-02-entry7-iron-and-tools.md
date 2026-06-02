# 入口7：铁矿、新资源与合成拓展 — 实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 新增铁矿生成、工具系统（镐/斧/铁剑）、资源分级（石头→圆石、铁矿→铁锭）、野果掉落、石箭矢，共10个合成配方。

**Architecture:** 方案A最小侵入——在现有BlockData/Material结构上扩展字段，在现有地形生成器中插入矿脉算法，在现有挖掘逻辑中替换硬编码为基于属性的公式。BlockId从16种扩展到28种（含物品ID）。

**Tech Stack:** C++23, SFML 3, OpenGL 4.6, glm, vcpkg

---

### Task 1: 扩展 BlockId.h — 新增方块/物品ID

**Files:**
- Modify: `MineCraft-One-Week-Challenge/Source/World/Block/BlockId.h`

所有新物品遵循现有模式（Stick/WoodenSword/RawMeat 也在 BlockId 中）。

- [ ] **Step 1: 在 NUM_TYPES 前插入新枚举值**

```cpp
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

    NUM_TYPES
};
```

### Task 2: 扩展 BlockData — 硬度/工具需求/掉落属性

**Files:**
- Modify: `MineCraft-One-Week-Challenge/Source/World/Block/BlockData.h`
- Modify: `MineCraft-One-Week-Challenge/Source/World/Block/BlockData.cpp`

- [ ] **Step 1: BlockDataHolder 新增字段**

在 `BlockDataHolder` 结构体中，`isCollidable` 之后添加：

```cpp
// 新增：方块物理/工具属性
float hardness = 1.0f;               // 挖掘时间倍率
uint8_t requiredToolClass = 0;       // 0=None, 1=Pickaxe, 2=Axe
uint8_t requiredToolLevel = 0;       // 0=任意, 1=木, 2=石, 3=铁, 4=钻石, 255=不可破坏
BlockId dropBlockId = BlockId::Air;  // Air=掉落自身BlockId
float dropProbability = 1.0f;        // 掉落概率，默认100%
```

完整结构变为：
```cpp
struct BlockDataHolder : public NonCopyable {
    BlockId id;
    sf::Vector2i texTopCoord;
    sf::Vector2i texSideCoord;
    sf::Vector2i texBottomCoord;

    BlockMeshType meshType;
    BlockShaderType shaderType;

    bool isOpaque;
    bool isCollidable;

    float hardness = 1.0f;
    uint8_t requiredToolClass = 0;
    uint8_t requiredToolLevel = 0;
    BlockId dropBlockId = BlockId::Air;
    float dropProbability = 1.0f;
};
```

需要添加 `<cstdint>` include（如果还没通过 BlockId.h 间接引入）。
实际上 BlockId.h 已经 include `<cstdint>`，BlockData.h 也 include BlockId.h，所以 `uint8_t` 可用。

- [ ] **Step 2: BlockData 解析器新增可选字段**

在 `BlockData::BlockData()` 的 while 循环中，紧跟现有 `Collidable` 解析后添加：

```cpp
else if (line == "Hardness") {
    inFile >> m_data.hardness;
}
else if (line == "ToolClass") {
    int val;
    inFile >> val;
    m_data.requiredToolClass = static_cast<uint8_t>(val);
}
else if (line == "ToolLevel") {
    int val;
    inFile >> val;
    m_data.requiredToolLevel = static_cast<uint8_t>(val);
}
else if (line == "DropBlock") {
    int val;
    inFile >> val;
    m_data.dropBlockId = static_cast<BlockId>(val);
}
else if (line == "DropProb") {
    inFile >> m_data.dropProbability;
}
```

这些字段可选——旧 .block 文件没有它们时，默认值自动生效（hardness=1.0, toolClass=0, toolLevel=0, dropBlockId=Air=掉自身, dropProb=1.0=100%）。

### Task 3: 创建所有新 .block 文件

**Files:**
- Create: `Res/Blocks/IronOre.block`
- Create: `Res/Blocks/Cobblestone.block`
- Create: `Res/Blocks/IronIngot.block`
- Create: `Res/Blocks/WildFruit.block`
- Create: `Res/Blocks/StoneArrow.block`
- Create: `Res/Blocks/WoodenPickaxe.block`
- Create: `Res/Blocks/StonePickaxe.block`
- Create: `Res/Blocks/IronPickaxe.block`
- Create: `Res/Blocks/WoodenAxe.block`
- Create: `Res/Blocks/StoneAxe.block`
- Create: `Res/Blocks/IronAxe.block`
- Create: `Res/Blocks/IronSword.block`
- Delete: `Res/Blocks/CobbleStone` (旧的无后缀文件，会被 Cobblestone.block 替代)

- [ ] **Step 1: IronOre.block**

```
Name
IronOre

Id
16

TexAll
6 1

Opaque
1

MeshType
0

ShaderType
0

Collidable
1

Hardness
1.5

ToolClass
1

ToolLevel
2
```

ToolClass=1 (镐), ToolLevel=2 (至少石镐)。

- [ ] **Step 2: Cobblestone.block**

```
Name
Cobblestone

Id
17

TexTop
2 1

TexSide
1 1

TexBottom
2 1

Opaque
1

MeshType
0

ShaderType
0

Collidable
1

Hardness
1.2

ToolClass
1

ToolLevel
0
```

ToolClass=1 (镐), ToolLevel=0 (木镐即可)。

- [ ] **Step 3: IronIngot.block**

```
Name
IronIngot

Id
18

TexAll
0 2

Opaque
0

MeshType
0

ShaderType
0

Collidable
0
```

物品（非方块），不可放置。

- [ ] **Step 4: WildFruit.block**

```
Name
WildFruit

Id
19

TexAll
1 2

Opaque
0

MeshType
0

ShaderType
0

Collidable
0
```

- [ ] **Step 5: StoneArrow.block**

```
Name
StoneArrow

Id
20

TexAll
2 2

Opaque
0

MeshType
0

ShaderType
0

Collidable
0
```

- [ ] **Step 6: WoodenPickaxe.block**

```
Name
WoodenPickaxe

Id
21

TexAll
3 2

Opaque
0

MeshType
0

ShaderType
0

Collidable
0
```

- [ ] **Step 7: StonePickaxe.block**

```
Name
StonePickaxe

Id
22

TexAll
4 2

Opaque
0

MeshType
0

ShaderType
0

Collidable
0
```

- [ ] **Step 8: IronPickaxe.block**

```
Name
IronPickaxe

Id
23

TexAll
5 2

Opaque
0

MeshType
0

ShaderType
0

Collidable
0
```

- [ ] **Step 9: WoodenAxe.block**

```
Name
WoodenAxe

Id
24

TexAll
6 2

Opaque
0

MeshType
0

ShaderType
0

Collidable
0
```

- [ ] **Step 10: StoneAxe.block**

```
Name
StoneAxe

Id
25

TexAll
7 2

Opaque
0

MeshType
0

ShaderType
0

Collidable
0
```

- [ ] **Step 11: IronAxe.block**

```
Name
IronAxe

Id
26

TexAll
8 2

Opaque
0

MeshType
0

ShaderType
0

Collidable
0
```

- [ ] **Step 12: IronSword.block**

```
Name
IronSword

Id
27

TexAll
9 2

Opaque
0

MeshType
0

ShaderType
0

Collidable
0
```

- [ ] **Step 13: 删除旧的 CobbleStone 文件**

```bash
rm "Res/Blocks/CobbleStone"
```

### Task 4: 更新现有 .block 文件 — 添加硬度和工具需求

**Files:**
- Modify: `Res/Blocks/Stone.block`
- Modify: `Res/Blocks/Dirt.block`
- Modify: `Res/Blocks/Grass.block`
- Modify: `Res/Blocks/OakBark.block`
- Modify: `Res/Blocks/OakLeaf.block`
- Modify: `Res/Blocks/Sand.block`
- Modify: `Res/Blocks/GoldBlock.block`

- [ ] **Step 1: Stone.block — 追加属性**

在 `Collidable` 行之后、文件末尾追加：
```
Hardness
1.2

ToolClass
1

ToolLevel
0
```

- [ ] **Step 2: Dirt.block — 追加**

```
Hardness
1.0
```
（无工具需求，默认即可）

- [ ] **Step 3: Grass.block — 追加**

```
Hardness
1.0
```

- [ ] **Step 4: OakBark.block — 追加**

```
Hardness
1.1

ToolClass
2

ToolLevel
0
```
ToolClass=2 (斧), ToolLevel=0 (木斧即可)。

- [ ] **Step 5: OakLeaf.block — 追加**

```
Hardness
0.5
```

- [ ] **Step 6: Sand.block — 追加**

```
Hardness
0.8
```

- [ ] **Step 7: GoldBlock.block — 追加**

```
ToolLevel
255
```
ToolLevel=255 表示不可破坏。硬度无需设置（不会进入挖掘计算）。

### Task 5: 注册新方块到 BlockDatabase

**Files:**
- Modify: `MineCraft-One-Week-Challenge/Source/World/Block/BlockDatabase.cpp`

- [ ] **Step 1: 添加新方块的注册行**

在构造函数中 `GoldBlock` 注册之后、`}` 之前添加：

```cpp
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
```

注意：`m_blocks` 是 `std::array<std::unique_ptr<BlockType>, (unsigned)BlockId::NUM_TYPES>`，大小由 NUM_TYPES 决定。Task 1 增加了 `IronOre=16` 到 `IronSword=27`，NUM_TYPES 自动变为 28，array 有足够空间。

### Task 6: 扩展 Material — 新物品 + 工具属性

**Files:**
- Modify: `MineCraft-One-Week-Challenge/Source/Item/Material.h`
- Modify: `MineCraft-One-Week-Challenge/Source/Item/Material.cpp`

- [ ] **Step 1: Material.h — 新增枚举值和字段**

`Material::ID` 枚举新增：
```cpp
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
    IronSword
};
```

新增静态实例声明（在已有声明之后）：
```cpp
const static Material COBBLESTONE, IRON_ORE_ITEM, IRON_INGOT, WILD_FRUIT,
    STONE_ARROW, WOODEN_PICKAXE, STONE_PICKAXE, IRON_PICKAXE,
    WOODEN_AXE, STONE_AXE, IRON_AXE, IRON_SWORD;
```

Material 类新增非 const 成员字段（在 `name` 之后）：
```cpp
// 入口7: 工具/属性系统
uint8_t toolClass = 0;        // 0=None, 1=Pickaxe, 2=Axe, 3=Sword
uint8_t toolTier = 0;         // 0=非工具, 1=木, 2=石, 3=铁, 4=钻石
float miningMultiplier = 1.0f; // 挖掘倍率
int attackBonus = 0;           // 攻击加成
```

构造函���签名更新（name 参数之后追加默认参数）：
```cpp
Material(Material::ID id, int maxStack, bool isBlock, std::string &&name,
         uint8_t toolClass = 0, uint8_t toolTier = 0,
         float miningMultiplier = 1.0f, int attackBonus = 0);
```

注意：`id`、`maxStackSize`、`isBlock`、`name` 保持 const。新字段为非 const。

- [ ] **Step 2: Material.cpp — 定义新实例**

修改 `STONE_BLOCK` 的堆叠从 99 改为 64：
```cpp
const Material Material::STONE_BLOCK(ID::Stone, 64, true, "Stone Block");
```

新增实例定义：
```cpp
const Material Material::COBBLESTONE(ID::Cobblestone, 64, true, "Cobblestone");
const Material Material::IRON_ORE_ITEM(ID::IronOre, 64, true, "Iron Ore");
const Material Material::IRON_INGOT(ID::IronIngot, 64, false, "Iron Ingot");
const Material Material::WILD_FRUIT(ID::WildFruit, 30, false, "Wild Fruit");
const Material Material::STONE_ARROW(ID::StoneArrow, 99, false, "Stone Arrow");

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
```

更新 `WOODEN_SWORD` 给上 toolClass 和 attackBonus：
```cpp
const Material Material::WOODEN_SWORD(ID::WoodenSword, 1, false, "Wooden Sword",
    3, 0, 1.0f, 10);  // Sword, +10 atk
```

更新构造函数实现以接受新参数：
```cpp
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
```

注意：旧实例（NOTHING, GRASS_BLOCK 等）不传后4个参数，依赖默认值，无需修改旧定义。

- [ ] **Step 3: Material.cpp — 扩展 toBlockID() 和 toMaterial()**

`toBlockID()` switch 新增：
```cpp
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
```

`toMaterial()` switch 新增：
```cpp
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
```

### Task 7: 扩展合成配方

**Files:**
- Modify: `MineCraft-One-Week-Challenge/Source/Item/CraftingRecipe.cpp`

- [ ] **Step 1: 在 initCraftingRecipes() 末尾添加10个新配方**

```cpp
// === 入口7 新增配方 ===

// 3 石头(左竖) → 1 圆石
g_recipes.push_back({{
    &Material::STONE_BLOCK, nullptr, nullptr,
    &Material::STONE_BLOCK, nullptr, nullptr,
    &Material::STONE_BLOCK, nullptr, nullptr,
}, &Material::COBBLESTONE, 1});

// 3 铁矿(左竖) → 1 铁锭（临时，入口8改熔炉）
g_recipes.push_back({{
    &Material::IRON_ORE_ITEM, nullptr, nullptr,
    &Material::IRON_ORE_ITEM, nullptr, nullptr,
    &Material::IRON_ORE_ITEM, nullptr, nullptr,
}, &Material::IRON_INGOT, 1});

// 木镐: 3橡木(顶行) + 2木棍(中下竖)
g_recipes.push_back({{
    &Material::OAK_BARK_BLOCK, &Material::OAK_BARK_BLOCK, &Material::OAK_BARK_BLOCK,
    nullptr,                   &Material::STICK,           nullptr,
    nullptr,                   &Material::STICK,           nullptr,
}, &Material::WOODEN_PICKAXE, 1});

// 石镐: 3圆石(顶行) + 2木棍(中下竖)
g_recipes.push_back({{
    &Material::COBBLESTONE, &Material::COBBLESTONE, &Material::COBBLESTONE,
    nullptr,                &Material::STICK,       nullptr,
    nullptr,                &Material::STICK,       nullptr,
}, &Material::STONE_PICKAXE, 1});

// 铁镐: 3铁锭(顶行) + 2木棍(中下竖)
g_recipes.push_back({{
    &Material::IRON_INGOT, &Material::IRON_INGOT, &Material::IRON_INGOT,
    nullptr,               &Material::STICK,      nullptr,
    nullptr,               &Material::STICK,      nullptr,
}, &Material::IRON_PICKAXE, 1});

// 木斧: 3橡木(左上L) + 2木棍(中下竖)
g_recipes.push_back({{
    &Material::OAK_BARK_BLOCK, &Material::OAK_BARK_BLOCK, nullptr,
    &Material::OAK_BARK_BLOCK, &Material::STICK,          nullptr,
    nullptr,                   &Material::STICK,          nullptr,
}, &Material::WOODEN_AXE, 1});

// 石斧: 3圆石(左上L) + 2木棍(中下竖)
g_recipes.push_back({{
    &Material::COBBLESTONE, &Material::COBBLESTONE, nullptr,
    &Material::COBBLESTONE, &Material::STICK,       nullptr,
    nullptr,                &Material::STICK,       nullptr,
}, &Material::STONE_AXE, 1});

// 铁斧: 3铁锭(左上L) + 2木棍(中下竖)
g_recipes.push_back({{
    &Material::IRON_INGOT, &Material::IRON_INGOT, nullptr,
    &Material::IRON_INGOT, &Material::STICK,      nullptr,
    nullptr,               &Material::STICK,      nullptr,
}, &Material::IRON_AXE, 1});

// 铁剑: 铁锭+铁锭+木棍(中竖)
g_recipes.push_back({{
    nullptr,               &Material::IRON_INGOT, nullptr,
    nullptr,               &Material::IRON_INGOT, nullptr,
    nullptr,               &Material::STICK,      nullptr,
}, &Material::IRON_SWORD, 1});

// 石箭矢: 圆石+木棍+木棍(左上到右下对角线)
g_recipes.push_back({{
    &Material::COBBLESTONE, nullptr,               nullptr,
    nullptr,                &Material::STICK,       nullptr,
    nullptr,                nullptr,               &Material::STICK,
}, &Material::STONE_ARROW, 4});
```

### Task 8: 铁矿脉生成

**Files:**
- Modify: `MineCraft-One-Week-Challenge/Source/World/Generation/Terrain/ClassicOverWorldGenerator.cpp`

- [ ] **Step 1: 在 setBlocks() 的 Stone 分支后插入矿脉生成**

在 `setBlocks()` 函数末尾，`for (auto &tree : trees)` 循环之前，添加：

```cpp
// 入口7: 铁矿脉生成 — Z ∈ [5, 20), 每区块 2~4 个矿脉
int veinCount = m_random.intInRange(2, 5); // [2, 4] inclusive
for (int v = 0; v < veinCount; v++) {
    int startX = m_random.intInRange(0, CHUNK_SIZE - 1);
    int startY = m_random.intInRange(5, 20);
    int startZ = m_random.intInRange(0, CHUNK_SIZE - 1);

    int veinSize = m_random.intInRange(3, 7); // [3, 6] ore blocks

    // 随机主轴方向: 0=X, 1=Y, 2=Z
    int axis = m_random.intInRange(0, 3);
    int cx = startX, cy = startY, cz = startZ;

    for (int step = 0; step < veinSize; step++) {
        // 在 (cx,cy,cz) 的 2×2×2 邻域内放置铁矿
        for (int dx = 0; dx < 2; dx++)
            for (int dy = 0; dy < 2; dy++)
                for (int dz = 0; dz < 2; dz++) {
                    int px = cx + dx;
                    int py = cy + dy;
                    int pz = cz + dz;
                    if (px >= 0 && px < CHUNK_SIZE &&
                        py >= 0 && py < 64 &&
                        pz >= 0 && pz < CHUNK_SIZE) {
                        // 仅替换石头
                        if (m_pChunk->getBlock(px, py, pz).id == (int)BlockId::Stone) {
                            m_pChunk->setBlock(px, py, pz, BlockId::IronOre);
                        }
                    }
                }

        // 沿主轴方向前进，允许随机偏移
        int dir = (m_random.intInRange(0, 5) < 3) ? 1 : -1; // 60%正向, 40%反向
        switch (axis) {
            case 0: cx += dir; break;
            case 1: cy += dir; break;
            case 2: cz += dir; break;
        }
        // 钳制在区块范围内
        cx = std::max(0, std::min(cx, CHUNK_SIZE - 1));
        cy = std::max(5, std::min(cy, 19));
        cz = std::max(0, std::min(cz, CHUNK_SIZE - 1));
    }
}
```

代码插入位置在 setBlocks() 函数中 `for (auto &plant : plants)` 循环之前（即三层 for 循环结束后）。

### Task 9: 重写挖掘逻辑 — Application.cpp + PlayerDigEvent.cpp

**Files:**
- Modify: `MineCraft-One-Week-Challenge/Source/Application.cpp`
- Modify: `MineCraft-One-Week-Challenge/Source/World/Event/PlayerDigEvent.cpp`

- [ ] **Step 1: Application.cpp — 射线检测改用 BlockData 属性，替换硬编码排除**

将第 153 行：
```cpp
if (id != BlockId::Air && id != BlockId::Water && id != BlockId::GoldBlock)
```

替换为使用 BlockData 属性判断。需要获取 BlockData：
```cpp
auto block = m_world.getBlock(x, y, z);
auto id = (BlockId)block.id;
auto& data = block.getData();

// 跳过空气、水、不可破坏方块（toolLevel=255）
if (id != BlockId::Air && id != BlockId::Water
    && data.requiredToolLevel != 255)
```

注意有两处此逻辑（初始射线 153 行、连续挖掘射线 196 行），两处都要改。

- [ ] **Step 2: 挖掘前检查工具等级是否足够**

在射线检测到方块、准备开始挖掘时（~160行），添加工具等级检查：

```cpp
// 检查工具等级是否足够
const auto& eqM = m_player.m_equipment[m_player.m_equipSlot].getMaterial();
if (data.requiredToolLevel > 0 && eqM.toolTier < data.requiredToolLevel) {
    // 工具等级不够，无法挖掘，射线不穿透
    break;
}
```

这段添加到 `if (!m_player.m_isMining || ...)` 条件判断之前，作为另一个 break 条件。

实际上更干净的做法是把工具等级检查放在射线循环中，发现方块后先判断能否挖掘，不能就 break（射线阻挡）：

```cpp
if (id != BlockId::Air && id != BlockId::Water
    && data.requiredToolLevel != 255) {
    // 工具等级检查
    const auto& eqM = m_player.m_equipment[m_player.m_equipSlot].getMaterial();
    if (data.requiredToolLevel > 0 && eqM.toolTier < data.requiredToolLevel) {
        break; // 无法挖掘，射线不穿透
    }
    if (!m_player.m_isMining
        || m_player.m_miningTarget.x != x
        ...) {
        // 开始挖掘
    }
    break;
}
```

- [ ] **Step 3: Application.cpp — 挖掘速度公式**

将第 173 行：
```cpp
m_player.m_miningProgress += delta / 0.3f;
```

替换为：
```cpp
// 获取当前目标方块的属性
auto targetBlock = m_world.getBlock(
    m_player.m_miningTarget.x,
    m_player.m_miningTarget.y,
    m_player.m_miningTarget.z);
auto& targetData = targetBlock.getData();

// 计算挖掘时间
const auto& eqM = m_player.m_equipment[m_player.m_equipSlot].getMaterial();
float multiplier = 1.0f;
// 工具类型匹配则应用倍率
if (targetData.requiredToolClass != 0
    && eqM.toolClass == targetData.requiredToolClass) {
    multiplier = eqM.miningMultiplier;
}
float digTime = targetData.hardness * 0.3f / multiplier;
m_player.m_miningProgress += delta / digTime;
```

- [ ] **Step 4: PlayerDigEvent.cpp — 使用 BlockData 掉落属性**

将 `dig()` 函数中 Left 按钮的逻辑改为：

```cpp
case sf::Mouse::Button::Left: {
    auto block = world.getBlock(x, y, z);
    auto& data = block.getData();

    // 确定掉落物：dropBlockId 非 Air 则用它，否则掉自身
    BlockId dropId = (data.dropBlockId != BlockId::Air)
        ? data.dropBlockId : (BlockId)block.id;

    // 概率掉落（用整数随机模拟浮点概率）
    bool shouldDrop = data.dropProbability >= 1.0f;
    if (!shouldDrop) {
        shouldDrop = RandomSingleton::get().intInRange(0, 99)
                     < static_cast<int>(data.dropProbability * 100);
    }
    if (shouldDrop) {
        world.spawnDrop(glm::ivec3(x, y, z), dropId);
    }

    // 特殊：树叶 10% 额外掉野果
    if (block.id == (int)BlockId::OakLeaf
        && RandomSingleton::get().intInRange(0, 99) < 10) {
        world.spawnDrop(glm::ivec3(x, y, z), BlockId::WildFruit);
    }

    world.updateChunk(x, y, z);
    world.setBlock(x, y, z, 0);
    break;
}
```

需要添加 include：
```cpp
#include "../../Util/Random.h"
```

### Task 10: Player — 攻击力从 Material 读取 + 野果食用

**Files:**
- Modify: `MineCraft-One-Week-Challenge/Source/Player/Player.cpp`

- [ ] **Step 1: getAttackPower() 改用 attackBonus 字段**

将第 1295-1303 行的硬编码判断：
```cpp
int Player::getAttackPower() const
{
    int weaponAtk = 0;
    const auto& eqM = m_equipment[m_equipSlot].getMaterial();
    if (eqM.id == Material::ID::WoodenSword) {
        weaponAtk = 10;
    }
    return m_baseAttack + weaponAtk;
}
```

替换为：
```cpp
int Player::getAttackPower() const
{
    const auto& eqM = m_equipment[m_equipSlot].getMaterial();
    return m_baseAttack + eqM.attackBonus;
}
```

- [ ] **Step 2: 野果食用逻辑（右键使用物品恢复HP）**

Player 已有右键放置方块的逻辑，但需要添加对非方块物品的"使用"处理。目前右键只处理 isBlock 的物品。需要在 Player::update() 或相关逻辑中添加食物消耗。

最简单方案：在玩家物品栏中右键点击野果直接消耗。背包 UI 中（Player::draw()），检测右键点击物品格，如果是 WildFruit，消耗 1 个恢复 5 HP。

在 Player.cpp 的 draw() 背包部分（m_backpackOpen 分支），物品格右键菜单中添加：
```cpp
// 在背包物品格右键检测处
if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
    if (mat.id == Material::ID::WildFruit && m_items[i].getNumInStack() > 0) {
        m_hp = std::min(m_hp + 5, m_maxHp);
        m_items[i].remove();
    }
}
```

此段需要精准定位到背包物品格渲染循环中。查看 Player.cpp 中 m_backpackOpen 时的物品格渲染（约 700-900 行），在已有左键拖拽逻辑后添加右键食物使用检测。

（注：入口10 体力系统上线后，野果恢复逻辑会迁移到体力系统）

### Task 11: 构建并验证

- [ ] **Step 1: CMake 配置和构建**

```bash
cd "D:/Yuga_Kshetra-The_Final_Dark_God/MineCraft-One-Week-Challenge"
cmake --build out/build/x64-Debug --config Debug
```

- [ ] **Step 2: 验证清单**

确保以下功能正常：
1. 地下 Z<20 可见铁矿石纹理
2. 木镐挖石头加速（约 0.18s → 应观察到进度条快于空手）
3. 石镐挖铁矿正常，木镐挖铁矿无效（进度条不推进）
4. 空手挖橡木速度正常（约 0.33s），木斧挖橡木加速
5. 树叶破坏掉落树叶方块 + 偶尔掉野果
6. 合成：石头→圆石、圆石→石镐、铁矿→铁锭→铁剑等全部配方
7. 铁剑攻击力 +20，木剑 +10
8. 野果右键食用恢复 5 HP
9. 金块不可挖掘（射线不穿透）
10. 世界重生后铁矿石出现在新位置
