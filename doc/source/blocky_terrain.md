Blocky 地形
=====================

本页更详细地介绍方块地形，即类 Minecraft 或由立方体构成的地形。


`VoxelMesherBlocky` with models
--------------------------------

该网格生成器将模型 ID 对应的小网格组合成区块。它会剔除相互遮挡的面，但不做贪心网格化。这与 Minecraft 中使用的技术类似。

该网格生成器使用的体素数据可以存储在以下通道中：`VoxelBuffer.CHANNEL_TYPE`

### 创建体素模型

网格生成器有一个 `library` 属性，类型为 `VoxelBlockyLibraryBase`。这是一个资源，包含构建体素网格时你希望使用的所有模型列表：草、泥土、木头、树叶、水、灌木、楼梯、门部件等。你可以在原地新建库，也可以保存到文件中以便在多处复用。你还可以通过代码创建它。

可以使用两种库：

- `VoxelBlockyLibrary`：一个简单的模型列表，列表中的索引对应体素数据中使用的 ID。
- `VoxelBlockyTypeLibrary`：一种更高级的库，存储 `VoxelBlockyType` 列表。这是一个类似 Minecraft 工作方式的实验性工作流，将在后面说明。

使用 `VoxelBlockyLibrary` 入门会更容易。

![VoxelBlockyLibrary 中体素列表的截图](images/voxel_library_voxels_list.webp)

每个槽位可以包含一个 `VoxelBlockyModel` 资源。其左侧显示的索引将作为它们在体素数据中使用的 ID。体素 `0` 是特殊情况：按惯例，它可以作为默认的“空气”体素使用。你可以为每个槽位分配一个新的 `VoxelBlockyModel` 资源，并填写它们的属性。

使用默认的 16 位体素数据，你最多可以创建 65,536 个模型。

!!! warning
	为了允许从模块的旧版本转换，`VoxelBlockyModel` 可以被实例化，这意味着检查器和脚本可以创建它。然而，这实际上并非预期用途。你不应创建 `VoxelBlockyModel` 的实例，而应使用它的派生类。

### 立方体

模型有几种类型。其中一种简单的是 `VoxelBlockyModelCube`，它在各面渲染带有指定纹理的立方体。

对于 `VoxelMesherBlocky`，建议使用纹理图集，以便复用材质并减少绘制调用次数。你可以创建一张包含所有体素可能用到的瓦片的纹理。例如，下面这张来自 blocky game 演示项目的图集：

![blocky game 演示中使用的图集](images/blocky_game_atlas.webp)

这个图集是正方形纹理，最多可包含 16x16 个瓦片。这个数字很重要，需要在 `VoxelBlockyModelCube` 的 `atlas_size_in_tiles` 属性上设置，这样才能正确生成纹理坐标。

立方体模型可以在每个面上使用不同的瓦片。你可以通过设置 `Voxel` 中 `Cube tiles` 类别下的属性来决定使用哪个瓦片。这里的坐标单位是瓦片，而不是像素。

![Voxel 立方体瓦片属性截图](images/voxel_cube_tiles_properties.webp)

例如，如果你想使用“木板”瓦片，可以使用 x=3 和 y=1：

![瓦片坐标](images/cube_tile_coordinates.webp)

到目前为止，我们定义了一个在各面带有特定纹理坐标的立方体体素，但我们仍然需要实际指定用于渲染它的纹理和材质。这可以在 `Material overrides` 部分完成，你可以在此处分配一个包含纹理的材质。

确保将它的 `albedo_texture` 设置为你的纹理。你也可以勾选 `Vertex Color/Use as albedo` 属性，因为这能让网格生成器在立方体边缘烘焙环境光遮蔽。

![方块地形材质](images/material_blocky_game.webp)

每个模型可以使用带有不同纹理的不同材质，但请记住，材质复用得越多越好。这可以减少绘制调用次数并加快渲染速度。

注意，材质会在多个层级上被应用，每一层会覆盖另一层：
- 网格上的材质是默认值（如果你显式使用网格）
- `VoxelBlockyModel` 上指定的材质会覆盖网格材质
- `VoxelTerrain` 上指定的材质会覆盖所有库材质

### 网格

使用 `Cube` 几何体创建体素类型是一种可用于简单体素的快捷方式，但最通用的工作流是使用实际网格。如果使用 `VoxelBlockyModelMesh`，你可以分配网格资源来代替。`Cube tiles` 属性不可用，因为你需要在 Blender 之类的 3D 建模软件中为网格指定纹理坐标。

