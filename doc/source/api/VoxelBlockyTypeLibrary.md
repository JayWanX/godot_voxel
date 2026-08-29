# VoxelBlockyTypeLibrary

继承自：[VoxelBlockyLibraryBase](VoxelBlockyLibraryBase.md)

!!! warning
    此类被标记为实验性。未来版本中可能发生变更或被移除。请自行判断使用风险。

## 属性：


类型                                                                                                | 名称                               | 默认值                 
------------------------------------------------------------------------------------------------- | -------------------------------- | --------------------
[PackedStringArray](https://docs.godotengine.org/en/stable/classes/class_packedstringarray.html)  | [_id_map_data](#i__id_map_data)  | PackedStringArray() 
[VoxelBlockyType[]](https://docs.godotengine.org/en/stable/classes/class_voxelblockytype[].html)  | [types](#i_types)                | []                  
<p></p>

## 方法：


返回值                                                                                               | 函数签名                                                                                                                                                                                                                                                                          
------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)                              | [get_model_index_default](#i_get_model_index_default) ( [StringName](https://docs.godotengine.org/en/stable/classes/class_stringname.html) type_name ) const                                                                                                                  
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)                              | [get_model_index_single_attribute](#i_get_model_index_single_attribute) ( [StringName](https://docs.godotengine.org/en/stable/classes/class_stringname.html) type_name, [Variant](https://docs.godotengine.org/en/stable/classes/class_variant.html) attrib_value ) const     
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)                              | [get_model_index_with_attributes](#i_get_model_index_with_attributes) ( [StringName](https://docs.godotengine.org/en/stable/classes/class_stringname.html) type_name, [Dictionary](https://docs.godotengine.org/en/stable/classes/class_dictionary.html) attribs_dict ) const 
[VoxelBlockyType](VoxelBlockyType.md)                                                             | [get_type_from_name](#i_get_type_from_name) ( [StringName](https://docs.godotengine.org/en/stable/classes/class_stringname.html) type_name ) const                                                                                                                            
[Array](https://docs.godotengine.org/en/stable/classes/class_array.html)                          | [get_type_name_and_attributes_from_model_index](#i_get_type_name_and_attributes_from_model_index) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) model_index ) const                                                                                  
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)                            | [load_id_map_from_json](#i_load_id_map_from_json) ( [String](https://docs.godotengine.org/en/stable/classes/class_string.html) json )                                                                                                                                         
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)                            | [load_id_map_from_string_array](#i_load_id_map_from_string_array) ( [PackedStringArray](https://docs.godotengine.org/en/stable/classes/class_packedstringarray.html) str_array )                                                                                              
[String](https://docs.godotengine.org/en/stable/classes/class_string.html)                        | [serialize_id_map_to_json](#i_serialize_id_map_to_json) ( ) const                                                                                                                                                                                                             
[PackedStringArray](https://docs.godotengine.org/en/stable/classes/class_packedstringarray.html)  | [serialize_id_map_to_string_array](#i_serialize_id_map_to_string_array) ( ) const                                                                                                                                                                                             
<p></p>

## 属性描述

### [PackedStringArray](https://docs.godotengine.org/en/stable/classes/class_packedstringarray.html)<span id="i__id_map_data"></span> **_id_map_data** = PackedStringArray()

*(此属性暂无文档)*

### [VoxelBlockyType[]](https://docs.godotengine.org/en/stable/classes/class_voxelblockytype[].html)<span id="i_types"></span> **types** = []

*(此属性暂无文档)*

## 方法描述

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_get_model_index_default"></span> **get_model_index_default**( [StringName](https://docs.godotengine.org/en/stable/classes/class_stringname.html) type_name ) 

*(此方法暂无文档)*

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_get_model_index_single_attribute"></span> **get_model_index_single_attribute**( [StringName](https://docs.godotengine.org/en/stable/classes/class_stringname.html) type_name, [Variant](https://docs.godotengine.org/en/stable/classes/class_variant.html) attrib_value ) 

*(此方法暂无文档)*

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_get_model_index_with_attributes"></span> **get_model_index_with_attributes**( [StringName](https://docs.godotengine.org/en/stable/classes/class_stringname.html) type_name, [Dictionary](https://docs.godotengine.org/en/stable/classes/class_dictionary.html) attribs_dict ) 

*(此方法暂无文档)*

### [VoxelBlockyType](VoxelBlockyType.md)<span id="i_get_type_from_name"></span> **get_type_from_name**( [StringName](https://docs.godotengine.org/en/stable/classes/class_stringname.html) type_name ) 

*(此方法暂无文档)*

### [Array](https://docs.godotengine.org/en/stable/classes/class_array.html)<span id="i_get_type_name_and_attributes_from_model_index"></span> **get_type_name_and_attributes_from_model_index**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) model_index ) 

*(此方法暂无文档)*

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_load_id_map_from_json"></span> **load_id_map_from_json**( [String](https://docs.godotengine.org/en/stable/classes/class_string.html) json ) 

*(此方法暂无文档)*

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_load_id_map_from_string_array"></span> **load_id_map_from_string_array**( [PackedStringArray](https://docs.godotengine.org/en/stable/classes/class_packedstringarray.html) str_array ) 

*(此方法暂无文档)*

### [String](https://docs.godotengine.org/en/stable/classes/class_string.html)<span id="i_serialize_id_map_to_json"></span> **serialize_id_map_to_json**( ) 

*(此方法暂无文档)*

### [PackedStringArray](https://docs.godotengine.org/en/stable/classes/class_packedstringarray.html)<span id="i_serialize_id_map_to_string_array"></span> **serialize_id_map_to_string_array**( ) 

*(此方法暂无文档)*

_生成于 2026-08-28_
