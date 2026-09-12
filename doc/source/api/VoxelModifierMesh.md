# VoxelModifierMesh

继承自：[VoxelModifier](VoxelModifier.md)

使用 [VoxelMeshSDF](VoxelMeshSDF.md) 的体素修改器。

## 属性：


类型                                                                        | 名称                       | 默认值 
------------------------------------------------------------------------- | ------------------------ | ----
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)  | [isolevel](#i_isolevel)  | 0.0 
[VoxelMeshSDF](VoxelMeshSDF.md)                                           | [mesh_sdf](#i_mesh_sdf)  |     
<p></p>

## 属性描述

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_isolevel"></span> **isolevel** = 0.0

偏移 SDF 网格的等值面。正值使对象看起来更厚也更平滑，负值则更薄。

### [VoxelMeshSDF](VoxelMeshSDF.md)<span id="i_mesh_sdf"></span> **mesh_sdf**

用于修改器的 SDF 网格。

_生成于 2026-09-12_
