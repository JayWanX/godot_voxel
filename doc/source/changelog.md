更新日志
============

这里是一个随时间累积的功能、变更和修复的高层列表。

目前，该模块没有明确的发布计划，因此本更新日志跟随 Godot 的版本号和二进制发布。这里提到的几乎每个版本都应该有一个关联的 Git 分支（针对本仓库，而非 Godot 的），其中包含该版本时的功能。目前还没有做反向移植（backport）。

我尽量减少破坏性变更，但通常每个版本都会有几个，我会详细列出，所以请注意那个部分。

1.7 - 20/08/2026 - tag `v1.7`
-------------------------------

- 改进
    - 添加了计算着色器缓存（感谢 chalecampb #866）
    - `VoxelNode`：公开 `get_voxel_tool`，而不是在其子类中重复定义。
    - `Voxel_FastNoiseLite`：编辑器：添加了对噪声分析窗口的支持，此前仅 `FastNoise2` 支持（这主要是用于图生成器内部开发的调试工具）。
    - 编辑器：范围分析调试现在也会在连接到 `SdfPreview` 节点的输出上显示实际的最小/最大值。这主要用于排查内部 bug。

- 修复
    - 扩展：修复在检视器中展开插件资源以及其他涉及预览的类似操作时的崩溃（参见 https://github.com/godotengine/godot-cpp/pull/1928）
    - 扩展：修复 .vox 导入器选项（#882）
    - 噪声：编辑器：允许通过手动输入数值来设置大于检视器滑块限制的 `period`。
    - `VoxelBlockSerializer`：修复使用压缩模式时序列化错误。LZ4 模式错误地写入了 LZ4_BE 头，导致无法反序列化（除非手动将头部替换为 LZ4，否则仍无法反序列化），而 ZSTD 模式却生成了 LZ4 数据。
    - `VoxelBlockyModelFluid`：修复另一个流体体素下方的流体体素会产生网格碰撞的问题。它们不应该产生碰撞，因为网格碰撞是用于固体物质的。
    - `VoxelBoxMover`：修复某些情况下台阶攀爬未触发或导致玩家与数据块相交的问题（感谢 NuclearPhoenixx 的帮助 #813）
    - `VoxelGeneratorGraph`：修复使用分形时 FastNoiseLite 细胞噪声范围估计不正确，导致近距离区块消失的问题
    - `VoxelInstanceLibraryMultimeshItem`：修复在 `VoxelLodTerrain` 使用 `Octree` 流式系统时，网格 LOD 距离未随 `lod_index` 正确缩放的问题
    - `VoxelLodTerrain`：添加手动设置每个 LOD 距离的选项（#379）
    - `VoxelStreamSQLite`：修复地形线程任务仍在使用数据流时，在主线程调用其函数导致的挂起和失败问题（#881）
    - `VoxelTool`：修复 `do_path` 有时会产生 `is_valid_block_position` 错误
    - `VoxelToolBuffer`：现在已实现 `paste_masked_writable_list`

- 破坏性变更
    - `VoxelTerrain` 和 `VoxelLodTerrain`：移除 `run_stream_in_editor`。数据流和生成器将始终在编辑器中运行，除非它们使用未处于工具模式下的脚本。


1.6 - 04/02/2026 - tag `v1.6`
-----------------------------------

- 改进
    - `VoxelBlockyModel`：添加 `tags_mask` 属性。最初，它可用于在使用 `VoxelToolTerrain.run_blocky_random_tick` 时过滤选中的体素。
    - `VoxelBoxMover`：
        - 添加对 `VoxelLodTerrain` 的支持。
        - 添加 `intersects` 方法，用于检查 AABB 是否与方块风体素重叠。
    - `VoxelGeneratorGraph`：
        - 编辑器：在右键菜单中添加 `Add Node` 项
        - 添加使用 GPU 时对 `FastNoiseLite` 和 `Voxel_FastNoiseLite` 资源的域扭曲（domain warp）支持（此前需要前置 `FastNoiseLiteGradient` 噪声）
        - 添加按名称获取节点输入和输出索引的方法
        - 添加 `generate_image_from_sdf`
        - 添加 `raycast_sdf_approx`，用于从射线找到表面位置
    - `VoxelLodTerrain`：添加调试标记以绘制体素元数据的位置
    - `VoxelNode`：添加 `convert_to_nodes` 方法，使用原生 Godot 节点创建地形快照。
    - `VoxelStream`：添加使用 ZSTD 而非 LZ4 压缩存档的选项
    - `VoxelToolLodTerrain`：
        - 实现 `get/set_voxel_metadata` 方法。警告：缓存默认关闭，因此在未编辑区域获取元数据会像 `get_voxel` 一样调用生成器。
        - 实现 `do_path`

- 修复
    - `VoxelAStarGrid3D`：修复使用负尺寸调用 `set_region` 后崩溃的问题
    - `VoxelBlockyModelFluid`：修复碰撞盒被库烘焙忽略的问题
    - `VoxelBuffer`：修复在非均匀缓冲区上使用 `get_channel_as_byte_array` 时崩溃的问题
    - `VoxelGeneratorGraph`：
        - 修复分形类型不为 `None` 且使用 GPU 时 `FastNoiseGradient` 不正确的问题
        - 修复 `auto_connect_default_inputs` 未被保存并恢复为默认值的问题
    - `VoxelGraphFunction`：修复 `set_node_default_input_by_name` 会匹配参数，但本应匹配输入的问题
    - `VoxelLodTerrain`：
        - 修复使用 Clipbox 时，一个观察者离开数据块而另一个非可视观察者仍保持其物理加载时可能出现的网格淡入淡出错误。
        - 修复 LOD 更新和非缓存数据块中某些可能忽略 `VoxelFormat` 覆盖的情况
    - `VoxelTool`：
        - `run_blocky_random_tick`：修复均匀数据块未被拾取的问题（PR #794）
        - `run_blocky_random_tick`：现在体素数量不能被批大小整除时，将对余数继续执行
        - 地形：修复 `set_voxel_f` 未正确缩放 SDF 的问题

- 破坏性变更
    - `VoxelStreamRegionFiles`：移除 `lod_count` 属性。无需预先配置即可保存任意 LOD。



1.5 - 16/09/2025 - tag `v1.5`
------------------------------

- 改进
    - `VoxelBuffer`：添加旋转/镜像内容的功能
    - `VoxelEngine`：添加手动更改线程数的函数（感谢 wildlachs）
    - `VoxelGeneratorGraph`：
        - 编辑器：现在可以使用右键菜单删除选中的节点和连接（在 Android 上可能是长按）
        - 实现常量折叠，如果图中包含常量分支，可在 CPU 上略微优化图的运行
    - `VoxelGeneratorHeightmap`：添加 `offset` 属性
    - `VoxelGraphFunction`：编辑器：预览节点现在应该可以正常工作
    - `VoxelInstanceLibraryItem`：公开 `floating_sdf_*` 参数，用于调整在其周围挖掘地面后如何检测浮动实例。
    - `VoxelInstanceLibraryMultiMeshItem`：
        - 添加 `removal_behavior` 属性，以便在实例被移除时触发某些操作
        - 添加 `collision_distance`，仅在与区块低于一定距离时才创建碰撞体
    - `VoxelInstanceGenerator`：
        - 添加基于体素生成器 SDF 吸附实例的选项（仅 `VoxelGeneratorGraph` 可用）。
        - 公开体素纹理过滤的阈值
        - 添加高度、坡度和噪声过滤的衰减设置，使密度可以渐进淡出（issue 784）。
        - 添加噪声阈值，用于扩大或缩小被噪声过滤的区域。
    - `VoxelInstancer`：
        - 添加 `remove_instances_in_sphere`
        - 添加淡入淡出系统，可使用着色器在实例加载和卸载时淡出
        - 略微改进实例在三角形上的随机分布
    - `VoxelMesherBlocky`：添加色调模式，使用 `COLOR` 通道调制体素颜色。
    - `VoxelMesherTransvoxel`：添加 `Single` 纹理模式，每个体素仅用一个字节存储纹理索引。`VoxelGeneratorGraph` 也已更新以包含此模式。
    - `VoxelTool`：
        - 添加 `do_mesh` 以取代 `stamp_sdf`。仅地形支持。
        - `copy` 和 `paste` 函数的 `channels_mask` 参数现在为可选，默认作用于所有通道
    - `VoxelTerrain`：添加调试标记以绘制体素元数据的位置
    - `FastNoise2`：
        - 公开 `CELLULAR_VALUE` 噪声类型
        - 公开用于选择距离/值计算中使用的单元格索引的属性
    - 构建系统：添加在进行自定义构建时关闭某些功能的选项
    - 引入 `VoxelFormat`，允许覆盖默认通道深度（使用新的 `Single` 体素纹理模式时需要）

