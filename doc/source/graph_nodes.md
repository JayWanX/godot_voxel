# VoxelGeneratorGraph 节点

本页面列出了所有可以在 [VoxelGeneratorGraph](api/VoxelGeneratorGraph.md) 和 [VoxelGraphFunction](api/VoxelGraphFunction.md) 中使用的节点。

## 输入

### CustomInput

输出: `value`

输出与节点同名的自定义输入的值。可用于 [VoxelGraphFunction](api/VoxelGraphFunction.md)。不会在 [VoxelGeneratorGraph](api/VoxelGeneratorGraph.md) 中使用。

### InputSDF

输出: `sdf`

输出当前体素处已有的符号距离。这只能在特定情况下使用，例如将图形用作程序化笔刷。

### InputX

输出: `x`

输出当前体素的 X 坐标。

### InputY

输出: `y`

输出当前体素的 Y 坐标。

### InputZ

输出: `z`

输出当前体素的 Z 坐标。

## SDF

### SdfBox

输入: `x`, `y`, `z`
输出: `sdf`
参数: `size_x`, `size_y`, `size_z`

返回以原点为中心、大小为 `(size_x, size_y, size_z)` 的轴对齐盒体在坐标 `(x, y, z)` 处的符号距离场。

### SdfPlane

输入: `y`, `height`
输出: `sdf`

返回面向 Y 轴、位于给定 `height` 处的平面在坐标 `y` 处的符号距离场。

### SdfPreview

输入: `value`
参数: `min_value`, `max_value`, `fraction_period`, `mode`

调试节点，不用于最终结果。在编辑器中，根据边界 `[min_value, max_value]` 显示其所连接输出发出的值的一个切片。切片将沿 XY 平面或 XZ 平面，具体取决于当前设置。

### SdfSmoothSubtract

输入: `a`, `b`
输出: `sdf`
参数: `smoothness`

使用与 `SdfSmoothUnion` 节点相同的平滑方式，从 `a` 中减去符号距离场 `b`。

### SdfSmoothUnion

输入: `a`, `b`
输出: `sdf`
参数: `smoothness`

返回两个符号距离场值 `a` 和 `b` 的平滑并集。平滑度通过 `smoothness` 参数控制。平滑度越高，会在两个输入形成的形状之间产生更大的“焊接”区域。

### SdfSphere

输入: `x`, `y`, `z`
输出: `sdf`
参数: `radius`

返回以原点为中心、给定 `radius` 的球体在坐标 `(x, y, z)` 处的符号距离场。

### SdfSphereHeightmap

输入: `x`, `y`, `z`
输出: `sdf`
参数: `image`, `radius`, `factor`

返回球形高度图在坐标 `(x, y, z)` 处的符号距离场近似值。高度图是使用全景投影的 `image`，类似于 Godot 中用于环境天空的图像。球体的半径通过 `radius` 指定。高度图的高度可以使用 `factor` 参数缩放。图像必须使用未压缩格式。

### SdfTorus

输入: `x`, `y`, `z`
输出: `sdf`
参数: `radius1`, `radius2`

返回以原点为中心、面向 Y 轴的环面在坐标 `(x, y, z)` 处的符号距离场。环的半径为 `radius1`，其厚度为 `radius2`。

## 向量

### Distance2D

输入: `x0`, `y0`, `x1`, `y1`
输出: `out`

返回两个 2D 点 `(x0, y0)` 和 `(x1, y1)` 之间的距离。

### Distance3D

输入: `x0`, `y0`, `z0`, `x1`, `y1`, `z1`
输出: `out`

返回两个 3D 点 `(x0, y0, z0)` 和 `(x1, y1, z1)` 之间的距离。

### Normalize

输入: `x`, `y`, `z`
输出: `nx`, `ny`, `nz`, `len`

返回给定 `(x, y, z)` 3D 向量的归一化坐标，使输出向量的长度为 1。

## 噪声

### FastNoise2D

输入: `x`, `y`
输出: `out`
参数: `noise`

使用 FastNoiseLite 库计算坐标 `(x, y)` 处的 2D 噪声并返回。`noise` 参数使用 [Voxel_FastNoiseLite](api/Voxel_FastNoiseLite.md) 资源的实例指定。

注意：该节点可能比 `Noise2D` 稍快。

### FastNoise2_2D

输入: `x`, `y`
输出: `out`
参数: `noise`

使用 FastNoise2 库计算坐标 `(x, y)` 处的 2D SIMD 噪声并返回。`noise` 参数使用 [FastNoise2](api/FastNoise2.md) 资源的实例指定。这是当前支持的最快噪声。

### FastNoise2_3D

输入: `x`, `y`, `z`
输出: `out`
参数: `noise`

使用 FastNoise2 库计算坐标 `(x, y, z)` 处的 3D SIMD 噪声并返回。`noise` 参数使用 [FastNoise2](api/FastNoise2.md) 资源的实例指定。这是当前支持的最快噪声。

### FastNoise3D