![Blender 中编辑方块 UV 的截图](images/blender_block_uv_mapping.webp)

网格可以是任意想要的形状，但有几条约束需要遵守：

- 网格的原点应位于其下角。
- Blender 的坐标系是 Z 轴向上，而 Godot 是 Y 轴向上。确保你导出的网格在导入 Godot 后不会进入负坐标。
- 顶点最好位于 0..1 范围内，在所有方向上都是如此
- 保持低多边形。网格生成器可以处理大型模型，但如果一个复杂模型大量出现，性能会迅速下降。
- 只有位于 1x1x1 单位立方体侧面上的面才能被网格生成器剔除。确保它们完美对齐。如果没有对齐，由于生成的几何体无法被剔除，可能会导致严重的性能下降。

![Blender 中面与立方体侧面对齐的截图](images/blender_face_cube_side.webp)

导出网格的最佳格式是 OBJ。Godot 默认将此格式作为网格资源导入。其他格式不合适，因为 Godot 会将它们作为场景导入，而 `VoxelBlockyModelMesh` 资源需要的是网格，而不是场景。
你也可以选择从这里导出材质，但建议在 Godot 中完成，因为这允许你复用它们。

!!! note
	每个模型可以使用第二个材质。如果某个网格同时需要透明和不透明部分，这会很有用。这按常规方式工作，即网格带有两个表面。然而，面剔除仍然会使用模型的属性。例如，如果一个模型的侧面不透明而中间透明，它可以被定义为不透明方块，因此当放置在其他不透明方块旁边时，其侧面几何体会被剔除。更多信息参见[透明](#transparency)一节。

!!! warning
	如果你的体素使用基于网格的物理（GodotPhysics、Jolt 等），并且你在导出预设的资源标签页中使用“Export as dedicated server”导出游戏，请确保你在 `VoxelBlockyModelMesh` 中使用的网格**不会**被剥离。运行时需要网格数据来生成碰撞器。如果不这样做，使用网格的模型将不会生成任何基于网格的碰撞。


### 流体

如果你需要制作具有不同水位且风格类似 Minecraft 的流动水，可以使用 [VoxelBlockyModelFluid](api/VoxelBlockyModelFluid.md)。
流体体素的顶角会被抬高，以匹配同类型最高相邻流体的高度。

这是一种程序化模型，因此预计算的数据较少，而是在每次数据块更新时由网格生成器计算。这是因为当流体有多个水位时，预计算模型需要太多变体，得不偿失。

!!! note
	与其他模型一样，流体不会自行模拟行为。这部分需要你自己实现。


#### 库设置

流体模型的工作方式与常规模型略有不同。首先你必须创建一个 [VoxelBlockyFluid](api/VoxelBlockyFluid.md) 资源，并将其保存为 `.tres` 文件。

然后，在你的库中，为流体的每个水位创建一个 [VoxelBlockyModelFluid](api/VoxelBlockyModelFluid.md)，并相应设置它们的 `fluid` 和 `level` 属性。每个水位使用连续的 ID 可能会更方便，但并非必需。
重要的是，在 `fluid` 属性上共享同一个流体（如果流体资源是文件，这会更容易）。这样，网格生成器就会将每个模型识别为同一流体的一部分。

为避免侧面剔除问题，请确保流体模型的 `transparency_index` 高于实心方块。


#### 材质设置

[VoxelBlockyFluid](api/VoxelBlockyFluid.md) 有一个 `material` 属性，该属性将用于该流体包含的每个模型。

为了向正确的方向流动并进行动画，你必须使用带有特殊着色器的 `ShaderMaterial`。网格生成器不会用纹理坐标填充 `UV`，而是存储流动信息：

- `UV.x` = 纹理投影到哪个轴。0=x，1=y，2=z（这也可以从顶点法线推导出来）。
- `UV.y` = 流动状态。指示流动方向，或是否处于静止状态。

流动状态      | 值
----------------|--------
正向 +X     | 0
对角 +X -Z  | 1
正向 -Z     | 2
对角 -X -Z  | 3
正向 -X     | 4
对角 -X +Z  | 5
正向 +Z     | 6
对角 +X +Z  | 7
静止            | 8

这些值的选择使其与角度成正比。

![3x3 水体素从中心向外流动的截图，顶部标注了流动状态编号。图中还标出了 OpenGL 坐标轴，X 向右，Z 向下。流动状态从右侧的 0 开始，逆时针递增，中心为 8。](images/blocky_fluid_flowing_states.webp)

基于此，可以使用局部顶点坐标来采样像素（类似于三平面映射），从而定位和动画化水体纹理。
这主要对流体体素的顶面有意义。侧面总是向下流动，底面则始终处于静止状态。
如果你希望结果看起来不同，例如希望流动纹理与静止纹理不同，则需要你自己调整着色器。

TODO 示例场景


#### 限制

- 最大水位数量有限制（参见 [VoxelBlockyModelFluid](api/VoxelBlockyModelFluid.md) API）
- 最大流体数量有限制（参见 [VoxelBlockyLibraryBase](api/VoxelBlockyLibraryBase.md) API）
- 流体体素顶面的法线保持不变，就像它是平坦的一样。只有角点位置会被移动。因此，顶面的明暗不会随坡度变化。出于性能原因，目前尚未实现这一点，直到找到快速方法为止（注意：Minecraft 似乎也做了同样的选择）。
- 目前不会生成背面。在 Minecraft 中，水实际上同时有背面和正面，在某些边缘情况下它们会被不同地剔除。目前引擎不区分这两者。人们常尝试的解决办法是在水材质中完全禁用背面剔除，但这可能导致其他问题。更多详情参见 issue 621


### 体素模型 ID 的使用

`VoxelBlockyLibrary` 中定义的体素 ID 就像 tilemap 中的瓦片：对于简单游戏，它们可以直接对应一种方块类型。然而，随着时间推移，你可能希望避免直接这样对待它们。相反，你可以定义自己的方块类型列表，每种类型可以对应一个或多个 `VoxelBlockyModel` ID。

来自 Minecraft 的例子：

- 楼梯可以以不同方向放置，有时外观也不同。这些实际上是多个体素 ID。
- 作物可以有多个生长阶段。对于同一种方块，每个阶段都是一个不同的体素 ID。
- 一扇门实际上由 2 个体素组成：顶部和底部。如果考虑门打开和关闭的状态，可能还要更多。
- 一条铁轨可以对应许多不同的体素：直轨、斜坡和转弯。它们都是铁轨，但处于不同的子配置中。

管理“游戏方块”ID 与体素 ID 之间的对应关系由你自己负责。

### 旋转模型

目前不支持自动旋转或翻转体素，因此你必须为某种体素创建可能需要用到的每种旋转版本。不过，你可以在编辑器中通过使用检查器中的旋转按钮来创建这些模型变体：

![检查器中带旋转按钮的模型预览](images/model_preview_with_rotation_buttons.webp)

这些按钮不是用于预览的，它们会实际旋转模型，并且模型在游戏中放置时会以该旋转状态出现。

### 透明<span id="transparency"></span>

你可能希望某些体素类型是透明的。实际上有两种主要方法可以实现这一点：

- 使用 Alpha 裁剪：透明像素被丢弃，从而可以通过不透明通道渲染，这避免了透明表面的一些典型问题。
- Alpha 混合：真正的透明，但当多个透明表面在彼此后面渲染时会有一些限制

两者都需要使用与默认材质不同的材质。注意，如果你使用纹理图集，典型设置只需要 3 个使用同一图集的材质：不透明、Alpha 裁剪和透明。

`VoxelBlockyModel` 资源还有一个 `transparency_index` 属性。该属性允许调整两个体素之间面的遮蔽方式。例如，假设你有两个透明体素：玻璃和树叶。默认情况下，如果将它们并排放置，它们共享的面会被剔除，从而可以从玻璃方块看透树叶：

![未利用透明索引的截图](images/transparency_index_example1.webp)

如果两个面相互接触，且它们的透明索引相同，它们就可能被剔除。但如果透明索引不同，则可能不会被剔除。这样就可以直接看到玻璃后面的树叶，而不是看到内部。

这里，玻璃的 `transparency_index=2`，树叶的 `transparency_index=1`：

![利用透明索引的截图](images/transparency_index_example2.webp)

`VoxelBlockyModel` 还有一个 `culls_neighbors` 属性。默认启用，可避免相邻体素侧面的不必要渲染。然而，对于某些透明体素，始终渲染相邻体素侧面可能更可取。例如，如果所有内部体素侧面都可见，树叶可以看起来更茂密。

这是一组 `culls_neighbors=true`（默认）的树叶：

![culls_neighbors 设置为 true 时树叶的截图](images/culls_neighbors_enabled.webp)

这是同一组 `culls_neighbors=false` 的树叶。体素之间的侧面会被渲染，使这组树叶看起来不那么空洞。

![culls_neighbors 设置为 false 时树叶的截图](images/culls_neighbors_disabled.webp)

### 剔除限制

虽然方块网格生成器会在相邻面相互覆盖时尝试剔除它们，但它实际上无法像 CSG 节点那样完美地裁剪它们。如果你有两个台阶在侧面相互接触，两侧都会被移除。

![多个模型相互接触且共享同形状侧面的截图](images/matching_model_sides.webp)

但如果你有一个立方体接触台阶，台阶会被覆盖并且其侧面会被剔除，但立方体的侧面仍会半可见。在这种情况下，引擎会让整个侧面的三角形保持可见。换句话说，侧面要么被完全剔除，要么不被剔除（Minecraft 中也是如此）。

![两个模型相互接触且共享不同形状侧面的截图](images/mismatching_model_sides.webp)

这主要是因为引擎在将模型网格化成区块时，没有时间对可能存在的每一种侧面组合进行完整的“CSG”。相反，它使用一个预计算的位掩码矩阵，因此判断一个面是否覆盖另一个面非常快。这也是为什么模型的旋转或翻转是按模型预计算的其中一个原因，因为它消除了大量可以归结为简单查找的工作。

如果模型需要透明，这可能会有问题，因为它会暴露底层几何体。目前除了避免在游戏中出现这种情况外，没有其他办法。例如，在 Minecraft 中，没有玻璃台阶或楼梯，水流动时模型侧面总是匹配的。

另一个更次要的细节是匹配面是如何被检测的。在库的烘焙过程中，引擎使用光栅化来检查两个侧面是否具有相同的形状，因此它可以收集整个库的所有形状并快速相互比较，生成网格生成器可以使用的“剔除”矩阵。如果你依赖非常小的三角形或非常精细的面，结果在某些情况下可能无法完美工作。


### 随机刻

`VoxelBlockyModel` 有一个名为 `random_tickable` 的属性。这用于 `VoxelToolTerrain` 的一个非常特定的函数：[run_blocky_random_tick](api/VoxelToolTerrain.md)


`VoxelMesherBlocky` with types
--------------------------------

!!! warning
	此功能是实验性的，可能存在 bug、缺少部分内容，并且可能随时间变化。它提出了一种为游戏组织模型的方式，并自动化了一些事情，但如果你希望按自己的方式处理，你仍然可以使用 `VoxelBlockyLibrary`。另请参阅对应的 issue。

存在一种替代库类型 `VoxelBlockyTypeLibrary`。它不是直接包含模型列表，而是包含 `VoxelBlockyType` 列表。类型更接近游戏中所谓的“方块类型”，并且这个系统的设计初衷是与 Minecraft 中方块的定义方式非常相似（灵感来自 https://docs.minecraftforge.net/en/1.19.2/blocks/states/）。


### 属性

一种类型可以有若干个 `VoxelBlockyAttribute`。每个属性就像一个表示该类型状态的变量。它们可以是原木的方向、按钮是否被按下、方块是门的上部还是下部、与邻居的连接，或作物的生长等级。

类型通常应该只有很少的属性，而且每个属性也只能取很少的值（0 到 255 之间）。这个限制与体素的轻量特性有关，你不能在单个体素中存储太多信息，否则它会失去作为包含数百万体素的地形一部分的能力。
如果你需要方块具有更复杂的状态，例如对象和事物列表，这个系统可能不适用于你的情况，你将不得不回退到使用体素元数据和实际节点（如 Minecraft 中的实体）。


### 变体模型

在你为类型分配属性后，检查器将显示每种状态组合对应的模型列表：

![变体模型列表](images/type_variant_models.webp)

你也可以使用检查器顶部类型 3D 预览旁边的侧面板来预览组合。

如果你有很多属性或很多状态，这个列表可能会变得非常大。在 Minecraft 中，红石粉技术上拥有 [1,296 个模型变体](https://minecraftitemids.com/item/redstone-dust)。因此它不是用变体列表制作的，而是用模型的条件组合。未来可能会实现类似的功能。


### 旋转

属性一个非常常见的用途是旋转。由于太常见，如果你使用内置的旋转属性，`VoxelBlockyType` 会自动生成旋转模型：

- `VoxelBlockyAttributeAxis`
- `VoxelBlockyAttributeDirection`
- `VoxelBlockyAttributeRotation`

每个旋转属性都带有一个默认旋转，你可以把模型设计成仿佛具有该旋转一样，这样其他所有旋转都可以被正确生成。

由于这些变体是自动生成的，它们不会显示在检查器中，但它们与其他变体一样在内部存储。

!!! note
	很容易想对所有内容都使用 `VoxelBlockyAttributeRotation`，但它拥有最多的变体数量。引擎必须为每种组合生成预旋转模型，因此会占用内存成本。同样，如果你不需要某些旋转，请考虑更改这些属性的属性（你可以排除垂直旋转）。


### 模型名称与数字 ID

类型和属性的名称很重要。它们用于唯一标识模型，格式如下：

```
<type_name>[attribute1=value,attribute2=value,...]
```

例如，某个特定的体素可以标识为 `button[direction=up,pressed=yes]`。如果你重命名、移除或添加属性导致此标识符改变，它实际上会成为一个不同的体素，并可能被赋予不同的 ID。这意味着如果你有一个使用旧名称保存的世界，在你更改类型后，这些名称将不会出现。

一种类型可以对应一个或多个具有不同数字 ID 的模型。与 `VoxelBlockyLibrary` 不同，你不能选择这些 ID。它们会根据你为每种类型提供的所有属性的所有组合自动生成。类型拥有的属性和状态越多，为它保留的模型 ID 就越多。

一旦某个特定模型被烘焙，它的名称和属性状态将与一个特定的数字 ID 关联。

数字 ID 不手动分配的一个原因是，当你有很多类型时，不出错地手动分配很繁琐，另一个原因是模组。在 Minecraft 中，由于资源包和可能添加不同模型的模组，每个世界的每个体素可能有不同的数字 ID。添加或移除模组不应导致 ID 相互冲突。
归根结底，唯一标识模型并不是靠数字 ID，而是使用类型和属性名称。类型名称甚至可以使用命名空间语法，例如 `minecraft:flower` 和 `mymod:thingy`。

数字 ID 仅在特定世界内唯一。它们用于存储体素数据并通过网络发送，这样更高效，但不能在不同世界之间移植。

数字 ID 和名称通过我们可以称之为“ID 映射”的东西进行映射。你可以在检查器中点击 `VoxelBlockyTypeLibrary` 最底部的 `Inspect model IDs` 来查看生成的 ID 列表。


### 在脚本中的使用

在处理体素数据时，你仍然需要使用 `VoxelTool` 获取和设置模型 ID，因为体素实际存储的就是这些。如果给定类型的体素需要改变状态，这意味着它的值会变成另一个模型 ID，就像你在基于经典 `VoxelBlockyLibrary` 的游戏中所做的那样。

`VoxelBlockyTypeLibrary` 有函数可以从类型名称及其每个属性的值获取模型 ID，反之亦然。如果你需要大量次数的特定 ID，考虑将它们缓存在局部变量中以提升性能。

类型名称和属性名称使用 `StringName` 而不是 `String`，与其他名称比较时效率稍高。因此你需要使用带有 `&` 的特定语法：

```gdscript
var model_id := library.get_model_index_with_attributes(&"button",
	# 注意，字典键不使用 `&`，因为 Godot 会将 StringName 键转换为 String。
	{
		"direction": VoxelBlockyAttributeDirection.DIR_POSITIVE_X,
		"active": 1,
		"powered": 0
	})
```

如果类型只有一个属性，你可以使用更快的快捷方式：

```gdscript
var model_id := library.get_model_index_single_attribute(&"log", VoxelBlockyAttributeAxis.AXIS_Z)
```

如果类型没有属性，或者你只想要它的默认状态：

```gdscript
var model_id := library.get_model_index_default(&"leaves")
```


`VoxelMesherCubes`
------------------

该网格生成器专门生成带有特定颜色的立方体。

TODO


快速碰撞的替代方案
------------------------------

### 移动与滑动

在 Godot 中，基于网格的碰撞相当精确且功能丰富，但它有一些缺点：

- 每次修改地形时都必须构建 Trimesh 碰撞形状，这非常慢。
- 物理引擎必须处理玩家附近的任意三角形，这无法利用特定情况，例如所有东西都是立方体
- 有时你可能还想要一个更简单、更面向游戏的碰撞系统

`VoxelBoxMover` 类提供了类似 Minecraft 的碰撞系统，可以像 `move_and_slide()` 那样使用。它更有限制性，但速度极快，并且不受隧穿效应影响。

下面的代码展示了如何使用它，完整代码请参见 blocky demo。

```gdscript
var box_mover = VoxelBoxMover.new()
var character_box  = AABB(Vector3(-0.4, -0.9, -0.4), Vector3(0.8, 1.8, 0.8))
var terrain = get_node("VoxelTerrain")

func _physics_process(delta):
	# ... 设置速度的输入命令写在这里 ...

    # 应用地形碰撞
	var motion : Vector3 = velocity * delta
	motion = box_mover.get_motion(get_translation(), motion, character_box, terrain)
	global_translate(motion)
	velocity = motion / delta
```

这种技术主要在使用 `VoxelMesherBlocky` 时有效，因为它会从与其一起使用的 `VoxelBlockyLibrary` 获取哪些方块可碰撞的信息。不过它在其他网格生成器中可能有一些有限的支持。

如果你使用 `VoxelMesherBlocky`，它会使用 `VoxelBlockyModel` 资源中指定的 AABB 列表。如果列表为空，体素将没有碰撞。你还可以通过设置 `VoxelBoxMover` 的 `collision mask` 属性来过滤掉一些碰撞。这会与 `VoxelBlockyModel` 资源上的 `collision mask` 属性进行匹配。


### 射线投射

还存在一个替代的射线投射函数，它返回体素特定的结果。如果你也关闭了经典碰撞，它可能会很有用。这可以通过 `VoxelTool` 类访问。可以使用 `get_voxel_tool()` 获取绑定到地形的实例。

```gdscript
var terrain : VoxelTerrain = get_node("VoxelTerrain")
var vt : VoxelTool = terrain.get_voxel_tool()
var hit = vt.raycast(origin, direction, 10)

if hit != null:
    # 返回的位置是体素坐标，
    # 可以用 `VoxelTool` 的其他函数访问该体素的值
    print("Hit voxel ", hit.position)
```

如果你使用 `VoxelMesherBlocky`，可以通过指定 `collision mask` 参数来过滤掉一些体素类型。这会与 `VoxelBlockyModel` 资源上的 `collision mask` 属性进行匹配。


自定义网格生成
----------------

使用方块体素的游戏通常有一种自定义的方式来解释体素数据并将其转换为几何体，原因有很多：

- 在体素数据中不仅使用 ID 来影响结果
- 在网格生成时处理旋转和变换，而不是预计算它们
- 将形状、纹理、颜色或其他属性与体素数据分离，而不是将一个 ID 映射到一个模型
- 根据与邻居的特殊规则，生成介于立方体素和平滑体素之间的程序化形状
- 打包顶点数据以用于自定义着色器
- 等等……

主要有两种实现方式：

- 编写你自己的自定义网格生成器，精确实现游戏所需的功能。但这要求你用 C++ 实现该逻辑，因为网格生成是资源密集型过程。这将是理想的，因为它允许你根据游戏逻辑和你希望解释体素数据的方式做出所有假设。这也是为什么这么多体素游戏使用自己的代码而不是库来替它们完成的一大原因。但显然这意味着引擎无法为你提供这样的网格生成器，否则会带来巨大的维护负担，而且为每个游戏提供一个网格生成器会使引擎变得臃肿。
- 编写一个依赖烘焙资源和设置、以基本统一方式工作的通用网格生成器。这会把问题变成资产管理问题，比编写网格生成器更容易解决。一个体素 = 一个模型。需要旋转的模型只是变体。不同的状态或纹理也是更多变体。这就是引擎采用的方法。缺点是你必须调整自己的需求以适应这个流水线。生成变体的负担落在你身上，并且在某些极端情况下可能无法奏效。

如果你仍然想使用自定义网格生成器，目前唯一的方式有：

- 创建你自己的 [C++ 模块](https://docs.godotengine.org/en/stable/engine_details/architecture/custom_modules_in_cpp.html)来创建你自己的网格生成器：无需修改体素模块，但你必须编译 Godot、继承基类 `VoxelMesher` 并实现其虚方法（查看引擎自身的网格生成器实现方式作为示例）。
- 修改模块中现有的网格生成器，或从其中一个副本开始。

虽然理论上我们可以为 GDScript 暴露一种继承 `VoxelMesher` 的方式，但脚本太慢，无法承担逐个多边形化每个体素的任务，而且 API 将采取什么形式还不清楚。一个开发分支 mesher_script 试图实现这一点，但尚未准备好使用。