- 修复
    - `VoxelBlockyType`：修复使用 `set_variant_model` 添加的模型并不总能被 `_variant_models_data` 返回的问题（不过由于其他原因仍可能发生，请参阅 `set_variant_model` 的文档）
    - `VoxelBlockyTypeLibrary`：
        - 修复将 `types` 设置为空数组时崩溃的问题
        - 修复当一个类型具有两个以上属性时 ID 映射加载错误的问题
    - `VoxelBuffer`：修复 `copy_voxel_metadata_in_area` 在某些情况下可能崩溃并报错 `Assertion failed: "is_position_valid(dst_pos)" is false.` 的问题
    - `VoxelInstancer`：
        - 修复至少有一个区块卸载后实例移除随机失败的问题
        - 修复在使用 `VoxelLodTerrain` 时，向下挖掘或向上建造到*已生成网格*但此前没有几何体的区块中会产生实例的问题
        - 修复过渡网格不应被用作生成表面的问题，它们会导致区块边界的密度偏差和位置偏差
        - 修复加载带有 `up_mode = SPHERE` 设置实例器的场景时报错的问题（issue 786；`voxel_instancer.cpp:906 - Condition "_parent == nullptr" is true`）
        - 修复地形已在场景树中后修改 `mesh_block_size` 可能出现的潜在问题，即实例器不会使用正确的数据块大小（导致 `ERROR: Condition "render_to_data_factor <= 0 || render_to_data_factor > 2" is true`）
    - `VoxelGeneratorGraph`：
        - 编辑器：修复关闭图编辑器后有时打印错误的问题
        - 编辑器：修复编辑图后错误刷屏 `Invalid param name` 的问题（在某些未知情况下）
        - 编辑器：修复搜索时节点对话框未自动选中第一个项目的问题
        - 编辑器：没有精确浮点表示的十进制数现在显示为四舍五入后的值，而不是过度加宽节点。精确值改为通过工具提示显示。
        - 编辑器：修复节点名称被错误翻译的问题
        - 修复在 Mixel4 与 OutputSingleTexture 以及 GPU 生成配合使用时纹理绘制错误导致黑色三角形的问题
        - 修复特定设置下等价节点多次连接到等价祖先时崩溃的问题（issue 783；`FATAL: Assertion failed: "p != equivalence" is false`）
        - 修复创建多个引用同一资源的节点时报错的问题（`ERROR: Signal 'changed' is already connected to given callable 'VoxelGraphFunction::_on_subresource_changed'`）
        - 修复启用 GPU 生成时，在范围分析假定为均匀的区域中出现错误的“墙体”的问题（例如，使用 Select 节点在某个区域输出 SDF=1.0；commit：350d3897dcec7e37016e4fb95851cd389947371b）
    - `VoxelMesherBlocky`：修复 `VoxelLodTerrain` 的区块边界存在无效模型 ID 时崩溃的问题
    - `VoxelMeshSDF`：修复从非索引网格烘焙时报错的问题（Godot 的 CSG 节点正是这种情况）
    - `VoxelMesherTransvoxel`：修复正 LOD 边界附近某些几何体变化错误的问题，尤其是在使用体素纹理时。仍然存在边界情况，目前可以通过着色器 hack 修复。
    - `VoxelStreamRegionFiles`：GDExtension：修复创建目录时报错的问题
    - `VoxelStreamSQLite`：
        - `preferred_coordinate_format` 被错误地公开（感谢 @beicause 修复）
        - 当数据流未配置路径时（尤其是在编辑器中分配新数据流时），将错误刷屏替换为单条警告
    - `VoxelTool`：
        - `is_area_editable` 尺寸偏差为 1，并且当 AABB 的任意分量小于 1 时总是返回 `true`
        - `paste_masked` 未检查正确的坐标来清除包含至少一个元数据的目标区域中的元数据。它还导致 `get_voxel` 在无效位置报错刷屏
        - `paste_masked_writable_list` 导致索引越界错误（感谢 @HiperSlug 发现）
        - `set_voxel_metadata`：地形：修复传入 `null` 时未像 `VoxelBuffers` 那样清除元数据的问题。
        - `copy`：修复源为地形时体素元数据未被复制的问题
    - `VoxelToolLodTerrain`：修复当变换被缩放且 `sdf_strength` 不为 1 时 `do_graph` 倾向于生成盒状的问题
    - `VoxelViewer`：重新设置父级（先 `remove_child` 再 `add_child`）不应再重新加载观察者周围的地形
    - `VoxelAStarGrid3D`：修复未先设置地形就调用 `find_path` 时崩溃的问题
    - `Voxel_SpotNoise`：修复 `get_spot_positions_in_area` 函数在 (0,0) 单元之外无法工作的问题

- 破坏性变更
    - `VoxelGeneratorGraph`：`SdfSphere` 节点：`radius` 现在是一个输入而非参数（仅当您使用脚本设置它时才会出现兼容性问题：将 `set_node_param(id, 0, radius)` 替换为 `set_node_default_input(id, 3, radius)`）
    - `VoxelTool.set_voxel_metadata`：在地形上，传入 `null` 现在会清除元数据，而不是创建值为 `null` 的元数据，以与 `VoxelBuffer` 保持一致并修复 issue 773。


1.4.1 - 29/03/2025 - tag `v1.4.1`
--------------------------------------

- `VoxelToolMultipassGenerator`：实现 `get/set_voxel_metadata`

- 修复
    - 修复体素生成或细节法线贴图渲染启用 GPU 时泄漏的问题，长时间运行可能导致崩溃
    - 地形在启用物理插值时不再进行不必要的插值（地形是静态的，目前不支持）。
    - `VoxelGeneratorGraph`：
        - 修复 `Curve` 节点在 GPU 上使用时结果不正确的问题。
        - 修复单体素查询在方块风体素上不工作的问题（尤其修复 VoxelLodTerrain 中的光线投射）
        - 编辑器：复制带有自动连接的节点时自动连接停止工作（如噪声；变通方法是重新加载图）
    - `VoxelGeneratorMultipassCB`：修复输出最终数据块时体素元数据未被保留的问题


1.4 - 03/03/2025 - tag `v1.4`
------------------------------

主要使用 Godot 4.4 开发。

- `VoxelBlockyModel`：添加在使用 `VoxelLodTerrain` 时关闭“LOD 裙边”的选项，这可能对透明模型有用
- `VoxelBlockyModelCube`：添加像 `VoxelBlockyMesh` 那样的网格旋转支持（此前，编辑器中的旋转按钮只会交换图块）
- `VoxelEngine`：在 `get_stats` 返回的字典中添加 `tasks.gpu` 条目，这对使用 GPU 功能时的加载画面很有用（尤其是异步编译计算着色器，可能会延迟生成）
- `VoxelBuffer`：
    - 添加从 SDF 通道创建/更新 `Texture3D` 的功能
    - 添加将整个通道作为原始 `PackedByteArray` 获取/设置的功能
- `VoxelInstanceGenerator`：
    - 添加 `OnePerTriangle` 发射模式
    - 添加按体素纹理索引过滤生成的功能，当 `VoxelMesherTransvoxel` 的 `texturing_mode` 设置为 `4-blend over 16 textures` 时可用
