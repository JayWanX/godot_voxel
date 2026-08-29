实例化
=============

该模块提供了一个使用 [VoxelInstancer](api/VoxelInstancer.md) 节点的实例化系统。这个节点必须作为体素地形的子节点添加。它支持在地形表面之上生成 3D 模型，之后可以在修改时移除它们。

它可以生成两种不同类型的对象：

- **多网格实例**。它们可以数量极多，并且可选地带有碰撞。
- **场景实例**。它们使用普通场景，但是速度要慢得多，所以应调整为较低的数量。

这个系统主要用于自然生成：草、岩石、树木和其他类型的半随机植被。它不适合复杂的、人造的结构，如房屋或村庄，不过如果可用功能适合你的游戏，在某些情况下也可以使用场景实例。


VoxelInstanceLibrary
---------------------

### 库的设置

为了生成物品，`VoxelInstancer` 需要一个 [VoxelInstanceLibrary](api/VoxelInstanceLibrary.md) 资源。该资源包含所有可以被生成的物品的列表，以及它们将如何被放置。

选择一个 `VoxelInstancer`。在检查器中，将库分配给 `library` 属性，或者新建一个内嵌的库。然后点击库资源。检查器顶部会出现按钮：

![VoxelInstanceLibrary 菜单截图](images/instance_library_menu.webp)

你可以通过点击 "+" 图标向库中添加物品，并选择 `Add Multimesh item`。

这样创建的物品带有默认设置，所以你应该能在体素表面之上看到一些东西出现。

!!! note
    如果你在制作一个星球，你可能想把 `VoxelInstancer` 的 `up_mode` 设置为 `Sphere` 模式。这会告诉实例化器向上的方向在哪里，并根据地形的局部原点对齐物品。


### 数据块 LOD

物品生成的范围基于体素地形本身的 LOD 系统，要么是高分辨率的数据块，要么是低分辨率的数据块。这在 [VoxelInstanceLibraryItem](api/VoxelInstanceLibraryItem.md) 的 `lod_index` 属性中配置。例如，选择 `0` 会使物品在最近距离处的 LOD0 数据块上生成，并在远处快速淡出。更高的索引会在覆盖更大范围的更大的数据块上生成。

![展示 lod_index 对实例范围影响的截图](images/instances_lod_index.webp)

通常景观可能由多个层组成，这样你越靠近，就会有越多细节出现。较大的物品使用较高的 lod 索引以便从远处看到，而较小的物品可能使用较低的索引。

![使用实例层的景观截图](images/landscape_with_instances.webp)

!!! note
    在制作草或其他物品时，使用自定义着色器根据与相机的距离淡出网格可能是个好主意，这样它们不会突然消失。使用颜色相近的地面纹理也有助于使其融合。

### 放置精度

在选择合适的 `lod_index` 时需要权衡：目前，较大的索引*非常不精确*，因为它们在较低分辨率的网格之上工作。这允许在更远的距离更快地生成实例，而无需计算全精度的 LOD。

缺点是，随着越来越近、地形计算出更高分辨率的 LOD，可能会看到这样的实例悬浮在地面之上，或沉入其中。这大多发生在有陡峭变化的区域，如山脊、裂缝或洞穴：

![未对齐实例的截图](images/misaligned_instances.webp)

#### 沿法线偏移

调整放置的第一个选项是调整与物品关联的 `generator` 中的 `offset_along_normal` 参数。这取决于资产，因此将它们设计成底部可以有一部分沉入地面，可以提供一定的误差余量。

#### 从生成器 SDF 吸附

另一个选项是启用 `snap_to_generator_sdf_enabled`，代价是实例生成速度变慢。这最好在 `lod_index` > 0 时使用，因为 LOD0 已经具有最大精度。

没有吸附：

![从 LOD2 生成并在 LOD0 下查看的实例截图，没有吸附，许多实例没有正确落在地面上](images/instance_gen_sdf_affining_off.webp)

