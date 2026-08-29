程序化生成
==========================

本节描述地形程序化生成所涉及的技术。其中一些是实验性的，可能会被调整。文章会使用图形或脚本，并且可能比文档的其他部分更高级，因此你应该先熟悉 API。


使用图形生成器生成洞穴
---------------------------------

可以通过从基础 SDF 地形中减去噪声“蠕虫”来生成洞穴。为简化方法，让我们先看看带几个倍频程的 2D 噪声是什么样的：

![噪声](images/noise.webp)

如果我们将该噪声乘以自身（即平方），会得到这样的结果：

![平方后的噪声](images/squared_noise.webp)

如果我们对其进行钳制以高亮低于接近零的阈值的部分，就能注意到类似路径的图案：

![平方噪声的路径高亮](images/squared_noise_with_highlight.webp)

在 2D（或使用归一化坐标的 3D）中，这是生成河流或峡谷的关键。但洞穴的问题在于要获得 3D 的圆形“蠕虫”，而不仅仅是 2D 形状。所以我们可以稍微取巧，仍然使用 2D 噪声，但改为沿着 Y 轴调节阈值。为此我们需要一条抛物线形曲线，可以通过 `y^2 - 1` 这样的二次多项式获得：

![洞穴阈值调节](images/cave_threshold_modulation.webp)

回到体素图形，我们可以直接将洞穴生成节点连接到输出，只预览它们的样子，而不包括其余地形：

![洞穴体素图形](images/caves_flat.webp)

调整噪声和其他值之后，我们得到了那些著名的蠕虫，但有两个问题：

- 洞穴仍然是扁平的，它们不会上下起伏
- 它们无限延伸，没有死胡同

我们可以通过向 Y 坐标添加额外一层 2D 噪声来解决第一个问题，使其能垂直扰动洞穴。复用地面噪声并附加一个乘数有时会很有效，这样我们就能避免额外计算噪声。

![洞穴扰动](images/caves_perturb.webp)

第二个问题也可以通过再添加一层低频噪声来解决，将其加到洞穴阈值上，使洞穴在某些区域收缩为死胡同。同样，添加乘数可能会改变这种过渡发生的陡峭程度。

![洞穴体素图形：扰动并调制](images/caves_perturb_modulated.webp)

最后，我们可以通过减去洞穴来将它们与地形混合。这可以使用 `SdfSmoothSubtract` 节点完成，本质上执行 `terrain - caves`。

![洞穴体素图形：减去地形](images/caves_composed.webp)

可能还有多种变体可以获得不同结果。


使用体素结构处理数据块边界
---------------------------------------------------

在 Minecraft 风格的地形中，一旦基础地形生成，一个非常常见的问题就是如何在其中种植树木，因为这样的结构本身也是体素。
在这种特定情况下，使用生成器处理时有一些注意事项，主要围绕这样一个事实：它们逐个处理区块，而不是一次性处理整个世界。

有几种方法可以处理这个问题：

- 利用程序化确定性来“猜测”树木会在相邻区块的什么位置生长，而无需生成完整的相邻区块
- 将生成拆分为多个遍次，并允许访问已经历过之前遍次的相邻区块

下文描述第一种方法，它完全不涉及访问相邻区块，并允许在基础高度既确定又易于计算的地形（例如 2D 噪声高度图）中生成树木。

