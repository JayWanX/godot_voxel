# VoxelVoxLoader

继承自：[RefCounted](https://docs.godotengine.org/en/stable/classes/class_refcounted.html)

用于从 MagicaVoxel 文件加载体素的工具类。

## 方法：


返回值                                                                   | 函数签名                                                                                                                                                                                                                                                                                             
--------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)  | [load_from_file](#i_load_from_file) ( [String](https://docs.godotengine.org/en/stable/classes/class_string.html) fpath, [VoxelBuffer](VoxelBuffer.md) voxels, [VoxelColorPalette](VoxelColorPalette.md) palette, [ChannelId](VoxelBuffer.md#enumerations) dst_channel=CHANNEL_COLOR (2) ) static 
<p></p>

## 方法描述

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_load_from_file"></span> **load_from_file**( [String](https://docs.godotengine.org/en/stable/classes/class_string.html) fpath, [VoxelBuffer](VoxelBuffer.md) voxels, [VoxelColorPalette](VoxelColorPalette.md) palette, [ChannelId](VoxelBuffer.md#enumerations) dst_channel=CHANNEL_COLOR (2) ) 

从 vox 文件中找到的第一个模型加载体素，并将其存储到单个 [VoxelBuffer](VoxelBuffer.md) 中。文件中可能存在的其他模型不会被加载。模型变换不会被考虑。

数据将存储到所提供缓冲区的一个通道中。不支持 64 位深度通道。

如果提供了调色板，它还将从文件中加载调色板，体素将是指向该调色板的索引。8 位通道深度是最优的（因为 MagicaVoxel 体素使用 8 位索引），但 16 位和 32 位也受支持。

如果未提供调色板（null），加载器将尝试根据所提供缓冲区的深度，将颜色按位打包直接存储到体素中（8 位时为每分量 2 位，16 位时为每分量 4 位，32 位时为每分量 8 位）。

返回一个 [Error](https://docs.godotengine.org/en/stable/classes/class_error.html) 枚举代码，以告知加载是否成功。

注意：MagicaVoxel 使用的轴约定与 Godot 不同：X 向右，Y 向前，Z 向上。在缓冲区中查找时体素坐标是相同的，但它们代表空间中的不同位置。

_生成于 2026-09-12_
