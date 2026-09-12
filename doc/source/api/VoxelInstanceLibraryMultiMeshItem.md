# VoxelInstanceLibraryMultiMeshItem

继承自：[VoxelInstanceLibraryItem](VoxelInstanceLibraryItem.md)

使用 [MultiMesh](https://docs.godotengine.org/en/stable/classes/class_multimesh.html) 渲染的实例化器模型。

## 描述：

该模型适合渲染数量非常庞大的简单实例，例如草和石头。

## 属性：


类型                                                                                                                                           | 名称                                                         | 默认值                                   
-------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------- | --------------------------------------
[PackedFloat32Array](https://docs.godotengine.org/en/stable/classes/class_packedfloat32array.html)                                           | [_mesh_lod_distance_ratios](#i__mesh_lod_distance_ratios)  | PackedFloat32Array(0.2, 0.35, 0.6, 1) 
[ShadowCastingSetting](https://docs.godotengine.org/en/stable/classes/class_renderingserver.html#enum-renderingserver-shadowcastingsetting)  | [cast_shadow](#i_cast_shadow)                              | 1                                     
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)                                                                     | [collision_distance](#i_collision_distance)                | -1.0                                  
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)                                                                         | [collision_layer](#i_collision_layer)                      | 1                                     
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)                                                                         | [collision_mask](#i_collision_mask)                        | 1                                     
[Array](https://docs.godotengine.org/en/stable/classes/class_array.html)                                                                     | [collision_shapes](#i_collision_shapes)                    | []                                    
[GIMode](https://docs.godotengine.org/en/stable/classes/class_geometryinstance3d.html#enum-geometryinstance3d-gimode)                        | [gi_mode](#i_gi_mode)                                      | 1                                     
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)                                                                       | [hide_beyond_max_lod](#i_hide_beyond_max_lod)              | false                                 
[Material](https://docs.godotengine.org/en/stable/classes/class_material.html)                                                               | [material_override](#i_material_override)                  |                                       
[Mesh](https://docs.godotengine.org/en/stable/classes/class_mesh.html)                                                                       | [mesh](#i_mesh)                                            |                                       
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)                                                                     | [mesh_lod0_distance_ratio](#i_mesh_lod0_distance_ratio)    | 0.2                                   
[Mesh](https://docs.godotengine.org/en/stable/classes/class_mesh.html)                                                                       | [mesh_lod1](#i_mesh_lod1)                                  |                                       
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)                                                                     | [mesh_lod1_distance_ratio](#i_mesh_lod1_distance_ratio)    | 0.35                                  
[Mesh](https://docs.godotengine.org/en/stable/classes/class_mesh.html)                                                                       | [mesh_lod2](#i_mesh_lod2)                                  |                                       
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)                                                                     | [mesh_lod2_distance_ratio](#i_mesh_lod2_distance_ratio)    | 0.6                                   
[Mesh](https://docs.godotengine.org/en/stable/classes/class_mesh.html)                                                                       | [mesh_lod3](#i_mesh_lod3)                                  |                                       
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)                                                                     | [mesh_lod3_distance_ratio](#i_mesh_lod3_distance_ratio)    | 1.0                                   
[RemovalBehavior](VoxelInstanceLibraryMultiMeshItem.md#enumerations)                                                                         | [removal_behavior](#i_removal_behavior)                    | REMOVAL_BEHAVIOR_NONE (0)             
[PackedScene](https://docs.godotengine.org/en/stable/classes/class_packedscene.html)                                                         | [removal_scene](#i_removal_scene)                          |                                       
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)                                                                         | [render_layer](#i_render_layer)                            | 1                                     
[PackedScene](https://docs.godotengine.org/en/stable/classes/class_packedscene.html)                                                         | [scene](#i_scene)                                          |                                       
<p></p>

## 方法：


返回值                                                                                     | 函数签名                                                                                                                                                                                                      
--------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
[void](#)                                                                               | [_on_instance_removed](#i__on_instance_removed) ( [VoxelInstancer](VoxelInstancer.md) instancer, [Transform3D](https://docs.godotengine.org/en/stable/classes/class_transform3d.html) transform ) virtual 
[StringName[]](https://docs.godotengine.org/en/stable/classes/class_stringname[].html)  | [get_collider_group_names](#i_get_collider_group_names) ( ) const                                                                                                                                         
[Mesh](https://docs.godotengine.org/en/stable/classes/class_mesh.html)                  | [get_mesh](#i_get_mesh) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) mesh_lod_index ) const                                                                                     
[void](#)                                                                               | [set_collider_group_names](#i_set_collider_group_names) ( [StringName[]](https://docs.godotengine.org/en/stable/classes/class_stringname[].html) names )                                                  
[void](#)                                                                               | [set_mesh](#i_set_mesh) ( [Mesh](https://docs.godotengine.org/en/stable/classes/class_mesh.html) mesh, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) mesh_lod_index )              
[void](#)                                                                               | [setup_from_template](#i_setup_from_template) ( [Node](https://docs.godotengine.org/en/stable/classes/class_node.html) node )                                                                             
<p></p>

## 枚举：<span id="enumerations"></span>

枚举 **RemovalBehavior**：

- <span id="i_REMOVAL_BEHAVIOR_NONE"></span>**REMOVAL_BEHAVIOR_NONE** = **0** --- 实例被移除时不会运行额外的逻辑。
- <span id="i_REMOVAL_BEHAVIOR_INSTANTIATE"></span>**REMOVAL_BEHAVIOR_INSTANTIATE** = **1** --- 为每个被移除的实例实例化 [removal_scene](VoxelInstanceLibraryMultiMeshItem.md#i_removal_scene)。该场景的根必须派生自 [Node3D](https://docs.godotengine.org/en/stable/classes/class_node3d.html)，在添加到场景树之前会被赋予与实例相同的变换。它将被添加到 [VoxelInstancer](VoxelInstancer.md) 节点之下。
- <span id="i_REMOVAL_BEHAVIOR_CALLBACK"></span>**REMOVAL_BEHAVIOR_CALLBACK** = **2** --- 实例被移除时调用 [_on_instance_removed](VoxelInstanceLibraryMultiMeshItem.md#i__on_instance_removed)。要实现该功能，你应当为条目附加一个脚本。 注意：每个资源都可以有 [Object.script](https://docs.godotengine.org/en/stable/classes/class_object.html#class-object-property-script)。但在编辑器中，如果资源出现在子检查器中，Godot 目前不会向你显示该属性。要绕过此限制，请右键单击资源所在的属性，然后选择“编辑”。这将在一个完整的检查器中打开该条目。另一种替代方案是将条目保存为文件，然后从文件浏览器中编辑它。


## 常量：

- <span id="i_MAX_MESH_LODS"></span>**MAX_MESH_LODS** = **4**

## 属性描述

### [PackedFloat32Array](https://docs.godotengine.org/en/stable/classes/class_packedfloat32array.html)<span id="i__mesh_lod_distance_ratios"></span> **_mesh_lod_distance_ratios** = PackedFloat32Array(0.2, 0.35, 0.6, 1)

*(此属性暂无文档)*

### [ShadowCastingSetting](https://docs.godotengine.org/en/stable/classes/class_renderingserver.html#enum-renderingserver-shadowcastingsetting)<span id="i_cast_shadow"></span> **cast_shadow** = 1

阴影投射的设置。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_collision_distance"></span> **collision_distance** = -1.0

当大于 0 且配置了碰撞体时，提示可创建碰撞体的距离下限。这允许在保持较高视觉视距的同时减少碰撞体的数量。

为负数时，将在所有距离创建碰撞体。

注意：实例化器按区块为单位创建/移除碰撞体，因此该距离是与到区块的距离比较，而非与单个实例比较。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_collision_layer"></span> **collision_layer** = 1

生成碰撞体的碰撞层。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_collision_mask"></span> **collision_mask** = 1

生成碰撞体的碰撞掩码。

### [Array](https://docs.godotengine.org/en/stable/classes/class_array.html)<span id="i_collision_shapes"></span> **collision_shapes** = []

由 [CollisionShape](https://docs.godotengine.org/en/stable/classes/class_collisionshape.html) 和 [Transform3D](https://docs.godotengine.org/en/stable/classes/class_transform3d.html) 交替组成的列表。先出现形状，随后是其相对于实例的局部变换。在编辑器中设置碰撞形状时，可能需要改用场景。

### [GIMode](https://docs.godotengine.org/en/stable/classes/class_geometryinstance3d.html#enum-geometryinstance3d-gimode)<span id="i_gi_mode"></span> **gi_mode** = 1

网格的全局光照模式。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_hide_beyond_max_lod"></span> **hide_beyond_max_lod** = false

超出最大 LOD 时是否隐藏网格。

### [Material](https://docs.godotengine.org/en/stable/classes/class_material.html)<span id="i_material_override"></span> **material_override**

网格的材质覆盖。

### [Mesh](https://docs.godotengine.org/en/stable/classes/class_mesh.html)<span id="i_mesh"></span> **mesh**

用于 LOD 0 的网格。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_mesh_lod0_distance_ratio"></span> **mesh_lod0_distance_ratio** = 0.2

LOD 0 网格的切换距离比例。

### [Mesh](https://docs.godotengine.org/en/stable/classes/class_mesh.html)<span id="i_mesh_lod1"></span> **mesh_lod1**

用于 LOD 1 的网格。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_mesh_lod1_distance_ratio"></span> **mesh_lod1_distance_ratio** = 0.35

LOD 1 网格的切换距离比例。

### [Mesh](https://docs.godotengine.org/en/stable/classes/class_mesh.html)<span id="i_mesh_lod2"></span> **mesh_lod2**

用于 LOD 2 的网格。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_mesh_lod2_distance_ratio"></span> **mesh_lod2_distance_ratio** = 0.6

LOD 2 网格的切换距离比例。

### [Mesh](https://docs.godotengine.org/en/stable/classes/class_mesh.html)<span id="i_mesh_lod3"></span> **mesh_lod3**

用于 LOD 3 的网格。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_mesh_lod3_distance_ratio"></span> **mesh_lod3_distance_ratio** = 1.0

LOD 3 网格的切换距离比例。

### [RemovalBehavior](VoxelInstanceLibraryMultiMeshItem.md#enumerations)<span id="i_removal_behavior"></span> **removal_behavior** = REMOVAL_BEHAVIOR_NONE (0)

指定实例被移除时应发生什么。如果它们应当变成包含动画或逻辑的更复杂对象，这会很有用。

### [PackedScene](https://docs.godotengine.org/en/stable/classes/class_packedscene.html)<span id="i_removal_scene"></span> **removal_scene**

当 [removal_behavior](VoxelInstanceLibraryMultiMeshItem.md#i_removal_behavior) 设置为 [REMOVAL_BEHAVIOR_INSTANTIATE](VoxelInstanceLibraryMultiMeshItem.md#i_REMOVAL_BEHAVIOR_INSTANTIATE) 时使用的场景。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_render_layer"></span> **render_layer** = 1

网格的渲染层。

### [PackedScene](https://docs.godotengine.org/en/stable/classes/class_packedscene.html)<span id="i_scene"></span> **scene**

用作配置而非手动属性的场景。它需要具备特定的节点结构才能被支持。参见 [https://voxel-tools.readthedocs.io/en/latest/instancing/#setting-up-a-multimesh-item-from-a-scene](https://voxel-tools.readthedocs.io/en/latest/instancing/#setting-up-a-multimesh-item-from-a-scene)

## 方法描述

### [void](#)<span id="i__on_instance_removed"></span> **_on_instance_removed**( [VoxelInstancer](VoxelInstancer.md) instancer, [Transform3D](https://docs.godotengine.org/en/stable/classes/class_transform3d.html) transform ) 

如果你将 [removal_behavior](VoxelInstanceLibraryMultiMeshItem.md#i_removal_behavior) 设置为 [REMOVAL_BEHAVIOR_CALLBACK](VoxelInstanceLibraryMultiMeshItem.md#i_REMOVAL_BEHAVIOR_CALLBACK)，将调用此方法。

注意：此方法可能在对 [VoxelInstancer](VoxelInstancer.md) 的子节点进行移除时被调用。在此上下文中，Godot 会阻止你添加新的子节点。你可以使用 [Object.call_deferred](https://docs.godotengine.org/en/stable/classes/class_object.html#class-object-method-call-deferred) 来绕过该限制。另请参见 [VoxelInstancerRigidBody.queue_free_and_notify_instancer](VoxelInstancerRigidBody.md#i_queue_free_and_notify_instancer)。

### [StringName[]](https://docs.godotengine.org/en/stable/classes/class_stringname[].html)<span id="i_get_collider_group_names"></span> **get_collider_group_names**( ) 

获取添加到碰撞体节点上的组名列表。

### [Mesh](https://docs.godotengine.org/en/stable/classes/class_mesh.html)<span id="i_get_mesh"></span> **get_mesh**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) mesh_lod_index ) 

获取指定 LOD 索引对应的网格。

### [void](#)<span id="i_set_collider_group_names"></span> **set_collider_group_names**( [StringName[]](https://docs.godotengine.org/en/stable/classes/class_stringname[].html) names ) 

设置将添加到为每个实例生成的碰撞体节点上的组名列表。

### [void](#)<span id="i_set_mesh"></span> **set_mesh**( [Mesh](https://docs.godotengine.org/en/stable/classes/class_mesh.html) mesh, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) mesh_lod_index ) 

设置指定 LOD 索引对应的网格。

### [void](#)<span id="i_setup_from_template"></span> **setup_from_template**( [Node](https://docs.godotengine.org/en/stable/classes/class_node.html) node ) 

从模板节点应用设置。

_生成于 2026-09-12_
