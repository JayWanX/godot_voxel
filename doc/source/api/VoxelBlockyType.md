# VoxelBlockyType

继承自：[Resource](https://docs.godotengine.org/en/stable/classes/class_resource.html)

## 属性：


类型                                                                                                          | 名称                                               | 默认值        
----------------------------------------------------------------------------------------------------------- | ------------------------------------------------ | -----------
[Array](https://docs.godotengine.org/en/stable/classes/class_array.html)                                    | [_variant_models_data](#i__variant_models_data)  | []         
[VoxelBlockyAttribute[]](https://docs.godotengine.org/en/stable/classes/class_voxelblockyattribute[].html)  | [attributes](#i_attributes)                      | []         
[VoxelBlockyModel](VoxelBlockyModel.md)                                                                     | [base_model](#i_base_model)                      |            
[StringName](https://docs.godotengine.org/en/stable/classes/class_stringname.html)                          | [unique_name](#i_unique_name)                    | &"unnamed" 
<p></p>

## 方法：


返回值                                              | 函数签名                                                                                                                                                                       
------------------------------------------------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
[VoxelBlockyAttribute](VoxelBlockyAttribute.md)  | [get_rotation_attribute](#i_get_rotation_attribute) ( ) const                                                                                                              
[void](#)                                        | [set_variant_model](#i_set_variant_model) ( [Array](https://docs.godotengine.org/en/stable/classes/class_array.html) key, [VoxelBlockyModel](VoxelBlockyModel.md) model )  
<p></p>

## 常量：

- <span id="i_MAX_ATTRIBUTES"></span>**MAX_ATTRIBUTES** = **4**

## 属性描述

### [Array](https://docs.godotengine.org/en/stable/classes/class_array.html)<span id="i__variant_models_data"></span> **_variant_models_data** = []

*(此属性暂无文档)*

### [VoxelBlockyAttribute[]](https://docs.godotengine.org/en/stable/classes/class_voxelblockyattribute[].html)<span id="i_attributes"></span> **attributes** = []

该类型可用的属性列表。

### [VoxelBlockyModel](VoxelBlockyModel.md)<span id="i_base_model"></span> **base_model**

该类型的基础模型。

### [StringName](https://docs.godotengine.org/en/stable/classes/class_stringname.html)<span id="i_unique_name"></span> **unique_name** = &"unnamed"

类型的唯一名称。

## 方法描述

### [VoxelBlockyAttribute](VoxelBlockyAttribute.md)<span id="i_get_rotation_attribute"></span> **get_rotation_attribute**( ) 

获取旋转属性（若存在）。

### [void](#)<span id="i_set_variant_model"></span> **set_variant_model**( [Array](https://docs.godotengine.org/en/stable/classes/class_array.html) key, [VoxelBlockyModel](VoxelBlockyModel.md) model ) 

明确设置对于给定的属性组合（键）使用哪个模型。

如果你有自动生成变体的属性（如旋转），你应仅为这些属性的默认值设置模型。其他模型将不会被保留。这是因为默认值将被用作参考来生成其他模型。

_生成于 2026-09-12_