- `VoxelTool`：`raycast` 还会根据体素数据返回一个 `normal`（在某些情况下可能与物理光线投射不同）
- `VoxelToolLodTerrain`：实现当网格生成器为 `VoxelMesherBlocky` 或 `VoxelMesherCubes` 时的光线投射
- `VoxelMesherBlocky`：添加对流体模型的基础支持

- 修复
    - 修复使用细节渲染和各种编辑功能时可能出现的死锁（感谢 lenesxy，issue #693）
    - `VoxelInstanceLibrary`：编辑器：重新设计了项目以 Blender 风格列表暴露的方式。现在当库作为子检视器打开时移除项目不再有问题
    - `VoxelInstancer`：
        - 修复当网格数据块大小设置为 32 时，持久实例以错误位置（空中、地下等）重新加载的问题
        - 编辑器：修复删除使用 `VoxelBlockyLibrary` 的 `VoxelInstancer` 后编辑该库时出现 `!is_inside_world()` 错误的问题
    - `VoxelLodTerrain`：
        - 修复使用 Clipbox 流式系统配合线程更新时可能崩溃的问题（感谢 lenesxy，issue #692）
        - 修复使用 Clipbox 卸载数据块时以错误的 LOD 索引保存，导致空洞和地形不匹配的问题（#691）
        - 修复使用 Clipbox 流式系统时，观察者远离边界时地形边界附近区块加载错误的问题
    - `VoxelStreamSQLite`：修复连接泄漏（感谢 lenesxy，issue #713）
    - `VoxelTerrain`：
        - 跨固定边界的编辑和复制不再表现得像地形在边界外生成一样（这曾导致出现“墙体”）。
        - 仅碰撞的观察者不应再导致可视网格出现
    - `VoxelGeneratorGraph`：
        - 修复启用优化执行映射时使用 `OutputWeight` 且权重被确定为局部常量时值错误的问题
        - 修复使用 `FastNoise3D` 节点配合 `OpenSimplex2S` 噪声类型时地形偶尔出现空洞的问题
        - 修复使用 `Distance3D` 节点时着色器生成错误的问题（vec2 而非 vec3，感谢 scwich）
        - 修复将空图像分配给 `Image` 节点时崩溃的问题
    - `VoxelMesherTransvoxel`：回退曾试图阻止空气体素参与贡献但降低质量的纹理逻辑。现在作为实验属性可选启用。
    - `VoxelStreamSQLite`：修复加载带有已编辑 `VoxelInstancer` 数据的区域时“空尺寸”错误的问题
    - `VoxelTool`：`raycast`：使用方块风体素时，返回的 `distance_along_ray` 现在会考虑非立方体素
    - `VoxelVoxLoader`：修复加载由 0.99.7 之后版本的 MagicaVoxel 保存的 `.vox` 文件的问题
    - `.vox` 场景导入器：禁用线程化导入，以解决保存网格时编辑器冻结的问题

- 破坏性变更
    - `VoxelInstanceLibrary`：不应再使用生成的属性（`item1`、`item2` 等）访问项目。请改用 `get_item`。
    - `VoxelMesherTransvoxel`：移除 `deep_sampling` 实验选项
    - `VoxelTool`：`do_hemisphere` 的 `flat_direction` 现在指向远离半球平面的一侧（如同其法线），而不是指向它
    - `VoxelToolLodTerrain`：`raycast` 此前接受地形空间坐标。现在为世界空间坐标，以与 `VoxelToolTerrain` 保持一致。


1.3 - 17/08/2024 - branch `1.3` - tag `v1.3.0`
----------------------------------------------

主要使用 Godot 4.3 开发。

- 添加项目设置 `voxel/ownership_checks`，用于关闭某些传递对象的虚函数（如 `_generate_block`）所做的健全性检查。这与 C# 相关，因为其垃圾回收模型会阻止此类检查正常工作。
- `VoxelBuffer`：添加多个对所有体素进行算术运算的函数
- `VoxelInstanceGenerator`：允许通过在字段中手动输入来设置超过 1、最高 10 的密度
- `VoxelMesherBlocky`：
    - 可与 `VoxelLodTerrain` 配合使用。基础支持：网格随 LOD 缩放，且 LOD>1 的区块具有额外几何体以减少 LOD 之间的裂缝
    - 添加实验性的“阴影遮挡体”：如果区块侧面被不透明体素覆盖，则生成四边形，以便在 DirectionalLight 没有可投影表面时强制阴影投射到洞穴中（参见 #622）。
- `VoxelMesherTransvoxel`：
    - 添加 `edge_clamp_margin` 属性，防止三角形变得过小，代价是保真度略有降低
    - 回退了对退化三角形的移除
- `VoxelStreamSQLite`：添加更改坐标格式的选项，现在默认使用允许更大坐标的格式。现有存档保持其原始格式。
- `VoxelToolLodTerrain`：添加 `run_blocky_random_tick`
- `VoxelViewer`：添加 `view_distance_vertical_ratio`，用于按水平距离的比例使用不同的垂直视距

- 修复
    - `VoxelBlockyModelMesh`：修复直接位于网格资源中的材质未生效的问题（此前仅应用了模型或地形上的覆盖）
    - `VoxelBlockyType`：修复指定了基础模型时关于缺少变体的配置警告
    - `VoxelGeneratorGraph`：修复使用非方形图像配合 `Image` 节点时崩溃的问题
    - `VoxelStreamSQLite`：
        - 修复 `set_key_cache_enabled(true)` 导致没有任何内容加载的问题
        - 修复数据库路径包含 `res://` 或 `user://` 时加载缓慢的问题
        - 修复数据库路径无效且 `set_key_cache_enabled(true)` 之后调用 `flush()` 时崩溃的问题
    - `VoxelInstancer`：
        - 修复即使不支持 LOD，`VoxelTerrain` 上仍生成 LOD > 0 实例的问题（最终位置异常）。不应生成任何实例。
        - 修复在编辑器中没有地形父节点时实例化节点导致的错误刷屏
    - `VoxelInstanceLibrary`：修复从 VoxelInstanceLibrary 移除项目时出现 `Assertion failed: "p_id < 0 || p_id >= MAX_ID" is false` 的问题
    - `VoxelMeshSDF`：修复在编辑器中尝试可视化最后一个切片时出错的问题（结果是偏差为 1）
    - `VoxelModifierMesh`：
        - 修复设置 `isolevel` 无效的问题
        - 修复在 `VoxelTerrain` 下设置父级时缺少配置警告的问题（仅支持 `VoxelLodTerrain`）

- 破坏性变更
    - `VoxelBlockyLibrary`：移除已弃用的方法 `get_voxel_index_from_name`，请改用 `get_model_index_from_resource_name`
    - `VoxelBlockyModel`：移除已弃用的 `transparent` 属性，请改用 `transparency_index`
    - `VoxelBuffer`：移除已弃用的 `optimize` 方法，请改用 `compress_uniform_channels`
    - `VoxelRaycastResult`：位置属性现在为 `Vector3i` 而非 `Vector3`（它们一直是整数，但 Godot 引入 `Vector3i` 时忘了修改）
    - `VoxelStream`：
        - 移除已弃用的 `emerge_block` 方法，请改用 `load_voxel_block`
        - 移除已弃用的 `immerge_block` 方法，请改用 `save_voxel_block`
    - `VoxelVoxLoader`：方法现在为静态方法，无需创建类的实例
    - 移除 `VoxelMesherDMC`


1.2 - 20/04/2024 - branch `1.2` - tag `v1.2.0`
------------------------------------------------

主要使用 Godot 4.2 开发。

