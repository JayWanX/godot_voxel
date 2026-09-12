# VoxelGeneratorGraph

继承自：[VoxelGenerator](VoxelGenerator.md)

基于图形的体素生成器。

## 描述：

通过逐体素运算的图形生成体素数据。

必须先创建并编译图形，之后才能生成数据块。

它可用于基于 SDF 的平滑地形，也可用于方块风地形。

警告：修改图形的方法只能在主线程中调用。

## 属性：


类型                                                                        | 名称                                                             | 默认值                     
------------------------------------------------------------------------- | -------------------------------------------------------------- | ------------------------
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)    | [debug_block_clipping](#i_debug_block_clipping)                | false                   
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)  | [sdf_clip_threshold](#i_sdf_clip_threshold)                    | 1.5                     
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)      | [subdivision_size](#i_subdivision_size)                        | 16                      
[TextureMode](VoxelGeneratorGraph.md#enumerations)                        | [texture_mode](#i_texture_mode)                                | TEXTURE_MODE_MIXEL4 (0) 
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)    | [use_optimized_execution_map](#i_use_optimized_execution_map)  | true                    
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)    | [use_subdivision](#i_use_subdivision)                          | true                    
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)    | [use_xz_caching](#i_use_xz_caching)                            | true                    
<p></p>

## 方法：


返回值                                                                                 | 函数签名                                                                                                                                                                                                                                                                                                                                                                                    
----------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
[void](#)                                                                           | [bake_sphere_bumpmap](#i_bake_sphere_bumpmap) ( [Image](https://docs.godotengine.org/en/stable/classes/class_image.html) im, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) ref_radius, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) sdf_min, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) sdf_max )  
[void](#)                                                                           | [bake_sphere_normalmap](#i_bake_sphere_normalmap) ( [Image](https://docs.godotengine.org/en/stable/classes/class_image.html) im, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) ref_radius, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) strength )                                                                               
[void](#)                                                                           | [clear](#i_clear) ( )                                                                                                                                                                                                                                                                                                                                                                   
[Dictionary](https://docs.godotengine.org/en/stable/classes/class_dictionary.html)  | [compile](#i_compile) ( )                                                                                                                                                                                                                                                                                                                                                               
[Vector2](https://docs.godotengine.org/en/stable/classes/class_vector2.html)        | [debug_analyze_range](#i_debug_analyze_range) ( [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) min_pos, [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) max_pos ) const                                                                                                                                                      
[void](#)                                                                           | [debug_load_waves_preset](#i_debug_load_waves_preset) ( )                                                                                                                                                                                                                                                                                                                               
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)            | [debug_measure_microseconds_per_voxel](#i_debug_measure_microseconds_per_voxel) ( [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html) use_singular_queries )                                                                                                                                                                                                         
[void](#)                                                                           | [generate_image_from_sdf](#i_generate_image_from_sdf) ( [Image](https://docs.godotengine.org/en/stable/classes/class_image.html) im, [Transform3D](https://docs.godotengine.org/en/stable/classes/class_transform3d.html) transform, [Vector2](https://docs.godotengine.org/en/stable/classes/class_vector2.html) size )                                                                
[VoxelGraphFunction](VoxelGraphFunction.md)                                         | [get_main_function](#i_get_main_function) ( ) const                                                                                                                                                                                                                                                                                                                                     
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)            | [raycast_sdf_approx](#i_raycast_sdf_approx) ( [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) ray_origin, [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) ray_end, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) stride ) const                                                                    
<p></p>

## 信号：<span id="signals"></span>

### node_name_changed( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) node_id ) 

*(此信号暂无文档)*

## 枚举：<span id="enumerations"></span>

枚举 **TextureMode**：

- <span id="i_TEXTURE_MODE_MIXEL4"></span>**TEXTURE_MODE_MIXEL4** = **0** --- 将纹理数据写入 [VoxelBuffer.CHANNEL_INDICES](VoxelBuffer.md#i_CHANNEL_INDICES)，作为 4 个打包的 4 位索引；写入 [VoxelBuffer.CHANNEL_WEIGHTS](VoxelBuffer.md#i_CHANNEL_WEIGHTS)，作为 4 个打包的 4 位权重。有关此数据的用法，请参阅 [VoxelMesherTransvoxel](VoxelMesherTransvoxel.md) 以及关于平滑体素的文档。
- <span id="i_TEXTURE_MODE_SINGLE"></span>**TEXTURE_MODE_SINGLE** = **1** --- 将纹理数据写入 [VoxelBuffer.CHANNEL_INDICES](VoxelBuffer.md#i_CHANNEL_INDICES)，每个体素一个 8 位值。如果你想使用此模式，请确保你的体素具有合适的格式（参见 [VoxelFormat](VoxelFormat.md)）。


## 属性描述

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_debug_block_clipping"></span> **debug_block_clipping** = false

启用后，如果图形输出 SDF 数据，那些本会被裁剪的生成数据块将被反转。这会使它们显示为"墙体伪影"，有助于可视化优化发生的位置。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_sdf_clip_threshold"></span> **sdf_clip_threshold** = 1.5

为地形生成 SDF 数据块时，如果某个数据块的范围分析超过此阈值，其 SDF 数据将被视为完全为空气或完全为实体（使用高常量，正或负）。这会优化内存，因为完全位于地下或空中的区块在每个体素上都具有相同的值。同时也会节省处理时间，因为会跳过 SDF 计算（噪声等）。要关闭此优化，请将其设置为一个较高的值。

缺点：如果你在假设 SDF 连续的情况下使用操作来编辑地形，那么在开始发生裁剪的边界处，这些操作可能会表现异常。典型的例子是 [VoxelTool.grow_sphere](VoxelTool.md#i_grow_sphere)。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_subdivision_size"></span> **subdivision_size** = 16

为地形生成 SDF 数据块时，如果区块大小可被此值整除，范围分析将基于该细分进行。这样可以优化掉更精确的区域。但是，该值不能设置得太小，否则开销将超过收益。

### [TextureMode](VoxelGeneratorGraph.md#enumerations)<span id="i_texture_mode"></span> **texture_mode** = TEXTURE_MODE_MIXEL4 (0)

设置纹理输出（如果存在）将产生的体素格式。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_use_optimized_execution_map"></span> **use_optimized_execution_map** = true

启用后，在为地形生成数据块时，如果发现某些节点在特定区域中不重要，生成器将尝试跳过它们。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_use_subdivision"></span> **use_subdivision** = true

启用后，将使用 [subdivision_size](VoxelGeneratorGraph.md#i_subdivision_size)。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_use_xz_caching"></span> **use_xz_caching** = true

启用后，生成器将只运行图形中仅依赖 X 和 Z 的分支一次。当图形的一部分生成高度图时，这会很有效，因为该部分不是体积化的。

## 方法描述

### [void](#)<span id="i_bake_sphere_bumpmap"></span> **bake_sphere_bumpmap**( [Image](https://docs.godotengine.org/en/stable/classes/class_image.html) im, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) ref_radius, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) sdf_min, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) sdf_max ) 

使用生成器产生的 SDF 输出（如果有）烘焙球形凹凸贴图（或高度图）。凹凸贴图使用全景投影。

`ref_radius`：采样高度的球体半径。

`strength`：生成法线的强度，默认为 1.0。

### [void](#)<span id="i_bake_sphere_normalmap"></span> **bake_sphere_normalmap**( [Image](https://docs.godotengine.org/en/stable/classes/class_image.html) im, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) ref_radius, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) strength ) 

使用生成器产生的 SDF 输出（如果有）烘焙球形法线贴图。法线贴图使用全景投影。假定生成器产生球形形状（如行星）。此类法线贴图可用于为使用此生成器的地形远距离视图添加更多细节。

`ref_radius`：采样法线的球体半径。

`strength`：生成法线的强度，默认为 1.0。

注意：另一种替代方案是与 [VoxelLodTerrain](VoxelLodTerrain.md) 一起使用距离法线功能。

### [void](#)<span id="i_clear"></span> **clear**( ) 

从图形中删除所有节点和连接。

### [Dictionary](https://docs.godotengine.org/en/stable/classes/class_dictionary.html)<span id="i_compile"></span> **compile**( ) 

编译图形以便用于生成数据块。

如果编译成功，返回结果为具有以下布局的字典：

```
{
"success": true
}
```
如果编译失败，返回结果可能包含消息和可能导致问题的图形节点 ID：

```
{
"success": false,
"node_id": int,
"message": String
}
```
如果错误与特定节点无关，节点 ID 将为 -1。

### [Vector2](https://docs.godotengine.org/en/stable/classes/class_vector2.html)<span id="i_debug_analyze_range"></span> **debug_analyze_range**( [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) min_pos, [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) max_pos ) 

分析指定范围内的输出值范围区间。

### [void](#)<span id="i_debug_load_waves_preset"></span> **debug_load_waves_preset**( ) 

加载用于调试的波浪演示预置图。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_debug_measure_microseconds_per_voxel"></span> **debug_measure_microseconds_per_voxel**( [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html) use_singular_queries ) 

测量每个体素生成所需的微秒数。

### [void](#)<span id="i_generate_image_from_sdf"></span> **generate_image_from_sdf**( [Image](https://docs.godotengine.org/en/stable/classes/class_image.html) im, [Transform3D](https://docs.godotengine.org/en/stable/classes/class_transform3d.html) transform, [Vector2](https://docs.godotengine.org/en/stable/classes/class_vector2.html) size ) 

在以给定变换为中心、跨越其 X 和 Y 轴的平面内生成 SDF 数据，并将结果存储到图像的像素中。采样点以像素为中心。

图像最好采用 32 位浮点格式，并且不能压缩。

### [VoxelGraphFunction](VoxelGraphFunction.md)<span id="i_get_main_function"></span> **get_main_function**( ) 

获取用于生成的图形。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_raycast_sdf_approx"></span> **raycast_sdf_approx**( [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) ray_origin, [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) ray_end, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) stride ) 

沿射线生成值，以找到第一个 SDF &lt 0.0 的位置，并返回沿射线的距离。如果未命中，则返回 -1.0。

这是近似值：返回值的误差范围最多为给定的步长。

距离越长，代价越高。

步长越小，代价越高但越精确。

_生成于 2026-09-12_
