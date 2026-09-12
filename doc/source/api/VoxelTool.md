# VoxelTool

继承自：[RefCounted](https://docs.godotengine.org/en/stable/classes/class_refcounted.html)

派生：[VoxelToolBuffer](VoxelToolBuffer.md), [VoxelToolLodTerrain](VoxelToolLodTerrain.md), [VoxelToolMultipassGenerator](VoxelToolMultipassGenerator.md), [VoxelToolTerrain](VoxelToolTerrain.md)

用于轻松访问和修改体素的辅助类。

## 描述：

用于访问和编辑体素的抽象接口。它允许访问单个体素，或执行批量操作，例如雕刻大块区域或复制/粘贴长方体。

它不是一个单独实例化的类，你可以从想要操作的体素对象中获取它，因为它有多个派生实现。

默认情况下，如果操作与不可编辑区域重叠（例如尚未加载），该操作将被取消。此行为在派生类中可能有所不同。

## 属性：


类型                                                                        | 名称                                     | 默认值 
------------------------------------------------------------------------- | -------------------------------------- | ----
[ChannelId](VoxelBuffer.md#enumerations)                                  | [channel](#i_channel)                  |     
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)      | [eraser_value](#i_eraser_value)        |     
[Mode](VoxelTool.md#enumerations)                                         | [mode](#i_mode)                        |     
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)  | [sdf_scale](#i_sdf_scale)              |     
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)  | [sdf_strength](#i_sdf_strength)        |     
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)  | [texture_falloff](#i_texture_falloff)  |     
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)      | [texture_index](#i_texture_index)      |     
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)  | [texture_opacity](#i_texture_opacity)  |     
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)      | [value](#i_value)                      |     
<p></p>

## 方法：


返回值                                                                             | 函数签名                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    
------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)            | [color_to_u16](#i_color_to_u16) ( [Color](https://docs.godotengine.org/en/stable/classes/class_color.html) color ) static                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)            | [color_to_u16_weights](#i_color_to_u16_weights) ( [Color](https://docs.godotengine.org/en/stable/classes/class_color.html) _unnamed_arg0 ) static                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)            | [color_to_u32](#i_color_to_u32) ( [Color](https://docs.godotengine.org/en/stable/classes/class_color.html) color ) static                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               
[void](#)                                                                       | [copy](#i_copy) ( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) src_pos, [VoxelBuffer](VoxelBuffer.md) dst_buffer, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) channels_mask=255, [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html) with_metadata=true )                                                                                                                                                                                                                                                                                                                                 
[void](#)                                                                       | [do_box](#i_do_box) ( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) begin, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) end )                                                                                                                                                                                                                                                                                                                                                                                                                                                                        
[void](#)                                                                       | [do_mesh](#i_do_mesh) ( [VoxelMeshSDF](VoxelMeshSDF.md) mesh_sdf, [Transform3D](https://docs.godotengine.org/en/stable/classes/class_transform3d.html) transform, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) isolevel=0.0 )                                                                                                                                                                                                                                                                                                                                                                                                               
[void](#)                                                                       | [do_path](#i_do_path) ( [PackedVector3Array](https://docs.godotengine.org/en/stable/classes/class_packedvector3array.html) points, [PackedFloat32Array](https://docs.godotengine.org/en/stable/classes/class_packedfloat32array.html) radii )                                                                                                                                                                                                                                                                                                                                                                                                                           
[void](#)                                                                       | [do_point](#i_do_point) ( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) pos )                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          
[void](#)                                                                       | [do_sphere](#i_do_sphere) ( [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) center, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) radius )                                                                                                                                                                                                                                                                                                                                                                                                                                                                      
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)            | [get_voxel](#i_get_voxel) ( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) pos )                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)        | [get_voxel_f](#i_get_voxel_f) ( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) pos )                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    
[Variant](https://docs.godotengine.org/en/stable/classes/class_variant.html)    | [get_voxel_metadata](#i_get_voxel_metadata) ( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) pos ) const                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                
[void](#)                                                                       | [grow_sphere](#i_grow_sphere) ( [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) sphere_center, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) sphere_radius, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) strength )                                                                                                                                                                                                                                                                                                                                                                 
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)          | [is_area_editable](#i_is_area_editable) ( [AABB](https://docs.godotengine.org/en/stable/classes/class_aabb.html) box ) const                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            
[Color](https://docs.godotengine.org/en/stable/classes/class_color.html)        | [normalize_color](#i_normalize_color) ( [Color](https://docs.godotengine.org/en/stable/classes/class_color.html) _unnamed_arg0 ) static                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
[void](#)                                                                       | [paste](#i_paste) ( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) dst_pos, [VoxelBuffer](VoxelBuffer.md) src_buffer, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) channels_mask=255 )                                                                                                                                                                                                                                                                                                                                                                                                                          
[void](#)                                                                       | [paste_masked](#i_paste_masked) ( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) dst_pos, [VoxelBuffer](VoxelBuffer.md) src_buffer, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) channels_mask, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) mask_channel, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) mask_value )                                                                                                                                                                                                                                            
[void](#)                                                                       | [paste_masked_writable_list](#i_paste_masked_writable_list) ( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) position, [VoxelBuffer](VoxelBuffer.md) voxels, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) channels_mask, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) src_mask_channel, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) src_mask_value, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) dst_mask_channel, [PackedInt32Array](https://docs.godotengine.org/en/stable/classes/class_packedint32array.html) dst_writable_list )  
[VoxelRaycastResult](VoxelRaycastResult.md)                                     | [raycast](#i_raycast) ( [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) origin, [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) direction, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) max_distance=10.0, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) collision_mask=4294967295 )                                                                                                                                                                                                                                                                       
[void](#)                                                                       | [set_raycast_normal_enabled](#i_set_raycast_normal_enabled) ( [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html) enabled )                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          
[void](#)                                                                       | [set_voxel](#i_set_voxel) ( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) pos, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) v )                                                                                                                                                                                                                                                                                                                                                                                                                                                                                
[void](#)                                                                       | [set_voxel_f](#i_set_voxel_f) ( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) pos, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) v )                                                                                                                                                                                                                                                                                                                                                                                                                                                                        
[void](#)                                                                       | [set_voxel_metadata](#i_set_voxel_metadata) ( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) pos, [Variant](https://docs.godotengine.org/en/stable/classes/class_variant.html) meta )                                                                                                                                                                                                                                                                                                                                                                                                                                                   
[void](#)                                                                       | [smooth_sphere](#i_smooth_sphere) ( [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) sphere_center, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) sphere_radius, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) blur_radius )                                                                                                                                                                                                                                                                                                                                                              
[Vector4i](https://docs.godotengine.org/en/stable/classes/class_vector4i.html)  | [u16_indices_to_vec4i](#i_u16_indices_to_vec4i) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) _unnamed_arg0 ) static                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           
[Color](https://docs.godotengine.org/en/stable/classes/class_color.html)        | [u16_weights_to_color](#i_u16_weights_to_color) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) _unnamed_arg0 ) static                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)            | [vec4i_to_u16_indices](#i_vec4i_to_u16_indices) ( [Vector4i](https://docs.godotengine.org/en/stable/classes/class_vector4i.html) _unnamed_arg0 ) static                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
<p></p>

## 枚举：<span id="enumerations"></span>

枚举 **Mode**：

- <span id="i_MODE_ADD"></span>**MODE_ADD** = **0** --- 编辑 [VoxelBuffer.CHANNEL_SDF](VoxelBuffer.md#i_CHANNEL_SDF) 时，将添加物质。适用于建造。
- <span id="i_MODE_REMOVE"></span>**MODE_REMOVE** = **1** --- 编辑 [VoxelBuffer.CHANNEL_SDF](VoxelBuffer.md#i_CHANNEL_SDF) 时，将减去物质。适用于挖掘。
- <span id="i_MODE_SET"></span>**MODE_SET** = **2** --- 替换体素值而不进行任何混合。适用于方块体素。
- <span id="i_MODE_TEXTURE_PAINT"></span>**MODE_TEXTURE_PAINT** = **3** --- 在平滑地形中编辑 [VoxelBuffer.CHANNEL_SDF](VoxelBuffer.md#i_CHANNEL_SDF) 时，启用纹理绘制。[texture_index](VoxelTool.md#i_texture_index) 的值将添加到受影响体素的纹理索引中。纹理的权重将根据 [texture_falloff](VoxelTool.md#i_texture_falloff) 和 [texture_opacity](VoxelTool.md#i_texture_opacity) 的值进行混合。结果将根据网格器使用的纹理模式而有所不同。


## 属性描述

### [ChannelId](VoxelBuffer.md#enumerations)<span id="i_channel"></span> **channel**

设置将编辑的通道。在地形节点上使用时，它将根据数据流和生成器默认为第一个可用通道。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_eraser_value"></span> **eraser_value**

设置在以 [MODE_REMOVE](VoxelTool.md#i_MODE_REMOVE) 模式编辑 [VoxelBuffer.CHANNEL_TYPE](VoxelBuffer.md#i_CHANNEL_TYPE) 通道时用于擦除体素的值。仅与方块体素相关。

### [Mode](VoxelTool.md#enumerations)<span id="i_mode"></span> **mode**

设置 `do_*` 函数的行为方式。这可能会因通道而异。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_sdf_scale"></span> **sdf_scale**

处理平滑体素时，对有符号距离场应用缩放。高缩放（1 或更高）往往会产生块状结果，低缩放（低于 1，但不要太接近零）往往会更平滑。


这与体素的 [Depth](VoxelBuffer.md#enumerations) 配置有关。对于 8 位和 16 位，有符号距离场可以取的值范围有限，默认情况下被限制在 -1..1，因此梯度只能跨越 2 个体素。但使用 LOD 时，最好将该范围拉伸到更长的距离，这可以通过缩放 SDF 值来实现。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_sdf_strength"></span> **sdf_strength**

在 [MODE_ADD](VoxelTool.md#i_MODE_ADD) 或 [MODE_REMOVE](VoxelTool.md#i_MODE_REMOVE) 模式下编辑平滑地形的 [VoxelBuffer.CHANNEL_SDF](VoxelBuffer.md#i_CHANNEL_SDF) 时，决定当前值与工具设置值之间的插值相位。可以理解为添加或减去的“物质”量。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_texture_falloff"></span> **texture_falloff**

范围 [0.001..1.0]。当工具设置为 [MODE_TEXTURE_PAINT](VoxelTool.md#i_MODE_TEXTURE_PAINT) 时，决定纹理混合强度。较低的值产生更锐利的过渡。可以类比图像编辑程序中的画笔硬度。这仅与平滑体素和支持长渐变的纹理模式相关。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_texture_index"></span> **texture_index**

平滑体素纹理绘制模式中使用的纹理索引。此索引的选择取决于你如何设置带纹理体素网格的渲染（例如，纹理数组中的图层索引）。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_texture_opacity"></span> **texture_opacity**

范围 [0.0..1.0]。当工具设置为 [MODE_TEXTURE_PAINT](VoxelTool.md#i_MODE_TEXTURE_PAINT) 时，决定 [texture_index](VoxelTool.md#i_texture_index) 的最大权重。可以类比图像编辑程序中的画笔不透明度。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_value"></span> **value**

设置将使用的体素值。编辑 [VoxelBuffer.CHANNEL_SDF](VoxelBuffer.md#i_CHANNEL_SDF) 时与此无关。

## 方法描述

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_color_to_u16"></span> **color_to_u16**( [Color](https://docs.godotengine.org/en/stable/classes/class_color.html) color ) 

将归一化的 4 浮点颜色编码为 16 位整数数据。它与 COLOR 通道一起使用，适用于该通道表示直接颜色（不使用调色板）的情况。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_color_to_u16_weights"></span> **color_to_u16_weights**( [Color](https://docs.godotengine.org/en/stable/classes/class_color.html) _unnamed_arg0 ) 

将归一化的 4 浮点颜色编码为 16 位整数数据，用于 WEIGHTS 通道。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_color_to_u32"></span> **color_to_u32**( [Color](https://docs.godotengine.org/en/stable/classes/class_color.html) color ) 

将归一化的 4 浮点颜色编码为 32 位整数数据，用于 COLOR 通道。

### [void](#)<span id="i_copy"></span> **copy**( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) src_pos, [VoxelBuffer](VoxelBuffer.md) dst_buffer, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) channels_mask=255, [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html) with_metadata=true ) 

复制长方体中的体素并将其存储到传入的缓冲区中。源格式将覆盖目标格式。

`src_pos` 是长方体的最低角，其大小由 `dst_buffer` 的大小决定。

`channels_mask` 是一个位掩码，每个位表示将复制哪些通道。示例：`1 << VoxelBuffer.CHANNEL_SDF` 仅获取 SDF 数据。如果全部都要，请使用 [VoxelBuffer.ALL_CHANNELS_MASK](VoxelBuffer.md#i_ALL_CHANNELS_MASK)。

### [void](#)<span id="i_do_box"></span> **do_box**( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) begin, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) end ) 

对地形的一个矩形长方体区域进行操作。

对于方块体素，`begin` 和 `end` 是包含的。

对于平滑体素，`end` 是不包含的。

在调用此函数之前，你可以通过设置 [mode](VoxelTool.md#i_mode) 选择要执行的操作。对于方块体素，你还可以设置 [value](VoxelTool.md#i_value) 来选择要使用的体素 ID。

### [void](#)<span id="i_do_mesh"></span> **do_mesh**( [VoxelMeshSDF](VoxelMeshSDF.md) mesh_sdf, [Transform3D](https://docs.godotengine.org/en/stable/classes/class_transform3d.html) transform, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) isolevel=0.0 ) 

将给定的网格形状应用于地形。模型必须使用 [VoxelMeshSDF](VoxelMeshSDF.md) 烘焙为有符号距离场。`transform` 可用于定位、旋转和缩放模型。非均匀缩放可能会引入伪影。`isolevel` 是一个距离，增加或减少会分别使模型膨胀或收缩。质量取决于模型 SDF 的分辨率，性能将低于 [do_sphere](VoxelTool.md#i_do_sphere) 等基本形状。

### [void](#)<span id="i_do_path"></span> **do_path**( [PackedVector3Array](https://docs.godotengine.org/en/stable/classes/class_packedvector3array.html) points, [PackedFloat32Array](https://docs.godotengine.org/en/stable/classes/class_packedfloat32array.html) radii ) 

沿着由一系列点定义的“管道”进行雕刻或放置，每个点都有对应的半径来控制管道在该点的宽度。路径的起点和终点是圆形的。这相当于放置/雕刻多个上下半径变化的相连胶囊体。路径不使用贝塞尔曲线或样条线，每个点都与下一个点线性连接。如果你需要更平滑，可以在需要的区域添加点。点越多，速度越慢。

### [void](#)<span id="i_do_point"></span> **do_point**( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) pos ) 

对单个体素进行操作。

在调用此函数之前，你可以通过设置 [mode](VoxelTool.md#i_mode) 选择要执行的操作。对于方块体素，你还可以设置 [value](VoxelTool.md#i_value) 来选择要使用的体素 ID。

此函数不适合平滑体素，可能会引入块状感。

### [void](#)<span id="i_do_sphere"></span> **do_sphere**( [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) center, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) radius ) 

对球体内的体素进行操作。

在调用此函数之前，你可以通过设置 [mode](VoxelTool.md#i_mode) 选择要执行的操作。对于方块体素，你还可以设置 [value](VoxelTool.md#i_value) 来选择要使用的体素 ID。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_get_voxel"></span> **get_voxel**( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) pos ) 

获取 `pos` 坐标处体素的数据。返回的值将是无符号整数。值的含义取决于工具当前设置的 [channel](VoxelTool.md#i_channel)。

对于平滑体素，使用 [VoxelBuffer.CHANNEL_SDF](VoxelBuffer.md#i_CHANNEL_SDF) 时返回的值将是编码值，因此你可以改用 [get_voxel_f](VoxelTool.md#i_get_voxel_f) 获取浮点值。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_get_voxel_f"></span> **get_voxel_f**( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) pos ) 

获取 `pos` 坐标处体素的数据，并将其解释为浮点 SDF 值。建议用它来查询平滑体素的 [VoxelBuffer.CHANNEL_SDF](VoxelBuffer.md#i_CHANNEL_SDF) 通道。

### [Variant](https://docs.godotengine.org/en/stable/classes/class_variant.html)<span id="i_get_voxel_metadata"></span> **get_voxel_metadata**( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) pos ) 

获取附加到特定体素上的任意数据。

### [void](#)<span id="i_grow_sphere"></span> **grow_sphere**( [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) sphere_center, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) sphere_radius, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) strength ) 

根据 `mode` 向球体内的所有体素添加/移除一个值。添加/移除的值在球体中心处等于 `strength`，并向球体表面线性递减为零。球体外的体素不受影响。

`sphere_center` 是地形中将被平滑处理的位置。

`sphere_radius` 是从 `center` 起的球体半径，在该范围内体素值将受到影响。应大于零。

`strength` 控制向体素添加/从体素移除的最大值。建议范围为 [0, 10]。

注意 1：目前仅对使用 SDF 数据（平滑体素）的地形实现。

注意 2：此函数旨在类比虚幻引擎 Voxel 插件中的 Surface 工具。

注意 3：此方法假设地形 SDF 是连贯的。如果不是，你可能会注意到地形侵蚀或生长的速度存在差异。例如，一些生成器在远离表面时会回退到恒定 SDF 以加速计算（参见 [VoxelGeneratorGraph.sdf_clip_threshold](VoxelGeneratorGraph.md#i_sdf_clip_threshold)）。

注意 4：如果你想每帧调用此方法以“平滑地”挖掘地形，另一种选择是使用 [do_sphere](VoxelTool.md#i_do_sphere)，但不要将中心放在表面上，而是将其移回约半径的 0.95%，这样只有一小部分球体会穿透，从而逐步挖掘出一个洞。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_is_area_editable"></span> **is_area_editable**( [AABB](https://docs.godotengine.org/en/stable/classes/class_aabb.html) box ) 

如果指定的体素区域可编辑，则返回 `true`。这也可以解释为该区域“已加载”。注意：使用 LOD 时，只有最近的 LOD（0）可编辑。其他因素也会影响区域是否可编辑，例如流式加载模式或地形边界。

### [Color](https://docs.godotengine.org/en/stable/classes/class_color.html)<span id="i_normalize_color"></span> **normalize_color**( [Color](https://docs.godotengine.org/en/stable/classes/class_color.html) _unnamed_arg0 ) 

一个辅助方法，用于将 `Color` 的通道之和设置为 1。

### [void](#)<span id="i_paste"></span> **paste**( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) dst_pos, [VoxelBuffer](VoxelBuffer.md) src_buffer, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) channels_mask=255 ) 

从给定缓冲区在特定位置粘贴一个长方体中的体素。

`dst_pos` 是长方体的最低角，其大小由 `src_buffer` 的大小决定。

`channels_mask` 是一个位掩码，每个位表示将修改哪些通道。示例：`1 << VoxelBuffer.CHANNEL_SDF` 仅写入 SDF 数据。如果全部都要，请使用 [VoxelBuffer.ALL_CHANNELS_MASK](VoxelBuffer.md#i_ALL_CHANNELS_MASK)。

### [void](#)<span id="i_paste_masked"></span> **paste_masked**( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) dst_pos, [VoxelBuffer](VoxelBuffer.md) src_buffer, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) channels_mask, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) mask_channel, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) mask_value ) 

从给定缓冲区在特定位置粘贴一个长方体中的体素。在掩码通道中具有特定值的体素将不会被粘贴。

`dst_pos` 是长方体的最低角，其大小由 `src_buffer` 的大小决定。

`channels_mask` 是一个位掩码，每个位表示将修改哪些通道。示例：`1 << VoxelBuffer.CHANNEL_SDF` 仅写入 SDF 数据。如果全部都要，请使用 [VoxelBuffer.ALL_CHANNELS_MASK](VoxelBuffer.md#i_ALL_CHANNELS_MASK)。

`src_mask_channel` 源缓冲区中用于查找掩码值的通道。

`src_mask_value` 如果源缓冲区的体素在用于掩码的通道中具有此值，则它们将不会被粘贴。

### [void](#)<span id="i_paste_masked_writable_list"></span> **paste_masked_writable_list**( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) position, [VoxelBuffer](VoxelBuffer.md) voxels, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) channels_mask, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) src_mask_channel, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) src_mask_value, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) dst_mask_channel, [PackedInt32Array](https://docs.godotengine.org/en/stable/classes/class_packedint32array.html) dst_writable_list ) 

从给定缓冲区在特定位置粘贴一个长方体中的体素。源缓冲区中在掩码通道中具有特定值的体素将不会被粘贴，并且目标处已有的体素只有在具有特定值时才会被修改。

`dst_pos` 是长方体的最低角，其大小由 `src_buffer` 的大小决定。

`channels_mask` 是一个位掩码，每个位表示将修改哪些通道。示例：`1 << VoxelBuffer.CHANNEL_SDF` 仅写入 SDF 数据。如果全部都要，请使用 [VoxelBuffer.ALL_CHANNELS_MASK](VoxelBuffer.md#i_ALL_CHANNELS_MASK)。

`src_mask_channel` 源缓冲区中用于查找掩码值的通道。

`src_mask_value` 如果源缓冲区的体素在用于掩码的通道中具有此值，则它们将不会被粘贴。

`dst_mask_channel` 目标处用于选择可写体素的通道。

`dst_writable_list` 目标体素必须具有的值列表，才能被写入。该列表中的值必须在 0 到 65535 之间。大量的值也可能影响性能。

### [VoxelRaycastResult](VoxelRaycastResult.md)<span id="i_raycast"></span> **raycast**( [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) origin, [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) direction, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) max_distance=10.0, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) collision_mask=4294967295 ) 

运行基于体素的射线投射，从原点沿方向查找第一个命中。坐标位于世界空间中。

如果命中了体素，则返回一个结果对象，否则返回 `null`。

当碰撞体不可靠时，这很有用。它也可能更快（至少在短距离内），并且能更精确地找到命中的体素。它在内部使用 DDA 算法。

`collision_mask` 目前仅用于方块体素。它与 [VoxelBlockyModel.collision_mask](VoxelBlockyModel.md#i_collision_mask) 结合使用，以决定射线可以与哪些体素类型碰撞。

### [void](#)<span id="i_set_raycast_normal_enabled"></span> **set_raycast_normal_enabled**( [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html) enabled ) 

设置 [raycast](VoxelTool.md#i_raycast) 是否计算命中法线。默认情况下为 true。

### [void](#)<span id="i_set_voxel"></span> **set_voxel**( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) pos, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) v ) 

设置当前通道上特定体素的原始整数值。

### [void](#)<span id="i_set_voxel_f"></span> **set_voxel_f**( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) pos, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) v ) 

设置特定体素的有符号距离场（SDF）值。这最好在 SDF 通道上使用。

### [void](#)<span id="i_set_voxel_metadata"></span> **set_voxel_metadata**( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) pos, [Variant](https://docs.godotengine.org/en/stable/classes/class_variant.html) meta ) 

在特定体素上附加任意数据。旧数据将被替换。传入 `null` 将清除元数据。

如果底层体素可以被保存，此元数据也会被保存，因此请确保数据支持序列化（即你不能在其中放入节点或任意对象）。

### [void](#)<span id="i_smooth_sphere"></span> **smooth_sphere**( [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) sphere_center, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) sphere_radius, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) blur_radius ) 

通过在球形区域中执行盒式模糊来平滑地形。强度在球体中心处最大，并向球体表面线性递减为零。球体外的体素不受影响。

`sphere_center` 是地形中将被平滑处理的位置。

`sphere_radius` 是从 `center` 起的球体半径，在该范围内体素值将受到影响。应大于零。

`blur_radius` 用于采样计算平均体素值的盒式模糊长度的一半。值越大，平滑越激进。应至少为 1。

注意 1：目前仅对使用 SDF 数据（平滑体素）的地形实现。

注意 2：注意不要使用过高的 `sphere_radius` 和过高的 `blur_radius`，因为如果每秒调用 60 次，性能会迅速下降。

### [Vector4i](https://docs.godotengine.org/en/stable/classes/class_vector4i.html)<span id="i_u16_indices_to_vec4i"></span> **u16_indices_to_vec4i**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) _unnamed_arg0 ) 

将 INDICES 通道的原始体素整数数据解码为 4 整数向量。

### [Color](https://docs.godotengine.org/en/stable/classes/class_color.html)<span id="i_u16_weights_to_color"></span> **u16_weights_to_color**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) _unnamed_arg0 ) 

将 WEIGHTS 通道的原始体素整数数据解码为归一化的 4 浮点颜色。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_vec4i_to_u16_indices"></span> **vec4i_to_u16_indices**( [Vector4i](https://docs.godotengine.org/en/stable/classes/class_vector4i.html) _unnamed_arg0 ) 

将 4 整数向量编码为 16 位整数体素数据，用于 INDICES 通道。

_生成于 2026-09-12_
