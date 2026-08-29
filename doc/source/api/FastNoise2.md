# FastNoise2

继承自：[Resource](https://docs.godotengine.org/en/stable/classes/class_resource.html)

[FastNoise2](https://github.com/Auburn/FastNoise2) 库的封装。

## 描述：

生成相干噪声，支持 SIMD 处理，与 [FastNoiseLite](https://docs.godotengine.org/en/stable/classes/class_fastnoiselite.html) 等标量实现相比可显著提升生成速度。

内部使用基于节点的结构进行配置，但目前无法在 Godot 内编辑。取而代之的是暴露简化的属性。可以使用外部 Noise Tool 编辑噪声图形，该工具可以将编码后的节点树导出为 base64 字符串。

当前集成使用 FastNoise2 0.10.0-alpha 版本。如果你使用 Noise Tool 制作的编码节点树，则必须与该版本匹配。

修改属性后，必须调用 [update_generator](FastNoise2.md#i_update_generator) 以重建内部图形并使更改生效。

## 属性：


类型                                                                          | 名称                                                           | 默认值                             
--------------------------------------------------------------------------- | ------------------------------------------------------------ | --------------------------------
[CellularDistanceFunction](FastNoise2.md#enumerations)                      | [cellular_distance_function](#i_cellular_distance_function)  | CELLULAR_DISTANCE_EUCLIDEAN (0) 
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)        | [cellular_index0](#i_cellular_index0)                        | 0                               
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)        | [cellular_index1](#i_cellular_index1)                        | 1                               
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)    | [cellular_jitter](#i_cellular_jitter)                        | 1.0                             
[CellularReturnType](FastNoise2.md#enumerations)                            | [cellular_return_type](#i_cellular_return_type)              | CELLULAR_RETURN_INDEX_0 (0)     
[String](https://docs.godotengine.org/en/stable/classes/class_string.html)  | [encoded_node_tree](#i_encoded_node_tree)                    | ""                              
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)    | [fractal_gain](#i_fractal_gain)                              | 0.5                             
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)    | [fractal_lacunarity](#i_fractal_lacunarity)                  | 2.0                             
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)        | [fractal_octaves](#i_fractal_octaves)                        | 3                               
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)    | [fractal_ping_pong_strength](#i_fractal_ping_pong_strength)  | 2.0                             
[FractalType](FastNoise2.md#enumerations)                                   | [fractal_type](#i_fractal_type)                              | FRACTAL_NONE (0)                
[NoiseType](FastNoise2.md#enumerations)                                     | [noise_type](#i_noise_type)                                  | TYPE_OPEN_SIMPLEX_2 (0)         
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)    | [period](#i_period)                                          | 64.0                            
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)      | [remap_enabled](#i_remap_enabled)                            | false                           
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)    | [remap_input_max](#i_remap_input_max)                        | 1.0                             
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)    | [remap_input_min](#i_remap_input_min)                        | -1.0                            
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)    | [remap_output_max](#i_remap_output_max)                      | 1.0                             
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)    | [remap_output_min](#i_remap_output_min)                      | -1.0                            
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)        | [seed](#i_seed)                                              | 1337                            
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)      | [terrace_enabled](#i_terrace_enabled)                        | false                           
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)    | [terrace_multiplier](#i_terrace_multiplier)                  | 1.0                             
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)    | [terrace_smoothness](#i_terrace_smoothness)                  | 0.0                             
<p></p>

## 方法：


返回值                                                                         | 函数签名                                                                                                                                                                                                          
--------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
[void](#)                                                                   | [generate_image](#i_generate_image) ( [Image](https://docs.godotengine.org/en/stable/classes/class_image.html) image, [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html) tileable ) const 
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)    | [get_noise_2d_single](#i_get_noise_2d_single) ( [Vector2](https://docs.godotengine.org/en/stable/classes/class_vector2.html) pos ) const                                                                      
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)    | [get_noise_3d_single](#i_get_noise_3d_single) ( [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) pos ) const                                                                      
[SIMDLevel](FastNoise2.md#enumerations)                                     | [get_simd_level](#i_get_simd_level) ( ) const                                                                                                                                                                 
[String](https://docs.godotengine.org/en/stable/classes/class_string.html)  | [get_simd_level_name](#i_get_simd_level_name) ( [SIMDLevel](FastNoise2.md#enumerations) level ) static                                                                                                        
[void](#)                                                                   | [update_generator](#i_update_generator) ( )                                                                                                                                                                   
<p></p>

## 枚举：<span id="enumerations"></span>

枚举 **NoiseType**：

- <span id="i_TYPE_OPEN_SIMPLEX_2"></span>**TYPE_OPEN_SIMPLEX_2** = **0** --- OpenSimplex2 噪声。
- <span id="i_TYPE_SIMPLEX"></span>**TYPE_SIMPLEX** = **1** --- Simplex 噪声。
- <span id="i_TYPE_PERLIN"></span>**TYPE_PERLIN** = **2** --- Perlin 噪声。
- <span id="i_TYPE_VALUE"></span>**TYPE_VALUE** = **3** --- 值噪声。
- <span id="i_TYPE_CELLULAR"></span>**TYPE_CELLULAR** = **4** --- 细胞（蜂窝）噪声。
- <span id="i_TYPE_ENCODED_NODE_TREE"></span>**TYPE_ENCODED_NODE_TREE** = **5** --- 使用编码的节点树。
- <span id="i_TYPE_CELLULAR_VALUE"></span>**TYPE_CELLULAR_VALUE** = **6** --- 细胞值噪声。

枚举 **FractalType**：

- <span id="i_FRACTAL_NONE"></span>**FRACTAL_NONE** = **0** --- 无分形。
- <span id="i_FRACTAL_FBM"></span>**FRACTAL_FBM** = **1** --- FBM（分形布朗运动）分形。
- <span id="i_FRACTAL_RIDGED"></span>**FRACTAL_RIDGED** = **2** --- 脊状（Ridged）分形。
- <span id="i_FRACTAL_PING_PONG"></span>**FRACTAL_PING_PONG** = **3** --- 乒乓（Ping-pong）分形。

枚举 **CellularDistanceFunction**：

- <span id="i_CELLULAR_DISTANCE_EUCLIDEAN"></span>**CELLULAR_DISTANCE_EUCLIDEAN** = **0** --- 欧几里得距离。
- <span id="i_CELLULAR_DISTANCE_EUCLIDEAN_SQ"></span>**CELLULAR_DISTANCE_EUCLIDEAN_SQ** = **1** --- 欧几里得距离的平方。
- <span id="i_CELLULAR_DISTANCE_MANHATTAN"></span>**CELLULAR_DISTANCE_MANHATTAN** = **2** --- 曼哈顿距离。
- <span id="i_CELLULAR_DISTANCE_HYBRID"></span>**CELLULAR_DISTANCE_HYBRID** = **3** --- 混合距离。
- <span id="i_CELLULAR_DISTANCE_MAX_AXIS"></span>**CELLULAR_DISTANCE_MAX_AXIS** = **4** --- 最大轴距离。

枚举 **CellularReturnType**：

- <span id="i_CELLULAR_RETURN_INDEX_0"></span>**CELLULAR_RETURN_INDEX_0** = **0** --- 返回最近单元格的距离。
- <span id="i_CELLULAR_RETURN_INDEX_0_ADD_1"></span>**CELLULAR_RETURN_INDEX_0_ADD_1** = **1** --- 返回最近与次近距离之和。
- <span id="i_CELLULAR_RETURN_INDEX_0_SUB_1"></span>**CELLULAR_RETURN_INDEX_0_SUB_1** = **2** --- 返回最近与次近距离之差。
- <span id="i_CELLULAR_RETURN_INDEX_0_MUL_1"></span>**CELLULAR_RETURN_INDEX_0_MUL_1** = **3** --- 返回最近与次近距离之积。
- <span id="i_CELLULAR_RETURN_INDEX_0_DIV_1"></span>**CELLULAR_RETURN_INDEX_0_DIV_1** = **4** --- 返回最近与次近距离之商。

枚举 **SIMDLevel**：

- <span id="i_SIMD_NULL"></span>**SIMD_NULL** = **0** --- 未定义的级别。
- <span id="i_SIMD_SCALAR"></span>**SIMD_SCALAR** = **1** --- 单个数字。最慢的级别。
- <span id="i_SIMD_SSE"></span>**SIMD_SSE** = **2** --- x86 CPU 单条指令处理 4 个浮点数。
- <span id="i_SIMD_SSE2"></span>**SIMD_SSE2** = **4** --- x86 CPU 单条指令处理 4 个浮点数，比 SSE 支持更多指令。
- <span id="i_SIMD_SSE3"></span>**SIMD_SSE3** = **8** --- x86 CPU 单条指令处理 4 个浮点数，比 SSE3 支持更多指令。
- <span id="i_SIMD_SSSE3"></span>**SIMD_SSSE3** = **16** --- x86 CPU 单条指令处理 4 个浮点数，比 SSE4 支持更多指令。
- <span id="i_SIMD_SSE41"></span>**SIMD_SSE41** = **32** --- x86 CPU 单条指令处理 4 个浮点数，比 SSE41 支持更多指令。
- <span id="i_SIMD_SSE42"></span>**SIMD_SSE42** = **64** --- x86 CPU 单条指令处理 4 个浮点数，比 SSE42 支持更多指令。
- <span id="i_SIMD_AVX"></span>**SIMD_AVX** = **128** --- x86 CPU 单条指令处理 8 个浮点数。
- <span id="i_SIMD_AVX2"></span>**SIMD_AVX2** = **256** --- x86 CPU 单条指令处理 8 个浮点数，比 AVX 支持更多指令。
- <span id="i_SIMD_AVX512"></span>**SIMD_AVX512** = **512** --- x86 CPU 单条指令处理 16 个浮点数。
- <span id="i_SIMD_NEON"></span>**SIMD_NEON** = **65536** --- ARM CPU 单条指令处理 4 个浮点数。


## 属性描述

### [CellularDistanceFunction](FastNoise2.md#enumerations)<span id="i_cellular_distance_function"></span> **cellular_distance_function** = CELLULAR_DISTANCE_EUCLIDEAN (0)

设置细胞噪声的距离函数。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_cellular_index0"></span> **cellular_index0** = 0

在细胞距离或值计算中使用的第 N 近的细胞。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_cellular_index1"></span> **cellular_index1** = 1

在某些细胞距离计算中用作第二分量的第 N 近的细胞。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_cellular_jitter"></span> **cellular_jitter** = 1.0

设置细胞噪声的抖动强度。

### [CellularReturnType](FastNoise2.md#enumerations)<span id="i_cellular_return_type"></span> **cellular_return_type** = CELLULAR_RETURN_INDEX_0 (0)

设置细胞噪声的返回值类型。

### [String](https://docs.godotengine.org/en/stable/classes/class_string.html)<span id="i_encoded_node_tree"></span> **encoded_node_tree** = ""

包含外部工具生成的编码节点树的 base64 字符串。设置后需要调用 [update_generator](FastNoise2.md#i_update_generator) 使更改生效。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_fractal_gain"></span> **fractal_gain** = 0.5

设置分形噪声的增益。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_fractal_lacunarity"></span> **fractal_lacunarity** = 2.0

设置分形噪声的隙度。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_fractal_octaves"></span> **fractal_octaves** = 3

设置分形噪声的八度数量。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_fractal_ping_pong_strength"></span> **fractal_ping_pong_strength** = 2.0

设置乒乓分形的强度。

### [FractalType](FastNoise2.md#enumerations)<span id="i_fractal_type"></span> **fractal_type** = FRACTAL_NONE (0)

设置分形噪声的类型。

### [NoiseType](FastNoise2.md#enumerations)<span id="i_noise_type"></span> **noise_type** = TYPE_OPEN_SIMPLEX_2 (0)

设置噪声的类型。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_period"></span> **period** = 64.0

设置噪声的周期。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_remap_enabled"></span> **remap_enabled** = false

启用后，输出值将被重映射到 [remap_output_min](FastNoise2.md#i_remap_output_min) 到 [remap_output_max](FastNoise2.md#i_remap_output_max) 的范围。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_remap_input_max"></span> **remap_input_max** = 1.0

重映射输入范围的最大值。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_remap_input_min"></span> **remap_input_min** = -1.0

重映射输入范围的最小值。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_remap_output_max"></span> **remap_output_max** = 1.0

重映射输出范围的最大值。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_remap_output_min"></span> **remap_output_min** = -1.0

重映射输出范围的最小值。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_seed"></span> **seed** = 1337

设置随机种子。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_terrace_enabled"></span> **terrace_enabled** = false

启用后，输出值将被量化为阶梯状（terrace）。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_terrace_multiplier"></span> **terrace_multiplier** = 1.0

设置阶梯（terrace）的乘数。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_terrace_smoothness"></span> **terrace_smoothness** = 0.0

设置阶梯（terrace）的平滑度。

## 方法描述

### [void](#)<span id="i_generate_image"></span> **generate_image**( [Image](https://docs.godotengine.org/en/stable/classes/class_image.html) image, [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html) tileable ) 

用噪声值填充一张灰度图像。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_get_noise_2d_single"></span> **get_noise_2d_single**( [Vector2](https://docs.godotengine.org/en/stable/classes/class_vector2.html) pos ) 

生成单个 2D 噪声值。

注意，逐个生成值所获得的 SIMD 性能提升不如一次性生成多个值。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_get_noise_3d_single"></span> **get_noise_3d_single**( [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) pos ) 

生成单个 3D 噪声值。

注意，逐个生成值所获得的 SIMD 性能提升不如一次性生成多个值。

### [SIMDLevel](FastNoise2.md#enumerations)<span id="i_get_simd_level"></span> **get_simd_level**( ) 

获取库检测到的 SIMD 级别。这可以反映当前 CPU 上的性能，因为不同型号可能支持不同的 SIMD 指令。

### [String](https://docs.godotengine.org/en/stable/classes/class_string.html)<span id="i_get_simd_level_name"></span> **get_simd_level_name**( [SIMDLevel](FastNoise2.md#enumerations) level ) 

获取 SIMD 级别的名称。

### [void](#)<span id="i_update_generator"></span> **update_generator**( ) 

修改属性后必须调用此方法，更改才会生效。

_生成于 2026-08-28_