- 添加 `Voxel_SpotNoise`，公开与图生成器的 `SpotNoise2D` 和 `SpotNoise3D` 节点相同的算法
- 使用 `save_all_modified_blocks` 保存时，现在会在完成时自动刷新 `VoxelStream` 实现的任何缓存
- 添加 `VoxelStreamMemory`，将数据存储在内存中而非文件系统。这主要用于测试目的。
- Godot 现在会跟踪更多内存分配（您可能会注意到 `OS.get_static_memory_usage()` 返回值略有增加）
- `VoxelBlockyModelMesh`：公开 `side_vertex_tolerance`，用于调整几何体何时被视为位于体素侧面
- `VoxelBuffer`：公开 `fill_area_f`
- `VoxelEngine`：添加获取体素引擎版本的方法
- `VoxelGeneratorGraph`：为 `Select` 节点添加 GPU 支持
- `VoxelLodTerrain`：
    - `save_all_modified_blocks` 现在返回类似 `VoxelTerrain` 的完成追踪器
    - 添加新的可选 LOD 流式系统 `Clipbox`（高级设置）：
        - 使用同心盒而非八叉树遍历，尽管部分逻辑仍与八叉树类似
        - 使用更少的 CPU
        - 支持多个观察者
        - 支持仅碰撞的观察者
        - 添加次要 LOD 距离参数，与 LOD0 分开控制 LOD1 及更高等级的覆盖范围（在旧系统中未使用）
        - 有其自身的局限性和待改进之处，可能会随时间逐步解决
        - 原系统现在被称为“Legacy Octree”。
    - 调试绘制现在以属性的方式公开。编辑器复选框已从地形菜单中移除
- `VoxelMesherTransvoxel`：空气体素（SDF>0）的纹理不再对网格产生贡献
- `VoxelStream`：
    - 添加 `flush` 方法，用于在数据流实现使用缓存时强制写入文件系统
- `VoxelStreamSQLite`：添加对 `user://` 路径的支持（通过内部调用 `ProjectSettings.globalize_path()`）
- `VoxelTool`：
    - 添加 `grow_sphere`，作为在平滑体素球形区域内渐进生长或缩小物质的替代方式（感谢 Piratux）
    - 平滑体素的 `do_box` 现在使用正确的盒状 SDF 以提高质量。之前是实心填充，可能导致伪影
    - 添加 `paste_masked_writable_list`，它会根据目标体素确定要复制的内容
- `VoxelToolBuffer`：即使受影响区域部分超出目标缓冲区边界，现在也允许编辑。结果将被裁剪。
- `VoxelToolLodTerrain`：
    - 通过扩大切割区域以包含更多梯度，改进了平滑地形上 `separate_floating_chunks` 的质量

- 修复
    - 修复特定游戏启动条件下玩家周围区块加载优先级错误的问题
    - 修复编辑器中的 `"plugins_list.has(p_plugin)" is true` 错误，代价是轻微的行为变化。这是由现有的防止 UI 意外隐藏的变通措施引起的，这些措施已被修改以避免该错误，但不幸的是仍然需要。
    - 修复导入 `.vox` 文件时出现 `Unimplemented _get_import_order in add-on` 错误的问题
    - 修复某些边界情况下，快速离开并返回已编辑区域时，由于区块在这些编辑被异步保存之前重新加载，导致编辑回退到之前状态的问题
    - 修复使用某些有时会跳过填充输出缓冲区（假定其已初始化为默认值）的生成器时，地形边界附近可能出现伪影的问题（issue #603）
    - `VoxelBlockyModel`：修复 `material_override_*` 属性全部表现如同同一材质的问题
    - `VoxelBlockyTypeLibrary`：修复保存带有空类型条目的库时崩溃的问题（感谢 ArchLinus）
    - `VoxelBoxMover`：修复当 `VoxelBlockyLibrary` 包含大量模型时性能下降的问题。
    - `VoxelGeneratorGraph`：
        - 修复 `OutputSingleTexture` 产生的模棱两可的体素纹理索引导致某些情况下绘制失败的问题
        - 修复使用 GPU 生成时输出节点的默认输入值始终为 0 的问题
        - 修复使用 16 个权重输出节点（即最大值）时崩溃的问题
        - 修复使用超过 12 个权重输出节点时报错的问题
        - 修复将图用作笔刷时在某些变换下不工作的问题
        - 修复 `Image` 节点在负坐标下的环绕错误
        - 修复生成足够大以触发“细分”功能的区块时行为错误和崩溃的问题
    - `VoxelInstanceLibraryMultimeshItem`：修复使用“Update From Scene”并尝试撤销/重做时报错的问题
    - `VoxelStreamSQLite`：修复使用 `set_key_cache_enabled(true)` 时崩溃的问题
    - `VoxelTool`：修复 `paste` 明明工作正常却错误打印错误信息的问题
    - `VoxelToolLodTerrain`：
        - `do_point` 和 `set_voxel` 并不总能更新区块边界附近的网格，留下空洞
        - 如果区域从未被编辑、数据流已开启且生成器是产生单纹理信息的 `VoxelGeneratorGraph`，`get_voxel` 在索引和权重通道中总是返回 0
        - 当数据流已开启且分配了生成器时，在未编辑区域使用 `copy` 会返回错误的缓冲区
        - 修复在未分配数据流时离开已编辑区块所打印的错误
        - 修复 `separate_floating_chunks` 生成的区块位置不完全一致的问题
        - 修复 `stamp_sdf` 偶尔不能正确工作的问题

- 破坏性变更
    - `VoxelBuffer`：
        - `get_voxel_f` 和 `set_voxel_f` 现在会自动重新缩放量化值。它们不再归一化到 -1..1，可能表示有符号距离，因此无需手动缩放（定点编码引起的不精确仍然存在）。
        - `debug_print_sdf_y_slices` 现在返回类型化数组而非无类型数组
    - `VoxelGeneratorGraph`：对 `Image` 坐标环绕的修复意味着结果将与之前有缺陷的版本不同（有缺陷的版本会在负坐标下对图像进行部分偏移）
    - `VoxelGraphFunction`：`NodeTypeID` 枚举的某些成员值已更改。不过，此枚举的值不应直接使用，也不应保存在存档中。
    - `VoxelStream`：体素的保存和加载方法现在接收数据块位置而非体素位置
    - `VoxelTool`：由于内部自动 SDF 重新缩放，如果您修改了 `sdf_scale`，可能需要调整它（如果设置为 0.002 可以移除）。
    - `VoxelToolMultipassGenerator`：将 `get_editable_area_max` 改为返回排他位置而非包含位置


1.1 - 29/12/2023 - branch `1.1` - tag `v1.1.0`
-----------------------------------------------

主要使用 Godot 4.1 开发。

