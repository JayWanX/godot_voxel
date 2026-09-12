# VoxelInstanceGenerator

继承自：[Resource](https://docs.godotengine.org/en/stable/classes/class_resource.html)

决定在体素表面何处生成实例。

## 描述：

生成在体素表面生成实例所需的必要信息。这可被 [VoxelInstancer](VoxelInstancer.md) 使用。

注意：若要生成体素，请参见 [VoxelGenerator](VoxelGenerator.md)。

## 属性：


类型                                                                                              | 名称                                                                                 | 默认值                        
----------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------- | ---------------------------
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)                        | [density](#i_density)                                                              | 0.1                        
[EmitMode](VoxelInstanceGenerator.md#enumerations)                                              | [emit_mode](#i_emit_mode)                                                          | EMIT_FROM_VERTICES (0)     
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)                        | [jitter](#i_jitter)                                                                | 1.0                        
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)                        | [max_height](#i_max_height)                                                        | 3.4028235e+38              
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)                        | [max_height_falloff](#i_max_height_falloff)                                        | 0.0                        
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)                        | [max_scale](#i_max_scale)                                                          | 1.0                        
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)                        | [max_slope_degrees](#i_max_slope_degrees)                                          | 180.0                      
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)                        | [max_slope_falloff_degrees](#i_max_slope_falloff_degrees)                          | 0.0                        
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)                        | [min_height](#i_min_height)                                                        | 1.1754944e-38              
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)                        | [min_height_falloff](#i_min_height_falloff)                                        | 0.0                        
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)                        | [min_scale](#i_min_scale)                                                          | 1.0                        
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)                        | [min_slope_degrees](#i_min_slope_degrees)                                          | 0.0                        
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)                        | [min_slope_falloff_degrees](#i_min_slope_falloff_degrees)                          | 0.0                        
[Noise](https://docs.godotengine.org/en/stable/classes/class_noise.html)                        | [noise](#i_noise)                                                                  |                            
[Dimension](VoxelInstanceGenerator.md#enumerations)                                             | [noise_dimension](#i_noise_dimension)                                              | DIMENSION_3D (1)           
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)                        | [noise_falloff](#i_noise_falloff)                                                  | 0.0                        
[VoxelGraphFunction](VoxelGraphFunction.md)                                                     | [noise_graph](#i_noise_graph)                                                      |                            
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)                        | [noise_on_scale](#i_noise_on_scale)                                                | 0.0                        
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)                        | [noise_threshold](#i_noise_threshold)                                              | 0.0                        
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)                        | [offset_along_normal](#i_offset_along_normal)                                      | 0.0                        
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)                          | [random_rotation](#i_random_rotation)                                              | true                       
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)                          | [random_vertical_flip](#i_random_vertical_flip)                                    | false                      
[Distribution](VoxelInstanceGenerator.md#enumerations)                                          | [scale_distribution](#i_scale_distribution)                                        | DISTRIBUTION_QUADRATIC (1) 
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)                          | [snap_to_generator_sdf_enabled](#i_snap_to_generator_sdf_enabled)                  | false                      
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)                            | [snap_to_generator_sdf_sample_count](#i_snap_to_generator_sdf_sample_count)        | 2                          
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)                        | [snap_to_generator_sdf_search_distance](#i_snap_to_generator_sdf_search_distance)  | 1.0                        
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)                        | [triangle_area_threshold](#i_triangle_area_threshold)                              | 0.0                        
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)                        | [vertical_alignment](#i_vertical_alignment)                                        | 1.0                        
[PackedInt32Array](https://docs.godotengine.org/en/stable/classes/class_packedint32array.html)  | [voxel_texture_filter_array](#i_voxel_texture_filter_array)                        | PackedInt32Array(0)        
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)                          | [voxel_texture_filter_enabled](#i_voxel_texture_filter_enabled)                    | false                      
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)                        | [voxel_texture_filter_threshold](#i_voxel_texture_filter_threshold)                | 0.5                        
<p></p>

## 方法：


返回值                                                                   | 函数签名                                                                                                                                             
--------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)  | [get_voxel_texture_filter_mask](#i_get_voxel_texture_filter_mask) ( ) const                                                                      
[void](#)                                                             | [set_voxel_texture_filter_mask](#i_set_voxel_texture_filter_mask) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) mask )  
<p></p>

## 枚举：<span id="enumerations"></span>

枚举 **EmitMode**：

- <span id="i_EMIT_FROM_VERTICES"></span>**EMIT_FROM_VERTICES** = **0** --- 使用网格的顶点来生成实例。这是最快的选项，但可能会产生明显的图案。
- <span id="i_EMIT_FROM_FACES_FAST"></span>**EMIT_FROM_FACES_FAST** = **1** --- 使用网格的面来生成实例。这是一个较为均衡的选项，采取了一些快捷方式，且不会产生太明显的图案。
- <span id="i_EMIT_FROM_FACES"></span>**EMIT_FROM_FACES** = **2** --- 使用网格的面来生成实例。这是最慢的选项，但应不会产生明显的图案。
- <span id="i_EMIT_ONE_PER_TRIANGLE"></span>**EMIT_ONE_PER_TRIANGLE** = **3** --- 使用网格的面（即三角形）来生成实例。每个三角形只生成一个实例。默认情况下，实例会生成在三角形的中心。可通过 [jitter](VoxelInstanceGenerator.md#i_jitter) 为该位置增加随机性。
- <span id="i_EMIT_MODE_COUNT"></span>**EMIT_MODE_COUNT** = **4**

枚举 **Distribution**：

- <span id="i_DISTRIBUTION_LINEAR"></span>**DISTRIBUTION_LINEAR** = **0** --- 均匀分布。
- <span id="i_DISTRIBUTION_QUADRATIC"></span>**DISTRIBUTION_QUADRATIC** = **1** --- 小条目更多、大条目更少的分布。
- <span id="i_DISTRIBUTION_CUBIC"></span>**DISTRIBUTION_CUBIC** = **2** --- 小条目更多、大条目更少的分布。
- <span id="i_DISTRIBUTION_QUINTIC"></span>**DISTRIBUTION_QUINTIC** = **3**
- <span id="i_DISTRIBUTION_COUNT"></span>**DISTRIBUTION_COUNT** = **4**

枚举 **Dimension**：

- <span id="i_DIMENSION_2D"></span>**DIMENSION_2D** = **0**
- <span id="i_DIMENSION_3D"></span>**DIMENSION_3D** = **1**
- <span id="i_DIMENSION_COUNT"></span>**DIMENSION_COUNT** = **2**


## 属性描述

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_density"></span> **density** = 0.1

控制生成的实例数量。根据所选发射类型的不同，结果可能会有所差异。

### [EmitMode](VoxelInstanceGenerator.md#enumerations)<span id="i_emit_mode"></span> **emit_mode** = EMIT_FROM_VERTICES (0)

实例主要按哪种方式发射。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_jitter"></span> **jitter** = 1.0

当 [emit_mode](VoxelInstanceGenerator.md#i_emit_mode) 设置为 [EMIT_ONE_PER_TRIANGLE](VoxelInstanceGenerator.md#i_EMIT_ONE_PER_TRIANGLE) 时，控制生成位置的随机程度。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_max_height"></span> **max_height** = 3.4028235e+38

实例不会在此高度以上创建。

这也取决于所选的 [VoxelInstancer.up_mode](VoxelInstancer.md#i_up_mode)。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_max_height_falloff"></span> **max_height_falloff** = 0.0

当低于 [max_height](VoxelInstanceGenerator.md#i_max_height) 时，密度逐渐衰减的距离范围。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_max_scale"></span> **max_scale** = 1.0

实例随机化时所使用的最小缩放。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_max_slope_degrees"></span> **max_slope_degrees** = 180.0

如果地面的坡度超过此角度，则不会生成实例。

这也取决于所选的 [VoxelInstancer.up_mode](VoxelInstancer.md#i_up_mode)。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_max_slope_falloff_degrees"></span> **max_slope_falloff_degrees** = 0.0

当低于 [max_slope_degrees](VoxelInstanceGenerator.md#i_max_slope_degrees) 时，密度逐渐衰减的角度范围。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_min_height"></span> **min_height** = 1.1754944e-38

实例不会在此高度以下创建。这也取决于所选的 [VoxelInstancer.up_mode](VoxelInstancer.md#i_up_mode)。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_min_height_falloff"></span> **min_height_falloff** = 0.0

当高于 [min_height](VoxelInstanceGenerator.md#i_min_height) 时，密度逐渐衰减的距离范围。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_min_scale"></span> **min_scale** = 1.0

实例随机化时所使用的最大缩放。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_min_slope_degrees"></span> **min_slope_degrees** = 0.0

如果地面的坡度低于此角度，则不会生成实例。

这也取决于所选的 [VoxelInstancer.up_mode](VoxelInstancer.md#i_up_mode)。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_min_slope_falloff_degrees"></span> **min_slope_falloff_degrees** = 0.0

当高于 [min_slope_degrees](VoxelInstanceGenerator.md#i_min_slope_degrees) 时，密度逐渐衰减的角度范围。

### [Noise](https://docs.godotengine.org/en/stable/classes/class_noise.html)<span id="i_noise"></span> **noise**

用于过滤已生成实例的噪声，以便实例可以按噪声所描述的图案生成。

### [Dimension](VoxelInstanceGenerator.md#enumerations)<span id="i_noise_dimension"></span> **noise_dimension** = DIMENSION_3D (1)

在计算 [noise](VoxelInstanceGenerator.md#i_noise) 和 [noise_graph](VoxelInstanceGenerator.md#i_noise_graph) 时应使用的维度。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_noise_falloff"></span> **noise_falloff** = 0.0

密度会逐渐衰减的噪声取值范围。例如，若衰减值为 0.3，则当噪声值在 0 到 0.3 之间时密度会逐渐衰减。

### [VoxelGraphFunction](VoxelGraphFunction.md)<span id="i_noise_graph"></span> **noise_graph**

用于过滤已生成实例的图函数，与 [noise](VoxelInstanceGenerator.md#i_noise) 类似，但允许更自定义的噪声计算。

如果 [noise_dimension](VoxelInstanceGenerator.md#i_noise_dimension) 为 2D，该图必须具有 2 个输入（X 和 Z）；如果为 3D，则必须具有 3 个输入（X、Y 和 Z）。且必须有一个 SDF 输出。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_noise_on_scale"></span> **noise_on_scale** = 0.0

[noise](VoxelInstanceGenerator.md#i_noise) 对实例缩放的额外影响程度。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_noise_threshold"></span> **noise_threshold** = 0.0

扩展或收缩噪声过滤。较高的值会扩大实例生成区域，较低的值会缩小这些区域。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_offset_along_normal"></span> **offset_along_normal** = 0.0

沿地面的法线偏移已生成的实例。

法线取决于 [VoxelInstancer.up_mode](VoxelInstancer.md#i_up_mode)，并且还受 [vertical_alignment](VoxelInstanceGenerator.md#i_vertical_alignment) 影响。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_random_rotation"></span> **random_rotation** = true

启用后，实例将获得随机旋转。若未启用，它们将根据地面坡度使用一致的旋转。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_random_vertical_flip"></span> **random_vertical_flip** = false

启用后，实例将随机上下翻转。这在配合小石头时可营造出更多样化的错觉。

### [Distribution](VoxelInstanceGenerator.md#enumerations)<span id="i_scale_distribution"></span> **scale_distribution** = DISTRIBUTION_QUADRATIC (1)

设置随机缩放值的分布方式。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_snap_to_generator_sdf_enabled"></span> **snap_to_generator_sdf_enabled** = false

启用吸附到生成器 SDF。将查询生成器的 SDF 值，使实例沿其法线移动以更接近地面。有助于减少靠近实例时出现“漂浮”或“埋入”的情况。这要求地形的生成器支持串联生成。副作用：从远处观看时实例可能会漂浮或埋入；生成可能变得不那么均匀。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_snap_to_generator_sdf_sample_count"></span> **snap_to_generator_sdf_sample_count** = 2

每个实例将从生成器采样多少次以逼近吸附位置。采样越多精度越高，但开销也越大。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_snap_to_generator_sdf_search_distance"></span> **snap_to_generator_sdf_search_distance** = 1.0

吸附搜索地面位置的上下距离范围。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_triangle_area_threshold"></span> **triangle_area_threshold** = 0.0

若设置为大于零的值，则地面中面积低于此阈值的三角形将被忽略。

某些网格化算法常常会生成细长或细小的三角形，从而影响生成实例的分布质量。如果出现这种情况，可使用此属性将其过滤掉。

注意：此属性相对于 LOD0。在不同 LOD 索引的网格上生成实例时，生成器会对其缩放。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_vertical_alignment"></span> **vertical_alignment** = 1.0

设置实例与地面的对齐程度。

若为 0，它们将完全与地面平齐。

若为 1，它们将完全与被视为“上方”的方向对齐。

这取决于 [VoxelInstancer.up_mode](VoxelInstancer.md#i_up_mode)。

### [PackedInt32Array](https://docs.godotengine.org/en/stable/classes/class_packedint32array.html)<span id="i_voxel_texture_filter_array"></span> **voxel_texture_filter_array** = PackedInt32Array(0)

指定允许实例在其上生成的体素纹理索引。

仅当启用了 [voxel_texture_filter_enabled](VoxelInstanceGenerator.md#i_voxel_texture_filter_enabled)，且 [VoxelMesherTransvoxel](VoxelMesherTransvoxel.md) 的 [VoxelMesherTransvoxel.texturing_mode](VoxelMesherTransvoxel.md#i_texturing_mode) 设置为 [VoxelMesherTransvoxel.TEXTURES_MIXEL4_S4](VoxelMesherTransvoxel.md#i_TEXTURES_MIXEL4_S4) 或 [VoxelMesherTransvoxel.TEXTURES_SINGLE_S4](VoxelMesherTransvoxel.md#i_TEXTURES_SINGLE_S4) 时才有效。索引目前限制在 0 到 31（含）之间。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_voxel_texture_filter_enabled"></span> **voxel_texture_filter_enabled** = false

为 true 时，启用基于体素纹理索引的实例过滤。参见 [voxel_texture_filter_array](VoxelInstanceGenerator.md#i_voxel_texture_filter_array)。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_voxel_texture_filter_threshold"></span> **voxel_texture_filter_threshold** = 0.5

当 [voxel_texture_filter_enabled](VoxelInstanceGenerator.md#i_voxel_texture_filter_enabled) 生效时，控制过滤后的纹理需存在多少比例实例才会生成。该值必须在 0 到 1 之间。

## 方法描述

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_get_voxel_texture_filter_mask"></span> **get_voxel_texture_filter_mask**( ) 

获取体素材质过滤掩码。

### [void](#)<span id="i_set_voxel_texture_filter_mask"></span> **set_voxel_texture_filter_mask**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) mask ) 

设置体素材质过滤掩码。

_生成于 2026-09-12_
