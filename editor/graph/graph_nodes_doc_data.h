// <GENERATED>
// clang-format off
namespace GraphNodesDocData {

struct Node {
    const char *name;
    const char *category;
    const char *description;
};

static const unsigned int COUNT = 59;
static const Node g_data[COUNT] = {
    {"Abs", "数学", "如果 [code]x[/code] 为负，则返回 [code]x[/code] 的正数形式。否则返回 [code]x[/code]。"},
    {"Add", "运算", "返回 [code]a[/code] 和 [code]b[/code] 的和"},
    {"Clamp", "数学", "如果 [code]x[/code] 低于 [code]min[/code]，返回 [code]min[/code]。如果 [code]x[/code] 高于 [code]max[/code]，返回 [code]max[/code]。否则返回 [code]x[/code]。"},
    {"ClampC", "数学", "如果 [code]x[/code] 低于 [code]min[/code]，返回 [code]min[/code]。如果 [code]x[/code] 高于 [code]max[/code]，返回 [code]max[/code]。否则返回 [code]x[/code]。\n该节点是 [code]Clamp[/code] 的替代方案，当 [code]min[/code] 和 [code]max[/code] 为常量时，在内部用作优化。"},
    {"Comment", "杂项", "一个带有描述的矩形区域，用于帮助组织图形。"},
    {"Constant", "杂项", "输出一个常量数字。"},
    {"Curve", "映射", "返回坐标 [code]x[/code] 处自定义 [code]curve[/code] 的值，其中 [code]x[/code] 在其域属性指定的范围内（在 Godot 4.3 及更早版本中，它在 [code][0..1][/code] 范围内）。[code]curve[/code] 使用 [url=Curve]Curve[/url] 资源指定。"},
    {"CustomInput", "输入", "输出与节点同名的自定义输入的值。可用于 [url=VoxelGraphFunction]VoxelGraphFunction[/url]。不会在 [url=VoxelGeneratorGraph]VoxelGeneratorGraph[/url] 中使用。"},
    {"CustomOutput", "输出", "设置与节点同名的自定义输出的值。可用于 [url=VoxelGraphFunction]VoxelGraphFunction[/url]。不会在 [url=VoxelGeneratorGraph]VoxelGeneratorGraph[/url] 中使用。"},
    {"Distance2D", "向量", "返回两个 2D 点 [code](x0, y0)[/code] 和 [code](x1, y1)[/code] 之间的距离。"},
    {"Distance3D", "向量", "返回两个 3D 点 [code](x0, y0, z0)[/code] 和 [code](x1, y1, z1)[/code] 之间的距离。"},
    {"Divide", "运算", "返回 [code]a / b[/code] 的结果。\n注意：除以零会输出 NaN。这不会导致崩溃，但很可能使结果出错。尽可能考虑使用 Multiply。"},
    {"Expression", "数学", "计算数学表达式。变量名可以写成节点的输入。可以使用一些函数，但它们首先必须是受支持的图形节点，因为表达式会在内部转换为节点。\n可用函数：\n[code]\nsin(x)\nfloor(x)\nabs(x)\nsqrt(x)\nfract(x)\nstepify(x, step)\nwrap(x, length)\nmin(a, b)\nmax(a, b)\nclamp(x, min, max)\nlerp(a, b, ratio)\n[/code]"},
    {"FastNoise2D", "噪声", "使用 FastNoiseLite 库计算坐标 [code](x, y)[/code] 处的 2D 噪声并返回。[code]noise[/code] 参数使用 [url=Voxel_FastNoiseLite]Voxel_FastNoiseLite[/url] 资源的实例指定。\n注意：该节点可能比 [code]Noise2D[/code] 稍快。"},
    {"FastNoise2_2D", "噪声", "使用 FastNoise2 库计算坐标 [code](x, y)[/code] 处的 2D SIMD 噪声并返回。`noise` 参数使用 [url=FastNoise2]FastNoise2[/url] 资源的实例指定。这是当前支持的最快噪声。"},
    {"FastNoise2_3D", "噪声", "使用 FastNoise2 库计算坐标 [code](x, y, z)[/code] 处的 3D SIMD 噪声并返回。[code]noise[/code] 参数使用 [url=FastNoise2]FastNoise2[/url] 资源的实例指定。这是当前支持的最快噪声。"},
    {"FastNoise3D", "噪声", "使用 FastNoiseLite 库计算坐标 [code](x, y, z)[/code] 处的 3D 噪声并返回。[code]noise[/code] 参数使用 [url=Voxel_FastNoiseLite]Voxel_FastNoiseLite[/url] 资源的实例指定。\n注意：该节点可能比 [code]Noise3D[/code] 稍快。"},
    {"FastNoiseGradient2D", "噪声", "使用 FastNoiseLite 库的噪声梯度扭曲 2D 坐标 [code](x, y)[/code]。[code]noise[/code] 参数使用 [url=FastNoiseLiteGradient]FastNoiseLiteGradient[/url] 资源的实例指定。"},
    {"FastNoiseGradient3D", "噪声", "使用 FastNoiseLite 库的噪声梯度扭曲 3D 坐标 [code](x, y, z)[/code]。[code]noise[/code] 参数使用 [url=FastNoiseLiteGradient]FastNoiseLiteGradient[/url] 资源的实例指定。"},
    {"Floor", "数学", "返回 [code]floor(x)[/code] 的结果，即小于或等于 [code]x[/code] 的最近整数。"},
    {"Fract", "数学", "返回 [code]x[/code] 的小数部分。无论正负，结果始终为正。"},
    {"Function", "杂项", "运行一个自定义函数，类似于可复用的子图形。该节点的第一个参数（参数 0）是对 [url=VoxelGraphFunction]VoxelGraphFunction[/url] 的引用。其余参数（从 1 开始）是函数暴露的参数。"},
    {"Image", "映射", "返回图像在坐标 [code](x, y)[/code] 处红色通道的值，其中 [code]x[/code] 和 [code]y[/code] 以像素为单位，返回值在 `[0..1]` 范围内（如果图像具有 HDR 格式则可能更大）。如果坐标超出图像范围，将被环绕。不执行任何过滤。图像必须使用未压缩格式。"},
    {"InputSDF", "输入", "输出当前体素处已有的符号距离。这只能在特定情况下使用，例如将图形用作程序化笔刷。"},
    {"InputX", "输入", "输出当前体素的 X 坐标。"},
    {"InputY", "输入", "输出当前体素的 Y 坐标。"},
    {"InputZ", "输入", "输出当前体素的 Z 坐标。"},
    {"Max", "数学", "返回 [code]a[/code] 和 [code]b[/code] 之间的较大值。"},
    {"Min", "数学", "返回 [code]a[/code] 和 [code]b[/code] 之间的较小值。"},
    {"Mix", "数学", "使用参数值 [code]t[/code] 在 [code]a[/code] 和 [code]b[/code] 之间插值。如果 [code]t[/code] 为 [code]0[/code]，将返回 [code]a[/code]。如果 [code]t[/code] 为 [code]1[/code]，将返回 [code]b[/code]。如果 [code]t[/code] 超出 [code][0..1][/code] 范围，返回值将是一种外推。"},
    {"Multiply", "运算", "返回 [code]a * b[/code] 的结果。"},
    {"Noise2D", "噪声", "使用 Godot 提供的 [url=Noise]Noise[/url] 子类之一返回坐标 `(x, y)` 处的 2D 噪声。"},
    {"Noise3D", "噪声", "使用 Godot 提供的 [url=Noise]Noise[/url] 子类之一返回坐标 `(x, y, z)` 处的 3D 噪声。"},
    {"Normalize", "向量", "返回给定 [code](x, y, z)[/code] 3D 向量的归一化坐标，使输出向量的长度为 1。"},
    {"OutputSDF", "输出", "设置当前体素的符号距离场值。"},
    {"OutputSingleTexture", "输出", "设置当前体素的纹理索引。如果你的体素只有一个纹理，这是使用 [code]OutputWeight[/code] 节点的替代方案。它更易于使用，但不允许长渐变。不支持将此节点与 [code]OutputWeight[/code] 组合使用。"},
    {"OutputType", "输出", "设置当前体素的 TYPE 索引。这用于 [url=VoxelMesherBlocky]VoxelMesherBlocky[/url]。如果使用此输出，则无需使用 [code]OutputSDF[/code]。"},
    {"OutputWeight", "输出", "设置当前体素的特定纹理权重的值。纹理通过 [code]layer[/code] 参数以索引形式指定。使用给定图层索引的输出只能有一个。"},
    {"Pow", "数学", "返回幂函数（[code]x ^ power[/code]）的结果。它可能相对较慢。"},
    {"Powi", "数学", "返回幂函数（[code]x ^ power[/code]）的结果，其中指数是常量正整数。可能比 [code]Pow[/code] 更快。"},
    {"Relay", "杂项", "直通节点，可以更好地组织长连接的路径。"},
    {"Remap", "数学", "对于 [code][min0, max0][/code] 范围内的输入值 [code]x[/code]，线性转换到 [code][min1, max1][/code] 范围。例如，如果 [code]x[/code] 是 [code]min0[/code]，将返回 [code]min1[/code]。如果 [code]x[/code] 是 [code]max0[/code]，将返回 [code]max1[/code]。如果 [code]x[/code] 超出 [code][min0, max0][/code] 范围，结果将是一种外推。"},
    {"SdfBox", "SDF", "返回以原点为中心、大小为 [code](size_x, size_y, size_z)[/code] 的轴对齐盒体在坐标 [code](x, y, z)[/code] 处的符号距离场。"},
    {"SdfPlane", "SDF", "返回面向 Y 轴、位于给定 [code]height[/code] 处的平面在坐标 [code]y[/code] 处的符号距离场。"},
    {"SdfPreview", "SDF", "调试节点，不用于最终结果。在编辑器中，根据边界 [code][min_value, max_value][/code] 显示其所连接输出发出的值的一个切片。切片将沿 XY 平面或 XZ 平面，具体取决于当前设置。"},
    {"SdfSmoothSubtract", "SDF", "使用与 [code]SdfSmoothUnion[/code] 节点相同的平滑方式，从 [code]a[/code] 中减去符号距离场 [code]b[/code]。"},
    {"SdfSmoothUnion", "SDF", "返回两个符号距离场值 [code]a[/code] 和 [code]b[/code] 的平滑并集。平滑度通过 [code]smoothness[/code] 参数控制。平滑度越高，会在两个输入形成的形状之间产生更大的“焊接”区域。"},
    {"SdfSphere", "SDF", "返回以原点为中心、给定 [code]radius[/code] 的球体在坐标 [code](x, y, z)[/code] 处的符号距离场。"},
    {"SdfSphereHeightmap", "SDF", "返回球形高度图在坐标 [code](x, y, z)[/code] 处的符号距离场近似值。高度图是使用全景投影的 [code]image[/code]，类似于 Godot 中用于环境天空的图像。球体的半径通过 [code]radius[/code] 指定。高度图的高度可以使用 [code]factor[/code] 参数缩放。图像必须使用未压缩格式。"},
    {"SdfTorus", "SDF", "返回以原点为中心、面向 Y 轴的环面在坐标 [code](x, y, z)[/code] 处的符号距离场。环的半径为 [code]radius1[/code]，其厚度为 [code]radius2[/code]。"},
    {"Select", "数学", "如果 [code]t[/code] 低于 [code]threshold[/code]，返回 [code]a[/code]。否则返回 [code]b[/code]。 "},
    {"Sin", "数学", "返回 [code]sin(x)[/code] 的结果"},
    {"Smoothstep", "数学", "根据 [code]x[/code] 相对于边缘 [code]egde0[/code] 和 [code]edge1[/code] 的位置，在 [code]0[/code] 和 [code]1[/code] 之间平滑插值 [code]x[/code] 的值。如果 [code]x <= edge0[/code]，返回值为 [code]0[/code]；如果 [code]x >= edge1[/code]，返回值为 [code]1[/code]。如果 [code]x[/code] 位于 [code]edge0[/code] 和 [code]edge1[/code] 之间，返回值遵循一条 S 形曲线，将 [code]x[/code] 映射到 [code]0[/code] 和 [code]1[/code] 之间。这条 S 形曲线是三次 Hermite 插值，公式为 [code]f(y) = 3*y^2 - 2*y^3[/code]，其中 [code]y = (x-edge0) / (edge1-edge0)[/code]。"},
    {"Spots2D", "噪声", "为“矿脉”生成优化的细胞噪声：将空间划分为 2D 网格，每个单元格包含一个圆形“斑点”。当位置在斑点内部时返回 1，否则返回 0。[code]jitter[/code] 或多或少会随机化斑点在每个单元格内的位置。限制：高抖动可能使斑点与单元格边界相交。这是有意为之的。如果你需要更通用的细胞噪声，请使用另一个节点。"},
    {"Spots3D", "噪声", "为“矿脉”生成优化的细胞噪声：将空间划分为 3D 网格，每个单元格包含一个圆形“斑点”。当位置在斑点内部时返回 1，否则返回 0。[code]jitter[/code] 或多或少会随机化斑点在每个单元格内的位置。限制：高抖动可能使斑点与单元格边界相交。这是有意为之的。如果你需要更通用的细胞噪声，请使用另一个节点。"},
    {"Sqrt", "数学", "返回 [code]x[/code] 的平方根。\n注意：与经典平方根不同，如果 [code]x[/code] 为负，此函数返回 [code]0[/code] 而不是 [code]NaN[/code]。"},
    {"Stepify", "数学", "将 [code]x[/code] 吸附到给定步长，类似于 GDScript 的函数 [code]stepify[/code]。"},
    {"Subtract", "运算", "返回 [code]a - b[/code] 的结果"},
    {"Wrap", "数学", "将 [code]x[/code] 环绕在 [code]0[/code] 和 [code]length[/code] 之间，类似于 GDScript 的函数 [code]wrapf(x, 0, max)[/code]。\n注意：如果 [code]length[/code] 为 0，该节点将返回 [code]NaN[/code]。即使发生这种情况，也不应该崩溃，但结果会出错。"},
};

} // namespace GraphNodesDocData
// clang-format on
// </GENERATED>
