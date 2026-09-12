# VoxelMesher

继承自：[Resource](https://docs.godotengine.org/en/stable/classes/class_resource.html)

派生：[VoxelMesherBlocky](VoxelMesherBlocky.md), [VoxelMesherCubes](VoxelMesherCubes.md), [VoxelMesherTransvoxel](VoxelMesherTransvoxel.md)

所有网格生成算法的基类。

## 描述：

为了让 Godot 能够渲染体素，可以将体素转换为网格。实现这一目标有多种方式，因此此类只是其他专用类的基类。体素节点会自动使用网格生成器，但你也可以手动生成网格。为此，你可以使用其中一个派生类。网格生成器可以被重复使用，这通常可以通过减少内存分配来获得更好的性能。

## 方法：


返回值                                                                     | 函数签名                                                                                                                                                                                                                                                                             
----------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
[Mesh](https://docs.godotengine.org/en/stable/classes/class_mesh.html)  | [build_mesh](#i_build_mesh) ( [VoxelBuffer](VoxelBuffer.md) voxel_buffer, [Material[]](https://docs.godotengine.org/en/stable/classes/class_material[].html) materials, [Dictionary](https://docs.godotengine.org/en/stable/classes/class_dictionary.html) additional_data={} )  
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)    | [get_maximum_padding](#i_get_maximum_padding) ( ) const                                                                                                                                                                                                                          
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)    | [get_minimum_padding](#i_get_minimum_padding) ( ) const                                                                                                                                                                                                                          
<p></p>

## 方法描述

### [Mesh](https://docs.godotengine.org/en/stable/classes/class_mesh.html)<span id="i_build_mesh"></span> **build_mesh**( [VoxelBuffer](VoxelBuffer.md) voxel_buffer, [Material[]](https://docs.godotengine.org/en/stable/classes/class_material[].html) materials, [Dictionary](https://docs.godotengine.org/en/stable/classes/class_dictionary.html) additional_data={} ) 

根据提供的体素构建网格。材质将根据提供的数组附加到每个表面。材质的使用方式可能取决于网格生成器的类型。

网格生成器最初是为数据块设计的，因此缓冲区外侧边距内的体素不会成为结果的一部分。它们被视为“相邻体素”，可能最终会影响面剔除。如果你想使用网格生成器制作独立的体素网格，请确保其周围用空气填充。该边距的大小由 [get_minimum_padding](VoxelMesher.md#i_get_minimum_padding) 和 [get_maximum_padding](VoxelMesher.md#i_get_maximum_padding) 确定。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_get_maximum_padding"></span> **get_maximum_padding**( ) 

获取网格生成器正常工作前，体素在其下角之前需要填充多少。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_get_minimum_padding"></span> **get_minimum_padding**( ) 

获取网格生成器正常工作前，体素在其上角之后需要填充多少。

_生成于 2026-09-12_
