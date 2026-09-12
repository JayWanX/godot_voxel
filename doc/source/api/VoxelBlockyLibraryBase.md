# VoxelBlockyLibraryBase

继承自：[Resource](https://docs.godotengine.org/en/stable/classes/class_resource.html)

派生：[VoxelBlockyLibrary](VoxelBlockyLibrary.md), [VoxelBlockyTypeLibrary](VoxelBlockyTypeLibrary.md)

包含一组可供 [VoxelMesherBlocky](VoxelMesherBlocky.md) 使用的模型列表。

## 描述：

[VoxelMesherBlocky](VoxelMesherBlocky.md) 使用的模型必须先烘焙，才能在运行时高效使用。此过程的执行方式取决于该类的实现。它可以是简单的模型列表，也可以是生成变体模型的高级类型列表。请查看子类以了解更多信息。

## 属性：


类型                                                                      | 名称                                 | 默认值  
----------------------------------------------------------------------- | ---------------------------------- | -----
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)  | [bake_tangents](#i_bake_tangents)  | true 
<p></p>

## 方法：


返回值                                                                                 | 函数签名                                        
----------------------------------------------------------------------------------- | --------------------------------------------
[void](#)                                                                           | [bake](#i_bake) ( )                         
[Material[]](https://docs.godotengine.org/en/stable/classes/class_material[].html)  | [get_materials](#i_get_materials) ( ) const 
<p></p>

## 常量：

- <span id="i_MAX_MODELS"></span>**MAX_MODELS** = **65536**
- <span id="i_MAX_MATERIALS"></span>**MAX_MATERIALS** = **65536**

## 属性描述

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_bake_tangents"></span> **bake_tangents** = true

如果你需要体素上的法线贴图，请启用此选项。如果不需要，禁用可以降低内存占用并获得少量性能提升。

## 方法描述

### [void](#)<span id="i_bake"></span> **bake**( ) 

烘焙库。模型数据会被优化，以便在生成体素网格时更高效地合并它们。

### [Material[]](https://docs.godotengine.org/en/stable/classes/class_material[].html)<span id="i_get_materials"></span> **get_materials**( ) 

获取库中所有模型里全部不同材质的列表。

注意，如果至少一个非空模型没有材质，此列表中将会有一个 `null` 条目来表示“默认材质”。

_生成于 2026-09-12_
