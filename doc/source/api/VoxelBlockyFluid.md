# VoxelBlockyFluid

继承自：[Resource](https://docs.godotengine.org/en/stable/classes/class_resource.html)

方块风流体的属性。

## 描述：

某种特定流体的通用属性。它可以在多个方块风模型之间共享，每个模型代表流体的一种液位/状态。

## 属性：


类型                                                                              | 名称                                                 | 默认值   
------------------------------------------------------------------------------- | -------------------------------------------------- | ------
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)          | [dip_when_flowing_down](#i_dip_when_flowing_down)  | false 
[Material](https://docs.godotengine.org/en/stable/classes/class_material.html)  | [material](#i_material)                            |       
<p></p>

## 属性描述

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_dip_when_flowing_down"></span> **dip_when_flowing_down** = false

启用后，如果给定流体系体素满足以下所有条件：

- 未达到最大液位

- 未被同类型流体的其他体素覆盖

- 可以向下流动（下方为空气或同类型流体）

则该体素的形状将变为“被向下挤压”的形状，从而形成更陡峭的坡度。注意，这也会导致体素在某些情况下看起来像是处于最低液位。不过在实践中，这些情况并不常发生。你可以根据流体的模拟方式来决定是否使用此选项。

### [Material](https://docs.godotengine.org/en/stable/classes/class_material.html)<span id="i_material"></span> **material**

流体所有状态所使用的材质。注意，流体的 UV 与普通模型不同，因此你可能需要一个 [ShaderMaterial](https://docs.godotengine.org/en/stable/classes/class_shadermaterial.html) 来处理流动动画。参见 [https://voxel-tools.readthedocs.io/en/latest/blocky_terrain/#fluids](https://voxel-tools.readthedocs.io/en/latest/blocky_terrain/#fluids)

_生成于 2026-09-12_
