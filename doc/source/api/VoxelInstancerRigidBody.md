# VoxelInstancerRigidBody

继承自：[RigidBody3D](https://docs.godotengine.org/en/stable/classes/class_rigidbody3d.html)

为 [VoxelInstancer](VoxelInstancer.md) 创建的每个可碰撞多网格实例生成的碰撞节点。

## 描述：

从 [VoxelInstanceLibraryMultiMeshItem](VoxelInstanceLibraryMultiMeshItem.md) 生成的实例不使用节点进行渲染。但是，它们可以以使用此类的刚体节点的形式获得碰撞。

在此节点的实例上调用 `queue_free()` 也会将该实例从 [VoxelInstancer](VoxelInstancer.md) 中注销。

## 方法：


返回值                                                                   | 函数签名                                                                       
--------------------------------------------------------------------- | ---------------------------------------------------------------------------
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)  | [get_library_item_id](#i_get_library_item_id) ( ) const                    
[void](#)                                                             | [queue_free_and_notify_instancer](#i_queue_free_and_notify_instancer) ( )  
<p></p>

## 方法描述

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_get_library_item_id"></span> **get_library_item_id**( ) 

获取用于创建拥有此碰撞体的实例的、实例化器 [VoxelInstanceLibrary](VoxelInstanceLibrary.md) 中条目的 ID。

### [void](#)<span id="i_queue_free_and_notify_instancer"></span> **queue_free_and_notify_instancer**( ) 

`queue_free` 的替代方法，用于你不想在 [VoxelInstanceLibraryMultiMeshItem._on_instance_removed](VoxelInstanceLibraryMultiMeshItem.md#i__on_instance_removed) 中使用 `call_deferred` 在 [VoxelInstancer](VoxelInstancer.md) 下添加节点的情况。

_生成于 2026-09-12_
