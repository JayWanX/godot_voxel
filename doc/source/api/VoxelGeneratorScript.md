# VoxelGeneratorScript

继承自：[VoxelGenerator](VoxelGenerator.md)

使用脚本定义的自定义生成器的基类。

## 描述：

重要：此引擎大量使用线程。生成器将在其中一个线程中运行，因此请确保不要在生成器内部访问场景树或其他不安全的 API。

## 方法：


返回值                                                                   | 函数签名                                                                                                                                                                                                                                                                  
--------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
[void](#)                                                             | [_generate_block](#i__generate_block) ( [VoxelBuffer](VoxelBuffer.md) out_buffer, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) origin_in_voxels, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) lod ) virtual 
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)  | [_get_used_channels_mask](#i__get_used_channels_mask) ( ) virtual const                                                                                                                                                                                               
<p></p>

## 方法描述

### [void](#)<span id="i__generate_block"></span> **_generate_block**( [VoxelBuffer](VoxelBuffer.md) out_buffer, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) origin_in_voxels, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) lod ) 

`out_buffer`：用于填充体素数据的缓冲区。它永远不会是 `null`，并且具有请求的大小。它仅在此函数内有效，切勿在函数结束后将其存储在任何地方。注意：此缓冲区可以具有任何非空尺寸，但根据你使用的地形节点，可以做一些假设。[VoxelTerrain](VoxelTerrain.md) 总是请求 16x16x16 大小的区块，但 [VoxelLodTerrain](VoxelLodTerrain.md) 可以请求不同大小的区块。

`origin_in_voxels`：要生成的盒体下角坐标，相对于 LOD0。盒体的大小可从 `out_buffer` 得知。

`lod`：用于此区块的细节级别索引。如果不使用 LOD 可以忽略它。它可以用作 2 的幂，表示一个体素有多大。例如，如果你使用循环配合噪声填充缓冲区，应从 `origin_in_voxels` 开始以 2^lod 的步长采样噪声（在代码中你可以使用 `1 << lod` 进行快速计算，而不是 `pow(2, lod)`）。你可能希望将迭代 `out_buffer` 中坐标的变量与用于在空间中生成体素值的变量分开。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i__get_used_channels_mask"></span> **_get_used_channels_mask**( ) 

使用此方法指示你的生成器将使用哪些通道。它返回一个位掩码，例如你可以提供这样的信息：`(1 << channel1) | (1 << channel2)`

_生成于 2026-09-12_
