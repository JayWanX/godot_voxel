# Voxel Tools

Voxel Tools 是一个用于在 [Godot Engine](https://godotengine.org/) 4.x 中创建体积世界的 **C++ 模块**。它支持程序化体素生成、平滑或方块风地形渲染、实时编辑、持久化存储与多线程流式加载，是构建可编辑体素地形与体积模型的完整解决方案。

![方块风地形](doc/source/images/blocky_screenshot.webp)

![平滑地形](doc/source/images/smooth_screenshot.webp)

![游戏示例](doc/source/images/game_examples.webp)

> **免责声明**：本模块是围绕体素地形进行业余实验的产物，绝非通用的万能解决方案。大量功能可用于游戏开发，但另一些部分仍有欠缺或可能发生变动。它也比通常的 Godot 3D 功能更具技术性，因此最好具备 3D 开发的先验经验。

## 特性

- **多通道体素存储**：基于 `VoxelBuffer` 的多通道（类型 / SDF / 颜色等）数据结构，支持可配置位深（8/16/32/64 位）与内存优化，恒定值通道不分配内存。
- **程序化生成**：内置多种生成器（噪声高度图、3D 噪声洞穴、平面、图节点生成器、脚本生成器等），并集成 FastNoise2 SIMD 噪声。
- **两种网格化方案**：
  - `VoxelMesherBlocky` / `VoxelMesherCubes`：Minecraft 风格的方块风地形。
  - `VoxelMesherTransvoxel`：基于 [Transvoxel](https://transvoxel.org/) 算法的平滑表面，支持多细节级别（LOD）过渡网格。
- **两种地形节点**：
  - `VoxelTerrain`：简单数据块网格，适合方块风与中等大小的体积。
  - `VoxelLodTerrain`：八叉树多 LOD 存储，可渲染极远距离，支持 GPU 计算着色器。
- **实时编辑**：通过 `VoxelTool` 编辑体素（挖洞、雕刻、融合简单形状），被修改的区域只重建对应网格。
- **持久化存储**：通过 `VoxelStream` 按数据块加载与保存，支持区域文件、SQLite、内存与脚本等多种实现。
- **多线程引擎**：`VoxelEngine` 提供带优先级调度的线程池，串行组处理 I/O 任务，后台完成生成与网格化，不阻塞主线程。
- **实例化**：`VoxelInstancer` 支持在生成的地形上散布植被、石块等实例。
- **其它能力**：`VoxelAStarGrid3D` 寻路、`VoxelBoxMover` 角色移动、修改器（Modifier）编辑、多玩家同步、MagicaVoxel `.vox` 导入等。

## 兼容性

- 本模块面向 **Godot 4.x**（`master` 分支与最新的稳定分支兼容，当前以 4.7 为基准）。
- 支持 Godot 支持的所有具备线程能力的平台。
- GPU 相关功能需要计算着色器支持（Forward+ 渲染器）。
- 详细平台限制与说明参见 [获取 Voxel Tools](doc/source/getting_the_module.md)。

## 获取方式

Voxel Tools 以 **Godot 模块**的形式打包进自定义构建与导出模板中，有以下获取途径：

1. **预编译版本**：从 [Releases](https://github.com/Voxel/godot_voxel/releases) 下载正式版（命名如 `Godot 4.x.x + Voxel Tools 1.x.x`）。
2. **开发版**：从 [GitHub Actions](https://github.com/Voxel/godot_voxel/actions) 下载最新构建（需要 GitHub 账号），包含最新的功能与修复。
3. **自行构建**：将本目录放入 Godot 源码树的 `godot/modules/voxel` 下，重新编译 Godot。完整步骤参见 [开发文档](doc/source/development.md)。

导出游戏时需要对应的自定义导出模板，否则场景无法正常打开。详细说明参见 [获取 Voxel Tools](doc/source/getting_the_module.md)。

## 快速开始

在场景中添加一个 `VoxelTerrain` 或 `VoxelLodTerrain` 节点，为其配置 `generator`（生成器）与 `mesher`（网格化器）资源，并放置至少一个 `VoxelViewer` 节点指定加载中心即可看到体素地形。详细的入门与脚本示例参见 [快速入门](doc/source/quick_start.md) 与 [脚本](doc/source/scripting.md)。

## 文档

完整的在线文档（中文）位于 `doc/source/` 目录，主要主题包括：

| 文档                                       | 说明                               |
| ------------------------------------------ | ---------------------------------- |
| [概述](doc/source/overview.md)             | 体素引擎的核心概念与组成           |
| [方块风地形](doc/source/blocky_terrain.md) | `VoxelMesherBlocky` 与方块风设置   |
| [平滑地形](doc/source/smooth_terrain.md)   | `VoxelMesherTransvoxel` 与平滑表面 |
| [生成器](doc/source/generators.md)         | 程序化生成器使用说明               |
| [图节点生成器](doc/source/graph_nodes.md)  | `VoxelGeneratorGraph` 可视化节点   |
| [数据流](doc/source/streams.md)            | `VoxelStream` 与持久化存储         |
| [实例化](doc/source/instancing.md)         | `VoxelInstancer` 与实例散布        |
| [多玩家](doc/source/multiplayer.md)        | 网络同步                           |
| [导航](doc/source/navigation.md)           | 寻路与 `VoxelAStarGrid3D`          |
| [性能](doc/source/performance.md)          | 性能分析与优化                     |
| [开发](doc/source/development.md)          | 构建、贡献、代码规范与调试         |
| [更新日志](doc/source/changelog.md)        | 版本变更历史                       |

- **类参考（API）**：`doc/classes/*.xml`（源文件）与 `doc/source/api/*.md`（生成的 Markdown）。
- **序列化格式规范**：`doc/source/specs/*.md`（区域文件、SQLite、数据块等格式）。

## 项目结构

| 目录          | 说明                                                            |
| ------------- | --------------------------------------------------------------- |
| `constants/`  | 整个引擎使用的常量与查找表                                      |
| `doc/`        | 文档（源文件、类参考、生成脚本与工具）                          |
| `edition/`    | 访问和修改体素的高级工具（`VoxelTool` 等）                      |
| `editor/`     | 编辑器专用代码（图编辑器、Voxel 导入器等）                      |
| `engine/`     | `VoxelEngine` 单例相关的全局内容                                |
| `generators/` | 程序化生成器（简单生成器与图生成器）                            |
| `meshers/`    | 体素网格化器（方块风 / 立方体 / Transvoxel）                    |
| `modifiers/`  | 修改器（Modifier）功能                                          |
| `shaders/`    | 引擎内部使用的着色器（文本形式与 C++ 形式）                     |
| `storage/`    | 体素存储与内存数据结构（`VoxelBuffer`、`VoxelData` 等）         |
| `streams/`    | 文件存储处理代码（区域文件、SQLite、Vox 等）                    |
| `terrain/`    | 所有节点（`VoxelTerrain`、`VoxelLodTerrain`、`VoxelViewer` 等） |
| `tests/`      | 内部单元测试（通过 SCons 选项编译）                             |
| `thirdparty/` | 第三方库（FastNoise2、LZ4、MeshOptimizer、SQLite 等）           |
| `util/`       | 通用工具函数与数据结构                                          |

## 构建与测试

构建模块、运行测试、Tracy 性能分析等开发相关内容参见 [开发文档](doc/source/development.md)。

测试在向 SCons 命令行传入 `voxel_tests=yes` 时编译，通过 `--run_voxel_tests` 命令行参数在 Godot 启动时运行。