有吸附：

![从 LOD2 生成并在 LOD0 下查看的实例截图，开启吸附后，定位更好](images/instance_gen_sdf_affining_on.webp)

该选项在浮点位置查询 `VoxelGenerator` 以近似表面在哪里，假设基于网格的位置是一个良好的起点。
生成器只能返回 SDF 值，它粗略地告诉每个 3D 点离表面有多"近"。只要有至少 2 个或更多的邻近采样，我们就可以插值以将位置吸附到更靠近该表面。所以这不是精确的吸附，但它可以显著改善定位。

限制：

- 需要一个支持系列生成的生成器（即查询任意浮点位置来获取体素数据，而不是体素数据块）。在撰写本文时，只有 `VoxelGeneratorGraph` 支持这一点。
- 如果地形被修改则不起作用：如果玩家回到被编辑的区域，实例不得不在其上重新生成，来自生成器的估算将不会考虑被编辑的区域。要解决这个问题，可以将实例设置为 `persistent`，这样编辑地形会将其标记为已修改并保存其位置，而不是重新生成。
- 当远处实例的低分辨率网格处于活动状态时，倾向于"掩埋"或"悬浮"它们，因为它们会被移近高分辨率的那一个。不过这应该不会像之前那样在近距离发生时那么明显。


#### 研究

如果当前的方法不够用，未来可能会研究更多选项：

- 随着更高分辨率的网格可用，逐步吸附实例。需要快速的网格射线投射，如果可能的话从线程中进行（这不幸地排除了 Godot 物理）。


### 网格 LOD

还包含一个次要的 LOD 系统，在一定限度内适用于网格本身。它允许在一组可见网格中，根据距离减少顶点数。它只适用于 `VoxelInstanceLibraryMultiMeshItem`。