- 通用
    - 为两种地形类型添加阴影投射设置
    - 添加用于重新生成所选地形的编辑器快捷键
    - 添加在 GPU 上进行数据块生成的支持（仅可用于同时支持 CPU 和 GPU 的生成器，目前仅 `VoxelGeneratorGraph`）。
    - 将 FastNoise2 更新到 0.10.0-alpha
    - 为方块风体素工作流开始了实验性类型系统。不过它尚未完全可用，其 API 将来可能会更改或部分移除。
    - 添加实验性的 `VoxelAStarGrid3D`，用于方块风体素上基于网格的寻路
    - 添加实验性的 `VoxelGeneratorMultipassCB`，以多趟方式实现跨区块的柱状生成
    - 为 `VoxelTerrain` 和 `VoxelLodTerrain` 添加 `render_layers_mask` 属性
    - 场景树暂停时，体素引擎处理不再停止
    - `VoxelGeneratorGraph`：
        - 添加 `Spots2D` 和 `Spots3D` 节点，针对生成“矿脉”进行了优化
        - 添加对 `FastNoiseGradient2D` 和 `FastNoiseGradient3D` 节点的着色器支持
        - 为 `Image` 节点添加双线性过滤选项
        - 编辑器：重新设计了添加节点的右键菜单，类似于 VisualShader。现在具有搜索栏、树视图和节点描述。
        - 编辑器：添加使用 Ctrl+C/Ctrl+V 快捷键的复制/粘贴
    - `VoxelGraphFunction`：
        - 编辑器：编辑时图现在会被编译，提供一些检查
        - 编辑器：默认情况下，图被编译时会自动设置 I/O。如有必要，未来可能公开手动设置。
    - `VoxelTerrain`：
        - 添加 `VoxelTerrainMultiplayerSynchronizer`，简化使用 Godot 高层多人游戏 API 的复制
        - 添加 `is_area_meshed`，作为使用网格碰撞体的游戏的 `VoxelTool.is_area_editable` 的替代方案
        - 添加 `do_path`，用于构建或雕刻不同半径的“通道”
        - 编辑器：边界为空时添加警告
    - `VoxelTool`：
        - 添加 `smooth_sphere`，使用盒式模糊平滑球形区域内的地形。仅适用于平滑/SDF 地形。（感谢 Piratux 的想法和初步实现）
        - 将 `paste` 拆分为 `paste` 和 `paste_masked` 函数。后者使用特定的通道和值进行遮罩。
    - `VoxelToolTerrain`：
        - 使用 `VoxelMesherBlocky` 对地形进行光线投射现在会考虑碰撞盒（感谢 Lry722）
    - `VoxelToolLodTerrain`：
        - 添加对 `paste` 的支持
    - `VoxelMesherCubes`：
        - 添加辅助函数，将图像转换为 1 体素厚的“精灵网格”
    - `VoxelInstancer`：
        - 为带有物理体节点的 multimesh 实例添加 `get_library_item_id`，使命中它们的光线投射能够判断它们属于哪个项目
    - `VoxelInstanceGenerator`：
        - 添加噪声图属性，实例也可以使用自定义 VoxelGraphFunction 进行过滤
    - `VoxelInstanceLibrary`：
        - 添加 `get_all_item_ids()`，允许遍历库的所有项目
    - `VoxelLibraryMultiMeshItem `：
        - 添加 `render_layer` 属性（感谢 m4nu3lf）
        - 添加 `gi_mode` 属性
        - 公开用于次要的基于距离的 LOD 系统的自定义距离比例
        - 添加超出其最大基于距离的 LOD 时隐藏实例的选项（仅与没有 LOD 的地形相关，或在 `VoxelLodTerrain` 的最后一个 LOD 上相关）
        - 模板场景中的节点组现在会添加到实例碰撞体上（如果存在）
    - `VoxelLodTerrain`：
        - 添加修改器边界的调试绘制
        - 添加 `is_area_meshed`，作为使用网格碰撞体的游戏的 `VoxelTool.is_area_editable` 的替代方案
        - 在编辑器中分配 `material` 属性时，地形现在会更新
        - 编辑器：边界为空时添加警告
    - `VoxelVoxLoader`：
        - 添加参数以允许将数据加载到自定义通道（而非颜色通道）
    - `VoxelBlockyModel`：
        - 在编辑器中添加 3D 预览
        - 添加在编辑器中旋转模型的能力（不仅是预览，实际旋转烘焙后的模型）
        - 改为使用 `Resource.name` 处理名称，这样也会在编辑器中的模型列表中显示
        - 添加 `culls_neighbors` 属性，控制一个模型的侧面是否可以剔除其他模型的侧面（感谢 spazzylemons）

- 修复
    - 修复在窄屏幕上选中地形时编辑器无法正确收缩的问题。如果区域太小，底部面板中显示的统计信息将使用滚动条。
    - `VoxelBoxMover`：处理台阶攀爬时的浮点错误，该错误可能导致角色穿过台阶掉落（取决于游戏代码在转换返回的运动时引入错误）
    - `VoxelGeneratorGraph`：
        - 修复当图包含同时具有已使用和未使用输出的节点并以 `debug=false` 编译时崩溃的问题
        - 修复添加 Constant 节点时报错的问题
        - 修复保存场景时图并不总是被保存的问题
        - 修复节点具有未连接输入时着色器生成器崩溃的问题
        - 修复在 GPU 上使用时的细胞噪声问题
        - 修复在负坐标采样时 Image 节点的问题
    - `VoxelGraphFunction`：
        - 修复默认输入值未被正确加载的问题
        - 修复使用多个自定义输入时出现意外的“missing node”错误的问题
    - `VoxelInstancer`：
        - 修复在编辑器中隐藏节点时崩溃的问题
        - 修复选中实例器节点时关闭场景崩溃的问题
        - 修复编辑器中使用“Re-generate”菜单在地形形状改变后实例未被清除的问题
    - `VoxelInstanceLibrary`：
        - 修复 `find_item_by_name` 找不到项目的问题
        - 修复当地形没有 LOD 时，编辑器中新建项目默认渲染效果不佳的问题。目前它们始终默认使用 LOD 0 而非 LOD 2。
    - `VoxelTerrain`：修复地形在未分配网格生成器时尝试更新导致崩溃的问题
    - `VoxelLodTerrain`：修复重新生成或销毁地形时的错误刷屏
    - `VoxelMesherBlocky`：修复使用超过 256 个材质时材质“环绕”的问题。已将上限提高到 65536。
    - `VoxelMesherTransvoxel`：移除了罕见的退化/微观三角形，它们曾导致 Jolt Physics 出错。不过，进行这些检查会使网格生成速度降低约 15%（未贴纹理时）。
    - `VoxelStreamRegionFiles`：修复 `block_size_po2` 无法正常工作的问题
    - `VoxelToolTerrain`：修复设置体素元数据时地形未被标记为已修改的问题
    - `VoxelToolLodTerrain`：
        - 修复 `stamp_sdf` 因提供烘焙网格时报错而无法工作的问题
        - 修复 `set_voxel` 产生伪影的问题
        - 修复 `separate_floating_chunks` 产生伪影的问题
    - `VoxelMeshSDF`：修复保存的资源无法正确加载的问题

- 破坏性变更
    - `VoxelBlockyLibrary`：
        - 将模型列表改为由类型化数组处理，而非单独的属性。在编辑器中打开时，旧资源将被转换。重新保存它们以使转换生效。
    - `VoxelBlockyModel`：
        - 该类被拆分为针对每种几何类型的多个子类。在编辑器中打开时，旧资源将被转换，但前提是它们是 `VoxelBlockyLibaray` 的一部分。如果它们是独立的资源文件则无法工作。
    - `VoxelNode`：
        - 移除 `GIMode` 枚举，改用 `GeometryInstance3D.GIMode`


1.0 - 12/03/2023 - `godot4.0`
------------------------------

此版本起需要 Godot 4。

- 通用
    - 为地形节点添加 `gi_mode`，用于选择它们与 Godot 全局光照的交互方式
    - 添加 `FastNoise2` 以获得更快的 SIMD 噪声
    - 添加实验性支持函数，帮助使用 `VoxelTerrain` 设置基础多人游戏（将来可能更改）
    - 改进对 64 位浮点数的支持
    - 添加 `Voxel_ThreadedTask`，允许使用线程池系统运行自定义任务
    - 添加 `VoxelMeshSDF`，从网格烘焙 SDF，可用于体素雕刻。
    - 使用 Godot Vulkan 渲染器时，网格资源现在完全在线程上构建
    - 编辑器：地形边界现在以最小/最大值而非位置/尺寸的形式显示在检视器中
    - 为 `VoxelToolTerrain` 和 `VoxelToolLodTerrain` 添加 `do_hemisphere`，可用作整平笔刷
    - `VoxelGeneratorGraph`：
        - 添加输出到 TYPE 通道的支持，允许与 `VoxelMesherBlocky` 配合使用
        - 编辑器：未连接的输入直接在节点上显示其默认值
        - 编辑器：允许更改预览节点 3D 切片的轴
        - 编辑器：从/向已有连接的输入端口拖拽时替换现有连接
        - 编辑器：创建噪声和曲线节点时现在会自动创建其资源，而不会显示为 null
        - 编辑器：添加图钉按钮，即使取消选中地形后也保持图编辑器可见。
        - 编辑器：添加弹出按钮，可在单独窗口中打开图编辑器
        - 添加注释节点
        - 添加中继节点
        - 使用新的 `VoxelGraphFunction` 资源添加自定义函数（初步实现，存在局限性）
        - 添加 `OutputSingleTexture` 节点，用于每个体素输出单个纹理索引，作为权重的替代方案。这专门用于平滑体素。
        - 添加数学表达式节点
        - 添加 Pow 和 Powi 节点
        - Clamp 现在接受 min 和 max 作为输入。如需使用常量参数的版本，请使用 ClampC（在当前状态下可能更快）。
        - 添加逐节点性能分析详情，以查看哪些节点占用大部分时间
        - 添加“live update”选项，在图被修改时自动重新生成地形
        - 某些节点具有默认输入连接，因此不再需要手动将它们连接到 (X,Y,Z) 输入
        - 添加轻微优化，共享执行相同计算的节点分支
    - `VoxelInstancer`：
        - 添加对 `VoxelTerrain` 的支持。这意味着只有 LOD0 有效，但网格 LOD 应该有效。
        - 编辑器：添加用于查看实例数量的基本 UI
        - 允许将 VoxelInstancer 转储为场景以便调试检查
        - 编辑器：选中节点时显示实例区块
        - 如果保存的实例使用不同的网格数据块大小，更改网格数据块大小不应再使它们失效
    - `VoxelInstanceLibraryMultiMeshItem`：
        - 支持从带有 `LODx` 后缀名称的场景设置网格 LOD
        - 支持直接设置场景，运行时转换为 multimesh（修复了一些工作流问题：场景更改时自动更新，使用导入的场景时不会在 `.tres` 文件中创建网格和纹理副本）
    - `VoxelLodTerrain`：为开发版本公开调试绘制选项

