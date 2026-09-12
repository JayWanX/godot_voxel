# VoxelInstanceLibrary

继承自：[Resource](https://docs.godotengine.org/en/stable/classes/class_resource.html)

包含一份模型列表，这些模型可与唯一 ID 关联并由 [VoxelInstancer](VoxelInstancer.md) 使用。

## 属性：


类型                                                                        | 名称                                   | 默认值 
------------------------------------------------------------------------- | ------------------------------------ | ----
[Array](https://docs.godotengine.org/en/stable/classes/class_array.html)  | [_data](#i__data)                    | [0] 
[VoxelInstanceLibraryItem](VoxelInstanceLibraryItem.md)                   | [_selected_item](#i__selected_item)  |     
<p></p>

## 方法：


返回值                                                                                             | 函数签名                                                                                                                                                               
----------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------
[void](#)                                                                                       | [add_item](#i_add_item) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) id, [VoxelInstanceLibraryItem](VoxelInstanceLibraryItem.md) item )  
[void](#)                                                                                       | [clear](#i_clear) ( )                                                                                                                                              
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)                            | [find_item_by_name](#i_find_item_by_name) ( [String](https://docs.godotengine.org/en/stable/classes/class_string.html) name ) const                                
[PackedInt32Array](https://docs.godotengine.org/en/stable/classes/class_packedint32array.html)  | [get_all_item_ids](#i_get_all_item_ids) ( ) const                                                                                                                  
[VoxelInstanceLibraryItem](VoxelInstanceLibraryItem.md)                                         | [get_item](#i_get_item) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) id ) const                                                          
[void](#)                                                                                       | [remove_item](#i_remove_item) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) id )                                                          
<p></p>

## 常量：

- <span id="i_MAX_ID"></span>**MAX_ID** = **65535**

## 属性描述

### [Array](https://docs.godotengine.org/en/stable/classes/class_array.html)<span id="i__data"></span> **_data** = [0]

*(此属性暂无文档)*

### [VoxelInstanceLibraryItem](VoxelInstanceLibraryItem.md)<span id="i__selected_item"></span> **_selected_item**

*(此属性暂无文档)*

## 方法描述

### [void](#)<span id="i_add_item"></span> **add_item**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) id, [VoxelInstanceLibraryItem](VoxelInstanceLibraryItem.md) item ) 

以指定 ID 添加项目。

### [void](#)<span id="i_clear"></span> **clear**( ) 

清空所有项目。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_find_item_by_name"></span> **find_item_by_name**( [String](https://docs.godotengine.org/en/stable/classes/class_string.html) name ) 

按名称查找项目 ID，未找到时返回 -1。

### [PackedInt32Array](https://docs.godotengine.org/en/stable/classes/class_packedint32array.html)<span id="i_get_all_item_ids"></span> **get_all_item_ids**( ) 

获取所有项目的 ID 列表。

### [VoxelInstanceLibraryItem](VoxelInstanceLibraryItem.md)<span id="i_get_item"></span> **get_item**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) id ) 

获取指定 ID 的项目。

### [void](#)<span id="i_remove_item"></span> **remove_item**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) id ) 

移除指定 ID 的项目。

_生成于 2026-09-12_
