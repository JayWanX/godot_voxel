脚本编写
=============

本页展示了一些如何使用脚本 API 的示例。


编辑地形
----------------------

### 使用 [VoxelTool](api/VoxelTool.md)

`VoxelTool` 是一个用于访问和修改体素数据的简化 API。可以从任何存储体素的类获取一个，使用 `get_voxel_tool()` 函数。该函数将返回一个与你获取它的体积绑定的 `VoxelTool`。

查看 [VoxelTool](api/VoxelTool.md) 了解可用函数。注意，取决于你从哪个类获取它，`VoxelTool` 的子类可能有更专门的函数。

可以将对 `VoxelTool` 的引用存储在成员变量中，以防你想从同一个体积多次访问体素。这样更高效，因为每次调用 `get_voxel_tool()` 都会创建一个新的实例。

在你开始修改体素之前，确保访问正确的通道。

```gdscript
# 如果你使用 VoxelMesherBlocky
voxel_tool.channel = VoxelBuffer.CHANNEL_TYPE
```

```gdscript
# 如果你使用 VoxelMesherTransvoxel
voxel_tool.channel = VoxelBuffer.CHANNEL_SDF
```

```gdscript
# 如果你使用 VoxelMesherCubes
voxel_tool.channel = VoxelBuffer.CHANNEL_COLOR
```

### 边界限制

当地形在流式加载/卸载区块时，无法编辑超过已加载边界的内容。要么你会得到一个错误，要么什么也不会发生。
你可以通过调用 `VoxelTool.is_area_editable()` 来测试你想要访问或编辑的区域是否可用。


### LOD 限制

与边界限制类似，当你在 `VoxelLodTerrain` 中使用 LOD 时，无法访问或编辑超过第一个 LOD 级别的体素。超过这个级别，体素数据不再以完整分辨率提供。


### 编辑性能

一般来说，逐个编辑体素是最慢的。如果只是获取几个，这是可以的，但如果你打算一次修改更大的区域，你可能更喜欢批量执行的函数，或复制/粘贴缓冲区。

参见 [访问体素和多线程](performance.md)


自定义生成器
------------------

你可以通过扩展 `VoxelGeneratorScript` 在 GDScript、C# 或 C++ 中提供你自己的体素生成器。

!!! note
    也可以不用脚本创建自定义生成器，使用 [VoxelGeneratorGraph](generators.md)

### 示例

#### 使用方块风体素

下面是如何制作一个可用于方块风地形的精简生成器。确保你使用 `VoxelMesherBlocky` 作为网格生成器（mesher）。

创建一个独立的脚本 `my_generator.gd`，内容如下：

```gdscript
@tool
extends VoxelGeneratorScript

const channel : int = VoxelBuffer.CHANNEL_TYPE

func _get_used_channels_mask() -> int:
    return 1 << channel
 
func _generate_block(buffer : VoxelBuffer, origin : Vector3i, lod : int) -> void:
	if lod != 0:
        return
	if origin.y < 0:
        buffer.fill(1, channel)
	if origin.x == origin.z and origin.y < 1:
        buffer.fill(1, channel)
```

