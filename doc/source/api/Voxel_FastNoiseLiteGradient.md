# Voxel_FastNoiseLiteGradient

继承自：[Resource](https://docs.godotengine.org/en/stable/classes/class_resource.html)

使用 [FastNoiseLite](https://github.com/Auburn/FastNoiseLite) 库生成相干噪声和分形噪声梯度。

## 描述：

这是 [FastNoiseLite](https://docs.godotengine.org/en/stable/classes/class_fastnoiselite.html) 的另一种实现，因为 Godot 的集成没有提供直接访问梯度的方法。

这些算法专门用于生成扰动位置的向量，可能比计算 2 或 3 次普通噪声更快。

## 属性：


类型                                                                        | 名称                                           | 默认值                  
------------------------------------------------------------------------- | -------------------------------------------- | ---------------------
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)  | [amplitude](#i_amplitude)                    | 30.0                 
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)  | [fractal_gain](#i_fractal_gain)              | 0.5                  
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)  | [fractal_lacunarity](#i_fractal_lacunarity)  | 2.0                  
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)      | [fractal_octaves](#i_fractal_octaves)        | 3                    
[FractalType](Voxel_FastNoiseLiteGradient.md#enumerations)                | [fractal_type](#i_fractal_type)              | FRACTAL_NONE (0)     
[NoiseType](Voxel_FastNoiseLiteGradient.md#enumerations)                  | [noise_type](#i_noise_type)                  | TYPE_VALUE (2)       
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)  | [period](#i_period)                          | 64.0                 
[RotationType3D](Voxel_FastNoiseLiteGradient.md#enumerations)             | [rotation_type_3d](#i_rotation_type_3d)      | ROTATION_3D_NONE (0) 
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)      | [seed](#i_seed)                              | 0                    
<p></p>

## 方法：


返回值                                                                           | 函数签名                                                                                                             
----------------------------------------------------------------------------- | -----------------------------------------------------------------------------------------------------------------
[Vector2](https://docs.godotengine.org/en/stable/classes/class_vector2.html)  | [warp_2d](#i_warp_2d) ( [Vector2](https://docs.godotengine.org/en/stable/classes/class_vector2.html) position )  
[Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html)  | [warp_3d](#i_warp_3d) ( [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) position )  
<p></p>

## 枚举：<span id="enumerations"></span>

枚举 **NoiseType**：

- <span id="i_TYPE_OPEN_SIMPLEX_2"></span>**TYPE_OPEN_SIMPLEX_2** = **0** --- OpenSimplex2 噪声。
- <span id="i_TYPE_OPEN_SIMPLEX_2_REDUCED"></span>**TYPE_OPEN_SIMPLEX_2_REDUCED** = **1** --- OpenSimplex2 的降噪（Reduced）变体。
- <span id="i_TYPE_VALUE"></span>**TYPE_VALUE** = **2** --- 值噪声。

枚举 **FractalType**：

- <span id="i_FRACTAL_NONE"></span>**FRACTAL_NONE** = **0** --- 无分形。
- <span id="i_FRACTAL_DOMAIN_WARP_PROGRESSIVE"></span>**FRACTAL_DOMAIN_WARP_PROGRESSIVE** = **1** --- 渐进式域扭曲分形。
- <span id="i_FRACTAL_DOMAIN_WARP_INDEPENDENT"></span>**FRACTAL_DOMAIN_WARP_INDEPENDENT** = **2** --- 独立式域扭曲分形。

枚举 **RotationType3D**：

- <span id="i_ROTATION_3D_NONE"></span>**ROTATION_3D_NONE** = **0** --- 无 3D 旋转。
- <span id="i_ROTATION_3D_IMPROVE_XY_PLANES"></span>**ROTATION_3D_IMPROVE_XY_PLANES** = **1** --- 改善 XY 平面的 3D 旋转。
- <span id="i_ROTATION_3D_IMPROVE_XZ_PLANES"></span>**ROTATION_3D_IMPROVE_XZ_PLANES** = **2** --- 改善 XZ 平面的 3D 旋转。


## 属性描述

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_amplitude"></span> **amplitude** = 30.0

设置扭曲向量的幅度。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_fractal_gain"></span> **fractal_gain** = 0.5

设置分形噪声的增益。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_fractal_lacunarity"></span> **fractal_lacunarity** = 2.0

设置分形噪声的隙度。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_fractal_octaves"></span> **fractal_octaves** = 3

设置分形噪声的八度数量。

### [FractalType](Voxel_FastNoiseLiteGradient.md#enumerations)<span id="i_fractal_type"></span> **fractal_type** = FRACTAL_NONE (0)

设置分形噪声的类型。

### [NoiseType](Voxel_FastNoiseLiteGradient.md#enumerations)<span id="i_noise_type"></span> **noise_type** = TYPE_VALUE (2)

设置噪声的类型。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_period"></span> **period** = 64.0

设置噪声的周期。

### [RotationType3D](Voxel_FastNoiseLiteGradient.md#enumerations)<span id="i_rotation_type_3d"></span> **rotation_type_3d** = ROTATION_3D_NONE (0)

设置用于 3D 旋转的类型。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_seed"></span> **seed** = 0

设置随机种子。

## 方法描述

### [Vector2](https://docs.godotengine.org/en/stable/classes/class_vector2.html)<span id="i_warp_2d"></span> **warp_2d**( [Vector2](https://docs.godotengine.org/en/stable/classes/class_vector2.html) position ) 

生成一个用于扰动给定 2D 位置的向量。

### [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html)<span id="i_warp_3d"></span> **warp_3d**( [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) position ) 

生成一个用于扰动给定 3D 位置的向量。

_生成于 2026-08-28_
