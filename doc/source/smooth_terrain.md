Smooth 地形
===================

可以使用有符号距离场和 `VoxelMesherTransvoxel` 来处理外观平滑的地形。


有符号距离场
-------------------------

### 概念

为了表示平滑地形，使用点只能为 0 或 1 的网格是不够的。这种网格适合多边形化方块表面，但不适合曲线。它可以通过某种方式取平均或模糊，但这会很昂贵。

对于空间中的任意一点，有符号距离场（SDF）是到最近表面的距离。它被称为“有符号”，因为如果点位于表面之下（“在某物内部”），该距离就会变为负值。这意味着表面由所有 SDF 为 0 的点定义。这个 `0` 通常被称为 `isolevel`。

例如，`1` 表示我们距离形状 1 个单位。`-1` 表示我们位于形状内部 1 个单位。`0` 表示我们正好在形状表面上。

SDF 通常用于光线步进着色器中作为定义体积的一种方式。它也被用于 Transvoxel 算法（Marching Cubes 的变体），该算法由本引擎实现。因此，体素存储的不是离散值，而是一种平滑的距离“梯度”。


### 技术说明

体素在 3D 中使用 SDF，为了帮助可视化，我们来看一下 2D 示例。
如果我们用方块体素表示一个球体，我们会将以下伪代码应用于每个体素：

```
if distance(center, position) < radius:
	voxel = 1
else:
	voxel = 0
```

结果如下：

![方块 SDF](images/sdf_example_blocky.webp)

每个体素都有二进制的值，要么是 1 要么是 0。但这并没有提供关于“物质”和“空气”之间过渡如何发生的信息，所以如果我们用 Transvoxel 渲染它，结果将是：

![方块 SDF 球体](images/sdf_sphere_blocky.webp)

它有点方块感。现在，我们可能确实想要这个结果（参见关于着色器的部分）。但如果不想要，我们将需要修改代码。事实上，如果我们后退一步，答案已经在那里了：

```
voxel = distance(origin, position) - radius
```

这就是球体的有符号距离。这里显示为归一化形式，因此接近 `0` 的体素是灰色的：

![真实 SDF](images/sdf_example_true.webp)

每个体素现在都包含一个缓慢变化的梯度，因此当 Transvoxel 遍历所有单元格寻找表面时，它会看到比仅仅 `0` 或 `1` 更精确的变化，这使它能够生成平滑的多边形。

![平滑 SDF 球体](images/sdf_sphere_smooth.webp)


### SDF 编码

#### 钳制

本引擎允许编辑并保存体素。存储真实的 SDF 对游戏来说可能很昂贵。例如，由于它是*距离*，如果玩家在地面上建造一座小塔，仅仅因为塔使地面离他们稍微近了一点，我们就必须保持天空中远处体素的最新状态。所以实际上，并不需要处理精确的 SDF。我们只需要足够好的东西，这样梯度可以以不同速度变化，修改也能保持“局部”性。

天空中远处的体素实际上对我们来说并不重要。表面才是我们真正需要的。所以我们可以钳制距离，使距离表面足够远的体素具有相同的值。如果整个区块具有相同的值，它就可以作为“均匀”区块被优化掉。

所以我们之前看到的球体 SDF 在数据中实际看起来像这样：

![钳制后的 SDF](images/sdf_example_clamped.webp)

在多个区块中，所有没有梯度的区域在内存中占用的空间都非常小。

#### 量化

为了进一步节省内存，本引擎不使用 32 位 `float` 数字存储 SDF（默认情况下）。相反，它使用 8 位或 16 位整数，这些整数被解释为定点小数。在内部，会选择一个实际的值范围来分配更有限的小数值。
默认情况下，引擎使用 16 位。

深度  | 最小值   | 最大值   | 步长
-------|-----------|-----------|-------
8-bit  | -10.0     | 10.0      | ~0.078
16-bit | -500.0    | 500.0     | ~0.015

当设置为 32 位时，缓冲区将直接使用 `float` 数字。

实际上，这意味着当你将有符号距离存储到 `VoxelBuffer` 中时，取回数值时会有一点不精确。

使用 LOD 时，16 位范围可以延伸得相对较远，这在区块变得越来越大时有助于保持梯度。使用 8 位时，梯度会变得相对很小，结果会开始被钳制，导致出现方块状外观。

#### 链接

关于 SDF 以及使用它们的其他领域的更多信息，你可以看看这些视频：

