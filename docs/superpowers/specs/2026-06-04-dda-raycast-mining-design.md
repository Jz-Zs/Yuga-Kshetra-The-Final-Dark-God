# DDA 射线检测 + 挖掘目标跟随修复

## 问题

当前射线检测使用"点采样"（每 0.05 单位取一点 + `std::floor`），在方块边界处存在浮点精度问题，导致挖掘到准星未选中的方块。此外，挖掘目标在首次点击后锁定，按住鼠标期间移动准星不会更新。

## 方案

1. 用 **Amanatides-Woo DDA 体素遍历** 替代点采样 Ray
2. 挖掘目标**每帧跟随准星**重新评估

---

## DDA 算法原理

按射线前进顺序，逐个访问穿过的每个体素。不采样点，而是计算到达各轴上**下一个体素边界**的距离，选择最近的边界跨越。

```
点采样: 玩家 → ···→·→·→·→ █    (可能跳过/误判)
DDA:    玩家 → ║→ ║→ ║→ █    (精确按序访问每个体素)
```

**初始化：**
- `voxel = floor(origin)`
- `step[i] = sign(dir[i])`
- `tMax[i] =` 从原点到第 i 轴下一个体素边界的距离
- `tDelta[i] = 1.0 / abs(dir[i])`（跨一个体素的距离）

**循环：**
- 选 `tMax` 最小的轴 → 该轴体素坐标 +step
- `tMax[axis] += tDelta[axis]`
- 报告新体素坐标

---

## 新 Ray API

```cpp
class Ray {
public:
    Ray(const glm::vec3 &origin, const glm::vec3 &direction);
    Ray(const glm::vec3 &origin, const glm::vec3 &rotation, bool fromRotation);

    bool advance();                    // 前进到下一个体素
    glm::ivec3 currentVoxel() const;   // 当前所在体素坐标
    const glm::vec3& getEnd() const;   // 当前位置（体素边界交点）
    float getLength() const;           // 已行进距离

private:
    glm::vec3 m_origin, m_dir;
    glm::ivec3 m_voxel, m_step;
    glm::vec3 m_tMax, m_tDelta;
    float m_length;
    glm::vec3 m_end;
};
```

### 方向向量计算

用标准球坐标替代当前的 `tan(pitch)` 方式，直接产生单位向量：

```cpp
// rotation = {pitch, yaw, _}
float yr = glm::radians(rotation.y);
float pr = glm::radians(rotation.x);
glm::vec3 dir(
     glm::sin(yr) * glm::cos(pr),
    -glm::sin(pr),
    -glm::cos(yr) * glm::cos(pr)
);
```

已验证与当前 step() 对所有旋转值的行为一致。

### 边界处理

- `dir[i] == 0`：`tDelta[i] = INFINITY`，该轴永不被选中
- 起点在方块内部：首次 `advance()` 仍正确工作
- 几乎平行于面：INFINITY 机制天然处理

---

## Application.cpp 改动

将全部 4 处射线检测改为 DDA：

| 位置 | 用途 | 改动 |
|------|------|------|
| L149 实体攻击 | 猪人/蜘蛛命中检测 | step() → advance() |
| L266 挖掘目标 | 左键首次命中方块 | step() → advance() |
| L342 连续挖掘 | 挖完后找下一个方块 | step() → advance() |
| L389 方块放置 | 右键找到放置位置 | step() → advance() |

### 挖掘目标跟随（核心修复）

将目标获取从 `if (leftClicked)` 中移出，改为每帧在 `leftPressed` 时执行：

```
每帧（leftPressed 且未命中实体）:
    射线检测 → 找到准星下的方块 target
    if (target != m_miningTarget):
        m_miningProgress = 0    // 重置进度
        m_miningTarget = target
    m_isMining = true
```

### 方块放置

DDA 下，命中实体方块时，放置位置为**上一个体素**（即射线进入方块前的空气位置），该位置可直接传给 PlayerDigEvent。

---

## 涉及文件

| 文件 | 改动 |
|------|------|
| `Maths/Ray.h` | 重写：DDA API |
| `Maths/Ray.cpp` | 重写：DDA 实现 |
| `Application.cpp` | 4 处射线循环 + 挖掘目标跟随修复 |
| `Player.h` | 无需改动 |

---

## 测试要点

1. 贴近方块挖掘 — 准星上方块、挖到上方块
2. 方块边界处（接缝）挖掘 — 无错位
3. 按住鼠标移动准星 — 目标跟随、进度重置
4. 连续挖掘（按住挖穿一排方块）— 正常运行
5. 实体（猪人/蜘蛛）攻击 — 命中判定不变
6. 右键放置方块 — 位置正确
7. 极端角度（俯视/仰视近 90°）— 无崩溃
