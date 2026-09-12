# VoxelBoxMover

继承自：[RefCounted](https://docs.godotengine.org/en/stable/classes/class_refcounted.html)

## 描述：

工具类，允许仅使用体素 AABB 重现简单的移动-滑动逻辑，类似于 Minecraft 物理。此类只能用于方块风体素。

将其实例存储在脚本的成员变量中，并在 [Node._process](https://docs.godotengine.org/en/stable/classes/class_node.html#class-node-method-process) 或 [Node._physics_process](https://docs.godotengine.org/en/stable/classes/class_node.html#class-node-method-physics-process) 中使用它（它可以在任何你喜欢的地方工作）。

```
var motion = Vector3(0, 0, -10 * delta) # 向前移动
motion = _box_mover.get_motion(get_translation(), motion, aabb, terrain_node)
global_translate(motion)
```

## 方法：


返回值                                                                           | 函数签名                                                                                                                                                                                                                                                                                                                                                                
----------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)          | [get_collision_mask](#i_get_collision_mask) ( ) const                                                                                                                                                                                                                                                                                                               
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)      | [get_max_step_height](#i_get_max_step_height) ( ) const                                                                                                                                                                                                                                                                                                             
[Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html)  | [get_motion](#i_get_motion) ( [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) pos, [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) motion, [AABB](https://docs.godotengine.org/en/stable/classes/class_aabb.html) aabb, [Node](https://docs.godotengine.org/en/stable/classes/class_node.html) terrain )  
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)        | [has_stepped_up](#i_has_stepped_up) ( ) const                                                                                                                                                                                                                                                                                                                       
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)        | [intersects](#i_intersects) ( [AABB](https://docs.godotengine.org/en/stable/classes/class_aabb.html) aabb, [Object](https://docs.godotengine.org/en/stable/classes/class_object.html) terrain ) const                                                                                                                                                               
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)        | [is_step_climbing_enabled](#i_is_step_climbing_enabled) ( ) const                                                                                                                                                                                                                                                                                                   
[void](#)                                                                     | [set_collision_mask](#i_set_collision_mask) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) mask )                                                                                                                                                                                                                                           
[void](#)                                                                     | [set_max_step_height](#i_set_max_step_height) ( [float](https://docs.godotengine.org/en/stable/classes/class_float.html) height )                                                                                                                                                                                                                                   
[void](#)                                                                     | [set_step_climbing_enabled](#i_set_step_climbing_enabled) ( [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html) enabled )                                                                                                                                                                                                                        
<p></p>

## 方法描述

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_get_collision_mask"></span> **get_collision_mask**( ) 

获取用于检测可碰撞体素的碰撞掩码。

此碰撞掩码特定于此碰撞系统，在 [VoxelBlockyModel.collision_mask](VoxelBlockyModel.md#i_collision_mask) 中定义。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_get_max_step_height"></span> **get_max_step_height**( ) 

获取可攀爬的最大台阶高度。

### [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html)<span id="i_get_motion"></span> **get_motion**( [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) pos, [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) motion, [AABB](https://docs.godotengine.org/en/stable/classes/class_aabb.html) aabb, [Node](https://docs.godotengine.org/en/stable/classes/class_node.html) terrain ) 

给定运动向量，返回一个修改后的向量，告诉你应该移动角色多少。这类似于 [KinematicBody.move_and_slide](https://docs.godotengine.org/en/stable/classes/class_kinematicbody.html#class-kinematicbody-method-move-and-slide)，不同之处在于你需要自己应用该移动。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_has_stepped_up"></span> **has_stepped_up**( ) 

启用台阶攀爬时，告知最后一次调用 [get_motion](VoxelBoxMover.md#i_get_motion) 是否发生了攀爬。

攀爬会将运动向量向上修改，使角色吸附在台阶顶部。这可能会对角色控制器代码产生影响，例如将角色视为站在地面上而不是发生了跳跃。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_intersects"></span> **intersects**( [AABB](https://docs.godotengine.org/en/stable/classes/class_aabb.html) aabb, [Object](https://docs.godotengine.org/en/stable/classes/class_object.html) terrain ) 

测试轴对齐盒是否与任何体素碰撞盒相交。

注意：由于浮点精度，当测试的盒与体素边界接触时，你可能会得到误报。如果不希望这样，你可能需要略微缩小 AABB。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_is_step_climbing_enabled"></span> **is_step_climbing_enabled**( ) 

告知是否启用了台阶攀爬。

### [void](#)<span id="i_set_collision_mask"></span> **set_collision_mask**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) mask ) 

设置用于检测可碰撞体素的碰撞掩码。

只有两个掩码之间至少共享一个位的体素才会被检测到。

此碰撞掩码特定于此碰撞系统，在 [VoxelBlockyModel.collision_mask](VoxelBlockyModel.md#i_collision_mask) 中定义。

### [void](#)<span id="i_set_max_step_height"></span> **set_max_step_height**( [float](https://docs.godotengine.org/en/stable/classes/class_float.html) height ) 

设置可像"楼梯"一样攀爬的最大高度。

### [void](#)<span id="i_set_step_climbing_enabled"></span> **set_step_climbing_enabled**( [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html) enabled ) 

启用后，[get_motion](VoxelBoxMover.md#i_get_motion) 将尝试攀爬较小的台阶。这允许实现类似 Minecraft 的楼梯。

_生成于 2026-09-12_