- [Glyphs, shapes, fonts, signed distance fields. (Martin Donald)](https://www.youtube.com/watch?v=1b5hIMqz_wM)
- [Coding Adventure: Marching Cubes (Sebastian Lague)](https://www.youtube.com/watch?v=M3iI2l0ltbE)
- [Painting a Cartoon Girl using Mathematics (Inigo Quilez)](https://www.youtube.com/watch?v=8--5LwHRhjk)
- [Code for a bunch of SDF functions and operations (Inigo Quilez)](https://iquilezles.org/articles/distfunctions/)


Transvoxel
-----------

### 定义

Transvoxel 是 Marching Cubes 的扩展，可用于从体素数据创建平滑网格。该算法的优势在于可以整合不同细节级别的拼接而不会产生裂缝，因此它可以用于渲染非常大的地形。

更多信息请访问 [https://transvoxel.org/](https://transvoxel.org/)。


### 顶点着色器中的平滑拼接<span id="smooth-stitches-in-vertex-shader"></span>

Transvoxel 使用特殊网格来拼接不同细节级别的数据块。然而接缝仍可能以偶尔出现的尖锐小阶梯形式可见。为了稍微平滑这一点，`VoxelMesherTransvoxel` 生成的网格在其 `CUSTOM0` 属性中包含额外信息，告诉如何移动顶点来平滑这些阶梯，并在网格的常规部分为它们留出空间。

在你的地形上创建并设置一个 `ShaderMaterial`，并将此代码片段整合到其中：

```glsl
// 该 uniform 由体素引擎在内部赋值。
// Layout: 00000000 00000000 0000000 00xxyyzz
// 其中：
// - xx 分别是 -x 和 x 的过渡，
// - yy 分别是 -y 和 y 的过渡，
// - zz 分别是 -z 和 z 的过渡，
uniform int u_transition_mask;

float get_transvoxel_secondary_factor(int idata) {
	int transition_mask = u_transition_mask & 0xff;

	int cell_border_mask = (idata >> 0) & 63; // 单元格接触的哪些边
	int vertex_border_mask = (idata >> 8) & 63; // 顶点接触的哪些边
	// 如果顶点靠近存在低分辨率邻居的边，
	// 将其移动到次要位置
	int m = transition_mask & cell_border_mask;
	float t = float(m != 0);
	// 如果顶点位于一条或多条边上，且至少有一条边没有低分辨率邻居，
	// 则不移动顶点。
	t *= float((vertex_border_mask & ~transition_mask) == 0);
	
	return t;
}

vec3 get_transvoxel_position(vec3 vertex_pos, vec4 fdata) {
	int idata = floatBitsToInt(fdata.a);

	// 移动顶点以平滑过渡
	float secondary_factor = get_transvoxel_secondary_factor(idata);
	vec3 secondary_position = fdata.xyz;
	vec3 pos = mix(vertex_pos, secondary_position, secondary_factor);

	// 如果网格合并了过渡且顶点属于某个过渡，
	// 当该过渡不激活时我们会改变顶点的位置，
	// 使所有三角形都退化而不可见。
	// 这是单独渲染它们的替代方案，
	// 在 Godot 中具有更少的绘制调用和更少的网格资源。
	// 理想情况下我会像 LOD 那样调整索引缓冲区，但 Godot 没有
	// 暴露任何以这种方式使用它的接口。
	int itransition = (idata >> 16) & 0xff; // 顶点是否在过渡网格上？
	float transition_cull = float(itransition == 0 || (itransition & u_transition_mask) != 0);
	pos *= transition_cull;

	return pos;
}

void vertex() {
	VERTEX = get_transvoxel_position(VERTEX, CUSTOM0);
    //...
}
```

导致这段代码的研究 issue：[Issue #2](https://github.com/Voxel/godot_voxel/issues/2)


纹理
-----------

为体素表面添加纹理可能比经典 3D 网格更困难，因为几何体事先未知，并且几乎可以是任何形状。因此在本节中，我们将回顾解决 UV 映射的方法、程序化技术以及从体素数据混合纹理的方法。

### 三平面映射

经典 UV 映射无法用于平滑体素表面，因为它可能包含任意形状。事实上，平滑网格生成器不提供任何合适的 UV。因此，我们可以改用三平面映射。

该方法涉及将纹理投影到直接面向 X 轴的物体部分，然后投影到直接面向 Y 轴的侧面，再对 Z 轴执行同样操作。这些投影的边缘随后以指定的锐度混合在一起。

看看右上角球体上砖块纹理是如何混合在一起的。

![三平面映射图像](images/triplanar_example.webp)

阅读关于 [Godot 中的三平面映射](https://docs.godotengine.org/en/latest/tutorials/3d/standard_material_3d.html#triplanar-mapping)。

也可以为 3 个轴选择不同的纹理。

下面是一个支持两种材质的着色器，例如顶部为草、侧面为岩石，每种材质都有三平面映射的 albedo、normal 和 AO 贴图，然后根据它们的法线是朝向正上方还是侧面来混合在一起。

你可以在[演示](https://github.com/Voxel/voxelgame)中找到可运行的示例，或者查看[着色器](https://github.com/Voxel/voxelgame/blob/godot4/project/smooth_terrain/transvoxel_terrain.gdshader)本身（三平面函数定义在 https://github.com/Voxel/voxelgame/blob/godot4/project/smooth_terrain/shaders/triplanar.gdshaderinc）。

在着色器参数中，添加你的两张 albedo 贴图，以及可选的 normal 和 AO 贴图。然后调整 `AB Mix 1` 和 `AB Mix 2` 滑块，以调整顶部和侧面的混合方式。其他设置应该不言自明。下面的截图还添加了一点雾和远景景深。

![带纹理的地形](images/textured-terrain.jpg)


### 程序化纹理

体素数据很重，因此如果你的游戏纹理规则足够简单，可以由着色器决定并且不影响游戏玩法，你就不需要在体素中定义任何额外数据。例如，你可以检查地形表面的法线来在草地和岩石纹理之间混合，并在一定高度以上使用雪。

### 体素纹理

另一方面，如果你确实需要让纹理成为体素本身的属性，就有必要以某种方式将这些信息存储在它们中。这意味着体素数据将占用更多空间，并且必须进行更多体积处理来应对这一点。

体素纹理可以分为 2 部分：

- 体素格式：体素实际包含什么。
- 顶点格式：用于由体素数据构建的网格中，以便着色器可以渲染它。

### 体素纹理格式

#### Single

最简单的格式是给每个体素一个 8 位索引，告诉它们具有什么纹理。这允许在 256 种可能纹理的池中进行选择。这通常远远超过平滑地形的需要。这种格式无法表示渐变，因此绘制没有衰减。

该数据存储在 [INDICES](api/VoxelBuffer.md#i_CHANNEL_INDICES) 通道中，深度为 8 位。

!!! note
	这种模式最简单，但并不是最先实现的。所以默认的体素格式不适合它。要解决这个问题，请为你的地形分配一个新的 [VoxelFormat](api/VoxelFormat.md) 资源，并将其 `indices_depth` 改为 `8-bit`。然后确保你的网格生成器将纹理模式设置为 `Single`。在某些情况下，你可能还需要配置你的生成器也输出该格式（尤其是 [VoxelGeneratorGraph](api/VoxelGeneratorGraph.md)）。

使用该格式编辑体素纹理就像编辑“方块体素”一样简单。因此编辑函数可以与 [SET](api/VoxelTool.md#i_MODE_SET) 模式一起使用。

```gdscript
# 在球体区域中绘制纹理 2（不创建物质）
voxel_tool.mode = VoxelTool.MODE_SET
voxel_tool.channel = VoxelBuffer.CHANNEL_INDICES
voxel_tool.value = 2
voxel_tool.do_sphere(hit_position, radius)
```

!!! note
	`VoxelTool` 有一个 `TEXTURE` 模式和 `texture_index` 属性，但这些目前仅用于 `Mixel4`。在编辑 `Single` 纹理数据时不应使用它们。


#### Mixel4

另一种方法是在每个体素中存储多个索引，以及使用权重表示它拥有其中每个索引的多少。
这允许定义渐变，因为一片土地可以逐渐在某种纹理较少到较多的体素之间过渡，而其他纹理则减少。该实现每个体素最多可以存储 4 种不同的纹理。

然而这种格式有很多缺点：它更重且操作复杂。该实现还将纹理数量限制为 16。

4 种纹理（`a`、`b`、`c`、`d`）的数据编码在 [INDICES](api/VoxelBuffer.md#i_CHANNEL_INDICES) 和 [WEIGHTS](api/VoxelBuffer.md#i_CHANNEL_WEIGHTS) 两个通道上，两者都使用 16 位深度。因此每种纹理只有 4 位精度。

```
          1st byte    2nd byte
INDICES:  aaaa bbbb   cccc dddd
WEIGHTS:  aaaa bbbb   cccc dddd
```

默认情况下，这些通道默认为索引 `(0,1,2,3)` 和权重 `(1,0,0,0)`，意味着体素总是以纹理 `0` 开始。另一条规则是索引不能出现两次（例如，索引 `(0,1,1,1)` 是无效的）。如果一个索引出现两次，尤其是在具有不同权重的情况下，可能会导致计算歧义。

要编辑这种格式，你必须使用 [TEXTURE_PAINT](api/VoxelTool.md#i_MODE_TEXTURE_PAINT) 模式。

```gdscript
# 在球体区域中绘制纹理 2（不创建物质）
voxel_tool.mode = VoxelTool.MODE_TEXTURE_PAINT
voxel_tool.texture_index = 2
voxel_tool.set_texture_opacity = 1.0
voxel_tool.do_sphere(hit_position, radius)
```

可以直接使用 `VoxelTool.set_voxel` 设置 mixel 值，但正确打包它们由你自己负责。

你可以使用 `VoxelTool` 辅助函数来编码/解码这些值：

- [vec4i_to_u16_indices](https://voxel-tools.readthedocs.io/en/latest/api/VoxelTool/#i_vec4i_to_u16_indices)
- [color_to_u16_weights](https://voxel-tools.readthedocs.io/en/latest/api/VoxelTool/#i_color_to_u16_weights)
- [u16_indices_to_vec4i](https://voxel-tools.readthedocs.io/en/latest/api/VoxelTool/#i_u16_indices_to_vec4i)
- [u16_weights_to_color](https://voxel-tools.readthedocs.io/en/latest/api/VoxelTool/#i_u16_weights_to_color)


也可以使用特殊输出在 `VoxelGeneratorGraph` 中生成它，但仍然需要一点数学计算来产生有效数据。

另请参阅这个[绘制演示](https://github.com/Voxel/voxelgame/tree/master/project/smooth_materials)。


### 网格数据

目前，所有体素纹理格式都被合并到相同的顶点格式中：在计算每个 marching cube 单元格时，网格生成器收集对应体素上最具代表性的纹理，并将结果存储在顶点数据中。这种格式在 `VoxelMesherTransvoxel` 中被称为 `S4`，因为它收集 4 个纹理为一组。

网格生成器将把纹理信息包含在顶点的 `CUSTOM1` 属性中。与体素值相反，打包的信息将有 8 位精度：

- `CUSTOM1.x` 将包含 4 个索引，编码为 4 个字节，可以通过将浮点数重新解释为整数并使用位移运算符来获得。
- `CUSTOM1.y` 将包含 4 个权重，同样编码为 4 个字节。

每个索引告诉需要使用哪种纹理，每个权重分别告诉应该混合多少该纹理。这本质上与经典的颜色 splatmap 相同，只是纹理可以变化，这允许超过 4 种可能的纹理。
一个次要缺点是每个体素不能混合超过 4 种纹理，所以如果发生这种情况，可能会导致伪影。但在实践中，可以认为这种情况非常罕见，可以忽略不计。


#### 着色器

要使用顶点中编码的数据渲染体素纹理，着色器是必需的。如果你可以有超过 4 种不同的纹理，也最好将你的纹理放在 [Texture2DArray](https://docs.godotengine.org/en/stable/classes/class_texture2darray.html) 中。

以下是你需要的着色器代码：

```glsl
shader_type spatial;

// 纹理最好放在 Texture2DArray 中，这样查找成本低
uniform sampler2DArray u_texture_array : source_color;

// 我们需要将数据从顶点着色器传递到片段着色器
varying vec4 v_indices;
varying vec4 v_weights;
varying vec3 v_normal;
varying vec3 v_pos;

// 我们将使用一个工具函数来解码分量。
// 它返回范围 [0..255] 内的 4 个值。
vec4 decode_8bit_vec4(float v) {
	uint i = floatBitsToUint(v);
	return vec4(
		float(i & uint(0xff)),
		float((i >> uint(8)) & uint(0xff)),
		float((i >> uint(16)) & uint(0xff)),
		float((i >> uint(24)) & uint(0xff)));
}

// 体素网格可能在任意方向有悬垂，
// 因此我们可能需要使用三平面映射函数。
vec3 get_triplanar_blend(vec3 world_normal) {
	vec3 blending = abs(world_normal);
	blending = normalize(max(blending, vec3(0.00001))); // 强制权重之和为 1.0
	float b = blending.x + blending.y + blending.z;
	return blending / vec3(b, b, b);
}

vec4 texture_array_triplanar(sampler2DArray tex, vec3 world_pos, vec3 blend, float i) {
	vec4 xaxis = texture(tex, vec3(world_pos.yz, i));
	vec4 yaxis = texture(tex, vec3(world_pos.xz, i));
	vec4 zaxis = texture(tex, vec3(world_pos.xy, i));
	// 混合 3 个平面投影的结果。
	return xaxis * blend.x + yaxis * blend.y + zaxis * blend.z;
}

void vertex() {
	// 索引是整数值，因此我们可以直接解码
	v_indices = decode_8bit_vec4(CUSTOM1.x);

	// 权重必须在 [0..1] 内，因此我们除以它们
	v_weights = decode_8bit_vec4(CUSTOM1.y) / 255.0;

	v_pos = VERTEX;
	v_normal = NORMAL;

	//...
}

void fragment() {
	// 为方便起见，定义一个纹理缩放。
	// 如果需要每个索引使用不同缩放，我们可以改用数组。
	float uv_scale = 0.5;

	// 采样 4 个混合纹理，全部使用三平面映射。
	// 我们可以为它们复用相同的三平面混合因子，因此将该部分
	// 从函数中分离出来可以稍微提升性能。
	vec3 blending = get_triplanar_blend(v_normal);
	vec3 col0 = texture_array_triplanar(u_texture_array, v_pos * uv_scale, blending, v_indices.x).rgb;
	vec3 col1 = texture_array_triplanar(u_texture_array, v_pos * uv_scale, blending, v_indices.y).rgb;
	vec3 col2 = texture_array_triplanar(u_texture_array, v_pos * uv_scale, blending, v_indices.z).rgb;
	vec3 col3 = texture_array_triplanar(u_texture_array, v_pos * uv_scale, blending, v_indices.w).rgb;

	// 获取权重并确保它们已归一化。
	// 我们可以添加一个微小的安全余量，以允许一定程度的误差。
	vec4 weights = v_weights;
	weights /= (weights.x + weights.y + weights.z + weights.w + 0.00001);

	// 计算 albedo
	vec3 col = 
		col0 * weights.r + 
		col1 * weights.g + 
		col2 * weights.b + 
		col3 * weights.a;

	ALBEDO = col;

	//...
}
```

![平滑体素绘制原型](images/smooth_voxel_painting_on_plane.webp)

!!! note
	如果你想使用 `Mixel4` 但只需要 4 种纹理，那么你可以将索引保留为默认值（包含 `0,1,2,3`）而只使用权重。使用 `VoxelTool` 时，你只能使用纹理索引 0、1、2 或 3。在这种情况下纹理数组不太相关。


### 推荐阅读

- [Shading Index](https://docs.godotengine.org/en/stable/tutorials/shading/index.html) - 教程和着色器语言 API
- 着色器 API 参考 - 一些最常访问的参考资料
	- [Shading Language](https://docs.godotengine.org/en/stable/tutorials/shading/shading_reference/shading_language.html)
	- [SpatialShader](https://docs.godotengine.org/en/stable/tutorials/shading/shading_reference/spatial_shader.html)



着色
---------

默认情况下，平滑体素通过共享顶点也会产生平滑的网格。这也有助于网格在内存中更小。

### 低多边形 / 平面着色外观

目前无法让网格生成器生成带有分离平面三角形的顶点，但你可以在片段着色器中使用这个。

使用 Vulkan 时：
```glsl
NORMAL = normalize(cross(dFdy(VERTEX), dFdx(VERTEX)));
```
使用 OpenGL 时：
```glsl
NORMAL = normalize(cross(dFdx(VERTEX), dFdy(VERTEX)));
```

![平面着色](images/flat_shading.webp)

### 方块外观

也可以给“平滑”体素赋予“方块”外观：

![方块 SDF](images/blocky_sdf.webp)

这可以通过在体素生成器中饱和 SDF 值来实现：它们必须始终为 -1 或 1，不能有过渡值。由于使用 `set_voxel_f` 时值会被钳制，乘以一个大数也同样有效。内置的基本生成器可能没有这个选项，但如果你使用自己的生成器脚本或 `VoxelGeneratorGraph`，你可以这样做。

你也可以在着色器中使明暗产生硬边，以获得更好的效果。


着色器 API 参考
----------------------

如果你在体素节点上使用 `ShaderMaterial`，模块将识别一些 uniform 名称（着色器参数）来提供额外信息。其中一些是功能正常工作所必需的。

参数名                          | 类型         | 描述
----------------------------------------|--------------|------------------------------
`u_lod_fade`                            | `vec2`       | 用于在多细节级别之间渐进淡化的信息。`x` 是淡化进度：在过渡期间将从 0.0 到 1.0。`y` 是淡化方向：`1.0` 表示淡入，`0.0` 表示淡出。仅在使用 `VoxelLodTerrain` 时可用。参见 [Lod fading](#lod-fading)
`u_block_local_transform`               | `mat4`       | 所渲染数据块的变换，相对于整个体积是局部的，因为它们可能由多个网格渲染。如果体积在移动，用于修复三平面映射很有用。目前仅在 `VoxelLodTerrain` 中可用。
`u_voxel_cell_lookup`                   | `usampler2D` | 3D `RG8` 纹理，每个像素包含打包在 `R` 和部分 `G` 字节中的单元格索引（`r + ((g & 0x3f) << 8)`），以及 `G` 的 2 位轴索引（`g >> 6`）。用于索引此纹理的位置相对于网格的原点。纹理是 2D 且为正方形，因此可以根据网格在体素中的大小计算坐标。仅在使用了[法线贴图](#detail-rendering)细节纹理的网格中赋值。
`u_voxel_normalmap_atlas`               | `sampler2D`  | 纹理图集，每个瓦片包含一个模型空间法线贴图（与常见法线贴图不同，它不是相对于表面的）。坐标可以从 `u_voxel_cell_lookup` 和 `u_voxel_virtual_texture_tile_size` 计算得出。UV 方向类似于三平面映射，但轴是从 `u_voxel_cell_lookup` 中的信息获知的。仅在使用了[法线贴图](#detail-rendering)细节纹理的网格中赋值。
`u_voxel_virtual_texture_tile_size`     | `int`        | `u_voxel_normalmap_atlas` 中每个瓦片的分辨率，以像素为单位。
`u_voxel_cell_size`                     | `float`      | 网格中一个立方体单元格的大小，以模型空间单位表示。在具有[法线贴图](#detail-rendering)的体素网格中将大于 0。
`u_voxel_block_size`                    | `int`        | 网格所表示的体素立方数据块的大小，以体素为单位。
`u_voxel_virtual_texture_fade`          | `float`      | 当启用 LOD 淡化时，这个值将在 0 到 1 之间，表示混合多少细节纹理（如 `u_voxel_normalmap_atlas`）。它们需要时间更新，因此这允许它们平滑出现。如果未启用淡化，值为 1；如果网格没有细节纹理，值为 0。
`u_voxel_virtual_texture_offset_scale`  | `vec4`       | 用于启用法线贴图的 LOD 地形中。包含采样 `u_voxel_cell_lookup` 和 `u_voxel_normalmap_atlas` 时要应用的变换。`x`、`y` 和 `z` 包含偏移，`w` 包含缩放。当当前网格的纹理尚未准备好时，这很有用，因此会回退到父级 LOD：父网格更大，所以我们需要采样一个子区域。
`u_transition_mask`                     | `int`        | 使用 `VoxelMesherTransvoxel` 时，这是一个存储有关不同细节级别相邻网格信息的位掩码。如果网格 6 个边中的某一个有低分辨率邻居，对应的位将为 `1`。边的顺序为 `-X`、`X`、`-Y`、`Y`、`-Z`、`Z`，存储在第一字节。布局：`00000000 00000000 00000000 00xxyyzz`。参见[顶点着色器中的平滑拼接](#smooth-stitches-in-vertex-shader)。
`u_voxel_lod_info`                      | `int`        | 将被赋值为数据块的 LOD 索引与 LOD 总数量的组合。布局：`000000 000000 cccccccc iiiiiiii`，其中 `c` 是 LOD 数量，`i` 是 LOD 索引。主要用于调试。


多细节级别（LOD）
-----------------------

`VoxelLodTerrain` 为平滑地形实现了动态多细节级别。

### 描述

LOD（Level Of Detail，多细节级别）是一种用于动态改变几何体数量的技术，使靠近观察者的网格具有高细节，而远离观察者的网格被简化。这旨在提高性能。

![LOD 示例](images/lod_example.webp)

!!! note
	注意：在本引擎中，`LOD` *级别*经常用 `0` 到 `N-1` 的数字表示，其中 `N` 是 LOD 的数量。`0` 是*最高的细节级别*，而 LOD `1`、`2` 等到 `N-1` 是*较低的细节级别*。

![带体素网格的多细节级别示意图](images/lod_density_schema.webp)

从 LOD `i` 到 `i+1` 时，体素和数据块的大小加倍，覆盖更多空间。然而数据块的分辨率不变，因此细节密度更低，消耗的资源更少。


### 八叉树

LOD 使用多个父子网格实现，每个网格的数据块大小是其子网格的两倍。根网格（最低 LOD）的数据块可以被视为八叉树：它可以在子网格中细分为 8 个更小的数据块，这些数据块本身又可以再细分 8 个，以此类推，直到达到 LOD0（最高细节级别）。

当观察者靠近时会发生细分。触发细分的阈值由 `lod_distance` 属性控制。它表示 LOD0 将在观察者周围延伸多远。它也可能影响其他 LOD 延伸多远，因此它整体上控制质量。

与 `VoxelTerrain` 类似，当观察者四处移动时，每个网格的数据块会在前方加载，而离得太远的会被卸载。这允许保持对“无限”地形的支持，而不必设置具有不必要深度级别的单个八叉树。所以从某种意义上说，存在多个“八叉树”。

观察者周围最大网格的大小取决于两个因素：

- `VoxelLodTerrain` 的 `view_distance` 参数
- `VoxelViewer` 上的 `view_distance` 参数。

最终的视距计算为两者的最小值，并用地形边界进行裁剪。


#### 数据流系统

目前有多个系统处理观察者周围 LOD 数据块的加载方式。这是出于遗留原因。每个系统都有不同的行为和功能。可以在 `VoxelLodTerrain` 的 Advanced 类别下以 `streaming_system` 选择。

- `Octree`：原始系统。使用显式八叉树数据结构，定期遍历，以球形模式加载数据块。经过了更多测试。然而，它只支持一个观察者。如果场景中有多个观察者，它会挑选一个，但不保证是哪一个。由于定期遍历，它可能不太适合大量数据块（较大的 `lod_distance`）。
- `Clipbox`：加载同心盒中的数据块，并在观察者移动时只更新差异部分，类似于 `VoxelTerrain`。它被添加以支持多个观察者，并解决 Octree 系统在多人在线方面的某些缺点。由于必须符合盒形，它加载数据块的模式可能不如八叉树精确。这个名字是虚构的，是参照高度图地形中外观相似的“clipmaps”而选择的。

在编辑器中，可以在 `Terrain` 菜单中显示调试绘制的“gizmos”。其中一些只在选中特定数据流系统时才显示内容。


### LOD 数量

增加 LOD 数量允许地形拥有更大的数据块，进而允许增加视距。它实际上并不会让 `LOD0` 更精细，而是反过来的：添加更远的地形（如果你期望相反的结果，也许你需要调整生成器以生成更大的形状、减小体素大小，或更改可能太小的游戏比例）。
如果你更改 LOD 数量，可能会注意到网格大小会变化：这是因为它会四舍五入到当前的 `view_distance`。

减少 LOD 数量会减小最大数据块的大小，但这也意味着需要更多数据块来填充直到 `view_distance` 的网格。请确保在 LOD 数量和 `view_distance` 之间保持一个良好的平衡点，使数据块密度不会太高。

如果你不是在制作无限地形，你可以使用 `bounds` 属性为它设置固定边界，并设置一个非常大的视距，使其保持在视野内。
`bounds` 将四舍五入到八叉树大小：例如，使用 4 个 LOD 和 16 的网格数据块大小，LOD0 数据块将是 16，LOD1 将是 32，LOD2 将是 64……而 LOD3（最大）将是 128。由于当前实现至少在原点周围保留 8 个数据块，此设置的最佳边界将是 256。

![固定边界 LOD 地形截图](images/fixed_bounds_octrees.webp)

按照同样的逻辑，固定边界 512 在 5 个 LOD 时最佳，1024 在 6 个 LOD 时最佳，以此类推。
这是基于 `16` 的网格数据块大小，所以如果你将其设置为 `32`，由于网格大了一倍，你可以减少一个 LOD。

有关编辑器中 LOD 行为的信息，请参见[编辑器中的相机选项](editor.md#camera-options)。


### 体素大小

目前，体素的大小固定为 1 个空间单位。未来版本可能会支持更改它。目前，一种变通方法是缩小节点。但是，请确保它是均匀缩放，并注意不要缩放得太小，否则可能会出问题。

Node3D 的 `scale` 一定不能与*大小*的概念混淆。如果你更改 `scale`，*它也会缩放体素网格*、视距、你可能在生成器中设置的所有尺寸，当然它也会应用于子节点。结果将*看起来一样*，只是更大，没有更多细节。所以如果你想要更大的东西*并且因此获得更多细节*，建议更改这些尺寸，而不是缩放所有内容。
例如，如果你的生成器包含一个球体和 Perlin 噪声，你可以更改球体的半径和噪声的频率/周期，而不是缩放节点。这样做可以保持体素的大小，从而保持精度。

Godot 也允许你进行非均匀缩放，但不推荐这样做（也可能导致碰撞问题）。


### 完全加载模式（即关闭数据流）

LOD 同时适用于网格和体素数据，使内存使用量保持相对恒定。根据你的设置，远处的体素不会加载全分辨率数据。只有全分辨率的体素才能被编辑，这意味着你只能在观察者周围有限的距离内修改地形。

如果这个限制不适合你的游戏，一种变通方法是启用 `full_load_mode`。这将加载 `stream` 中存在的所有已编辑区块（如果有的话），从而使所有数据都可用，并且可以在任何地方无需等待地编辑。未编辑的区块将导致即时查询生成器，而不是缓存。由于不会进行数据流传输，请记住，地形包含的已编辑区块越多，使用的内存就越多。


### LOD 淡化<span id="lod-fading"></span>

LOD 变化可能会在地形中引入一些轻微的“跳变”，这可能有点令人不安。减弱这个问题的一种方法是在网格从两个不同细节级别切换时淡化它们。当“父”网格细分为更高分辨率的“子”网格时，它们可以在短时间内同时渲染，父网格淡出而子网格淡入，反之亦然。
这个技巧要求你在 `VoxelLodTerrain` 上使用 `ShaderMaterial`，因为渲染部分需要在片段着色器中添加额外的代码。

`VoxelLodTerrain` 有一个属性 `lod_fade_duration`，以秒为单位。默认值为 `0`，使其处于非活动状态。将其设置为 `0.25` 这样的小值将启用它。

在你的着色器中，添加以下 uniform：

```glsl
// 这是由体素节点自动识别和赋值的
uniform vec2 u_lod_fade;
```

还要添加这个函数（除非你已经有了）：

```glsl
float get_hash(vec2 c) {
	return fract(sin(dot(c.xy, vec2(12.9898,78.233))) * 43758.5453);
}
```

并在 `fragment()` 的*末尾*添加以下内容：

```glsl
// 逐步丢弃像素。
// 它必须放在最后，以规避 https://github.com/godotengine/godot/issues/34966
float h = get_hash(SCREEN_UV);
if (u_lod_fade.y > 0.5) {
	// 淡入
	if (u_lod_fade.x < h) {
		discard;
	}
} else {
	// 淡出
	if (u_lod_fade.x > h) {
		discard;
	}
}
```

注意：这是一个实现示例。可能有更优化的方法。

这将丢弃像素，使两个网格的像素互补而不重叠。使用 `discard` 是为了让网格可以在同一渲染通道（通常是不透明通道）中保持渲染。

这种技术有一些限制：

- 当淡化的网格彼此相距足够远时，阴影贴图仍会产生自阴影。虽然两个网格都渲染以交叉淡化，但其中一个最终会在另一个上投射阴影。这会产生很多噪点斑块。关闭其中一个的阴影并不能修复另一个，而关闭阴影会让它们跳变。我还没有找到解决方案。参见 [Godot proposal #692 上的评论](https://github.com/godotengine/godot-proposals/issues/692#issuecomment-782331429)


### 细节渲染<span id="detail-rendering"></span>

LOD 会很快地减少远处的几何细节。它可以调高，但生成和渲染大量多边形很快就会变得非常昂贵。另一种方法是改为为中/远距离网格生成法线贴图，在原本平坦的多边形上营造出细节的错觉。

本引擎包含一个针对体素网格改编的此类技术实现（感谢 [Victor Careil](https://twitter.com/phyronnaz/status/1544005424495607809) 的见解！），因此即使有悬垂也能工作。

以下是没有该功能的地形：

![没有细节法线的地形](images/distance_normals_off.webp)

启用该功能后：

![具有细节法线的地形](images/distance_normals_on.webp)

多边形数量相同：

![地形线框](images/distance_normals_wireframe.webp)

使用 `VoxelLodTerrain` 时，可以在检查器中打开此功能。代价是网格生成变慢，以及存储法线贴图纹理需要更多内存。

此功能仅在 `VoxelLodTerrain` 中可用。它在关闭数据流（`full_load_mode_enabled`）时效果最好，因为能够从远处看到所有细节要求不卸载已编辑的数据块。如果数据流开启，它仍会使用生成器，但你将看不到已编辑的区域。

尽管以低于几何体的成本改善细节，但细节渲染比生成常规数据块要昂贵得多。优化它的几种方法有：

- 调整瓦片大小和 LOD 级别。瓦片越大，质量越高，但成本也越高。此外，如果在 LOD 0 和 1 使用，该功能通常起不了太大作用，所以通常一个好的默认设置是从 LOD 2 开始，最小瓦片大小为 4，最大为 16。
- 使用简化版的 `VoxelGenerator`。可以使用 `VoxelLodTerrain.set_normalmap_generator_override` 覆盖用于计算法线贴图的生成器。一个典型用例是生成器可以产生洞穴时。洞穴对表面视觉效果影响不大，因此在某个 LOD 级别以上可以忽略它们。
- 使用 SIMD 噪声。噪声通常是最大的瓶颈。一般来说，像 `FastNoise2` 这样的 SIMD 噪声性能会更好。

#### 着色器 

渲染这些法线需要在地形材质中使用特殊的着色器代码。

```glsl
// 注意：这不是完整的着色器代码，只是此功能所需的代码部分

// TODO Godot 没有正确绑定整数采样器。
// 参见 https://github.com/godotengine/godot/issues/57841
// TODO 使用 float texelFetch 的变通方法也不起作用……
// 参见 https://github.com/godotengine/godot/issues/31732
//uniform usampler2D u_voxel_cell_lookup;
uniform sampler2D u_voxel_cell_lookup : filter_nearest;

uniform sampler2D u_voxel_normalmap_atlas;
uniform int u_voxel_virtual_texture_tile_size;
uniform float u_voxel_cell_size;
uniform int u_voxel_block_size;
// 当回退到父网格的细节纹理时使用。
// 纹理将覆盖一个更大的立方体，因此我们使用此信息
// 只在子区域内查询。
// (x, y, z) 是偏移，(w) 是缩放。
uniform vec4 u_voxel_virtual_texture_offset_scale;

varying vec3 v_vertex_pos_model;


vec2 pad_uv(vec2 uv, float amount) {
	return uv * (1.0 - 2.0 * amount) + vec2(amount);
}

// https://knarkowicz.wordpress.com/2014/04/16/octahedron-normal-vector-encoding/
vec3 octahedron_decode(vec2 f) {
	f = f * 2.0 - 1.0;
	// https://twitter.com/Stubbesaurus/status/937994790553227264
	vec3 n = vec3(f.x, f.y, 1.0 - abs(f.x) - abs(f.y));
	float t = clamp(-n.z, 0.0, 1.0);
	// GLSL 不接受这个的向量版本。有影响吗？
	n.x += n.x >= 0.0 ? -t : t;
	n.y += n.y >= 0.0 ? -t : t;
	return /*f == vec2(0.0) ? vec3(0.0) : */normalize(n);
}

vec3 get_voxel_normal_model() {
	float cell_size = u_voxel_cell_size;
	int block_size = u_voxel_block_size;
	int normalmap_tile_size = u_voxel_virtual_texture_tile_size;

	vec3 cell_posf = v_vertex_pos_model / cell_size;
	cell_posf = cell_posf * u_voxel_virtual_texture_offset_scale.w + u_voxel_virtual_texture_offset_scale.xyz;
	ivec3 cell_pos = ivec3(floor(cell_posf));
	vec3 cell_fract = fract(cell_posf);

	int cell_index = cell_pos.x + cell_pos.y * block_size + cell_pos.z * block_size * block_size;
	int lookup_sqri = int(ceil(sqrt(float(block_size * block_size * block_size))));
	ivec2 lookup_pos = ivec2(cell_index % lookup_sqri, cell_index / lookup_sqri);
	//uvec3 lookup_value = texelFetch(u_voxel_cell_lookup, lookup_pos, 0).rgb;
	//vec3 lookup_valuef = texelFetch(u_voxel_cell_lookup, lookup_pos, 0).rgb;
	vec2 lookup_valuef = texture(u_voxel_cell_lookup, (vec2(lookup_pos) + vec2(0.5)) / float(lookup_sqri)).rg;
	ivec2 lookup_value = ivec2(round(lookup_valuef * 255.0));
	int tile_index = lookup_value.r | ((lookup_value.g & 0x3f) << 8);
	int tile_direction = lookup_value.g >> 6;

	vec2 tile_texcoord = vec2(0.0);
	// TODO 可以用加权加法实现非分支
	switch(tile_direction) {
		case 0:
			tile_texcoord = cell_fract.zy;
			break;
		case 1:
			tile_texcoord = cell_fract.xz;
			break;
		case 2:
			tile_texcoord = cell_fract.xy;
			break;
	}
	float padding = 0.5 / float(normalmap_tile_size);
	tile_texcoord = pad_uv(tile_texcoord, padding);

	ivec2 atlas_size = textureSize(u_voxel_normalmap_atlas, 0);
	int tiles_per_row = atlas_size.x / normalmap_tile_size;
	ivec2 tile_pos_pixels = ivec2(tile_index % tiles_per_row, tile_index / tiles_per_row) * normalmap_tile_size;
	vec2 atlas_texcoord = (vec2(tile_pos_pixels) + float(normalmap_tile_size) * tile_texcoord) / vec2(atlas_size);
	vec3 encoded_normal = texture(u_voxel_normalmap_atlas, atlas_texcoord).rgb;

	// 你可以根据是否使用八面体压缩在这两个代码片段之间切换
	// 1) XYZ
	vec3 tile_normal_model = 2.0 * encoded_normal - vec3(1.0);
	// 2) Octahedral
	// vec3 tile_normal_model = octahedron_decode(encoded_normal.rg);

	return tile_normal_model;
}

vec3 get_voxel_normal_view(vec3 geometry_normal_view, mat4 model_to_view) {
	if (u_voxel_cell_size == 0.0) {
		// 此网格中不提供细节纹理
		return geometry_normal_view;
	}

	vec3 tile_normal_model = get_voxel_normal_model();
	vec3 tile_normal_view = (model_to_view * vec4(tile_normal_model, 0.0)).xyz;
	// 在某些边缘情况下，法线可能无效（长度接近零），导致黑色伪影。
	// 通过回退到几何法线来解决此问题。
	vec3 normal = mix(geometry_normal_view, tile_normal_view, dot(tile_normal_view, tile_normal_view));
	return normal;
}

void vertex() {
	// [...]

	// 注意，如果你使用 Transvoxel，这可能放在对 `VERTEX` 的修改之后
	v_vertex_pos_model = VERTEX;

	// [...]
}

void fragment() {
	// [...]

	NORMAL = get_voxel_normal_view(NORMAL, VIEW_MATRIX * MODEL_MATRIX);

	// [...]
}
```

#### 技术细节

法线贴图通常需要纹理坐标（UV）。然而，平滑体素网格在运行时进行 UV 映射并不容易。有一些方法可以在完全任意的网格上生成 UV 贴图，但它们对实时来说太昂贵，或者不适合无缝分块地形。因此，我们可以使用类似“虚拟纹理”的方法。

网格首先被细分为单元格网格（我们可以使用现成的 Transvoxel 单元格）。在每个单元格中，我们选择一个与单元格三角形配合最好的轴对齐投影，使用它们法线的平均值。然后可以通过将其像素投影到三角形上、从体素数据评估法线并将其存储在图集（可以使用 TextureArray，但它的层数更有限）中来生成一个瓦片。着色器随后可以使用查找纹理读取图集以找到瓦片。查找纹理是一种 3D 纹理，它告诉每个“单元格”图集中的瓦片在哪里（但它可以存储为 2D 纹理）。

![体素法线图集的图像](images/virtual_normalmap.webp)

要生成每个瓦片的像素，我们需要从两个来源访问 SDF 数据：

- 程序化生成器
- LOD 0 处已编辑的体素

使用经典方法获取法线：在所需位置，我们取 4 个以小步长偏移的采样点，计算它们的差值，并对结果进行归一化。这被称为“前向差分”（参见 [Inigo Quilez 关于 SDF 法线的文章](https://iquilezles.org/articles/normalsSDF/)）。

由于每个网格都有自己的纹理，另一种有用的技术是[八面体压缩](https://knarkowicz.wordpress.com/2014/04/16/octahedron-normal-vector-encoding/)。这些法线是世界空间的，天真地将它们编码在纹理中每像素需要 3 个字节（用于 X、Y、Z）。通过八面体压缩，我们以一点质量为代价换取每像素 2 个字节的更小体积。


#### 在 GPU 上渲染

此功能非常昂贵，因此如果显卡支持 Vulkan，可以在其上运行。这可以通过在地形检查器中勾选 `run on GPU` 来启用。

限制：

- GPU 法线贴图目前不支持已编辑的体素。已编辑区域将回退使用 CPU。
- 你使用的生成器必须支持使用 GLSL 的着色器变体。`VoxelGeneratorGraph` 是目前唯一支持它的。它的一些节点不支持它。
