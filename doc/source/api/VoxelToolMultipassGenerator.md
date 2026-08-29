# VoxelToolMultipassGenerator

继承自：[VoxelTool](VoxelTool.md)

用于在多通道地形生成上下文中编辑体素。

## 描述：

此工具允许在 3D 长方体内部编辑体素，该长方体以与要生成的当前数据块或列相对应的特定区域为中心。

根据上下文，还可以编辑距离主区域一定距离的体素。

“主”区域与“总”区域的区别在于，“主”区域是应该生成内容的地方，而“总”区域仅在你生成的内容需要与主区域外部重叠时可用。


此类的实例是临时的，并且不是线程安全的。它们绝不能重复使用或存储在成员变量中。

## 方法：


返回值                                                                             | 函数签名                                                        
------------------------------------------------------------------------------- | ------------------------------------------------------------
[Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html)  | [get_editable_area_max](#i_get_editable_area_max) ( ) const 
[Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html)  | [get_editable_area_min](#i_get_editable_area_min) ( ) const 
[Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html)  | [get_main_area_max](#i_get_main_area_max) ( ) const         
[Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html)  | [get_main_area_min](#i_get_main_area_min) ( ) const         
<p></p>

## 方法描述

### [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html)<span id="i_get_editable_area_max"></span> **get_editable_area_max**( ) 

获取总可编辑区域的上角，单位为体素，不包含该角。

### [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html)<span id="i_get_editable_area_min"></span> **get_editable_area_min**( ) 

获取总可编辑区域的下角，单位为体素。

### [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html)<span id="i_get_main_area_max"></span> **get_main_area_max**( ) 

获取主可编辑区域的上角，单位为体素，不包含该角。

### [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html)<span id="i_get_main_area_min"></span> **get_main_area_min**( ) 

获取主可编辑区域的下角，单位为体素。

_生成于 2026-08-28_
