# VoxelGraphFunction

继承自：[Resource](https://docs.godotengine.org/en/stable/classes/class_resource.html)

用于生成或处理一系列 3D 值的图形。

## 描述：

包含一个可用于处理一系列值的图形，例如体素位置（当用作生成器的主函数时），或用于在其它图形中复用（如子图形）。

目前此类只存储图形，无法自行运行实际处理。它通常被嵌入到另一个资源中，由该资源以特定方式使用此图形。

要使用它生成体素，请参见 [VoxelGeneratorGraph](VoxelGeneratorGraph.md)。

节点可以从其输出连接到下一个节点的输入。未连接的输入可以具有默认值或默认的隐式连接。

节点还可以具有“参数”，即每个节点配置的常量。

节点分为 3 大类：输入（只有输出）、输出（只有输入）以及其他（既有输入也有输出，用于进行某些计算）。

节点类型使用枚举 [NodeTypeID](VoxelGraphFunction.md#enumerations) 标识。此枚举不应在持久化场景（如存档文件）中使用，因为其值可能在版本之间发生变化。

图形只能处理 32 位浮点值。

节点类型的描述位于图形编辑器的节点对话框中，或参见 [https://voxel-tools.readthedocs.io/en/latest/graph_nodes](https://voxel-tools.readthedocs.io/en/latest/graph_nodes)。

## 属性：


类型                                                                        | 名称                                           | 默认值 
------------------------------------------------------------------------- | -------------------------------------------- | ----
[Array](https://docs.godotengine.org/en/stable/classes/class_array.html)  | [input_definitions](#i_input_definitions)    | []  
[Array](https://docs.godotengine.org/en/stable/classes/class_array.html)  | [output_definitions](#i_output_definitions)  | []  
<p></p>

## 方法：


返回值                                                                                             | 函数签名                                                                                                                                                                                                                                                                                                                                                                                        
----------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
[void](#)                                                                                       | [add_connection](#i_add_connection) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) src_node_id, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) src_port_index, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) dst_node_id, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) dst_port_index )        
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)                          | [can_connect](#i_can_connect) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) src_node_id, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) src_port_index, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) dst_node_id, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) dst_port_index ) const        
[void](#)                                                                                       | [clear](#i_clear) ( )                                                                                                                                                                                                                                                                                                                                                                       
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)                            | [create_function_node](#i_create_function_node) ( [VoxelGraphFunction](VoxelGraphFunction.md) function, [Vector2](https://docs.godotengine.org/en/stable/classes/class_vector2.html) position, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) id=0 )                                                                                                                  
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)                            | [create_node](#i_create_node) ( [NodeTypeID](VoxelGraphFunction.md#enumerations) type_id, [Vector2](https://docs.godotengine.org/en/stable/classes/class_vector2.html) position, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) id=0 )                                                                                                                                
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)                            | [find_node_by_name](#i_find_node_by_name) ( [StringName](https://docs.godotengine.org/en/stable/classes/class_stringname.html) name ) const                                                                                                                                                                                                                                                 
[Array](https://docs.godotengine.org/en/stable/classes/class_array.html)                        | [get_connections](#i_get_connections) ( ) const                                                                                                                                                                                                                                                                                                                                             
[Variant](https://docs.godotengine.org/en/stable/classes/class_variant.html)                    | [get_node_default_input](#i_get_node_default_input) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) node_id, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) input_index ) const                                                                                                                                                                
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)                          | [get_node_default_inputs_autoconnect](#i_get_node_default_inputs_autoconnect) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) node_id ) const                                                                                                                                                                                                                        
[Vector2](https://docs.godotengine.org/en/stable/classes/class_vector2.html)                    | [get_node_gui_position](#i_get_node_gui_position) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) node_id ) const                                                                                                                                                                                                                                                    
[Vector2](https://docs.godotengine.org/en/stable/classes/class_vector2.html)                    | [get_node_gui_size](#i_get_node_gui_size) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) node_id ) const                                                                                                                                                                                                                                                            
[PackedInt32Array](https://docs.godotengine.org/en/stable/classes/class_packedint32array.html)  | [get_node_ids](#i_get_node_ids) ( ) const                                                                                                                                                                                                                                                                                                                                                   
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)                            | [get_node_input_index](#i_get_node_input_index) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) node_id, [String](https://docs.godotengine.org/en/stable/classes/class_string.html) input_name ) const                                                                                                                                                               
[StringName](https://docs.godotengine.org/en/stable/classes/class_stringname.html)              | [get_node_name](#i_get_node_name) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) node_id ) const                                                                                                                                                                                                                                                                    
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)                            | [get_node_output_index](#i_get_node_output_index) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) node_id, [String](https://docs.godotengine.org/en/stable/classes/class_string.html) output_name ) const                                                                                                                                                            
[Variant](https://docs.godotengine.org/en/stable/classes/class_variant.html)                    | [get_node_param](#i_get_node_param) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) node_id, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) param_index ) const                                                                                                                                                                                
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)                            | [get_node_type_count](#i_get_node_type_count) ( ) const                                                                                                                                                                                                                                                                                                                                     
[NodeTypeID](VoxelGraphFunction.md#enumerations)                                                | [get_node_type_id](#i_get_node_type_id) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) node_id ) const                                                                                                                                                                                                                                                              
[Dictionary](https://docs.godotengine.org/en/stable/classes/class_dictionary.html)              | [get_node_type_info](#i_get_node_type_info) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) type_id ) const                                                                                                                                                                                                                                                          
[void](#)                                                                                       | [paste_graph_with_pre_generated_ids](#i_paste_graph_with_pre_generated_ids) ( [VoxelGraphFunction](VoxelGraphFunction.md) graph, [PackedInt32Array](https://docs.godotengine.org/en/stable/classes/class_packedint32array.html) node_ids, [Vector2](https://docs.godotengine.org/en/stable/classes/class_vector2.html) gui_offset )                                                         
[void](#)                                                                                       | [remove_connection](#i_remove_connection) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) src_node_id, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) src_port_index, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) dst_node_id, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) dst_port_index )  
[void](#)                                                                                       | [remove_node](#i_remove_node) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) node_id )                                                                                                                                                                                                                                                                              
[void](#)                                                                                       | [set_expression_node_inputs](#i_set_expression_node_inputs) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) node_id, [PackedStringArray](https://docs.godotengine.org/en/stable/classes/class_packedstringarray.html) names )                                                                                                                                        
[void](#)                                                                                       | [set_node_default_input](#i_set_node_default_input) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) node_id, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) input_index, [Variant](https://docs.godotengine.org/en/stable/classes/class_variant.html) value )                                                                                  
[void](#)                                                                                       | [set_node_default_input_by_name](#i_set_node_default_input_by_name) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) node_id, [String](https://docs.godotengine.org/en/stable/classes/class_string.html) input_name, [Variant](https://docs.godotengine.org/en/stable/classes/class_variant.html) value )                                                             
[void](#)                                                                                       | [set_node_default_inputs_autoconnect](#i_set_node_default_inputs_autoconnect) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) node_id, [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html) enabled )                                                                                                                                              
[void](#)                                                                                       | [set_node_gui_position](#i_set_node_gui_position) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) node_id, [Vector2](https://docs.godotengine.org/en/stable/classes/class_vector2.html) position )                                                                                                                                                                   
[void](#)                                                                                       | [set_node_gui_size](#i_set_node_gui_size) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) node_id, [Vector2](https://docs.godotengine.org/en/stable/classes/class_vector2.html) size )                                                                                                                                                                               
[void](#)                                                                                       | [set_node_name](#i_set_node_name) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) node_id, [StringName](https://docs.godotengine.org/en/stable/classes/class_stringname.html) name )                                                                                                                                                                                 
[void](#)                                                                                       | [set_node_param](#i_set_node_param) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) node_id, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) param_index, [Variant](https://docs.godotengine.org/en/stable/classes/class_variant.html) value )                                                                                                  
[void](#)                                                                                       | [set_node_param_by_name](#i_set_node_param_by_name) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) node_id, [String](https://docs.godotengine.org/en/stable/classes/class_string.html) param_name, [Variant](https://docs.godotengine.org/en/stable/classes/class_variant.html) value )                                                                             
[void](#)                                                                                       | [set_node_param_null](#i_set_node_param_null) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) node_id, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) param_index )                                                                                                                                                                            
<p></p>

## 信号：<span id="signals"></span>

### compiled( ) 

图形编译完成后发出，即使编译失败也会发出。

### node_name_changed( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) node_id ) 

*(此信号暂无文档)*

## 枚举：<span id="enumerations"></span>

枚举 **NodeTypeID**：

- <span id="i_NODE_CONSTANT"></span>**NODE_CONSTANT** = **0**
- <span id="i_NODE_INPUT_X"></span>**NODE_INPUT_X** = **1**
- <span id="i_NODE_INPUT_Y"></span>**NODE_INPUT_Y** = **2**
- <span id="i_NODE_INPUT_Z"></span>**NODE_INPUT_Z** = **3**
- <span id="i_NODE_OUTPUT_SDF"></span>**NODE_OUTPUT_SDF** = **4**
- <span id="i_NODE_CUSTOM_INPUT"></span>**NODE_CUSTOM_INPUT** = **52**
- <span id="i_NODE_CUSTOM_OUTPUT"></span>**NODE_CUSTOM_OUTPUT** = **53**
- <span id="i_NODE_ADD"></span>**NODE_ADD** = **5**
- <span id="i_NODE_SUBTRACT"></span>**NODE_SUBTRACT** = **6**
- <span id="i_NODE_MULTIPLY"></span>**NODE_MULTIPLY** = **7**
- <span id="i_NODE_DIVIDE"></span>**NODE_DIVIDE** = **8**
- <span id="i_NODE_SIN"></span>**NODE_SIN** = **9**
- <span id="i_NODE_FLOOR"></span>**NODE_FLOOR** = **10**
- <span id="i_NODE_ABS"></span>**NODE_ABS** = **11**
- <span id="i_NODE_SQRT"></span>**NODE_SQRT** = **12**
- <span id="i_NODE_FRACT"></span>**NODE_FRACT** = **13**
- <span id="i_NODE_STEPIFY"></span>**NODE_STEPIFY** = **14**
- <span id="i_NODE_WRAP"></span>**NODE_WRAP** = **15**
- <span id="i_NODE_MIN"></span>**NODE_MIN** = **16**
- <span id="i_NODE_MAX"></span>**NODE_MAX** = **17**
- <span id="i_NODE_DISTANCE_2D"></span>**NODE_DISTANCE_2D** = **18**
- <span id="i_NODE_DISTANCE_3D"></span>**NODE_DISTANCE_3D** = **19**
- <span id="i_NODE_CLAMP"></span>**NODE_CLAMP** = **20**
- <span id="i_NODE_MIX"></span>**NODE_MIX** = **22**
- <span id="i_NODE_REMAP"></span>**NODE_REMAP** = **23**
- <span id="i_NODE_SMOOTHSTEP"></span>**NODE_SMOOTHSTEP** = **24**
- <span id="i_NODE_CURVE"></span>**NODE_CURVE** = **25**
- <span id="i_NODE_SELECT"></span>**NODE_SELECT** = **26**
- <span id="i_NODE_NOISE_2D"></span>**NODE_NOISE_2D** = **27**
- <span id="i_NODE_NOISE_3D"></span>**NODE_NOISE_3D** = **28**
- <span id="i_NODE_IMAGE_2D"></span>**NODE_IMAGE_2D** = **29**
- <span id="i_NODE_SDF_PLANE"></span>**NODE_SDF_PLANE** = **30**
- <span id="i_NODE_SDF_BOX"></span>**NODE_SDF_BOX** = **31**
- <span id="i_NODE_SDF_SPHERE"></span>**NODE_SDF_SPHERE** = **32**
- <span id="i_NODE_SDF_TORUS"></span>**NODE_SDF_TORUS** = **33**
- <span id="i_NODE_SDF_PREVIEW"></span>**NODE_SDF_PREVIEW** = **34**
- <span id="i_NODE_SDF_SPHERE_HEIGHTMAP"></span>**NODE_SDF_SPHERE_HEIGHTMAP** = **35**
- <span id="i_NODE_SDF_SMOOTH_UNION"></span>**NODE_SDF_SMOOTH_UNION** = **36**
- <span id="i_NODE_SDF_SMOOTH_SUBTRACT"></span>**NODE_SDF_SMOOTH_SUBTRACT** = **37**
- <span id="i_NODE_NORMALIZE_3D"></span>**NODE_NORMALIZE_3D** = **38**
- <span id="i_NODE_FAST_NOISE_2D"></span>**NODE_FAST_NOISE_2D** = **39**
- <span id="i_NODE_FAST_NOISE_3D"></span>**NODE_FAST_NOISE_3D** = **40**
- <span id="i_NODE_FAST_NOISE_GRADIENT_2D"></span>**NODE_FAST_NOISE_GRADIENT_2D** = **41**
- <span id="i_NODE_FAST_NOISE_GRADIENT_3D"></span>**NODE_FAST_NOISE_GRADIENT_3D** = **42**
- <span id="i_NODE_OUTPUT_WEIGHT"></span>**NODE_OUTPUT_WEIGHT** = **43**
- <span id="i_NODE_OUTPUT_SINGLE_TEXTURE"></span>**NODE_OUTPUT_SINGLE_TEXTURE** = **45**
- <span id="i_NODE_EXPRESSION"></span>**NODE_EXPRESSION** = **46**
- <span id="i_NODE_POWI"></span>**NODE_POWI** = **47**
- <span id="i_NODE_POW"></span>**NODE_POW** = **48**
- <span id="i_NODE_INPUT_SDF"></span>**NODE_INPUT_SDF** = **49**
- <span id="i_NODE_COMMENT"></span>**NODE_COMMENT** = **50**
- <span id="i_NODE_FUNCTION"></span>**NODE_FUNCTION** = **51**
- <span id="i_NODE_RELAY"></span>**NODE_RELAY** = **54**
- <span id="i_NODE_SPOTS_2D"></span>**NODE_SPOTS_2D** = **55**
- <span id="i_NODE_SPOTS_3D"></span>**NODE_SPOTS_3D** = **56**
- <span id="i_NODE_TYPE_COUNT"></span>**NODE_TYPE_COUNT** = **59**
- <span id="i_NODE_FAST_NOISE_2_2D"></span>**NODE_FAST_NOISE_2_2D** = **57**
- <span id="i_NODE_FAST_NOISE_2_3D"></span>**NODE_FAST_NOISE_2_3D** = **58**


## 属性描述

### [Array](https://docs.godotengine.org/en/stable/classes/class_array.html)<span id="i_input_definitions"></span> **input_definitions** = []

*(此属性暂无文档)*

### [Array](https://docs.godotengine.org/en/stable/classes/class_array.html)<span id="i_output_definitions"></span> **output_definitions** = []

*(此属性暂无文档)*

## 方法描述

### [void](#)<span id="i_add_connection"></span> **add_connection**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) src_node_id, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) src_port_index, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) dst_node_id, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) dst_port_index ) 

将一个节点的输出连接到另一个节点的输入。不支持将节点连接到自身，或以某种方式使其回到自身。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_can_connect"></span> **can_connect**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) src_node_id, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) src_port_index, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) dst_node_id, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) dst_port_index ) 

测试两个端口是否能够连接在一起。

### [void](#)<span id="i_clear"></span> **clear**( ) 

从图形中移除所有节点。输入和输出定义不会被清除。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_create_function_node"></span> **create_function_node**( [VoxelGraphFunction](VoxelGraphFunction.md) function, [Vector2](https://docs.godotengine.org/en/stable/classes/class_vector2.html) position, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) id=0 ) 

基于现有图形创建节点（创建"子图形实例"）。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_create_node"></span> **create_node**( [NodeTypeID](VoxelGraphFunction.md#enumerations) type_id, [Vector2](https://docs.godotengine.org/en/stable/classes/class_vector2.html) position, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) id=0 ) 

在特定的可视位置创建给定类型的图形节点。

`position` 参数不会影响图形的运行方式，但它有助于整理节点。

可以指定一个可选的 ID。如果保持为 0，则将自动生成 ID。

然后此函数返回节点的 ID，这可能有助于稍后修改节点的其它属性。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_find_node_by_name"></span> **find_node_by_name**( [StringName](https://docs.godotengine.org/en/stable/classes/class_stringname.html) name ) 

查找具有指定名称的节点并返回其 ID。如果未找到节点，则返回 0。

### [Array](https://docs.godotengine.org/en/stable/classes/class_array.html)<span id="i_get_connections"></span> **get_connections**( ) 

获取描述节点之间所有连接的数组。

该数组具有以下格式：

	```
	[
		{
			"src_node_id": int,
			"src_port_index": int,
			"dst_node_id": int,
			"dst_port_index": int
		},
		...
	]
	```

### [Variant](https://docs.godotengine.org/en/stable/classes/class_variant.html)<span id="i_get_node_default_input"></span> **get_node_default_input**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) node_id, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) input_index ) 

*(此方法暂无文档)*

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_get_node_default_inputs_autoconnect"></span> **get_node_default_inputs_autoconnect**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) node_id ) 

*(此方法暂无文档)*

### [Vector2](https://docs.godotengine.org/en/stable/classes/class_vector2.html)<span id="i_get_node_gui_position"></span> **get_node_gui_position**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) node_id ) 

获取节点在图形编辑器中的位置。

### [Vector2](https://docs.godotengine.org/en/stable/classes/class_vector2.html)<span id="i_get_node_gui_size"></span> **get_node_gui_size**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) node_id ) 

获取节点在图形编辑器中的大小。

### [PackedInt32Array](https://docs.godotengine.org/en/stable/classes/class_packedint32array.html)<span id="i_get_node_ids"></span> **get_node_ids**( ) 

获取图形中所有节点的 ID 列表。

注意：添加或移除节点后，ID 的返回顺序不保证保持不变。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_get_node_input_index"></span> **get_node_input_index**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) node_id, [String](https://docs.godotengine.org/en/stable/classes/class_string.html) input_name ) 

根据输入的名称获取节点的输入索引。

### [StringName](https://docs.godotengine.org/en/stable/classes/class_stringname.html)<span id="i_get_node_name"></span> **get_node_name**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) node_id ) 

获取节点的用户自定义名称。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_get_node_output_index"></span> **get_node_output_index**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) node_id, [String](https://docs.godotengine.org/en/stable/classes/class_string.html) output_name ) 

根据输出的名称获取节点的输出索引。

### [Variant](https://docs.godotengine.org/en/stable/classes/class_variant.html)<span id="i_get_node_param"></span> **get_node_param**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) node_id, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) param_index ) 

获取节点的参数。参数索引对应于该参数在编辑器中出现的位置。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_get_node_type_count"></span> **get_node_type_count**( ) 

获取图形系统中存在多少种节点类型。

### [NodeTypeID](VoxelGraphFunction.md#enumerations)<span id="i_get_node_type_id"></span> **get_node_type_id**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) node_id ) 

获取图形中节点类型的 ID。

### [Dictionary](https://docs.godotengine.org/en/stable/classes/class_dictionary.html)<span id="i_get_node_type_info"></span> **get_node_type_info**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) type_id ) 

从 [NodeTypeID](VoxelGraphFunction.md#enumerations) 获取关于节点类型的信息。

返回的数据具有以下结构：

```
{
	"name": String,
	"inputs": [
		{"name": String},
		...
	],
	"outputs": [
		{"name": String},
		...
	],
	"params": [
		{
			"name": String,
			"type": int (Variant::Type),
			"class_name": String,
			"default_value": Variant
		},
		...
	]
}
```

### [void](#)<span id="i_paste_graph_with_pre_generated_ids"></span> **paste_graph_with_pre_generated_ids**( [VoxelGraphFunction](VoxelGraphFunction.md) graph, [PackedInt32Array](https://docs.godotengine.org/en/stable/classes/class_packedint32array.html) node_ids, [Vector2](https://docs.godotengine.org/en/stable/classes/class_vector2.html) gui_offset ) 

将节点以及它们之间的连接复制到另一个图形中。

如果节点参数中的资源没有文件路径，它们将被复制。

如果提供了非零大小的 `node_ids`，则定义复制的节点在目标图形中将具有的 ID，顺序与源图形的 [get_node_ids](VoxelGraphFunction.md#i_get_node_ids) 相同。数组的大小必须与复制的节点数量相同，且 ID 不得已在目标图形中存在。如果数组为空，则将自动生成 ID。

### [void](#)<span id="i_remove_connection"></span> **remove_connection**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) src_node_id, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) src_port_index, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) dst_node_id, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) dst_port_index ) 

移除图形中两个节点之间的现有连接。

### [void](#)<span id="i_remove_node"></span> **remove_node**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) node_id ) 

从图形中移除一个节点。

### [void](#)<span id="i_set_expression_node_inputs"></span> **set_expression_node_inputs**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) node_id, [PackedStringArray](https://docs.godotengine.org/en/stable/classes/class_packedstringarray.html) names ) 

为表达式节点配置输入。`names` 是表达式中使用的输入名称列表。

如果你通过代码创建表达式节点，之后应调用此方法。

### [void](#)<span id="i_set_node_default_input"></span> **set_node_default_input**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) node_id, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) input_index, [Variant](https://docs.godotengine.org/en/stable/classes/class_variant.html) value ) 

设置节点输入在保持未连接时将具有的值。

### [void](#)<span id="i_set_node_default_input_by_name"></span> **set_node_default_input_by_name**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) node_id, [String](https://docs.godotengine.org/en/stable/classes/class_string.html) input_name, [Variant](https://docs.godotengine.org/en/stable/classes/class_variant.html) value ) 

设置节点输入在保持未连接时将具有的值。输入通过其在编辑器中显示的名称来指定。

### [void](#)<span id="i_set_node_default_inputs_autoconnect"></span> **set_node_default_inputs_autoconnect**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) node_id, [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html) enabled ) 

设置没有入站连接的节点输入在图形编译时是否会自动创建默认连接。

这仅适用于特定节点（例如，2D 或 3D 噪声默认使用 XYZ 输入）。在其他节点上，它没有效果。

### [void](#)<span id="i_set_node_gui_position"></span> **set_node_gui_position**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) node_id, [Vector2](https://docs.godotengine.org/en/stable/classes/class_vector2.html) position ) 

设置图形节点的可视位置，即它在编辑器中显示的位置。

### [void](#)<span id="i_set_node_gui_size"></span> **set_node_gui_size**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) node_id, [Vector2](https://docs.godotengine.org/en/stable/classes/class_vector2.html) size ) 

设置图形节点的可视大小，即它在编辑器中显示的大小。

### [void](#)<span id="i_set_node_name"></span> **set_node_name**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) node_id, [StringName](https://docs.godotengine.org/en/stable/classes/class_stringname.html) name ) 

为节点设置自定义名称。

### [void](#)<span id="i_set_node_param"></span> **set_node_param**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) node_id, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) param_index, [Variant](https://docs.godotengine.org/en/stable/classes/class_variant.html) value ) 

设置节点的参数。参数索引对应于该参数在编辑器中显示的位置。

### [void](#)<span id="i_set_node_param_by_name"></span> **set_node_param_by_name**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) node_id, [String](https://docs.godotengine.org/en/stable/classes/class_string.html) param_name, [Variant](https://docs.godotengine.org/en/stable/classes/class_variant.html) value ) 

使用节点参数在编辑器中显示的名称来设置其参数。

### [void](#)<span id="i_set_node_param_null"></span> **set_node_param_null**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) node_id, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) param_index ) 

将节点的参数设置为 null。此方法仅用于规避 Godot 的 UndoRedo 系统的一个问题。建议使用 [set_node_param](VoxelGraphFunction.md#i_set_node_param)。

_生成于 2026-08-28_
