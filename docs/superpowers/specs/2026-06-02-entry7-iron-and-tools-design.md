# 入口7：铁矿、新资源与合成拓展 — 设计文档

## 概述

在 Phase 1 MVP 基础上扩展资源分层和工具体系：新增铁矿生成、树叶掉落野果、石制/铁制工具（镐/斧/剑）、石箭矢，以及对应的合成配方。

---

## 一、硬度体系

以草/泥土=1.0 为基准。

| 方块 | 硬度 | 所需工具 | 最低工具等级 |
|------|------|----------|--------------|
| 泥土/草方块 | 1.0 | 无 | 0 |
| 沙子 | 0.8 | 无 | 0 |
| 树叶 | 0.5 | 无 | 0 |
| 橡木 | 1.1 | 斧 | 0(木) |
| 石头 | 1.2 | 镐 | 0(木) |
| 圆石 | 1.2 | 镐 | 0(木) |
| 铁矿 | 1.5 | 镐 | 2(石) |
| 金块 | — | — | —(不可破坏) |

工具等级：0=None, 1=木, 2=石, 3=铁, 4=钻石(预留)

挖掘公式：`实际时间 = 方块硬度 × 0.3s / 工具倍率`

- 工具类型不匹配：×1.0（无加成，无惩罚）
- 工具等级不够：无法破坏（射线不穿透）
- 工具类型匹配且等级够：乘对应倍率（木×2 / 石×4 / 铁×6）

---

## 二、素材分级

```
一级素材（直接掉落）: 橡木、石头、铁矿、野果
二级素材（合成加工）: 木棍、圆石、铁锭
```

- 挖石头 → 石头（一级）→ 3:1 合成 → 圆石（二级）
- 挖铁矿 → 铁矿（一级）→ 3:1 合成 → 铁锭（二级），入口8改为熔炉

---

## 三、新增 BlockId

| BlockId | 值 | 说明 |
|---------|-----|------|
| IronOre | 16 | 铁矿石 |
| Cobblestone | 17 | 圆石 |

---

## 四、新增 Material

| Material | 堆叠 | isBlock | toolClass | toolTier | miningMul | attackBonus |
|----------|------|---------|-----------|----------|-----------|-------------|
| COBBLESTONE | 64 | true | — | — | — | — |
| IRON_ORE | 64 | true | — | — | — | — |
| IRON_INGOT | 64 | false | — | — | — | — |
| WILD_FRUIT | 30 | false | — | — | — | — |
| STONE_ARROW | 99 | false | — | — | — | — |
| WOODEN_PICKAXE | 1 | false | 1(Pickaxe) | 1 | 2.0 | 1 |
| STONE_PICKAXE | 1 | false | 1(Pickaxe) | 2 | 4.0 | 2 |
| IRON_PICKAXE | 1 | false | 1(Pickaxe) | 3 | 6.0 | 3 |
| WOODEN_AXE | 1 | false | 2(Axe) | 1 | 2.0 | 2 |
| STONE_AXE | 1 | false | 2(Axe) | 2 | 4.0 | 3 |
| IRON_AXE | 1 | false | 2(Axe) | 3 | 6.0 | 4 |
| IRON_SWORD | 1 | false | 3(Sword) | — | — | +20 |

已有 WOODEN_SWORD 补标记 toolClass=Sword。

---

## 五、合成配方总表

已有配方保留（橡木→木棍，木棍→木剑）。

**新增配方：**

| 成品 | 配方网格 | 产出 |
|------|----------|------|
| 圆石 | 3石头(左竖) | 1 |
| 铁锭 | 3铁矿(左竖) | 1（临时，入口8改熔炉）|
| 木镐 | 3橡木(顶行) + 2木棍(中下竖) | 1 |
| 石镐 | 3圆石(顶行) + 2木棍(中下竖) | 1 |
| 铁镐 | 3铁锭(顶行) + 2木棍(中下竖) | 1 |
| 木斧 | 3橡木(左上L) + 2木棍(中下竖) | 1 |
| 石斧 | 3圆石(左上L) + 2木棍(中下竖) | 1 |
| 铁斧 | 3铁锭(左上L) + 2木棍(中下竖) | 1 |
| 铁剑 | 铁锭+铁锭+木棍(中竖) | 1 |
| 石箭矢 | 圆石+木棍+木棍(对角线↘) | 4 |

---

## 六、铁矿生成规则

在 `ClassicOverWorldGenerator::setBlocks()` 的 Stone 分支中插入：

- 深度：Z ∈ [5, 20)
- 密度：每个区块 2~4 个矿脉
- 矿脉大小：3~6 个铁矿方块
- 算法：随机起点 → 随机轴向(X/Y/Z)延伸 2~4 步 → 每步在 2×2×2 邻域填铁矿
- 仅替换石头，不替换草/泥土/沙子/水

---

## 七、树叶掉落野果

破坏 OakLeaf 时：
- 10% 概率掉落 1 个野果
- 树叶方块本身也掉落（OAK_LEAF_BLOCK 进背包）
- 需要给 BlockData 新增 dropBlockId 和 dropProbability 字段支持概率掉落

---

## 八、方块属性系统改动