!!! note
    生成器脚本可以在编辑器中以工具模式直接运行，**但需要格外小心**。更多信息，参见[工具脚本](editor.md#tool-scripts)。

在你的地形场景中，给一个节点添加另一个脚本，它会在游戏开始时设置你的生成器。代码可能因你的场景结构方式而略有不同。

```gdscript
extends Node # 或你的根节点是什么
const MyGenerator = preload("my_generator.gd")

# 获取地形
@onready var terrain = $VoxelTerrain

func _ready():
	terrain.generator = MyGenerator.new()
```

确保场景中相机下有一个 `VoxelViewer` 节点。你可能还想把它移高、向下看，并添加一个 `DirectionalLight3D` 和 `WorldEnvironment`（否则一切看起来都是灰色的）。

![自定义数据流](images/custom-stream.jpg)

虽然 `VoxelBuffer.fill()` 可能不是你想使用的，但上面是一个快速示例。Generate_block 通常会一次性给你一个 16x16x16 的立方体数据块来填充，所以你也可以使用 `VoxelBuffer.set_voxel()` 来逐个指定每个体素。

#### 使用平滑体素

要像前面的示例那样用平滑体素获得类似的结果更加棘手，所以我们将改用另一个示例。

首先你必须把你的网格生成器（mesher）改成 `VoxelMesherTranvoxel`。接下来，下面是你如何生成高度变化的地面：

```gdscript
@tool
extends VoxelGeneratorScript

# 将通道改为 SDF
const channel : int = VoxelBuffer.CHANNEL_SDF

func _generate_block(out_buffer : VoxelBuffer, origin_in_voxels : Vector3i, lod : int) -> void:
	# 这次我们必须遍历区块中的每一个 3D 体素
	for rz in out_buffer.get_size().z:
		for rx in out_buffer.get_size().x:
			# 以下部分只依赖于 `x` 和 `z`，
			# 所以把它移出最内层循环可以稍微优化一下。

            # 获取体素的世界位置。
			# 为了考虑 LOD，我们将局部坐标乘以 2^lod。
			# 这可以通过二进制左移比 `pow()` 更快地完成。
            # 省略 Y，因为我们会在内层循环中计算它。
			var pos_world := Vector3(origin_in_voxels) + Vector3(rx << lod, 0, rz << lod)

			# 生成无限的"波浪"山丘。
			var height := 10.0 * (sin(pos_world.x * 0.1) + cos(pos_world.z * 0.1))

            # 最内层循环
			for ry in out_buffer.get_size().y:
				pos_world.y = origin_in_voxels.y + (ry << lod)

                # 这是高度场有符号距离的一种廉价近似
				var signed_distance := pos_world.y - height

				# 输出有符号距离时，使用 `set_voxel_f` 而不是 `set_voxel`
				out_buffer.set_voxel_f(signed_distance, rx, ry, rz, channel)
```

对于有符号距离场，负值表示"内部"，而正值表示"外部"。输出*梯度*也很重要，而不是仅仅把体素设置为 1 或 0。这就是为什么我们不能在这里使用 `fill`。实际上你还需要使用噪声和真正的 SDF 函数。参见[有符号距离场](smooth_terrain.md/#signed-distance-fields)。

还可以进一步优化，例如，如果你知道传入的区块足够远，不会与任何出现地表特征的区域相交，你可以提前返回并输出 `fill_f(100.0)`。这相当于说"这里只有空气，而且远离一切"。类似地，你可以做 `fill_f(-100.0)` 来表示"这个区块里只有物质，而且远离任何表面"。


### 线程安全

生成器会从多个线程调用。确保你的代码是线程安全的。

如果你的生成器使用了你希望在它运行时更改的资源或导出参数，你应该确保它们是只读的或按线程复制的，这样如果资源从外部或另一个线程被修改，就不会干扰生成器。

你可以使用 `Mutex` 来强制对可以被修改的变量进行单线程访问：

```gdscript
var _dictionary := {}
var _dictionary_mutex := Mutex.new()

func _generate_block(...):
    # ...

    _dictionary_mutex.lock()

    var x := 0
    if not _dictionary.has(key):
        _dictionary[key] = x
    else:
        x = _dictionary[key]
    
    _dictionary_mutex.unlock()

    # ...
```

然而，互斥锁必须非常小心地使用：如果它们被频繁锁定或保持锁定太长时间，你可能最终会把性能限制在单个线程上（而其他线程在等待锁被释放）。如果你使用多个互斥锁并以不同的顺序锁定它们，也可能导致[死锁](https://en.wikipedia.org/wiki/Deadlock)。
使用读写锁和线程局部变量是取决于情况的好选择，不幸的是 Godot 脚本 API 不提供这些。

小心惰性初始化，如果两个线程同时运行它，可能会导致崩溃。`Curve` 就是这样一个做惰性初始化的资源：如果你调用 `interpolate_baked()` 而它还没有被烘焙，它会在最后一刻被烘焙。这涉及修改内部状态，可能与做同样事情的其他线程重叠。下面是一个绕过这个问题的示例：

```gdscript
const MountainsCurve : Curve = preload("moutains_curve.tres")

# 这在生成器创建时被调用
func _init():
    # 调用 `bake()` 以确保它不会稍后在 `generate_block()` 内部发生。
    MountainsCurve.bake()

# ...
```


自定义数据流<span id="custom-stream"></span>
---------------

制作自定义数据流的工作方式与自定义生成器类似。

你必须扩展类 `VoxelStreamScript` 并重写方法 `_load_block` 和 `_save_block`。
参见

TODO 自定义数据流的脚本示例
