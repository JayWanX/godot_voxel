# VoxelStreamRegionFiles

继承自：[VoxelStream](VoxelStream.md)

在目录下，将数据块加载和保存到按世界位置索引的区域文件中。

## 描述：

在目录下，将数据块加载和保存到文件系统中多个按世界位置索引的区域文件中。区域会将许多数据块打包在一起，从而减少文件切换并提高性能。灵感来自 [Seed of Andromeda](https://www.seedofandromeda.com/blogs/1-creating-a-region-file-system-for-a-voxel-game) 和 Minecraft。

区域文件不是线程安全的。因此，内部互斥锁可能常常将使用限制为仅单线程。

## 属性：


类型                                                                          | 名称                                     | 默认值 
--------------------------------------------------------------------------- | -------------------------------------- | ----
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)        | [block_size_po2](#i_block_size_po2)    | 4   
[String](https://docs.godotengine.org/en/stable/classes/class_string.html)  | [directory](#i_directory)              | ""  
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)        | [region_size_po2](#i_region_size_po2)  | 4   
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)        | [sector_size](#i_sector_size)          | 512 
<p></p>

## 方法：


返回值                                                                           | 函数签名                                                                                                                                   
----------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------
[void](#)                                                                     | [convert_files](#i_convert_files) ( [Dictionary](https://docs.godotengine.org/en/stable/classes/class_dictionary.html) new_settings )  
[Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html)  | [get_region_size](#i_get_region_size) ( ) const                                                                                        
<p></p>

## 属性描述

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_block_size_po2"></span> **block_size_po2** = 4

数据块大小的对数值。

### [String](https://docs.godotengine.org/en/stable/classes/class_string.html)<span id="i_directory"></span> **directory** = ""

保存数据的目录。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_region_size_po2"></span> **region_size_po2** = 4

区域大小的对数值。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_sector_size"></span> **sector_size** = 512

区域文件中的扇区大小（字节）。

## 方法描述

### [void](#)<span id="i_convert_files"></span> **convert_files**( [Dictionary](https://docs.godotengine.org/en/stable/classes/class_dictionary.html) new_settings ) 

将旧格式的区域文件转换为新元数据格式。

### [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html)<span id="i_get_region_size"></span> **get_region_size**( ) 

获取区域大小（以数据块数为单位）。

_生成于 2026-09-12_
