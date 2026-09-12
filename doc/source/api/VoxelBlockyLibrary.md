# VoxelBlockyLibrary

继承自：[VoxelBlockyLibraryBase](VoxelBlockyLibraryBase.md)

包含一组可供 [VoxelMesherBlocky](VoxelMesherBlocky.md) 使用的模型列表。

## 描述：

提供一组可供 [VoxelMesherBlocky](VoxelMesherBlocky.md) 使用的模型列表。每个模型对应体素数据中的一个 ID，通常由网格定义。还可以定义一些额外属性，例如各面如何被相邻体素剔除，或体素引擎的某些功能如何处理它。

如果你通过代码创建此库，最后需要使用 [VoxelBlockyLibraryBase.bake](VoxelBlockyLibraryBase.md#i_bake) 函数进行烘焙。

第一个模型（索引为 0）通常用于“空气”或“空”。

## 属性：


类型                                                                                                  | 名称                   | 默认值 
--------------------------------------------------------------------------------------------------- | -------------------- | ----
[VoxelBlockyModel[]](https://docs.godotengine.org/en/stable/classes/class_voxelblockymodel[].html)  | [models](#i_models)  | []  
<p></p>

## 方法：


返回值                                                                   | 函数签名                                                                                                                                                                  
--------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)  | [add_model](#i_add_model) ( [VoxelBlockyModel](VoxelBlockyModel.md) model )                                                                                           
[VoxelBlockyModel](VoxelBlockyModel.md)                               | [get_model](#i_get_model) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) index ) const                                                        
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)  | [get_model_index_from_resource_name](#i_get_model_index_from_resource_name) ( [String](https://docs.godotengine.org/en/stable/classes/class_string.html) name ) const 
<p></p>

## 属性描述

### [VoxelBlockyModel[]](https://docs.godotengine.org/en/stable/classes/class_voxelblockymodel[].html)<span id="i_models"></span> **models** = []

所有模型的数组。每个模型的索引对应体素数据 TYPE 通道中表示它们的值。

## 方法描述

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_add_model"></span> **add_model**( [VoxelBlockyModel](VoxelBlockyModel.md) model ) 

向库中添加一个模型。返回其索引，该索引将作为表示该模型的体素的值。

### [VoxelBlockyModel](VoxelBlockyModel.md)<span id="i_get_model"></span> **get_model**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) index ) 

根据索引获取模型。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_get_model_index_from_resource_name"></span> **get_model_index_from_resource_name**( [String](https://docs.godotengine.org/en/stable/classes/class_string.html) name ) 

查找具有指定资源名称的第一个模型的索引。如果未找到，返回 `null`。

_生成于 2026-09-12_
