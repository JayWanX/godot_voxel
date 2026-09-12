# VoxelGeneratorMultipassCB

继承自：[VoxelGenerator](VoxelGenerator.md)

基于脚本、按数据块列和多遍处理工作的生成器。

## 描述：

此生成器可以用脚本实现，以数据块列的形式生成地形，而不仅仅是一个区块接一个区块地生成。

它允许使用多遍处理，每一遍都可以访问之前各遍的结果，并允许访问相邻列。

列的高度不是无限的，但可以通过每区块单遍的兜底方式定义上方和下方生成什么。

它只能与 [VoxelTerrain](VoxelTerrain.md) 一起使用。

## 属性：


类型                                                                    | 名称                                               | 默认值 
--------------------------------------------------------------------- | ------------------------------------------------ | ----
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)  | [column_base_y_blocks](#i_column_base_y_blocks)  | -4  
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)  | [column_height_blocks](#i_column_height_blocks)  | 8   
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)  | [pass_count](#i_pass_count)                      | 1   
<p></p>

## 方法：


返回值                                                                                       | 函数签名                                                                                                                                                                                                                  
----------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
[void](#)                                                                                 | [_generate_block_fallback](#i__generate_block_fallback) ( [VoxelBuffer](VoxelBuffer.md) out_buffer, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) origin_in_voxels ) virtual         
[void](#)                                                                                 | [_generate_pass](#i__generate_pass) ( [VoxelToolMultipassGenerator](VoxelToolMultipassGenerator.md) voxel_tool, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) pass_index ) virtual             
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)                      | [_get_used_channels_mask](#i__get_used_channels_mask) ( ) virtual const                                                                                                                                               
[VoxelBuffer[]](https://docs.godotengine.org/en/stable/classes/class_voxelbuffer[].html)  | [debug_generate_test_column](#i_debug_generate_test_column) ( [Vector2i](https://docs.godotengine.org/en/stable/classes/class_vector2i.html) column_position_blocks )                                                 
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)                      | [get_pass_extent_blocks](#i_get_pass_extent_blocks) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) pass_index ) const                                                                         
[void](#)                                                                                 | [set_pass_extent_blocks](#i_set_pass_extent_blocks) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) pass_index, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) extent )  
<p></p>

## 常量：

- <span id="i_MAX_PASSES"></span>**MAX_PASSES** = **4**
- <span id="i_MAX_PASS_EXTENT"></span>**MAX_PASS_EXTENT** = **2**

## 属性描述

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_column_base_y_blocks"></span> **column_base_y_blocks** = -4

列的最低高度，以区块为单位。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_column_height_blocks"></span> **column_height_blocks** = 8

列的高度，以区块为单位。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_pass_count"></span> **pass_count** = 1

列在被视为完全生成之前需要经历的遍数。更多的遍数会增加内存和处理开销。

## 方法描述

### [void](#)<span id="i__generate_block_fallback"></span> **_generate_block_fallback**( [VoxelBuffer](VoxelBuffer.md) out_buffer, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) origin_in_voxels ) 

为基于列的区域上方或下方的每个区块调用。例如，你可以决定在上方生成空气，在下方生成基岩。

### [void](#)<span id="i__generate_pass"></span> **_generate_pass**( [VoxelToolMultipassGenerator](VoxelToolMultipassGenerator.md) voxel_tool, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) pass_index ) 

对每个数据块列，每一遍调用一次。

必须使用传入的 `voxel_tool` 获取要生成区域的信息，并用体素填充/编辑该区域。重要：不要将该对象保存在成员变量中以便后续复用。你只能在本次调用此方法时使用它。

你可以使用 `pass_index` 在每一遍中执行不同的操作。例如，0 可以是使用 Perlin 噪声的基础地面，1 可以种植树木和其他结构。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i__get_used_channels_mask"></span> **_get_used_channels_mask**( ) 

使用此方法指示你的生成器将使用哪些通道。它返回一个位掩码，例如你可以提供这样的信息：`(1 << channel1) | (1 << channel2)`

### [VoxelBuffer[]](https://docs.godotengine.org/en/stable/classes/class_voxelbuffer[].html)<span id="i_debug_generate_test_column"></span> **debug_generate_test_column**( [Vector2i](https://docs.godotengine.org/en/stable/classes/class_vector2i.html) column_position_blocks ) 

测试方法，将完整生成特定列的所有区块并返回它们。

此函数不使用任何线程，也不使用内部缓存，因此会非常慢。不过，它可以让你更轻松地测试或调试脚本，例如使用一个独立的场景。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_get_pass_extent_blocks"></span> **get_pass_extent_blocks**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) pass_index ) 

获取某一遍可以访问其周围多少个区块（注意：默认情况下一个区块为 16x16x16 个体素）。

### [void](#)<span id="i_set_pass_extent_blocks"></span> **set_pass_extent_blocks**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) pass_index, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) extent ) 

设置某一遍在列生成时可以访问其周围多少个区块（注意：默认情况下一个区块为 16x16x16 个体素）。

按设计，第一遍不允许访问相邻区块，因此它将保持为 0。

后续各遍设计为至少可以访问 1 个区块的距离。此类遍不支持 0，因为那等同于将你的逻辑直接放在上一遍中。

增加范围也会增加生成器的开销，无论是内存还是处理时间，因此应谨慎权衡。

_生成于 2026-09-12_