如果你想使用第二种方法，可以查看[多遍生成器](generators.md#multi-pass-generation-with-voxelgeneratormultipasscb)。

### 确定性方法<span id="deterministic-approach"></span>

#### 找出树木应在 (X, Z) 的哪个位置生长

首先要弄清楚在当前数据块中树木应该在哪些位置生长。为简单起见，我们将其视为 2D 问题，即树木可以在特定的 (X, Z) 位置生长（因为 Y 朝上）。为此，我们需要找到刚好在地面之上的体素位置，并以确定性的方式完成，以便相同的种子产生相同的结果。

但如何使其具有确定性呢？我们可以使用 `RandomNumberGenerator` 实例的种子。但如果给它世界的种子，每个数据块中的树木都会位于相同的位置。我们真正需要的是每个数据块唯一的种子。我们可以通过使用数据块 2D 坐标的哈希来实现：

```gdscript
var block_position := Vector3i(
    origin_in_voxels.x >> 4,
    origin_in_voxels.y >> 4,
    origin_in_voxels.z >> 4) # 向下取整除以 16

var rng := RandomNumberGenerator.new()
rng.seed = global_seed + hash(Vector2i(block_position.x, block_position.z))
```

现在我们可以生成数据块中有多少棵树以及它们的位置：

```gdscript
var block_size := out_buffer.get_size()
var tree_count := rng.randi_range(0, 2)
var tree_positions := []
tree_positions.resize(tree_count)
for i in tree_count:
    var tree_pos := Vector3i(
        rng.randi_range(0, block_size.x), 0, # 我们稍后再处理 Y
        rng.randi_range(0, block_size.z))
    # 注意，这些位置是相对于数据块的局部坐标
    tree_positions[i] = tree_pos
```

#### 找出海拔（Y）

但我们仍然需要计算树木将生长的高度（Y 坐标）。一个问题是引擎生成的是立方体数据块，因此在生成给定的 16x16x16 体素时，你无法访问其下方的内容，只能知道数据块区域内的情况。

然而，如果基础地形使用 2D 高度图噪声生成，那么我们可以在任何需要的地方重新计算高度函数，从而算出任意 (x, z) 坐标处的地形高度。
假设我们已有这样的函数 `func get_height(x: float, y: float) -> float`，我们可以像这样补全 Y 坐标：

```gdscript
for i in len(tree_positions):
    var tree_pos_local : Vector3i = tree_positions[i]
    # 这里使用世界坐标
    var tree_pos_global := tree_pos_local + origin_in_voxels
    tree_pos_global.y := get_height(tree_pos_global.x, tree_pos_global.z)
    # 并转换回局部坐标
    tree_pos_local = tree_pos_global - origin_in_voxels
    # 并存储回数组
    tree_positions[i] = tree_pos_local
```

#### 放置树木

现在我们应该能够放置树木了，但如果我们找到的位置在数据块之外怎么办？我们无法在这些位置设置体素。

我们可以先确定树会有多大。一旦知道它的包围盒，我们就可以放置体素，但只放置那些与我们的数据块相交的体素。

要确定树有多大，听起来我们必须先生成树，然后确定它的体素包围盒。我们可以在一个足够大的独立空白缓冲中完成，或者使用以 `Vector3i` 为键、`int` 为值的 `Dictionary`。但最终，最好将结果存储在大小合适的优化后的 `VoxelBuffer` 中。

我们不会在这里描述如何生成树本身，这并非本文的重点，而且可能因众多因素而不同。但它可以只是一根竖直的树干体素柱，顶部带一个球形树叶。
可以通过预先（或手工）生成一批树并存储到列表中，来优化这一步，这样所有树的边界都是已知的，无需花时间详细生成它们。

我们可以将树的数据封装到一个类中：

```gdscript
class TreeInfo:
    # 树相对于当前数据块的位置
    var instance_position := Vector3i()
    # 只存储树本身的缓冲，如同模型，以便之后粘贴到世界中
    var voxels : VoxelBuffer
    # 树干底部在包含树模型的 VoxelBuffer 中的位置
    var trunk_base_position := Vector3i()
```

因此我们可以拥有一份树的列表，而不只是它们的位置：

```gdscript
var trees : Array[TreeInfo] = []
for tree_pos in tree_positions:
    var tree : TreeInfo = generate_tree(rng)
    tree.position = tree_pos
    trees.append(tree)
```

我们可以将这段逻辑封装进函数 `func generate_trees_for_block(block_position: Vector3i) -> Array[TreeInfo]`，因为它以后可能会用到。

一旦我们知道每棵树的边界，就可以检查它们是否与当前数据块相交。如果相交，我们可以使用 `paste_masked` 方法只种植树，而不会用树的 `VoxelBuffer` 中的空体素替换实心体素：

```gdscript
# 当前数据块的 AABB，使用局部坐标
var block_aabb := AABB(Vector3(), block_size.get_size() + Vector3i(1, 1, 1))

var voxel_tool := out_buffer.get_voxel_tool()

# 粘贴相交的树
for tree in trees:
    var lower_corner_pos := tree.instance_position - tree.trunk_base_position
    var tree_aabb := AABB(lower_corner_pos, tree.voxels.get_size() + Vector3(1,1,1))
    
    if tree_aabb.intersects(block_aabb):
        voxel_tool.paste_masked(lower_corner_pos, tree.voxels, 
            # 我们要粘贴哪个通道
            1 << VoxelBuffer.CHANNEL_TYPE,
            # 掩码为 0，因为 0 被视为空气
            VoxelBuffer.CHANNEL_TYPE, 0)
```

#### 修复重叠

现在树木应该会出现在世界中，但当它们与数据块边界重叠时，会被截断。原因是每个数据块都不知道其相邻数据块，它们只在 X 和 Z 轴上生成源于自身内部的树，并且只影响自身的体素，因为它们无法修改相邻数据块。

![单个数据块生成被截断树木的示意图](images/tree_generation_cutoff_schema.webp)

我们可以决定钳制它们的位置，使它们永远不会重叠，但鉴于它们在游戏中看起来会非常“整齐划一”，这可能不可接受。

我们可以应用与获取高度时相同的推理来解决这个问题。除了只考虑当前数据块中的树，我们还可以检查会在相邻数据块中生成的树，因为我们可以从给定的数据块位置重新运行函数来确定性地获取它们。然后我们要做的只是保留那些与我们的数据块相交的树。这样每个数据块生成时都会在正确的位置包含相邻的树。

![生成相邻树木以考虑与当前数据块重叠的示意图](images/tree_generation_neighbor_fix_schema.webp)

请注意，这意味着每个数据块都会重新计算自己的树和相邻树的位置，因此世界生成过程中，给定数据块中的树会被计算不止一次。这也意味着 `generate_tree` 会被调用不止一次。但如果我们提前（游戏开始前）缓存生成的树模型，这个过程会便宜得多。

```gdscript
var trees : Array[TreeInfo] = []

# 获取源自当前数据块及其相邻数据块的树
for nz in range(-1, 2):
    for nx in range(-1, 2):
        var trees_in_block := generate_trees_for_block(block_position + Vector3i(nx, 0, nz))
        trees.append_array(trees_in_block)

# 粘贴相交的树
for tree in trees:
    # 之前粘贴树的代码
    # ...
```

这种方法已在[这个演示](https://github.com/Voxel/voxelgame/blob/2fa552abfdf52c688bbec27edd676018a31373e0/project/blocky_game/generator/generator.gd#L144)中实现，尽管代码略有不同。

这种方法也用于 Voronoi 噪声（在 FastNoiseLite 中也称为细胞噪声），以生成无缝的单元格。

#### 限制

当然，这种方法也有其局限性：如果我们的地形不只是高度图，还包含浮空岛屿、复杂的雕刻或 3D 噪声结构，那么寻找高度的过程会变得更加复杂。在最坏的情况下，仅为了找到最高的体素，就必须生成相邻的体素列或整个数据块，这会使其变得太慢。

为了应对这一点，你可以查看[多遍生成器](generators.md#multi-pass-generation-with-voxelgeneratormultipasscb)。
