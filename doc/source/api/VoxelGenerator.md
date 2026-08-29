# VoxelGenerator

继承自：[Resource](https://docs.godotengine.org/en/stable/classes/class_resource.html)

派生：[VoxelGeneratorFlat](VoxelGeneratorFlat.md), [VoxelGeneratorGraph](VoxelGeneratorGraph.md), [VoxelGeneratorHeightmap](VoxelGeneratorHeightmap.md), [VoxelGeneratorMultipassCB](VoxelGeneratorMultipassCB.md), [VoxelGeneratorNoise](VoxelGeneratorNoise.md), [VoxelGeneratorScript](VoxelGeneratorScript.md)

所有体素程序化生成器的基类。

## 方法：


返回值        | 函数签名                                                                                                                                                                                                                                                       
---------- | -----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
[void](#)  | [generate_block](#i_generate_block) ( [VoxelBuffer](VoxelBuffer.md) out_buffer, [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) origin_in_voxels, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) lod )  
<p></p>

## 方法描述

### [void](#)<span id="i_generate_block"></span> **generate_block**( [VoxelBuffer](VoxelBuffer.md) out_buffer, [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) origin_in_voxels, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) lod ) 

在指定的世界区域内生成一块体素。

`out_buffer`：用于生成体素数据的缓冲区。它不能为 `null`，且必须具有非空的尺寸。

`origin_in_voxels`：要生成的盒体下角坐标，相对于 LOD0。

`lod`：用于此数据块的细节级别索引。某些生成器可能不支持 LOD，此时可将其保持为 0。在 LOD 0 时，传入缓冲区的每个单元格跨越 1 个空间单位；LOD 1 时为 2 个单位；LOD 2 时为 4 个单位，依此类推。

_生成于 2026-08-28_
