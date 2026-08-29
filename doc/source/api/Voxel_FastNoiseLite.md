# Voxel_FastNoiseLite

继承自：[Resource](https://docs.godotengine.org/en/stable/classes/class_resource.html)

使用 [FastNoiseLite](https://github.com/Auburn/FastNoiseLite) 库生成相干噪声和分形噪声。

## 描述：

这是基于同一库的 [FastNoiseLite](https://docs.godotengine.org/en/stable/classes/class_fastnoiselite.html) 的替代实现。一些差异包括不同的默认值，以及在分形参数中使用周期代替频率。它也在体素模块中使用，以绕过 Godot 调用的开销。

## 属性：


类型                                                                        | 名称                                                           | 默认值                                
------------------------------------------------------------------------- | ------------------------------------------------------------ | -----------------------------------
[CellularDistanceFunction](Voxel_FastNoiseLite.md#enumerations)           | [cellular_distance_function](#i_cellular_distance_function)  | CELLULAR_DISTANCE_EUCLIDEAN_SQ (1) 
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)  | [cellular_jitter](#i_cellular_jitter)                        | 1.0                                
[CellularReturnType](Voxel_FastNoiseLite.md#enumerations)                 | [cellular_return_type](#i_cellular_return_type)              | CELLULAR_RETURN_DISTANCE (1)       
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)  | [fractal_gain](#i_fractal_gain)                              | 0.5                                
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)  | [fractal_lacunarity](#i_fractal_lacunarity)                  | 2.0                                
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)      | [fractal_octaves](#i_fractal_octaves)                        | 3                                  
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)  | [fractal_ping_pong_strength](#i_fractal_ping_pong_strength)  | 2.0                                
[FractalType](Voxel_FastNoiseLite.md#enumerations)                        | [fractal_type](#i_fractal_type)                              | FRACTAL_FBM (1)                    
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)  | [fractal_weighted_strength](#i_fractal_weighted_strength)    | 0.0                                
[NoiseType](Voxel_FastNoiseLite.md#enumerations)                          | [noise_type](#i_noise_type)                                  | TYPE_OPEN_SIMPLEX_2 (0)            
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)  | [period](#i_period)                                          | 64.0                               
[RotationType3D](Voxel_FastNoiseLite.md#enumerations)                     | [rotation_type_3d](#i_rotation_type_3d)                      | ROTATION_3D_NONE (0)               
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)      | [seed](#i_seed)                                              | 0                                  
[Voxel_FastNoiseLiteGradient](Voxel_FastNoiseLiteGradient.md)             | [warp_noise](#i_warp_noise)                                  |                                    
<p></p>

## 方法：


返回值                                                                       | 函数签名                                                                                                                                                                                                                                                                    
------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)  | [get_noise_2d](#i_get_noise_2d) ( [float](https://docs.godotengine.org/en/stable/classes/class_float.html) x, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) y )                                                                              
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)  | [get_noise_2dv](#i_get_noise_2dv) ( [Vector2](https://docs.godotengine.org/en/stable/classes/class_vector2.html) position )                                                                                                                                             
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)  | [get_noise_3d](#i_get_noise_3d) ( [float](https://docs.godotengine.org/en/stable/classes/class_float.html) x, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) y, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) z )  
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)  | [get_noise_3dv](#i_get_noise_3dv) ( [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) position )                                                                                                                                             
<p></p>

## 枚举：<span id="enumerations"></span>

枚举 **NoiseType**：

- <span id="i_TYPE_OPEN_SIMPLEX_2"></span>**TYPE_OPEN_SIMPLEX_2** = **0** --- OpenSimplex2 噪声。
- <span id="i_TYPE_OPEN_SIMPLEX_2S"></span>**TYPE_OPEN_SIMPLEX_2S** = **1** --- OpenSimplex2S 噪声。
- <span id="i_TYPE_CELLULAR"></span>**TYPE_CELLULAR** = **2** --- 细胞（蜂窝）噪声。
- <span id="i_TYPE_PERLIN"></span>**TYPE_PERLIN** = **3** --- Perlin 噪声。
- <span id="i_TYPE_VALUE_CUBIC"></span>**TYPE_VALUE_CUBIC** = **4** --- 三次插值值噪声。
- <span id="i_TYPE_VALUE"></span>**TYPE_VALUE** = **5** --- 值噪声。

枚举 **FractalType**：

- <span id="i_FRACTAL_NONE"></span>**FRACTAL_NONE** = **0** --- 无分形。
- <span id="i_FRACTAL_FBM"></span>**FRACTAL_FBM** = **1** --- FBM（分形布朗运动）分形。
- <span id="i_FRACTAL_RIDGED"></span>**FRACTAL_RIDGED** = **2** --- 脊状（Ridged）分形。
- <span id="i_FRACTAL_PING_PONG"></span>**FRACTAL_PING_PONG** = **3** --- 乒乓（Ping-pong）分形。

枚举 **RotationType3D**：

- <span id="i_ROTATION_3D_NONE"></span>**ROTATION_3D_NONE** = **0** --- 无 3D 旋转。
- <span id="i_ROTATION_3D_IMPROVE_XY_PLANES"></span>**ROTATION_3D_IMPROVE_XY_PLANES** = **1** --- 改善 XY 平面的 3D 旋转。
- <span id="i_ROTATION_3D_IMPROVE_XZ_PLANES"></span>**ROTATION_3D_IMPROVE_XZ_PLANES** = **2** --- 改善 XZ 平面的 3D 旋转。

枚举 **CellularDistanceFunction**：

- <span id="i_CELLULAR_DISTANCE_EUCLIDEAN"></span>**CELLULAR_DISTANCE_EUCLIDEAN** = **0** --- 欧几里得距离。
- <span id="i_CELLULAR_DISTANCE_EUCLIDEAN_SQ"></span>**CELLULAR_DISTANCE_EUCLIDEAN_SQ** = **1** --- 欧几里得距离的平方。
- <span id="i_CELLULAR_DISTANCE_MANHATTAN"></span>**CELLULAR_DISTANCE_MANHATTAN** = **2** --- 曼哈顿距离。
- <span id="i_CELLULAR_DISTANCE_HYBRID"></span>**CELLULAR_DISTANCE_HYBRID** = **3** --- 混合距离。

枚举 **CellularReturnType**：

- <span id="i_CELLULAR_RETURN_CELL_VALUE"></span>**CELLULAR_RETURN_CELL_VALUE** = **0** --- 返回单元格的值。
- <span id="i_CELLULAR_RETURN_DISTANCE"></span>**CELLULAR_RETURN_DISTANCE** = **1** --- 返回最近单元格的距离。
- <span id="i_CELLULAR_RETURN_DISTANCE_2"></span>**CELLULAR_RETURN_DISTANCE_2** = **2** --- 返回次近单元格的距离。
- <span id="i_CELLULAR_RETURN_DISTANCE_2_ADD"></span>**CELLULAR_RETURN_DISTANCE_2_ADD** = **3** --- 返回最近与次近距离之和。
- <span id="i_CELLULAR_RETURN_DISTANCE_2_SUB"></span>**CELLULAR_RETURN_DISTANCE_2_SUB** = **4** --- 返回最近与次近距离之差。
- <span id="i_CELLULAR_RETURN_DISTANCE_2_MUL"></span>**CELLULAR_RETURN_DISTANCE_2_MUL** = **5** --- 返回最近与次近距离之积。
- <span id="i_CELLULAR_RETURN_DISTANCE_2_DIV"></span>**CELLULAR_RETURN_DISTANCE_2_DIV** = **6** --- 返回最近与次近距离之商。


## 属性描述

### [CellularDistanceFunction](Voxel_FastNoiseLite.md#enumerations)<span id="i_cellular_distance_function"></span> **cellular_distance_function** = CELLULAR_DISTANCE_EUCLIDEAN_SQ (1)

设置细胞噪声的距离函数。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_cellular_jitter"></span> **cellular_jitter** = 1.0

设置细胞噪声的抖动强度。

### [CellularReturnType](Voxel_FastNoiseLite.md#enumerations)<span id="i_cellular_return_type"></span> **cellular_return_type** = CELLULAR_RETURN_DISTANCE (1)

设置细胞噪声的返回值类型。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_fractal_gain"></span> **fractal_gain** = 0.5

设置分形噪声的增益。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_fractal_lacunarity"></span> **fractal_lacunarity** = 2.0

设置分形噪声的隙度。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_fractal_octaves"></span> **fractal_octaves** = 3

设置分形噪声的八度数量。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_fractal_ping_pong_strength"></span> **fractal_ping_pong_strength** = 2.0

设置乒乓分形的强度。

### [FractalType](Voxel_FastNoiseLite.md#enumerations)<span id="i_fractal_type"></span> **fractal_type** = FRACTAL_FBM (1)

设置分形噪声的类型。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_fractal_weighted_strength"></span> **fractal_weighted_strength** = 0.0

设置分形噪声的加权强度。

### [NoiseType](Voxel_FastNoiseLite.md#enumerations)<span id="i_noise_type"></span> **noise_type** = TYPE_OPEN_SIMPLEX_2 (0)

设置噪声的类型。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_period"></span> **period** = 64.0

设置噪声的周期。

### [RotationType3D](Voxel_FastNoiseLite.md#enumerations)<span id="i_rotation_type_3d"></span> **rotation_type_3d** = ROTATION_3D_NONE (0)

设置用于 3D 旋转的类型。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_seed"></span> **seed** = 0

设置随机种子。

### [Voxel_FastNoiseLiteGradient](Voxel_FastNoiseLiteGradient.md)<span id="i_warp_noise"></span> **warp_noise**

设置用于域扭曲的梯度噪声。

## 方法描述

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_get_noise_2d"></span> **get_noise_2d**( [float](https://docs.godotengine.org/en/stable/classes/class_float.html) x, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) y ) 

获取给定 2D 坐标处的噪声值。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_get_noise_2dv"></span> **get_noise_2dv**( [Vector2](https://docs.godotengine.org/en/stable/classes/class_vector2.html) position ) 

获取给定位置处的 2D 噪声值。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_get_noise_3d"></span> **get_noise_3d**( [float](https://docs.godotengine.org/en/stable/classes/class_float.html) x, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) y, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) z ) 

获取给定 3D 坐标处的噪声值。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_get_noise_3dv"></span> **get_noise_3dv**( [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) position ) 

获取给定位置处的 3D 噪声值。

_生成于 2026-08-28_
