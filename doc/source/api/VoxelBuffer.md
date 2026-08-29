# VoxelBuffer

继承自：[RefCounted](https://docs.godotengine.org/en/stable/classes/class_refcounted.html)

存储体素数据的 3D 网格。

## 描述：

此为稠密体素数据存储（每个单元格都保存数据，没有稀疏化的空间优化）。它像普通 3D 网格一样工作，每个单元格包含一个体素值。按可配置位深的通道组织。值可以解释为无符号整数、定点数或浮点数。更多信息参见 [Depth](VoxelBuffer.md#enumerations)。

还可以存储任意元数据，既可以为整个缓冲区存储，也可以按体素存储，但成本更高。此元数据可以随体素一起保存和加载，但你必须确保数据是可序列化的（即不应包含节点或任意对象）。

## 方法：


返回值                                                                                           | 函数签名                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     
--------------------------------------------------------------------------------------------- | -----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
[void](#)                                                                                     | [clear](#i_clear) ( )                                                                                                                                                                                                                                                                                                                                                                                                                                                                    
[void](#)                                                                                     | [clear_voxel_metadata](#i_clear_voxel_metadata) ( )                                                                                                                                                                                                                                                                                                                                                                                                                                      
[void](#)                                                                                     | [clear_voxel_metadata_in_area](#i_clear_voxel_metadata_in_area) ( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) min_pos, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) max_pos )                                                                                                                                                                                                                                       
[void](#)                                                                                     | [compress_uniform_channels](#i_compress_uniform_channels) ( )                                                                                                                                                                                                                                                                                                                                                                                                                            
[void](#)                                                                                     | [copy_channel_from](#i_copy_channel_from) ( [VoxelBuffer](VoxelBuffer.md) other, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) channel )                                                                                                                                                                                                                                                                                                                          
[void](#)                                                                                     | [copy_channel_from_area](#i_copy_channel_from_area) ( [VoxelBuffer](VoxelBuffer.md) other, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) src_min, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) src_max, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) dst_min, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) channel )                                        
[void](#)                                                                                     | [copy_voxel_metadata_in_area](#i_copy_voxel_metadata_in_area) ( [VoxelBuffer](VoxelBuffer.md) src_buffer, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) src_min_pos, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) src_max_pos, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) dst_min_pos )                                                                                           
[void](#)                                                                                     | [create](#i_create) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) sx, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) sy, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) sz )                                                                                                                                                                                                                                        
[ImageTexture3D](https://docs.godotengine.org/en/stable/classes/class_imagetexture3d.html)    | [create_3d_texture_from_sdf_zxy](#i_create_3d_texture_from_sdf_zxy) ( [Format](https://docs.godotengine.org/en/stable/classes/class_image.html#enum-image-format) output_format ) const                                                                                                                                                                                                                                                                                                  
[Image[]](https://docs.godotengine.org/en/stable/classes/class_image[].html)                  | [debug_print_sdf_y_slices](#i_debug_print_sdf_y_slices) ( [float](https://docs.godotengine.org/en/stable/classes/class_float.html) scale=1.0 ) const                                                                                                                                                                                                                                                                                                                                     
[void](#)                                                                                     | [decompress_channel](#i_decompress_channel) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) channel )                                                                                                                                                                                                                                                                                                                                                             
[void](#)                                                                                     | [downscale_to](#i_downscale_to) ( [VoxelBuffer](VoxelBuffer.md) dst, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) src_min, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) src_max, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) dst_min ) const                                                                                                                                      
[void](#)                                                                                     | [fill](#i_fill) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) value, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) channel=0 )                                                                                                                                                                                                                                                                                                           
[void](#)                                                                                     | [fill_area](#i_fill_area) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) value, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) min, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) max, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) channel=0 )                                                                                                                         
[void](#)                                                                                     | [fill_area_f](#i_fill_area_f) ( [float](https://docs.godotengine.org/en/stable/classes/class_float.html) value, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) min, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) max, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) channel )                                                                                                                   
[void](#)                                                                                     | [fill_f](#i_fill_f) ( [float](https://docs.godotengine.org/en/stable/classes/class_float.html) value, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) channel=0 )                                                                                                                                                                                                                                                                                                   
[void](#)                                                                                     | [for_each_voxel_metadata](#i_for_each_voxel_metadata) ( [Callable](https://docs.godotengine.org/en/stable/classes/class_callable.html) callback ) const                                                                                                                                                                                                                                                                                                                                  
[void](#)                                                                                     | [for_each_voxel_metadata_in_area](#i_for_each_voxel_metadata_in_area) ( [Callable](https://docs.godotengine.org/en/stable/classes/class_callable.html) callback, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) min_pos, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) max_pos )                                                                                                                                        
[Allocator](VoxelBuffer.md#enumerations)                                                      | [get_allocator](#i_get_allocator) ( ) const                                                                                                                                                                                                                                                                                                                                                                                                                                              
[Variant](https://docs.godotengine.org/en/stable/classes/class_variant.html)                  | [get_block_metadata](#i_get_block_metadata) ( ) const                                                                                                                                                                                                                                                                                                                                                                                                                                    
[PackedByteArray](https://docs.godotengine.org/en/stable/classes/class_packedbytearray.html)  | [get_channel_as_byte_array](#i_get_channel_as_byte_array) ( [ChannelId](VoxelBuffer.md#enumerations) channel_index ) const                                                                                                                                                                                                                                                                                                                                                               
[Compression](VoxelBuffer.md#enumerations)                                                    | [get_channel_compression](#i_get_channel_compression) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) channel ) const                                                                                                                                                                                                                                                                                                                                             
[Depth](VoxelBuffer.md#enumerations)                                                          | [get_channel_depth](#i_get_channel_depth) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) channel ) const                                                                                                                                                                                                                                                                                                                                                         
[Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html)                | [get_size](#i_get_size) ( ) const                                                                                                                                                                                                                                                                                                                                                                                                                                                        
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)                          | [get_voxel](#i_get_voxel) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) x, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) y, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) z, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) channel=0 ) const                                                                                                                                               
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)                      | [get_voxel_f](#i_get_voxel_f) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) x, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) y, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) z, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) channel=0 ) const                                                                                                                                           
[Variant](https://docs.godotengine.org/en/stable/classes/class_variant.html)                  | [get_voxel_metadata](#i_get_voxel_metadata) ( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) pos ) const                                                                                                                                                                                                                                                                                                                                                 
[VoxelTool](VoxelTool.md)                                                                     | [get_voxel_tool](#i_get_voxel_tool) ( )                                                                                                                                                                                                                                                                                                                                                                                                                                                  
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)                          | [get_voxel_v](#i_get_voxel_v) ( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) pos, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) channel=0 ) const                                                                                                                                                                                                                                                                               
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)                        | [is_uniform](#i_is_uniform) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) channel ) const                                                                                                                                                                                                                                                                                                                                                                       
[void](#)                                                                                     | [mirror](#i_mirror) ( [Axis](https://docs.godotengine.org/en/stable/classes/class_vector3i.html#enum-vector3i-axis) axis )                                                                                                                                                                                                                                                                                                                                                               
[void](#)                                                                                     | [op_add_buffer_f](#i_op_add_buffer_f) ( [VoxelBuffer](VoxelBuffer.md) other, [ChannelId](VoxelBuffer.md#enumerations) channel )                                                                                                                                                                                                                                                                                                                                                          
[void](#)                                                                                     | [op_max_buffer_f](#i_op_max_buffer_f) ( [VoxelBuffer](VoxelBuffer.md) other, [ChannelId](VoxelBuffer.md#enumerations) channel )                                                                                                                                                                                                                                                                                                                                                          
[void](#)                                                                                     | [op_min_buffer_f](#i_op_min_buffer_f) ( [VoxelBuffer](VoxelBuffer.md) other, [ChannelId](VoxelBuffer.md#enumerations) channel )                                                                                                                                                                                                                                                                                                                                                          
[void](#)                                                                                     | [op_mul_buffer_f](#i_op_mul_buffer_f) ( [VoxelBuffer](VoxelBuffer.md) other, [ChannelId](VoxelBuffer.md#enumerations) channel )                                                                                                                                                                                                                                                                                                                                                          
[void](#)                                                                                     | [op_mul_value_f](#i_op_mul_value_f) ( [float](https://docs.godotengine.org/en/stable/classes/class_float.html) other, [ChannelId](VoxelBuffer.md#enumerations) channel )                                                                                                                                                                                                                                                                                                                 
[void](#)                                                                                     | [op_select_less_src_f_dst_i_values](#i_op_select_less_src_f_dst_i_values) ( [VoxelBuffer](VoxelBuffer.md) src, [ChannelId](VoxelBuffer.md#enumerations) src_channel, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) threshold, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) value_if_less, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) value_if_more, [ChannelId](VoxelBuffer.md#enumerations) dst_channel )  
[void](#)                                                                                     | [op_sub_buffer_f](#i_op_sub_buffer_f) ( [VoxelBuffer](VoxelBuffer.md) other, [ChannelId](VoxelBuffer.md#enumerations) channel )                                                                                                                                                                                                                                                                                                                                                          
[void](#)                                                                                     | [remap_values](#i_remap_values) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) channel, [PackedInt32Array](https://docs.godotengine.org/en/stable/classes/class_packedint32array.html) map )                                                                                                                                                                                                                                                                     
[void](#)                                                                                     | [rotate_90](#i_rotate_90) ( [Axis](https://docs.godotengine.org/en/stable/classes/class_vector3i.html#enum-vector3i-axis) axis, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) turns )                                                                                                                                                                                                                                                                             
[void](#)                                                                                     | [set_block_metadata](#i_set_block_metadata) ( [Variant](https://docs.godotengine.org/en/stable/classes/class_variant.html) meta )                                                                                                                                                                                                                                                                                                                                                        
[void](#)                                                                                     | [set_channel_depth](#i_set_channel_depth) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) channel, [Depth](VoxelBuffer.md#enumerations) depth )                                                                                                                                                                                                                                                                                                                   
[void](#)                                                                                     | [set_channel_from_byte_array](#i_set_channel_from_byte_array) ( [ChannelId](VoxelBuffer.md#enumerations) channel_index, [PackedByteArray](https://docs.godotengine.org/en/stable/classes/class_packedbytearray.html) data )                                                                                                                                                                                                                                                              
[void](#)                                                                                     | [set_voxel](#i_set_voxel) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) value, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) x, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) y, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) z, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) channel=0 )                                                                         
[void](#)                                                                                     | [set_voxel_f](#i_set_voxel_f) ( [float](https://docs.godotengine.org/en/stable/classes/class_float.html) value, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) x, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) y, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) z, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) channel=0 )                                                                 
[void](#)                                                                                     | [set_voxel_metadata](#i_set_voxel_metadata) ( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) pos, [Variant](https://docs.godotengine.org/en/stable/classes/class_variant.html) value )                                                                                                                                                                                                                                                                   
[void](#)                                                                                     | [set_voxel_v](#i_set_voxel_v) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) value, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) pos, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) channel=0 )                                                                                                                                                                                                         
[void](#)                                                                                     | [update_3d_texture_from_sdf_zxy](#i_update_3d_texture_from_sdf_zxy) ( [ImageTexture3D](https://docs.godotengine.org/en/stable/classes/class_imagetexture3d.html) existing_texture ) const                                                                                                                                                                                                                                                                                                
<p></p>

## 枚举：<span id="enumerations"></span>

枚举 **ChannelId**：

- <span id="i_CHANNEL_TYPE"></span>**CHANNEL_TYPE** = **0** --- 用于存储体素类型的通道。由 [VoxelMesherBlocky](VoxelMesherBlocky.md) 使用。
- <span id="i_CHANNEL_SDF"></span>**CHANNEL_SDF** = **1** --- 用于存储 SDF 数据（有符号距离场）的通道。由 [VoxelMesherTransvoxel](VoxelMesherTransvoxel.md) 和其他平滑网格生成器使用。值最好以浮点数形式访问。负值位于等值面以下（物质内部），正值位于表面以上（物质外部）。
- <span id="i_CHANNEL_COLOR"></span>**CHANNEL_COLOR** = **2** --- 用于存储颜色数据的通道。由 [VoxelMesherCubes](VoxelMesherCubes.md) 使用。
- <span id="i_CHANNEL_INDICES"></span>**CHANNEL_INDICES** = **3** --- 用于存储材质索引的通道。与平滑体素一起使用。
- <span id="i_CHANNEL_WEIGHTS"></span>**CHANNEL_WEIGHTS** = **4** --- 当每个体素可以存储多个索引时，用于存储材质权重的通道。与平滑体素一起使用。
- <span id="i_CHANNEL_DATA5"></span>**CHANNEL_DATA5** = **5** --- 空闲通道。引擎尚未使用。
- <span id="i_CHANNEL_DATA6"></span>**CHANNEL_DATA6** = **6** --- 空闲通道。引擎尚未使用。
- <span id="i_CHANNEL_DATA7"></span>**CHANNEL_DATA7** = **7** --- 空闲通道。引擎尚未使用。
- <span id="i_MAX_CHANNELS"></span>**MAX_CHANNELS** = **8** --- 一个 [VoxelBuffer](VoxelBuffer.md) 可以拥有的最大通道数。

枚举 **ChannelMask**：

- <span id="i_CHANNEL_TYPE_BIT"></span>**CHANNEL_TYPE_BIT** = **1** --- 在与 [CHANNEL_TYPE](VoxelBuffer.md#i_CHANNEL_TYPE) 对应的位置上置位的位掩码。
- <span id="i_CHANNEL_SDF_BIT"></span>**CHANNEL_SDF_BIT** = **2** --- 在与 [CHANNEL_SDF](VoxelBuffer.md#i_CHANNEL_SDF) 对应的位置上置位的位掩码。
- <span id="i_CHANNEL_COLOR_BIT"></span>**CHANNEL_COLOR_BIT** = **4** --- 在与 [CHANNEL_COLOR](VoxelBuffer.md#i_CHANNEL_COLOR) 对应的位置上置位的位掩码。
- <span id="i_CHANNEL_INDICES_BIT"></span>**CHANNEL_INDICES_BIT** = **8** --- 在与 [CHANNEL_INDICES](VoxelBuffer.md#i_CHANNEL_INDICES) 对应的位置上置单个位的位掩码。
- <span id="i_CHANNEL_WEIGHTS_BIT"></span>**CHANNEL_WEIGHTS_BIT** = **16** --- 在与 [CHANNEL_WEIGHTS](VoxelBuffer.md#i_CHANNEL_WEIGHTS) 对应的位置上置位的位掩码。
- <span id="i_CHANNEL_DATA5_BIT"></span>**CHANNEL_DATA5_BIT** = **32** --- 在与 [CHANNEL_DATA5](VoxelBuffer.md#i_CHANNEL_DATA5) 对应的位置上置位的位掩码。
- <span id="i_CHANNEL_DATA6_BIT"></span>**CHANNEL_DATA6_BIT** = **64** --- 在与 [CHANNEL_DATA6](VoxelBuffer.md#i_CHANNEL_DATA6) 对应的位置上置位的位掩码。
- <span id="i_CHANNEL_DATA7_BIT"></span>**CHANNEL_DATA7_BIT** = **128** --- 在与 [CHANNEL_DATA7](VoxelBuffer.md#i_CHANNEL_DATA7) 对应的位置上置位的位掩码。
- <span id="i_ALL_CHANNELS_MASK"></span>**ALL_CHANNELS_MASK** = **255** --- 所有通道位全部置位的位掩码。

枚举 **Depth**：

- <span id="i_DEPTH_8_BIT"></span>**DEPTH_8_BIT** = **0** --- 体素将以 8 位存储。原始值的范围是 0 到 255。浮点值可以取分布在 -10.0 到 10.0 之间的 255 个值。超出范围的值将被钳制。
- <span id="i_DEPTH_16_BIT"></span>**DEPTH_16_BIT** = **1** --- 体素将以 16 位存储。原始值的范围是 0 到 65,535。浮点值可以取分布在 -500.0 到 500.0 之间的 65,535 个值。超出范围的值将被钳制。
- <span id="i_DEPTH_32_BIT"></span>**DEPTH_32_BIT** = **2** --- 体素将以 32 位存储。原始值的范围是 0 到 4,294,967,295，浮点值将使用常规的 IEEE 754 表示（`float`）。
- <span id="i_DEPTH_64_BIT"></span>**DEPTH_64_BIT** = **3** --- 体素将以 64 位存储。原始值的范围是 0 到 18,446,744,073,709,551,615，浮点值将使用常规的 IEEE 754 表示（`double`）。
- <span id="i_DEPTH_COUNT"></span>**DEPTH_COUNT** = **4** --- 位深配置的总数量。

枚举 **Compression**：

- <span id="i_COMPRESSION_NONE"></span>**COMPRESSION_NONE** = **0** --- 通道未压缩。每个值都单独存储在内存中的数组里。
- <span id="i_COMPRESSION_UNIFORM"></span>**COMPRESSION_UNIFORM** = **1** --- 通道的所有体素都具有相同的值，因此它们被存储为单个值以节省空间。
- <span id="i_COMPRESSION_COUNT"></span>**COMPRESSION_COUNT** = **2** --- 一共有多少种压缩模式。

枚举 **Allocator**：

- <span id="i_ALLOCATOR_DEFAULT"></span>**ALLOCATOR_DEFAULT** = **0** --- 使用 Godot 的默认内存分配器（在撰写本文时是 `malloc`）。适合偶尔创建、尺寸不常见或非常大的缓冲区。
- <span id="i_ALLOCATOR_POOL"></span>**ALLOCATOR_POOL** = **1** --- 使用池分配器。当缓冲区以相似尺寸非常频繁地创建时，这可能比默认分配器更快。此内存在使用后会保持已分配状态，假设其他缓冲区很快也会需要它。不支持非常大的缓冲区（大于 2 兆字节）。
- <span id="i_ALLOCATOR_COUNT"></span>**ALLOCATOR_COUNT** = **2**


## 常量：

- <span id="i_MAX_SIZE"></span>**MAX_SIZE** = **65535** --- 缓冲区序列化时可以具有的最大尺寸。包含均匀压缩体素的缓冲区可以达到此限制，但实际上，该限制要低得多，并取决于可用内存。

## 方法描述

### [void](#)<span id="i_clear"></span> **clear**( ) 

清除缓冲区的所有内容并将其尺寸重置为零。通道位深和默认值会被保留。

### [void](#)<span id="i_clear_voxel_metadata"></span> **clear_voxel_metadata**( ) 

清除所有按体素存储的元数据。

### [void](#)<span id="i_clear_voxel_metadata_in_area"></span> **clear_voxel_metadata_in_area**( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) min_pos, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) max_pos ) 

清除指定区域内按体素存储的元数据。

### [void](#)<span id="i_compress_uniform_channels"></span> **compress_uniform_channels**( ) 

找出所有体素值都相同的通道，并只存储一个值以降低内存占用。例如当地形的大部分区域充满空气时，这很有效。

### [void](#)<span id="i_copy_channel_from"></span> **copy_channel_from**( [VoxelBuffer](VoxelBuffer.md) other, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) channel ) 

将另一个 [VoxelBuffer](VoxelBuffer.md) 的通道中的所有值复制到当前缓冲区的相同通道中。位深格式必须匹配。

### [void](#)<span id="i_copy_channel_from_area"></span> **copy_channel_from_area**( [VoxelBuffer](VoxelBuffer.md) other, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) src_min, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) src_max, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) dst_min, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) channel ) 

将另一个 [VoxelBuffer](VoxelBuffer.md) 通道的子区域中的值复制到当前缓冲区的相同通道中，复制到特定位置。位深格式必须匹配。

如果区域的角表示负尺寸区域，它们将被重新排序。

如果坐标完全或部分超出边界，它们将被自动裁剪。

不支持在同一缓冲区中向重叠区域复制。这种情况下你可以使用一个中间缓冲区。

### [void](#)<span id="i_copy_voxel_metadata_in_area"></span> **copy_voxel_metadata_in_area**( [VoxelBuffer](VoxelBuffer.md) src_buffer, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) src_min_pos, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) src_max_pos, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) dst_min_pos ) 

将另一个 [VoxelBuffer](VoxelBuffer.md) 子区域中的按体素元数据复制到当前缓冲区的特定位置。值将是浅拷贝。

如果区域的角表示负尺寸区域，它们将被重新排序。

如果坐标完全或部分超出边界，它们将被自动裁剪。

不支持在同一缓冲区中向重叠区域复制。这种情况下你可以使用一个中间缓冲区。

### [void](#)<span id="i_create"></span> **create**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) sx, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) sy, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) sz ) 

清除缓冲区并为其指定尺寸。

### [ImageTexture3D](https://docs.godotengine.org/en/stable/classes/class_imagetexture3d.html)<span id="i_create_3d_texture_from_sdf_zxy"></span> **create_3d_texture_from_sdf_zxy**( [Format](https://docs.godotengine.org/en/stable/classes/class_image.html#enum-image-format) output_format ) 

从 SDF 通道创建 3D 纹理。

如果 `output_format` 是 8 位像素格式，纹理将包含归一化的有符号距离，其中 0.5 为等值面，0 为表面下方最远处，1 为表面上方最远处。

仅支持 16 位 SDF 通道。

仅支持 [Image.FORMAT_R8](https://docs.godotengine.org/en/stable/classes/class_image.html#class-image-constant-FORMAT-R8) 和 [Image.FORMAT_L8](https://docs.godotengine.org/en/stable/classes/class_image.html#class-image-constant-FORMAT-L8) 输出格式。

注意：在着色器中采样此纹理时，需要使用 `.yxz` 对 3D 坐标进行 swizzle。这是体素的内部存储方式，此函数不会改变此约定。

### [Image[]](https://docs.godotengine.org/en/stable/classes/class_image[].html)<span id="i_debug_print_sdf_y_slices"></span> **debug_print_sdf_y_slices**( [float](https://docs.godotengine.org/en/stable/classes/class_float.html) scale=1.0 ) 

将 SDF 通道的内容渲染到图像中，蓝色渐变表示负值（表面以下），黄色渐变表示正值（表面以上）。每张图像对应缓冲区的一个 XZ 切片。

`scale` 参数可用于通过缩放 SDF 来改变图像的对比度。

### [void](#)<span id="i_decompress_channel"></span> **decompress_channel**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) channel ) 

如果给定通道当前已压缩，则将其解压缩，使所有体素值都被完整地单独存储。这会占用更多内存。

### [void](#)<span id="i_downscale_to"></span> **downscale_to**( [VoxelBuffer](VoxelBuffer.md) dst, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) src_min, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) src_max, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) dst_min ) 

生成此缓冲区缩小 2 倍的版本，不进行任何形式的插值（即使用最近邻）。

元数据不会被复制。

### [void](#)<span id="i_fill"></span> **fill**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) value, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) channel=0 ) 

用特定的原始值填充此缓冲区的一个通道。

### [void](#)<span id="i_fill_area"></span> **fill_area**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) value, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) min, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) max, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) channel=0 ) 

用特定的原始值填充此缓冲区中一个通道的某个区域。

### [void](#)<span id="i_fill_area_f"></span> **fill_area_f**( [float](https://docs.godotengine.org/en/stable/classes/class_float.html) value, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) min, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) max, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) channel ) 

用特定的 SDF 值填充此缓冲区中一个通道的某个区域。

### [void](#)<span id="i_fill_f"></span> **fill_f**( [float](https://docs.godotengine.org/en/stable/classes/class_float.html) value, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) channel=0 ) 

用特定的 SDF 值填充此缓冲区的一个通道。

### [void](#)<span id="i_for_each_voxel_metadata"></span> **for_each_voxel_metadata**( [Callable](https://docs.godotengine.org/en/stable/classes/class_callable.html) callback ) 

对此缓冲区中每个有关联元数据的体素执行一个函数。

函数的参数必须是 (position: Vector3i, metadata: Variant)。

重要提示：不允许在此函数内部插入新的元数据或移除元数据。

### [void](#)<span id="i_for_each_voxel_metadata_in_area"></span> **for_each_voxel_metadata_in_area**( [Callable](https://docs.godotengine.org/en/stable/classes/class_callable.html) callback, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) min_pos, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) max_pos ) 

在指定区域内，对此缓冲区中每个有关联元数据的体素执行一个函数。

重要提示：不允许在此函数内部插入新的元数据或移除元数据。

### [Allocator](VoxelBuffer.md#enumerations)<span id="i_get_allocator"></span> **get_allocator**( ) 

获取此缓冲区使用的内存分配器。

### [Variant](https://docs.godotengine.org/en/stable/classes/class_variant.html)<span id="i_get_block_metadata"></span> **get_block_metadata**( ) 

获取与此 [VoxelBuffer](VoxelBuffer.md) 关联的元数据。

### [PackedByteArray](https://docs.godotengine.org/en/stable/classes/class_packedbytearray.html)<span id="i_get_channel_as_byte_array"></span> **get_channel_as_byte_array**( [ChannelId](VoxelBuffer.md#enumerations) channel_index ) 

以未压缩的原始字节形式获取通道中的体素数据。有关数据格式的信息，请参见 [Depth](VoxelBuffer.md#enumerations)。

注意：如果通道已压缩，它会在返回的数组中即时解压缩。如果你希望在这种情况下有不同的行为，请在调用此方法之前检查 [get_channel_compression](VoxelBuffer.md#i_get_channel_compression)。

### [Compression](VoxelBuffer.md#enumerations)<span id="i_get_channel_compression"></span> **get_channel_compression**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) channel ) 

获取指定通道使用的压缩模式。

### [Depth](VoxelBuffer.md#enumerations)<span id="i_get_channel_depth"></span> **get_channel_depth**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) channel ) 

获取指定通道使用的位深。

### [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html)<span id="i_get_size"></span> **get_size**( ) 

以体素为单位获取缓冲区的 3D 尺寸。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_get_voxel"></span> **get_voxel**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) x, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) y, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) z, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) channel=0 ) 

获取此缓冲区中一个体素的原始值。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_get_voxel_f"></span> **get_voxel_f**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) x, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) y, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) z, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) channel=0 ) 

获取此缓冲区中一个体素的浮点值。如果你处理 SDF 体积（平滑体素），可以使用此函数。

### [Variant](https://docs.godotengine.org/en/stable/classes/class_variant.html)<span id="i_get_voxel_metadata"></span> **get_voxel_metadata**( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) pos ) 

获取附加到此缓冲区中特定体素上的元数据。

### [VoxelTool](VoxelTool.md)<span id="i_get_voxel_tool"></span> **get_voxel_tool**( ) 

构造一个绑定到此缓冲区的 [VoxelTool](VoxelTool.md) 实例。这可让你访问一些额外的常用函数。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_get_voxel_v"></span> **get_voxel_v**( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) pos, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) channel=0 ) 

获取此缓冲区中一个体素的原始值。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_is_uniform"></span> **is_uniform**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) channel ) 

检查通道中的每个体素是否都具有相同的值。

### [void](#)<span id="i_mirror"></span> **mirror**( [Axis](https://docs.godotengine.org/en/stable/classes/class_vector3i.html#enum-vector3i-axis) axis ) 

沿指定轴镜像体素值。

### [void](#)<span id="i_op_add_buffer_f"></span> **op_add_buffer_f**( [VoxelBuffer](VoxelBuffer.md) other, [ChannelId](VoxelBuffer.md#enumerations) channel ) 

计算当前缓冲区与另一个缓冲区中对应体素的和，并将结果存储到当前缓冲区中。体素被解释为有符号距离（通常是 [CHANNEL_SDF](VoxelBuffer.md#i_CHANNEL_SDF) 通道）。

### [void](#)<span id="i_op_max_buffer_f"></span> **op_max_buffer_f**( [VoxelBuffer](VoxelBuffer.md) other, [ChannelId](VoxelBuffer.md#enumerations) channel ) 

计算当前缓冲区与另一个缓冲区中对应体素的最大值，并将结果存储到当前缓冲区中。体素被解释为有符号距离（通常是 [CHANNEL_SDF](VoxelBuffer.md#i_CHANNEL_SDF) 通道）。

### [void](#)<span id="i_op_min_buffer_f"></span> **op_min_buffer_f**( [VoxelBuffer](VoxelBuffer.md) other, [ChannelId](VoxelBuffer.md#enumerations) channel ) 

计算当前缓冲区与另一个缓冲区中对应体素的最小值，并将结果存储到当前缓冲区中。体素被解释为有符号距离（通常是 [CHANNEL_SDF](VoxelBuffer.md#i_CHANNEL_SDF) 通道）。

### [void](#)<span id="i_op_mul_buffer_f"></span> **op_mul_buffer_f**( [VoxelBuffer](VoxelBuffer.md) other, [ChannelId](VoxelBuffer.md#enumerations) channel ) 

计算当前缓冲区与另一个缓冲区中对应体素的乘积，并将结果存储到当前缓冲区中。体素被解释为有符号距离（通常是 [CHANNEL_SDF](VoxelBuffer.md#i_CHANNEL_SDF) 通道）。

### [void](#)<span id="i_op_mul_value_f"></span> **op_mul_value_f**( [float](https://docs.godotengine.org/en/stable/classes/class_float.html) other, [ChannelId](VoxelBuffer.md#enumerations) channel ) 

将缓冲区中的每个体素乘以给定值。体素被解释为有符号距离（通常是 [CHANNEL_SDF](VoxelBuffer.md#i_CHANNEL_SDF) 通道）。

### [void](#)<span id="i_op_select_less_src_f_dst_i_values"></span> **op_select_less_src_f_dst_i_values**( [VoxelBuffer](VoxelBuffer.md) src, [ChannelId](VoxelBuffer.md#enumerations) src_channel, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) threshold, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) value_if_less, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) value_if_more, [ChannelId](VoxelBuffer.md#enumerations) dst_channel ) 

对于源缓冲区中的每个体素，如果其值低于阈值，则将当前缓冲区中对应的体素设置为特定的整数值，否则设置为另一个值。源缓冲区中的体素被解释为有符号距离（通常是 [CHANNEL_SDF](VoxelBuffer.md#i_CHANNEL_SDF) 通道）。

### [void](#)<span id="i_op_sub_buffer_f"></span> **op_sub_buffer_f**( [VoxelBuffer](VoxelBuffer.md) other, [ChannelId](VoxelBuffer.md#enumerations) channel ) 

计算当前缓冲区与另一个缓冲区中对应体素的差（`current - other`），并将结果存储到当前缓冲区中。体素被解释为有符号距离（通常是 [CHANNEL_SDF](VoxelBuffer.md#i_CHANNEL_SDF) 通道）。

### [void](#)<span id="i_remap_values"></span> **remap_values**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) channel, [PackedInt32Array](https://docs.godotengine.org/en/stable/classes/class_packedint32array.html) map ) 

使用传入的 `map` 查找表重映射通道中的整数值。`map` 中的每个索引对应一个原始值，并将被替换为 `map[original]`。

### [void](#)<span id="i_rotate_90"></span> **rotate_90**( [Axis](https://docs.godotengine.org/en/stable/classes/class_vector3i.html#enum-vector3i-axis) axis, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) turns ) 

沿指定轴将体素值旋转 90 度。旋转 1 次为 90 度，2 次为 180 度，3 次为 270 度（或 -90 度），4 次无效果。负数旋转沿另一个方向进行。如果缓冲区不是立方体，这也会旋转缓冲区的尺寸。

### [void](#)<span id="i_set_block_metadata"></span> **set_block_metadata**( [Variant](https://docs.godotengine.org/en/stable/classes/class_variant.html) meta ) 

在此缓冲区上设置任意数据。旧数据会被替换。注意，这与按体素存储的元数据是分开的存储。

如果保存此 [VoxelBuffer](VoxelBuffer.md)，这些元数据也会随体素一起保存，因此请确保数据支持序列化（即你不能在其中放入节点或任意对象）。

### [void](#)<span id="i_set_channel_depth"></span> **set_channel_depth**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) channel, [Depth](VoxelBuffer.md#enumerations) depth ) 

更改给定通道的位深。这控制通道可以保存的值的范围。更多信息请参见 [Depth](VoxelBuffer.md#enumerations)。

### [void](#)<span id="i_set_channel_from_byte_array"></span> **set_channel_from_byte_array**( [ChannelId](VoxelBuffer.md#enumerations) channel_index, [PackedByteArray](https://docs.godotengine.org/en/stable/classes/class_packedbytearray.html) data ) 

用原始体素数据覆盖通道的内容。有关预期数据格式的信息，请参见 [Depth](VoxelBuffer.md#enumerations)。

### [void](#)<span id="i_set_voxel"></span> **set_voxel**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) value, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) x, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) y, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) z, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) channel=0 ) 

设置体素的原始值。如果你使用平滑体素，可能更倾向于使用 [set_voxel_f](VoxelBuffer.md#i_set_voxel_f)。

### [void](#)<span id="i_set_voxel_f"></span> **set_voxel_f**( [float](https://docs.godotengine.org/en/stable/classes/class_float.html) value, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) x, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) y, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) z, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) channel=0 ) 

设置体素的浮点值。如果你处理 SDF 数据（平滑体素），应使用此方法。

### [void](#)<span id="i_set_voxel_metadata"></span> **set_voxel_metadata**( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) pos, [Variant](https://docs.godotengine.org/en/stable/classes/class_variant.html) value ) 

在特定体素上附加任意数据。旧数据会被替换。传入 `null` 将清除元数据。

如果保存此 [VoxelBuffer](VoxelBuffer.md)，这些元数据也会随体素一起保存，因此请确保数据支持序列化（即你不能在其中放入节点或任意对象）。

### [void](#)<span id="i_set_voxel_v"></span> **set_voxel_v**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) value, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) pos, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) channel=0 ) 

*(此方法暂无文档)*

### [void](#)<span id="i_update_3d_texture_from_sdf_zxy"></span> **update_3d_texture_from_sdf_zxy**( [ImageTexture3D](https://docs.godotengine.org/en/stable/classes/class_imagetexture3d.html) existing_texture ) 

从 SDF 通道更新现有的 3D 纹理。更多信息请参见 [create_3d_texture_from_sdf_zxy](VoxelBuffer.md#i_create_3d_texture_from_sdf_zxy)。

_生成于 2026-08-28_