输入: `x`, `y`, `z`
输出: `out`
参数: `noise`

使用 FastNoiseLite 库计算坐标 `(x, y, z)` 处的 3D 噪声并返回。`noise` 参数使用 [Voxel_FastNoiseLite](api/Voxel_FastNoiseLite.md) 资源的实例指定。

注意：该节点可能比 `Noise3D` 稍快。

### FastNoiseGradient2D

输入: `x`, `y`
输出: `out_x`, `out_y`
参数: `noise`

使用 FastNoiseLite 库的噪声梯度扭曲 2D 坐标 `(x, y)`。`noise` 参数使用 [FastNoiseLiteGradient](https://docs.godotengine.org/en/stable/classes/class_fastnoiselitegradient.html) 资源的实例指定。

### FastNoiseGradient3D

输入: `x`, `y`, `z`
输出: `out_x`, `out_y`, `out_z`
参数: `noise`

使用 FastNoiseLite 库的噪声梯度扭曲 3D 坐标 `(x, y, z)`。`noise` 参数使用 [FastNoiseLiteGradient](https://docs.godotengine.org/en/stable/classes/class_fastnoiselitegradient.html) 资源的实例指定。

### Noise2D

输入: `x`, `y`
输出: `out`
参数: `noise`

使用 Godot 提供的 [Noise](https://docs.godotengine.org/en/stable/classes/class_noise.html) 子类之一返回坐标 `(x, y)` 处的 2D 噪声。

### Noise3D

输入: `x`, `y`, `z`
输出: `out`
参数: `noise`

使用 Godot 提供的 [Noise](https://docs.godotengine.org/en/stable/classes/class_noise.html) 子类之一返回坐标 `(x, y, z)` 处的 3D 噪声。

### Spots2D

输入: `x`, `y`, `spot_radius`
输出: `out`
参数: `seed`, `cell_size`, `jitter`

为“矿脉”生成优化的细胞噪声：将空间划分为 2D 网格，每个单元格包含一个圆形“斑点”。当位置在斑点内部时返回 1，否则返回 0。`jitter` 或多或少会随机化斑点在每个单元格内的位置。限制：高抖动可能使斑点与单元格边界相交。这是有意为之的。如果你需要更通用的细胞噪声，请使用另一个节点。

### Spots3D

输入: `x`, `y`, `z`, `spot_radius`
输出: `out`
参数: `seed`, `cell_size`, `jitter`

为“矿脉”生成优化的细胞噪声：将空间划分为 3D 网格，每个单元格包含一个圆形“斑点”。当位置在斑点内部时返回 1，否则返回 0。`jitter` 或多或少会随机化斑点在每个单元格内的位置。限制：高抖动可能使斑点与单元格边界相交。这是有意为之的。如果你需要更通用的细胞噪声，请使用另一个节点。

## 映射

### Curve

输入: `x`
输出: `out`
参数: `curve`

返回坐标 `x` 处自定义 `curve` 的值，其中 `x` 在其域属性指定的范围内（在 Godot 4.3 及更早版本中，它在 `[0..1]` 范围内）。`curve` 使用 [Curve](https://docs.godotengine.org/en/stable/classes/class_curve.html) 资源指定。

### Image

输入: `x`, `y`
输出: `out`
参数: `image`, `filter`

返回图像在坐标 `(x, y)` 处红色通道的值，其中 `x` 和 `y` 以像素为单位，返回值在 `[0..1]` 范围内（如果图像具有 HDR 格式则可能更大）。如果坐标超出图像范围，将被环绕。不执行任何过滤。图像必须使用未压缩格式。

## 数学

### Abs

输入: `x`
输出: `out`

如果 `x` 为负，则返回 `x` 的正数形式。否则返回 `x`。

### Clamp

输入: `x`, `min`, `max`
输出: `out`

如果 `x` 低于 `min`，返回 `min`。如果 `x` 高于 `max`，返回 `max`。否则返回 `x`。

### ClampC

输入: `x`
输出: `out`
参数: `min`, `max`

如果 `x` 低于 `min`，返回 `min`。如果 `x` 高于 `max`，返回 `max`。否则返回 `x`。

该节点是 `Clamp` 的替代方案，当 `min` 和 `max` 为常量时，在内部用作优化。

### Expression

输出: `out`
参数: `expression`

计算数学表达式。变量名可以写成节点的输入。可以使用一些函数，但它们首先必须是受支持的图形节点，因为表达式会在内部转换为节点。

可用函数：

```
sin(x)
floor(x)
abs(x)
sqrt(x)
fract(x)
stepify(x, step)
wrap(x, length)
min(a, b)
max(a, b)
clamp(x, min, max)
lerp(a, b, ratio)
```

### Floor

输入: `x`
输出: `out`

返回 `floor(x)` 的结果，即小于或等于 `x` 的最近整数。

### Fract

输入: `x`
输出: `out`

返回 `x` 的小数部分。无论正负，结果始终为正。

### Max

输入: `a`, `b`
输出: `out`

返回 `a` 和 `b` 之间的较大值。

### Min

输入: `a`, `b`
输出: `out`

返回 `a` 和 `b` 之间的较小值。

### Mix

输入: `a`, `b`, `ratio`
输出: `out`

使用参数值 `t` 在 `a` 和 `b` 之间插值。如果 `t` 为 `0`，将返回 `a`。如果 `t` 为 `1`，将返回 `b`。如果 `t` 超出 `[0..1]` 范围，返回值将是一种外推。

### Pow

输入: `x`, `p`
输出: `out`

返回幂函数（`x ^ power`）的结果。它可能相对较慢。

### Powi

输入: `x`
输出: `out`
参数: `power`

返回幂函数（`x ^ power`）的结果，其中指数是常量正整数。可能比 `Pow` 更快。

### Remap

输入: `x`
输出: `out`
参数: `min0`, `max0`, `min1`, `max1`

对于 `[min0, max0]` 范围内的输入值 `x`，线性转换到 `[min1, max1]` 范围。例如，如果 `x` 是 `min0`，将返回 `min1`。如果 `x` 是 `max0`，将返回 `max1`。如果 `x` 超出 `[min0, max0]` 范围，结果将是一种外推。

### Select

输入: `a`, `b`, `t`
输出: `out`
参数: `threshold`

如果 `t` 低于 `threshold`，返回 `a`。否则返回 `b`。

### Sin

输入: `x`
输出: `out`

返回 `sin(x)` 的结果

### Smoothstep

输入: `x`
输出: `out`
参数: `edge0`, `edge1`

根据 `x` 相对于边缘 `egde0` 和 `edge1` 的位置，在 `0` 和 `1` 之间平滑插值 `x` 的值。如果 `x <= edge0`，返回值为 `0`；如果 `x >= edge1`，返回值为 `1`。如果 `x` 位于 `edge0` 和 `edge1` 之间，返回值遵循一条 S 形曲线，将 `x` 映射到 `0` 和 `1` 之间。这条 S 形曲线是三次 Hermite 插值，公式为 `f(y) = 3*y^2 - 2*y^3`，其中 `y = (x-edge0) / (edge1-edge0)`。

### Sqrt

输入: `x`
输出: `out`

返回 `x` 的平方根。

注意：与经典平方根不同，如果 `x` 为负，此函数返回 `0` 而不是 `NaN`。

### Stepify

输入: `x`, `step`
输出: `out`

将 `x` 吸附到给定步长，类似于 GDScript 的函数 `stepify`。

### Wrap

输入: `x`, `length`
输出: `out`

将 `x` 环绕在 `0` 和 `length` 之间，类似于 GDScript 的函数 `wrapf(x, 0, max)`。

注意：如果 `length` 为 0，该节点将返回 `NaN`。即使发生这种情况，也不应该崩溃，但结果会出错。

## 运算

### Add

输入: `a`, `b`
输出: `out`

返回 `a` 和 `b` 的和

### Divide

输入: `a`, `b`
输出: `out`

返回 `a / b` 的结果。

注意：除以零会输出 NaN。这不会导致崩溃，但很可能使结果出错。尽可能考虑使用 Multiply。

### Multiply

输入: `a`, `b`
输出: `out`

返回 `a * b` 的结果。

### Subtract

输入: `a`, `b`
输出: `out`

返回 `a - b` 的结果

## 输出

### CustomOutput

输入: `value`

设置与节点同名的自定义输出的值。可用于 [VoxelGraphFunction](api/VoxelGraphFunction.md)。不会在 [VoxelGeneratorGraph](api/VoxelGeneratorGraph.md) 中使用。

### OutputSDF

输入: `sdf`

设置当前体素的符号距离场值。

### OutputSingleTexture

输入: `index`

设置当前体素的纹理索引。如果你的体素只有一个纹理，这是使用 `OutputWeight` 节点的替代方案。它更易于使用，但不允许长渐变。不支持将此节点与 `OutputWeight` 组合使用。

### OutputType

输入: `type`

设置当前体素的 TYPE 索引。这用于 [VoxelMesherBlocky](api/VoxelMesherBlocky.md)。如果使用此输出，则无需使用 `OutputSDF`。

### OutputWeight

输入: `weight`
参数: `layer`

设置当前体素的特定纹理权重的值。纹理通过 `layer` 参数以索引形式指定。使用给定图层索引的输出只能有一个。

## 杂项

### Comment

参数: `text`

一个带有描述的矩形区域，用于帮助组织图形。

### Constant

输出: `value`
参数: `value`

输出一个常量数字。

### Function

参数: `_function`

运行一个自定义函数，类似于可复用的子图形。该节点的第一个参数（参数 0）是对 [VoxelGraphFunction](api/VoxelGraphFunction.md) 的引用。其余参数（从 1 开始）是函数暴露的参数。

### Relay

输入: `in`
输出: `out`

直通节点，可以更好地组织长连接的路径。

