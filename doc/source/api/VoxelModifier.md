# VoxelModifier

继承自：[Node3D](https://docs.godotengine.org/en/stable/classes/class_node3d.html)

派生：[VoxelModifierMesh](VoxelModifierMesh.md), [VoxelModifierSphere](VoxelModifierSphere.md)

体素修改器的基类。

## 描述：

修改器旨在作为地形生成器的扩展，以非破坏性的方式影响有限的体积。可与其他修改器叠加。来自 [VoxelTool](VoxelTool.md) 的运行时编辑会覆盖修改器的值。

注意 1：仅适用于 [VoxelLodTerrain](VoxelLodTerrain.md)。

注意 2：仅适用于平滑地形（SDF）。

## 属性：


类型                                                                        | 名称                           | 默认值               
------------------------------------------------------------------------- | ---------------------------- | ------------------
[Operation](VoxelModifier.md#enumerations)                                | [operation](#i_operation)    | OPERATION_ADD (0) 
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)  | [smoothness](#i_smoothness)  | 0.0               
<p></p>

## 枚举：<span id="enumerations"></span>

枚举 **Operation**：

- <span id="i_OPERATION_ADD"></span>**OPERATION_ADD** = **0** --- 执行 SDF 并集。
- <span id="i_OPERATION_REMOVE"></span>**OPERATION_REMOVE** = **1** --- 执行 SDF 差集。


## 属性描述

### [Operation](VoxelModifier.md#enumerations)<span id="i_operation"></span> **operation** = OPERATION_ADD (0)

修改器对地形或其他修改器执行的操作。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_smoothness"></span> **smoothness** = 0.0

增大此值会使形状在或大或小的距离上与周围环境"融合"。

注意，它假定基础生成器产生一致的梯度。但情况并非总是如此。值得注意的是，生成器普遍采用一种优化方式，即避免计算距表面超过一定距离的梯度。如果平滑度过大，或生成器的截止距离过低，可能会导致生成的网格出现缝隙，通常在区块边界处。

_生成于 2026-08-28_
