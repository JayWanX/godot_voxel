# VoxelBlockyModelMesh

继承自：[VoxelBlockyModel](VoxelBlockyModel.md)

基于自定义网格生成模型。

## 描述：

[VoxelMesherBlocky](VoxelMesherBlocky.md) 不要求模型必须是立方体。归根结底，模型的视觉效果都是网格。这是制作模型最灵活的选择。工作流程是在 Blender 等 3D 编辑器中制作这些模型，确保它们被限制在从 (0,0) 到 (1,1) 的盒子内。纹理通过经典的 UV 映射来分配。

## 属性：


类型                                                                        | 名称                                                         | 默认值   
------------------------------------------------------------------------- | ---------------------------------------------------------- | ------
[Mesh](https://docs.godotengine.org/en/stable/classes/class_mesh.html)    | [mesh](#i_mesh)                                            |       
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)      | [mesh_ortho_rotation_index](#i_mesh_ortho_rotation_index)  | 0     
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)    | [side_cutout_enabled](#i_side_cutout_enabled)              | false 
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)  | [side_vertex_tolerance](#i_side_vertex_tolerance)          | 0.001 
<p></p>

## 属性描述

### [Mesh](https://docs.godotengine.org/en/stable/classes/class_mesh.html)<span id="i_mesh"></span> **mesh**

将用于此模型的网格。不能为空（如果你想要空模型，请使用 [VoxelBlockyModelEmpty](VoxelBlockyModelEmpty.md)）。如果它有超过 2 个表面（2 个材质），后续的表面将被忽略。表面必须由三角形组成。必须是索引网格（如果你使用 [SurfaceTool](https://docs.godotengine.org/en/stable/classes/class_surfacetool.html) 生成网格，请使用 [SurfaceTool.index](https://docs.godotengine.org/en/stable/classes/class_surfacetool.html#class-surfacetool-method-index)）。理想情况下，几何体应包含在 0..1 的区域内。只有位于该立方体区域各面上的三角形才会被考虑用于相邻面的剔除（另见 [side_vertex_tolerance](VoxelBlockyModelMesh.md#i_side_vertex_tolerance)）。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_mesh_ortho_rotation_index"></span> **mesh_ortho_rotation_index** = 0

烘焙时应用到网格的正交旋转。取值遵循与 [GridMap](https://docs.godotengine.org/en/stable/classes/class_gridmap.html) 瓦片相同的约定。

（[GridMap](https://docs.godotengine.org/en/stable/classes/class_gridmap.html) 提供了从 [Basis](https://docs.godotengine.org/en/stable/classes/class_basis.html) 转换的方法，但遗憾的是它不是一个静态方法，因此需要存在一个 [GridMap](https://docs.godotengine.org/en/stable/classes/class_gridmap.html) 实例。如果将来有需求，可以添加一个辅助方法。）

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_side_cutout_enabled"></span> **side_cutout_enabled** = false

当相邻体素部分覆盖此体素的一面时，该面被遮挡的几何体将被切除。这仅在两面的形状均为矩形时有效。注意，启用此选项实际上可能会产生更多三角形。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_side_vertex_tolerance"></span> **side_vertex_tolerance** = 0.001

低于此边距时，位于体素 6 个面之一附近的三角形将被视为位于该面。通过比较相邻面的三角形来决定各面是否被剔除。

_生成于 2026-08-28_
