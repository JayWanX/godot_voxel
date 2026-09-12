# VoxelBlockyAttribute

继承自：[Resource](https://docs.godotengine.org/en/stable/classes/class_resource.html)

派生：[VoxelBlockyAttributeAxis](VoxelBlockyAttributeAxis.md), [VoxelBlockyAttributeCustom](VoxelBlockyAttributeCustom.md), [VoxelBlockyAttributeDirection](VoxelBlockyAttributeDirection.md), [VoxelBlockyAttributeRotation](VoxelBlockyAttributeRotation.md)

## 方法：


返回值                                                                                 | 函数签名                                                  
----------------------------------------------------------------------------------- | ------------------------------------------------------
[StringName](https://docs.godotengine.org/en/stable/classes/class_stringname.html)  | [get_attribute_name](#i_get_attribute_name) ( ) const 
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)                | [get_default_value](#i_get_default_value) ( ) const   
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)                | [get_value_count](#i_get_value_count) ( ) const       
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)              | [is_rotation](#i_is_rotation) ( ) const               
<p></p>

## 常量：

- <span id="i_MAX_VALUES"></span>**MAX_VALUES** = **256**

## 方法描述

### [StringName](https://docs.godotengine.org/en/stable/classes/class_stringname.html)<span id="i_get_attribute_name"></span> **get_attribute_name**( ) 

获取属性名称。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_get_default_value"></span> **get_default_value**( ) 

获取属性的默认取值。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_get_value_count"></span> **get_value_count**( ) 

获取取值范围的大小。注意：这实际上是最大值加一，并非取值的总个数。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_is_rotation"></span> **is_rotation**( ) 

该属性是否表示旋转。

_生成于 2026-09-12_