`BlockDataHolder` 新增字段：
```cpp
float hardness = 1.0f;
uint8_t requiredToolClass = 0;   // 0=None, 1=Pickaxe, 2=Axe
uint8_t requiredToolLevel = 0;   // 0=任意, 1=木, 2=石, 3=铁, 4=钻石
BlockId dropBlockId = BlockId::Air;  // Air=掉落自身
float dropProbability = 1.0f;    // 掉落概率，默认100%
```

`.block` 文件新增可选字段（向后兼容，缺省=默认值）：
```
Hardness
1.0

ToolClass
1

ToolLevel
2

DropBlock
5

DropProb
0.1
```

---

## 九、Material 改动

`Material` 新增字段：
```cpp
uint8_t toolClass = 0;       // 0=None, 1=Pickaxe, 2=Axe, 3=Sword
uint8_t toolTier = 0;        // 工具等级
float miningMultiplier = 1.0f;
int attackBonus = 0;         // 攻击加成（当前 Sword 的 +10 改为用此字段）
```

---

## 十、挖掘逻辑改动 (PlayerDigEvent)

1. 查方块 requiredToolClass 和 requiredToolLevel
2. 查手持工具 toolClass 和 toolTier
3. requiredToolLevel > 0 且 toolTier < requiredToolLevel → 无法破坏 + 射线阻挡
4. toolClass 不匹配 → ×1.0（无加成）
5. toolClass 匹配 → 挖掘速度 = hardness × 0.3s / miningMultiplier
6. 方块破坏后查 dropBlockId + dropProbability 决定掉落

---

## 十一、不可破坏方块射线阻挡（修复现有 bug）

当前金块（GoldBlock）存在可以穿透挖掘的问题。入口7 统一处理：
- 当方块被判定为"不可破坏"时，不推进挖掘进度，且不消耗
- 射线检测到不可破坏方块时视为碰撞体，不穿透

---

## 十二、纹理坐标

| 坐标 | 内容 | 用途 |
|------|------|------|
| (1,1) | 圆石侧面 | Cobblestone TexSide |
| (2,1) | 圆石顶底 | Cobblestone TexTop/TexBottom |
| (6,1) | 铁矿石 | IronOre TexAll |
| (0,2) | 铁锭 | IronIngot 物品图标 |
| (1,2) | 野果 | WildFruit 物品图标 |
| (2,2) | 石箭矢 | StoneArrow 物品图标 |
| (3,2) | 木镐 | WoodenPickaxe |
| (4,2) | 石镐 | StonePickaxe |
| (5,2) | 铁镐 | IronPickaxe |
| (6,2) | 木斧 | WoodenAxe |
| (7,2) | 石斧 | StoneAxe |
| (8,2) | 铁斧 | IronAxe |
| (9,2) | 铁剑 | IronSword |

---

## 十三、文件改动清单

| 文件 | 改动类型 | 说明 |
|------|----------|------|
| `BlockId.h` | 修改 | +IronOre(16), +Cobblestone(17) |
| `BlockData.h` | 修改 | +hardness/toolClass/toolLevel/dropBlockId/dropProb |
| `BlockData.cpp` | 修改 | 解析新字段 |
| `BlockDatabase.cpp` | 修改 | 注册 IronOre, Cobblestone |
| `Material.h` | 修改 | +新 Material ID, +toolClass/toolTier/miningMul/attackBonus |
| `Material.cpp` | 修改 | +新 Material 实例 |
| `CraftingRecipe.cpp` | 修改 | +10个新配方 |
| `ClassicOverWorldGenerator.cpp` | 修改 | 铁矿脉生成 |
| `PlayerDigEvent.cpp` | 修改 | 工具检测+挖掘速度公式+不可破坏阻挡 |
| `Player.cpp` | 修改 | 攻击力读取 Material.attackBonus，野果食用(+5HP) |
| `World.cpp` | 修改 | 树叶概率掉落 |
| `Res/Blocks/IronOre.block` | 新建 | 铁矿石方块定义 |
| `Res/Blocks/Cobblestone.block` | 新建 | 圆石方块定义 |
| `Res/Blocks/IronIngot.block` | 新建 | 铁锭物品定义 |
| `Res/Blocks/WildFruit.block` | 新建 | 野果物品定义 |
| `Res/Blocks/StoneArrow.block` | 新建 | 石箭矢物品定义 |
| `Res/Blocks/WoodenPickaxe.block` | 新建 | 木镐物品定义 |
| `Res/Blocks/StonePickaxe.block` | 新建 | 石镐物品定义 |
| `Res/Blocks/IronPickaxe.block` | 新建 | 铁镐物品定义 |
| `Res/Blocks/WoodenAxe.block` | 新建 | 木斧物品定义 |
| `Res/Blocks/StoneAxe.block` | 新建 | 石斧物品定义 |
| `Res/Blocks/IronAxe.block` | 新建 | 铁斧物品定义 |
| `Res/Blocks/IronSword.block` | 新建 | 铁剑物品定义 |

---

## 十四、入口7 不包含的内容

- 钻石工具（系统预留 toolTier=4，入口7 不生成、无配方）
- 弓、铁箭矢、蛛丝箭矢（弓在蜘蛛之后，等入口9）
- 熔炉（入口8）
- 耐久系统（保持简单，工具无耐久）
- 蜘蛛怪、动态难度（入口9）
- 体力系统（入口10）