- 平滑体素
    - SDF 数据现在使用 `inorm8` 和 `inorm16` 编码，而非某个任意的 `unorm8` 和 `unorm16` 版本。已提供迁移代码来加载旧存档文件，但*在运行新版本的项目之前请做好备份*。
    - `VoxelTool`：添加 `set_sdf_strength()`，用于在雕刻平滑体素时控制笔刷强度（此前表现得如同 1.0）
    - `VoxelLodTerrain`：添加*实验性*的 `full_load_mode`，一次加载所有已编辑数据，允许随时编辑任何区域。适用于某些固定大小的体积。
    - `VoxelLodTerrain`：添加可选的远处法线贴图计算以改进 LOD 质量。也可以在 GPU 上运行以获得更快的执行（仅 `VoxelGeneratorGraph`）。
    - `VoxelLodTerrain`：
        - 编辑器：添加在编辑器中显示八叉树节点的选项
        - 编辑器：添加在编辑器中显示八叉树网格的选项，现在默认关闭
        - 添加将大部分处理逻辑运行到另一个线程的选项
        - 添加调试小工具以查看网格更新
    - `VoxelToolLodTerrain`：
        - 添加*实验性*的 `do_sphere_async`，这是 `do_sphere` 的替代版本，将任务推迟到线程上执行，以减少受影响区域较大时的卡顿。
        - 添加 `stamp_sdf` 函数，将烘焙的网格 SDF 放置到地形上
        - 添加 `do_graph`，在特定区域运行基于 `VoxelGeneratorGraph` 的自定义笔刷。为支持 SDF 修改，添加了 `InputSDF` 节点。
    - `VoxelMesherTransvoxel`：
        - 初步支持深度 SDF 采样，以在低细节级别下微调顶点位置（目前是缓慢且有限的可行性验证）。
        - 可变 LOD：常规和过渡网格现在合并为每个区块的单个网格。渲染它需要着色器，但生成的网格资源大大减少，并减少了绘制调用数量。

- 方块风体素
    - `VoxelMesherBlocky`：
        - 材质现在不受限制，并在每个模型中指定，可以作为覆盖或直接来自网格（使用大量材质时仍需考虑绘制调用）
        - 每个模型最多可以有 2 个材质（即表面）
        - 网格碰撞：添加指定哪些表面具有碰撞的支持
    - `VoxelBoxMover`：添加对台阶攀爬的基础支持

- 修复
    - `VoxelBlockyLibrary`：编辑器在出现空模型条目后不再过一段时间就崩溃。
    - `VoxelBlockyModel`：更改 `geometry_type` 时检视器的属性未刷新
    - `VoxelBuffer`：频繁创建大小始终不同的缓冲区不再浪费内存
    - `VoxelGeneratorGraph`：
        - 编辑器：修复删除节点后检视器因仍在检查该节点而开始报错的问题
        - 编辑器：修复将 SdfPreview 节点连接到输入时崩溃的问题。不过目前尚不支持此操作。
        - 编辑器：修复某些双显示器配置下右键菜单位置错误的问题
        - 编辑器：修复节点 UI 布局更新时偶尔随机崩溃的问题
        - 修复 Image2D 节点不接受 L8 和 LA8 图像格式的问题
        - 修复图包含资源时内存泄漏的问题
        - 某些特定节点图排序不正确
        - SmoothUnion 和 SmoothSubtract 导致运行时优化错误跳过分支，产生空数据块
    - `VoxelGeneratorFlat`：修复地下 SDF 值为 0 而非负值的问题
    - `VoxelInstancer`：
        - 修复项目被修改且网格数据块大小为 32 时实例不刷新的问题
        - 修复实例器节点正在使用某项目时从库中移除该项目导致崩溃的问题
        - 修复移除场景实例时的错误
        - 修复保存场景实例时的位置问题
        - 修复网格数据块大小设置为 32 时保存实例的位置问题
    - `VoxelLodTerrain`：
        - 修复 `lod_fade_duration` 属性不接受小数的问题
        - 启用 LOD 淡入淡出时，接缝处不再出现裂缝
    - `VoxelMesherCubes`：
        - 编辑器：颜色模式现在是一个正确的下拉框
        - 修复原始颜色模式无法正常工作的问题
        - 透明与实心立方体之间的 alpha 检查错误
    - `VoxelMesherTransvoxel`：
        - 修复表面恰好与整数坐标对齐时不出现在网格中的问题
        - 修复某些特定配置下几何体偶尔出现空洞和“尖刺”的问题
    - `VoxelStreamScript`：修复返回 `BLOCK_FOUND` 时体素数据未被获取的问题
    - `VoxelTerrain`：
        - 修复某些情况下可能出现的 `Condition "mesh_block == nullptr" is true`
        - 更改材质现在会更新现有网格，而不仅仅是新网格
    - `VoxelTool`：`raycast` 在发送包含 NaN 的 Vector3 时卡死
    - `VoxelToolLodTerrain`：修复整数 `do_sphere` 半径结果不一致的问题
    - `VoxelToolTerrain`：`run_blocky_random_tick` 不再以难以理解的方式将区域边界吸附到区块边界

- 破坏性变更
    - 某些函数现在接收 `Vector3i` 而非 `Vector3`。如果您过去发送 `Vector3` 而没有使用 `floor()` 或 `round()`，在负坐标下可能会产生副作用。
    - `VoxelTerrain`：指定材质的主要方式不再在这里，而是在网格生成器中。
    - `VoxelLodTerrain`：`set_process_mode` 和 `get_process_mode` 已重命名为 `set_process_callback` 和 `get_process_callback`（由于名称冲突）
    - `VoxelLodTerrain`：`has_block` 已重命名为 `has_data_block`
    - `VoxelMesherTransvoxel`：着色器 API：`COLOR` 和 `UV` 中的数据已分别移至 `CUSTOM0` 和 `CUSTOM1`（旧属性对此用途不再有效）
    - `VoxelMesherTransvoxel`：可变 LOD：现在需要着色器才能正确渲染过渡
    - `Voxel` 已重命名为 `VoxelBlockyModel`
    - `VoxelLibrary` 已重命名为 `VoxelBlockyLibrary`
    - `VoxelVoxImporter` 已重命名为 `VoxelVoxSceneImporter`
    - `VoxelInstanceLibraryItem` 已重命名为 `VoxelInstanceLibraryMultiMeshItem`
    - `VoxelInstanceLibraryItemBase` 已重命名为 `VoxelInstanceLibraryItem`
    - `VoxelServer`：重命名为 `VoxelEngine`
    - `VoxelStream`：
        - `emerge_block` 重命名为 `load_voxel_block`
        - `immerge_block` 重命名为 `save_voxel_block`
    - `VoxelStreamScript`：
        - `_emerge_block` 重命名为 `_load_voxel_block`
        - `_immerge_block` 重命名为 `_save_voxel_block`
    - `VoxelGeneratorGraph`：`Select` 节点的 `threshold` 端口现在改为参数。
    - `FastNoiseLite` 已重命名为 `Voxel_FastNoiseLite`，因为现在 Godot 4 自带自己的实现，略有差异。
    - 移除 `VoxelStreamBlockFiles`

