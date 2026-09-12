# VoxelBlockyModelFluid

继承自：[VoxelBlockyModel](VoxelBlockyModel.md)

表示流体某种特定状态的模型。

## 属性：


类型                                                                    | 名称                 | 默认值 
--------------------------------------------------------------------- | ------------------ | ----
[VoxelBlockyFluid](VoxelBlockyFluid.md)                               | [fluid](#i_fluid)  |     
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)  | [level](#i_level)  | 0   
<p></p>

## 常量：

- <span id="i_MAX_LEVELS"></span>**MAX_LEVELS** = **256** --- 支持的流体液位最大数量。

## 属性描述

### [VoxelBlockyFluid](VoxelBlockyFluid.md)<span id="i_fluid"></span> **fluid**

此模型属于哪种流体。注意，流体资源应在多个模型之间共享，以便使这些模型被识别为该流体的状态。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_level"></span> **level** = 0

流体的液位，通常表示模型包含多少流体。液位应从 0 开始，且必须低于 256。流体可以有多个具有相同液位的模型。最好为每个液位至少定义一个模型（避免缺少液位）。还建议将液位连续的模型分配给连续的库 ID，但这不是必须的。

_生成于 2026-09-12_
