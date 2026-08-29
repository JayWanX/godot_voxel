# VoxelStream

继承自：[Resource](https://docs.godotengine.org/en/stable/classes/class_resource.html)

派生：[VoxelStreamMemory](VoxelStreamMemory.md), [VoxelStreamRegionFiles](VoxelStreamRegionFiles.md), [VoxelStreamSQLite](VoxelStreamSQLite.md), [VoxelStreamScript](VoxelStreamScript.md)

实现体素数据块的加载和保存，主要使用文件。

## 属性：


类型                                                                      | 名称                                                 | 默认值                 
----------------------------------------------------------------------- | -------------------------------------------------- | --------------------
[Compression](VoxelBlockSerializer.md#enumerations)                     | [compression_mode](#i_compression_mode)            | COMPRESSION_LZ4 (1) 
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)  | [save_generator_output](#i_save_generator_output)  | false               
<p></p>

## 方法：


返回值                                                                           | 函数签名                                                                                                                                                                                                                                                                 
----------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
[void](#)                                                                     | [flush](#i_flush) ( )                                                                                                                                                                                                                                                
[Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html)  | [get_block_size](#i_get_block_size) ( ) const                                                                                                                                                                                                                        
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)          | [get_used_channels_mask](#i_get_used_channels_mask) ( ) const                                                                                                                                                                                                        
[ResultCode](VoxelStream.md#enumerations)                                     | [load_voxel_block](#i_load_voxel_block) ( [VoxelBuffer](VoxelBuffer.md) out_buffer, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) block_position, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) lod_index )  
[void](#)                                                                     | [save_voxel_block](#i_save_voxel_block) ( [VoxelBuffer](VoxelBuffer.md) buffer, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) block_position, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) lod_index )      
<p></p>

## 枚举：<span id="enumerations"></span>

枚举 **ResultCode**：

- <span id="i_RESULT_ERROR"></span>**RESULT_ERROR** = **0** --- 加载数据块时发生错误。请求将被中止。
- <span id="i_RESULT_BLOCK_FOUND"></span>**RESULT_BLOCK_FOUND** = **2** --- 找到该数据块。
- <span id="i_RESULT_BLOCK_NOT_FOUND"></span>**RESULT_BLOCK_NOT_FOUND** = **1** --- 未找到该数据块。如果有生成器，请求方可以回退使用生成器。


## 属性描述

### [Compression](VoxelBlockSerializer.md#enumerations)<span id="i_compression_mode"></span> **compression_mode** = COMPRESSION_LZ4 (1)

指定保存数据块时使用哪种压缩算法。这可以减小存档文件的大小，但会牺牲保存/加载性能。

先前使用不同压缩模式的现有数据块仍然可以加载，如果再次保存，将使用新模式。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_save_generator_output"></span> **save_generator_output** = false

启用此选项后，如果在数据流中找不到某个数据块且它被生成，则生成的数据块将立即保存到数据流中。如果生成器过于昂贵而无法即时运行（就像 Minecraft 所做的那样），可以使用此选项，但它会占用更多磁盘空间（I/O 次数和空间量）并增加网络流量。如果关闭此设置，则只保存被修改的数据块。

## 方法描述

### [void](#)<span id="i_flush"></span> **flush**( ) 

强制将缓存的数据保存到文件系统。某些数据流可能使用缓存来提高频繁 I/O 的性能。

如果在意性能，不应频繁调用此方法。当你需要立即写入所有数据时可以使用。注意，当资源被销毁或其配置更改时，实现应当已经自动完成此操作。某些没有缓存的实现可能什么都不做。

注意，地形是异步保存的，因此如果保存任务仍在排队且尚未调用 [VoxelStream](VoxelStream.md)，刷新可能并不总能达成你的目标。参见 [VoxelTerrain.save_modified_blocks](VoxelTerrain.md#i_save_modified_blocks) 或 [VoxelLodTerrain.save_modified_blocks](VoxelLodTerrain.md#i_save_modified_blocks)。

### [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html)<span id="i_get_block_size"></span> **get_block_size**( ) 

*(此方法暂无文档)*

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_get_used_channels_mask"></span> **get_used_channels_mask**( ) 

*(此方法暂无文档)*

### [ResultCode](VoxelStream.md#enumerations)<span id="i_load_voxel_block"></span> **load_voxel_block**( [VoxelBuffer](VoxelBuffer.md) out_buffer, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) block_position, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) lod_index ) 

`out_buffer`：要加载的体素数据块。必须是预先创建的实例（不能为 null）。

`block_position`：指定 LOD 内以数据块坐标表示的区块位置。

### [void](#)<span id="i_save_voxel_block"></span> **save_voxel_block**( [VoxelBuffer](VoxelBuffer.md) buffer, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) block_position, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) lod_index ) 

`buffer`：要保存的体素数据块。强烈建议之后不要保留该数据的引用，因为数据流允许缓存它，而且保存的数据必须表示快照（副本），或在其所属体积被销毁后对该数据的最后引用。

`block_position`：指定 LOD 内以数据块坐标表示的区块位置。

_生成于 2026-08-28_