- 已知问题
    - 由于 Godot4 在属性为资源时引入的警告，某些节点和资源不再以预定义属性开始。
    - SDFGI 并非始终有效，只能通过离开再回来、预先生成地形或开关切换来强制更新。这是 Godot 不能很好地支持动态创建网格的局限。


0.5.x - Legacy Godot 3 branch - `godot3.x`
------------------------------------

此分支是最后一个支持 Godot 3 的分支

- 平滑体素
    - `VoxelLodTerrain`：添加*实验性*的 `full_load_mode`，一次加载所有已编辑数据，允许随时编辑任何区域。适用于某些固定大小的体积。
    - `VoxelToolLodTerrain`：添加*实验性*的 `do_sphere_async`，这是 `do_sphere` 的替代版本，将任务推迟到线程上执行，以减少受影响区域较大时的卡顿。
    - `VoxelInstanceLibraryItem`：支持从带有 `LODx` 后缀名称的场景设置网格 LOD

- 修复
    - `VoxelBuffer`：频繁创建大小始终不同的缓冲区不再浪费内存
    - `Voxel`：更改 `geometry_type` 时属性未刷新
    - `VoxelGeneratorGraph`：
        - 修复 Image2D 节点不接受 L8 和 LA8 图像格式的问题
        - 编辑器：修复将 SdfPreview 节点连接到输入时崩溃的问题。不过目前尚不支持此操作。
        - 修复图包含资源时内存泄漏的问题
    - `VoxelTerrain`：修复某些情况下可能出现的 `Condition "mesh_block == nullptr" is true`
    - `VoxelTool`：`raycast` 在发送包含 NaN 的 Vector3 时卡死
    - `VoxelToolLodTerrain`：修复整数 `do_sphere` 半径结果不一致的问题
    - `VoxelInstancer`：
        - 修复项目被修改且网格数据块大小为 32 时实例不刷新的问题
        - 修复实例器节点正在使用某项目时从库中移除该项目导致崩溃的问题
        - 修复移除场景实例时的错误
    - `VoxelStreamScript`：修复返回 `BLOCK_FOUND` 时体素数据未被获取的问题
    - 使用房间/传送门系统时地形不可见。目前它不会被房间剔除。


0.5 - 06/11/2021 - `godot3.4`
-------------------------

- 通用
    - `VoxelTerrain`：添加 `get_data_block_size()`
    - `VoxelToolTerrain`：添加 `for_each_voxel_metadata_in_area()`，用于快速查找一个盒内的所有元数据
    - `FastNoiseLiteGradient`：公开缺失的扭曲函数
    - 为地形节点添加配置碰撞边距的属性
    - 线程数现在根据 CPU 支持的并发线程数自动确定
    - 添加用于配置线程数的项目设置
    - 在编辑器的“关于”窗口中添加第三方许可证

- 方块风体素
    - 添加 *.vox 导入器，可将 MagicaVoxel 文件作为场景或网格导入

- 平滑体素
    - `VoxelMesherTransvoxel`：
        - 初步支持体素中的纹理数据，使用 4 位索引和权重
    - `VoxelMesherTransvoxel`：
        - 优化热路径，速度提高约 20%
        - 添加使用 MeshOptimizer 简化网格的选项
    - `VoxelToolLodTerrain`：
        - 添加 `copy` 函数
        - 添加 `get_voxel_f_interpolated` 函数，用于获取插值后的 SDF
        - 添加在指定区域内将浮动区块分离为刚体的函数
    - `VoxelInstanceGenerator`：添加更精确地从面发射的额外选项，尤其是网格被简化后（比其他选项慢）
    - `VoxelInstancer`：
        - 添加从场景设置 multimesh 项目的菜单（类似于 GridMap），也可用于设置碰撞体
        - 添加对常规场景实例化的初步支持（比 multimesh 慢）
        - 添加关闭随机旋转的选项
    - `VoxelInstanceLibrary`：将添加/移除/更新项目的菜单移到检视器中，而不是 3D 编辑器工具栏

- 破坏性变更
    - `VoxelBuffer`：通道 `DATA3` 和 `DATA4` 已重命名为 `INDICES` 和 `WEIGHTS`
    - `VoxelInstanceGenerator`：`EMIT_FROM_FACES` 已重命名为 `EMIT_FROM_FACES_FAST`。`EMIT_FROM_FACES` 仍然存在，但是不同的算法。
    - `VoxelServer`：`get_stats()` 格式已更改，请查看文档
    - `VoxelLodTerrain`：`get_statistics()` 格式已更改：`time_process_update_responses` 和 `remaining_main_thread_blocks` 不再可用
    - `VoxelLodTerrain`：最大 LOD 数量已减少到 24，仍然足够使用。更高的最大值很可能导致整数溢出。
    - `VoxelTerrain`：`get_statistics()` 格式已更改：`time_process_update_responses` 和 `remaining_main_thread_blocks` 不再可用
    - `VoxelViewer`：`requires_collisions` 现在默认值为 `true`

- 修复
    - `VoxelGeneratorGraph`：
        - 节点属性的更改现在会正确保存
        - 修复退出时某些每线程内存未释放的问题
        - `debug_analyze_range` 在收到负尺寸区域时崩溃
    - `VoxelBuffer`：
        - `copy_voxel_metadata_in_area` 错误地检查源盒
        - 如果通道不均匀，多次使用不同尺寸调用 `create()` 可能导致堆损坏
        - 如果源和目的地大小相同且完全复制，`copy_channel_from_area` 可能导致堆损坏
    - `VoxelMesherTransvoxel`：输入缓冲区非立方体时不再崩溃
    - `VoxelLodTerrain`：
        - 修复在加载边界附近编辑体素时的错误和崩溃
        - 修复 LOD 数量设置为 1 时编辑几次后崩溃的问题
        - 修复地形在编辑器中加载时关闭 `run stream in editor` 导致崩溃的问题
    - 从分配了数据流的地形使用 `get_voxel_tool` 时，`VoxelTool` 通道不再默认使用 7。而是选择网格生成器第一个使用的通道（回退顺序为网格生成器、生成器、数据流）。
    - `VoxelInstancer`：
        - 修复节点可见性变化时报错的问题
        - 修复顶点发射模式下密度为 1 时不生成实例的问题
    - `VoxelInstanceLibraryItem`：修复编辑器中设置的碰撞形状未被保存的问题
    - `VoxelInstanceLibrarySceneItem`：修复关联场景未被保存的问题
    - `VoxelTerrain`：修复材质显示在错误的检视器类别下的问题
    - `VoxelStreamRegionFiles`：修复元数据文件有时以错误的深度值写入导致的错误
    - `VoxelStreamBlockFiles`：修复场景树中始终显示通道警告的问题
    - `VoxelStreamSQLite`：修复 LOD0 以上的数据块被保存在错误位置，导致它们经常在漂浮在空中时被重新加载的问题
    - 修复所有 PoolVector 分配都在使用时发生的某些崩溃（Godot 3.x 的限制）。它将改为打印错误，但崩溃仍可能发生在 Godot 代码内部，因为它不常检查这一点
    - 修复向 AABB 函数参数发送负尺寸时发生的某些崩溃


0.4 - 09/05/2021 - `godot3.3`
-----------------------