!!! note
	这个 LOD 系统的存在是因为 Godot 3 没有。Godot 4 添加了[自动网格 LOD](https://docs.godotengine.org/en/stable/tutorials/3d/mesh_lod.html)，你也许可以不用这个系统而使用它。如果你想控制每个 LOD 使用的网格和材质，你可以使用这个系统。
	虽然[可见范围](https://docs.godotengine.org/en/stable/tutorials/3d/visibility_ranges.html#doc-visibility-ranges)被作为替代方案提出，但在使用 `MultiMesh` 时它需要不必要地复制数据并增加要处理的场景对象数量。

要使用这个，你必须在你的 `VoxelInstanceLibraryMultiMeshItem` 上填写 3 个网格 LOD 属性：

![网格 LOD 属性截图](images/mesh_lod_properties.webp)

如果只设置了 `mesh` 属性，将不会使用任何 LOD。

![带颜色的网格 LOD 截图](images/mesh_lods.webp)

如果你需要更少的 LOD，可以分配两次相同的网格。

!!! note
    伪装网格是简单的四边形，可以在远距离伪装真实模型的存在。例如，这是从远处渲染森林的一种非常快的方式，而在靠近时可以使用详细的树木。

可以在 `Mesh LOD settings` 属性组下自定义网格 LOD 切换的距离。它们被定义为相对于相应地形 LOD（由 `lod_index` 定义）视野距离的比率（通常在 0 和 1 之间）。

也可以隐藏超出最后一个 LOD 最大距离的实例。这主要在地形没有 LOD 时有用，不过请记住，实例仍会被生成，只是不会被渲染。


!!! warning
	目前当 `MultiMesh.mesh` 被更改为不同的 LOD 时存在性能问题。一旦 [这个 PR](https://github.com/godotengine/godot/pull/79833) 被合并到 Godot 中，应该会更容易修复。


### 编辑

目前还不支持在编辑器中手动编辑实例。只能通过使用程序化生成来定义实例在哪里生成。

不过可以在游戏中移除它们，当挖掘它们生成所在的地面时。


### 持久化

某些物品可以是持久的。这个选项可以通过 [VoxelInstanceLibraryItem](api/VoxelInstanceLibraryItem.md) 的 `persistent` 属性启用。如果父级地形有一个支持它的 `VoxelStream`，那么来自被编辑数据块的实例将被保存到数据流中，并且下次玩家靠近该区域时不会重新生成。非持久的实例总是会在每个满足程序化条件的地表上重新生成。

持久物品的 ID 很重要，因为它会被用于保存的数据中。如果你删除了一个物品并尝试从仍包含它们的存档中加载实例，会出现警告。

在撰写本文时，只有 [VoxelStreamSQLite](api/VoxelStreamSQLite.md) 支持保存实例。

保存格式在[这个文档](specs/instances_format_v1.md)中描述。


### 从场景设置多网格物品

可以从现有场景设置多网格物品，作为在检查器中设置的一种替代方式。场景将被转换以适应多网格渲染。你可能需要这样做的一个原因是设置碰撞器，因为虽然它们受支持，但目前无法在检查器中设置它们。在 3D 编辑器中使用节点来设计实例也更方便。

有两种从场景设置的方式：

- 分配 `scene` 属性。这会在运行时转换场景。场景将与物品链接，因此如果场景改变，它会保持更新。
- 使用检查器顶部的 `Setup from scene` 按钮。这不会链接场景，而是分配在编辑器中执行转换的手动属性。如果场景改变，物品不会更新。如果场景嵌入了网格、材质或纹理，它们可能会被复制到物品的资源文件中。

转换过程期望你的场景遵循特定的结构：

```
- PhysicsBody (StaticBody, RigidBody...)
	- MeshInstance_LOD0 <-- "LOD" suffixes are optional but allow to specify the 4 LODs if needed
	- MeshInstance_LOD1
	- MeshInstance_LOD2
	- MeshInstance_LOD3
	- CollisionShape1
	- CollisionShape2
	- ...
```

材质可以通过两种方式设置：

- `MeshInstance` 上的 `material_override`
- 直接使用网格资源上的材质

`MeshInstance` 节点上的表面材质属性不受支持。

### 场景实例

多网格物品快速且高效，但有局限性。

通过添加 `VoxelInstanceLibrarySceneItem` 类型的物品来支持实例化场景。与生成多网格不同，将创建普通场景实例作为 `VoxelInstancer` 的子节点。优点是能够为它们添加更多样的行为，例如脚本、声音、动画，甚至更多的生成逻辑或交互。唯一的约束是，场景的根必须是 `Node3D` 或从它派生。

与多网格实例相比，这种自由要付出高昂的代价。添加许多实例会很快变慢，所以当你从编辑器创建这些物品时，默认密度较低。强烈建议不要使用太复杂的场景，因为根据设置的不同，如果你的电脑无法处理太多实例，可能会导致 Godot 冻结或崩溃。

!!! warning
    如果你向库中添加一个场景，然后尝试从同一个场景加载该库，Godot 会崩溃。这是一个循环引用，目前在所有情况下都很难检测到。


程序化生成
-----------------------

### 内置生成器

![使用噪声的实例层截图](images/instances_procgen.webp)

物品添加时带有默认的内置生成器，所以它们已经会基于程序化规则生成，而不是手动绘制。你可以通过检查 [VoxelInstanceLibraryItem](api/VoxelInstanceLibraryItem.md) 的 `generator` 属性来调整生成器。

位于游戏中已被编辑的数据块内的持久实例将不再重新生成。

### 自定义实例生成器

该功能很新，API 可能仍会变化，所以目前还不能通过脚本使用。


流式事件（高级）
----------------------------

`VoxelInstancer` 通过向父级的区块事件注册自身来知道何时生成东西。目前这在 `VoxelLodTerrain` 的脚本 API 中不可用，但未来可能会添加。