- 通用
    - 引入 `VoxelServer`，在所有体素节点之间共享线程任务
    - 体素数据发送到处理线程时不再被复制，减少某些场景下较高的内存峰值
    - 添加用于加载由 MagicaVoxel 创建的 `.vox` 文件的工具类（仅限脚本）
    - 体素节点可以移动、缩放和旋转
    - 体素节点可以限制在特定边界内，而不是无限分页的体积（块大小的倍数）。
    - 网格生成器现在为资源，因此可以为每个地形选择和配置
    - 添加 [FastNoiseLite](https://github.com/Auburn/FastNoise) 以获得更多种类的噪声
    - 生成器不再局限于单个后台线程
    - 添加 `VoxelStreamSQLite`，允许将体积保存为单个 SQLite 数据库
    - 为 `VoxelToolTerrain` 实现 `copy` 和 `paste`
    - 添加将网格和实例化使用的数据块大小加倍的能力，以提升渲染速度，代价是修改速度变慢
    - 添加碰撞层和遮罩属性

- 编辑器
    - 流式/LOD 可以设置为跟随编辑器相机，而不是以世界原点为中心。请谨慎使用，快速的大幅移动和缩放可能导致卡顿
    - 选中节点时现在会显示待处理后台任务的数量
    - 添加“关于”窗口

- 平滑体素
    - 着色器现在可以访问每个数据块的变换，对移动体积上的三平面映射很有用
    - Transvoxel 运行更快（几乎 2 倍加速）
    - SDF 通道现在默认 16 位而非 8 位，可减少大型地形中的台阶效应
    - 优化 `VoxelGeneratorGraph`，使其更准确地检测空数据块并按缓冲区处理
    - `VoxelGeneratorGraph` 现在公开性能调优参数
    - 向体素图添加 `SdfSphereHeightmap` 和 `Normalize` 节点，有助于制作行星
    - 向体素图添加 `SdfSmoothUnion` 和 `SdfSmoothSubtract` 节点
    - 添加 `VoxelInstancer`，用于在 `VoxelLodTerrain` 上实例化项目，旨在生成岩石和植被等自然元素
    - 实现 `VoxelToolLodterrain.raycast()`
    - 添加用于 LOD 淡入淡出的实验性 API，存在一些限制。

- 方块风体素
    - 引入第二个专门的方块风网格生成器，用于彩色立方体，支持贪婪网格化和调色板
    - 用 `transparency_index` 替换 `transparent` 属性，以更好地控制透明面的剔除
    - TYPE 通道现在默认 16 位而非 8 位，最多可存储 65,536 种类型（该通道的一部分将来可能实际用于存储旋转）
    - 添加法线贴图支持
    - `VoxelRaycastResult` 现在还包含命中距离，因此可以确定精确的命中位置
    - `VoxelBoxMover` 支持缩放/平移的地形和 `VoxelMesherCubes`（仅限于非零颜色体素）
    - `VoxelColorPalette` 颜色可以在检视器中编辑
    - `VoxelToolTerrain.raycast` 会考虑缩放和旋转，并支持 VoxelMesherCubes（非零值）

- 破坏性变更
    - `VoxelViewer` 现在取代 `VoxelTerrain` 上的 `viewer_path` 属性，并允许多个加载点
    - 在 `VoxelBuffer` 中定义 `COLOR` 通道，此前称为 `DATA2`
    - `VoxelGenerator` 不再作为基于脚本的生成器的基类，请改用 `VoxelGeneratorScript`
    - `VoxelGenerator` 不再继承 `VoxelStream`
    - `VoxelStream` 不再作为基于脚本的数据流的基类，请改用 `VoxelStreamScript`
    - 生成器和数据流已经拆分。数据流更专注于文件并使用单个后台线程。生成器专注于生成，可由多个后台线程使用。地形为两者各有一个属性。
    - 网格系统不再“猜测”体素的外观。而是使用分配给地形的网格生成器。
    - SDF 和 TYPE 通道具有不同的默认深度，因此如果您依赖 8 位深度，可能需要在生成器中明确设置该格式，以避免与现有存档不匹配
    - 数据块序列化格式已更改，且未实现迁移，因此使用旧格式的存档无法使用。更多信息请参阅文档。
    - 地形在销毁时分配了 `stream` 不再自动保存。您必须在此之前显式调用 `save_modified_blocks()`。
    - `VoxelLodTerrain.lod_split_scale` 已替换为 `lod_distance` 以求清晰。它是观察者到第一个 LOD 级别可以扩展处的距离。

- 修复
    - C# 现在应该能够正确实现生成器/数据流函数

- 已知问题
    - `VoxelLodTerrain` 并未完全支持 `VoxelViewer`，但计划对其进行重构。


0.3 - 08/09/2020 - `godot3.2.3`
--------------------------

- 通用
    - 添加逐体素和逐数据块元数据，由文件数据流与体素数据一起保存
    - 尽可能使用 `StringName` 调用脚本函数，以减少开销
    - 公开数据块序列化器，允许从脚本为网络或文件编码体素
    - 添加用于显式触发保存的地形方法
    - 模块仅在引擎处于详细模式时打印调试日志
    - `VoxelTerrain` 现在会在数据块加载和卸载时发出信号

- 编辑器
    - 地形节点现在在编辑器中渲染，除非涉及脚本（可以通过选项更改）

- 方块风体素
    - 为方块风体素添加碰撞遮罩，与 `VoxelBoxMover` 和体素光线投射配合生效
    - 为方块风地形添加随机 tick API
    - 方块风网格碰撞形状现在考虑所有表面

- 平滑体素
    - 添加基于图的生成器，目前专用于 SDF 数据

- 修复
    - 修复 `VoxelTerrain` 未使用数据流时崩溃的问题
    - 修复 `Voxel.duplicate()` 未正确实现的问题
    - 修复仅第一个网格表面生成碰撞形状的问题
    - 修复 `VoxelTool` 截断 64 位值的问题


0.2 - 27/03/2020 - (no branch) - Tokisan Games binary version
--------------------------------------------------------

- 通用
    - 使用池优化 `VoxelBuffer` 内存分配（当大小经常相同时效果最佳）
    - 通过从世界中移除网格而非设置 `visible` 属性来优化数据块可见性
    - 为 `VoxelBuffer` 添加可自定义的位深（8、16、32 和 64 位）
    - 在地形节点设置错误时添加节点配置警告
    - 为 VoxelBuffer 实现 VoxelTool
    - 添加 `VoxelStreamNoise2D`
    - 在 `VoxelGeneratorImage` 中添加模糊选项
    - 生成器现在都继承 `VoxelGenerator`

- 方块风体素
    - 方块风体素可以使用自定义网格定义，即使不是立方体，侧面也会自动剔除
    - 方块风体素可以有自定义碰撞 AABB，用于 `VoxelBoxMover`
    - 3D 噪声生成器现在可以与 `VoxelTerrain` 一起在方块风模式下工作
    - 方块风体素支持 8 位和 16 位深度

- 平滑体素
    - 实现 Transvoxel 网格生成器，支持无缝 LOD 的过渡网格。现在它默认取代 DMC。
    - 高度图生成器现在具有 `iso_scale` 调优属性，有助于减少台阶效应

- 修复
    - 修复平滑地形上偶尔出现暗色法线的问题
    - 修复 Transvoxel 网格生成器相比其他网格生成器偏移的问题
    - 修复 `ShaderMaterial` 创建导致的卡顿，现在已池化
    - 修复 `VoxelLodTerrain` 远距离传送时卡住的问题
    - 修复 `VoxelStreamBlockFiles` 和 `VoxelStreamRegionFiles` 的一些 bug

- 破坏性变更
    - 从 `Voxel` 移除通道枚举，它与 `VoxelBuffer` 冗余
    - 将 `VoxelBuffer.CHANNEL_ISOLEVEL` 重命名为 `CHANNEL_SDF`
    - 移除 `VoxelIsoSurfaceTool`，由 `VoxelTool` 取代
    - 移除 `VoxelMesherMC`，由 `VoxelMesherTransvoxel` 取代
    - 用 `VoxelGeneratorWaves` 和 `VoxelGeneratorFlat` 替换 `VoxelGeneratorTest`


0.1 - 03/10/2019 - `godot3.1`
------------------------

初始参考版本。

- 支持方块风、双行进立方体和简单行进立方体网格生成器
- 具有恒定 LOD 分页或可变 LOD 的地形（仅限平滑体素）
- 体素可在游戏中编辑（限于 LOD0）
- 各种用于生成、保存和加载体素地形数据的简单数据流
- 网格生成器和存储原语可供脚本使用

...

- 01/05/2016 - 创建模块，使用 Godot 3.0 beta
