开发
=====================

本页将提供一些关于项目内部结构以及如何编译它的信息。
如果你想参与贡献，或者为你的游戏编写自定义 C++ 代码以获得更好的性能，这可能会很有用。

源代码可以在本仓库中找到。

!!! note
    本文档指的是模块开发。该项目主要是作为模块进行开发的。


构建
----------

### 作为模块构建<span id="building"></span>

#### 构建 Godot

1. 按照[官方指南](https://docs.godotengine.org/en/latest/development/compiling/index.html)下载并编译 [Godot 源码](https://github.com/godotengine/godot)。如果你想经常更新你的构建（推荐），请使用 Git 克隆仓库，而不是下载 zip 文件。
1. 确保选择合适的分支。如果你想要最新的开发版本，请使用 Godot 的 `master` 分支。如果你想要跟随最新稳定版的更稳定构建，请使用该版本的分支（例如 `4.0`）或特定版本标签（如 `4.0.2-stable`）。如果你想要 Godot 3，请使用 Godot 的 `3.x` 分支和模块的 `godot3.x` 分支（但该分支已不再维护）。
1. 在添加任何其他模块之前先构建 Godot，并确保它能生成可执行文件。
1. 运行位于 `godot/bin` 中新构建的可执行文件。查看 Help/About，确认版本字符串表明你运行的正是你想要的那个版本（例如 `3.2dev.custom_build.ee5ba3e`）。


#### 添加 Voxel Tools

1. 下载或克隆 Voxel Tools 仓库。如果你想让更新构建变得容易，请使用 Git 克隆该仓库（推荐）。
1. 默认情况下，模块的 `master` 分支应与 Godot 最新的稳定分支兼容。模块还有"快照"分支，它们是在特定 Godot 版本发布时创建的（例如 `godot4.0`），但不会再更新。
1. 将 Voxel Tools 目录放到你的 Godot 源码树中，即 `godot/modules` 目录内。
1. 将 Voxel Tools 文件夹重命名为 `voxel`。完成后，文件（例如 README.md）应位于 `godot/modules/voxel` 中。**这很重要！**
1. 重新构建 Godot，并确保它能生成可执行文件。
1. 测试你的构建是否具有 Voxel 支持：
	1. 运行你的新 Godot 构建。
	1. 创建一个新项目。
	1. 创建一个新的 3D 场景。
	1. 添加一个新节点，搜索 "Voxel"，看看是否出现 "VoxelTerrain"。如果是，说明你构建成功了。如果不是，请检查这些说明和构建日志，看是否漏掉了某一步或某个环节出了问题。


#### 更新你的构建

如果你克隆了 Godot 和 Voxel Tools，可以使用 git 更新本地代码。

1. 进入你的本地 Godot 源码目录 `godot` 并运行 `git pull`。它将从仓库下载所有更新并合并到你的本地源码中。
1. 进入 `godot/modules/voxel` 并运行 `git pull`。Git 将更新 Voxel Tools。
1. 重新构建 Godot。

!!! note
	由于你从两个由不同人开发的项目拉取更新，偶尔可能出现你的构建无法编译、项目无法打开、Voxel Tools 无法正常工作甚至使 Godot 崩溃的情况。为尽量减少停机时间，请保存你成功的构建。把它们移出构建文件夹，并用版本号重命名（例如 godot-3.2+ee5ba3e.exe）。这样，在 Godot 或 Voxel 开发者修复问题之前，你可以继续使用以前能正常工作的构建。通常所有人都希望发布到仓库的代码至少能编译，但事情总有意外。


#### Web 构建

为 Web 构建需要 WebAssembly 编译器 [Emscripten](https://emscripten.org/)。

然后在 SCons 命令行中使用相应的平台：
```
scons platform=web [etc...]
```

重要提示：你需要使用与 Godot 的 Web 导出模板所用相同的 Emscripten 版本。
TODO: 使用官方导出模板时，我们到哪里可以获得应使用的确切 Emscripten 版本？这似乎没有被记录。

当前的猜测：

- 4.0.11（[根据 Github Actions](https://github.com/godotengine/godot/blob/bf95b62586e31b8a3503f5903d7764d7c52bf2ab/.github/workflows/web_builds.yml#L12)）
- 4.0.20（在 Godot 4.6 中，[根据这篇文章](https://github.com/godotengine/godot-cpp/issues/1907#issuecomment-3790865190)）
- 如果你调试一个 Web 构建，它会打印在 Godot 的编辑器控制台中（即你需要先做一个没有扩展的基础游戏来尝试这个）
- 截至 2026/02/14，5.0.0 不适用

或者，你可以[构建你自己的 Godot 导出模板](https://docs.godotengine.org/en/stable/engine_details/development/compiling/compiling_for_web.html#compiling-for-the-web)，使用你选择的 Emscripten 版本。

可能还有一些其他注意事项。


贡献
--------------

要为项目做贡献，你需要使用 [Git](https://git-scm.com/) 克隆仓库，并在 Github 上创建你的分支，以便能够提交 Pull Requests。

### C++ 代码

建议阅读官方 [Godot 文档](https://docs.godotengine.org/en/stable/) 中的 **Engine Development**（引擎开发）章节。它解释了如何编译引擎、设置 IDE 以及如何制作自定义模块。

与 Voxel Tools 相关的代码规范，请参阅[代码规范](#code-guidelines)。

### 主文档

文档使用 Markdown 编写，使用 [Mkdocs](https://www.mkdocs.org/) 格式化，并在 [ReadTheDocs](https://readthedocs.org/) 上以网站形式提供。

要为这些主页面做贡献，请修改位于 `doc/docs` 文件夹下的 `.md` 文件，并在 Github 上提交 PR。

### API 文档

要为类参考（API）做贡献，你可以像处理普通 Godot 模块或核心类那样，编辑 `doc/classes` 下的 XML 文件。

更改 XML 文件后，可以使用 `doc/tools` 中的 `build.py` 脚本将其转换为对应的 Markdown 版本，使用此命令：
```
python build.py -a
```

`build.py` 还有其他参数可用于执行其他操作。如果你不带参数运行它，将打印帮助信息。

### 图表节点文档

[VoxelGeneratorGraph] 的图表节点目前不使用 Godot 类来表示。因此它们使用 XML 文件中的自有文档，工作流程与类类似。该 XML 文件是节点描述和类别的唯一事实来源，既用于编辑器中的显示，也用于在线文档。

可以通过使用以下参数运行 Godot，从引擎中存在的节点生成或更新 XML 文件：

`--voxel_doc_tool src dst`

其中 `src` 是原始 XML 的路径，`dst` 是写入更新后 XML 的路径。两个路径可以相同。该 XML 文件应已版本化，位于 `doc/graph_nodes.xml`。

然后可以使用该 XML 文件生成多种文档：

- 出现在网站上的 Markdown 文档
- 出现在编辑器中的 C++ 文档

两者都由在 `doc/tools/` 内运行脚本 `graph_nodes_doc.py` 生成。


分层
-------

### 主要分层

本模块有 3 个主要层次：

- Voxel：体素引擎。封装在 `voxel::` 命名空间中。
- Util：函数、辅助方法和数据结构的库，不依赖 Voxel。封装在 `voxel::` 命名空间中。
- Thirdparty：第三方库。

### 文件夹

模块分为多个文件夹，各自具有不同的依赖关系。因此，可以独立使用 `VoxelMesher`、`VoxelGenerator` 或 `VoxelStream`，而无需使用 `VoxelTerrain` 节点等。

目录            | 描述
-------------- | -------------------------------------------------------------------------------------------------------
constants/     | 整个引擎使用的常量和查找表。
doc/           | 包含文档
edition/       | 访问和修改体素的高级工具。可能依赖体素节点。
editor/        | 编辑器专用代码。也可能依赖体素节点。
engine/        | 包含与 VoxelEngine 单例相关的全局内容。依赖网格化器、数据流、存储，但不直接依赖节点。
generators/    | 程序化生成器。它们只依赖体素存储和数学。
meshers/       | 只依赖体素存储、数学和一些 Godot 图形 API。
misc/          | 各种脚本和配置文件，存放在这里以避免主文件夹杂乱。
modifiers/     | 与修改器功能相关的文件。
shaders/       | 引擎内部使用的着色器，既有文本形式也有格式化的 C++ 形式。
storage/       | 存储和内存数据结构。
streams/       | 文件存储处理代码。只依赖文件系统和存储。
terrain/       | 包含所有节点。依赖模块的其余部分，但编辑器专用部分除外。
tests/         | 包含测试。如果在构建脚本中启用并通过命令行指定，这些测试会在 Godot 启动时运行。
thirdparty/    | 第三方库，以源代码形式提供。它们被静态编译，因此 Godot 仍然是一个单一可执行文件。
util/          | 通用工具函数和数据结构。它们不依赖体素相关内容。

<p></p>

### 代码

除了文件夹结构所反映的分层之外，Godot 与本模块之间还有一个隐含的区别：如果一段代码不需要依赖 Godot，那么它往往就不会依赖 Godot。

例如，Transvoxel 的*实现*对 Godot 的依赖非常少。确实，它不在乎资源是什么，不需要 Variant，不需要绑定，不需要使用面向对象编程等。这就是为什么*网格化器资源*不包含*逻辑*，而是充当算法与其在 Godot 中用途之间的"桥梁"。

`VoxelBuffer` 也是如此：这个类实际上并不是一个完整的 Godot 对象。它比那要轻得多，因为它可能有数千个实例，甚至支持在栈上分配和移动。它被包装成一个对象暴露出来，仅用于脚本编写者必须与它交互的少数情况。

### 命名空间

命名空间                  | 描述
--------------------------|--------------
`voxel`                  | 通用用途，不一定与 Godot 相关
`voxel::math`            | 数学工具
`voxel::godot`           | 通用 Godot 工具
`voxel`           | 体素引擎
`voxel::ops`      | 体素编辑工具
`voxel::godot`    | 某些类有特定于 Godot 的包装器，以便暴露给脚本 API，它们位于此命名空间中，从而允许使用与 `voxel` 中相同的名称
`voxel::magica`   | MagicaVoxel 函数
`voxel::pg`       | 图表处理功能

可能还有更多较小的命名空间，可以在代码中记录。


测试
-------

测试不是强制性的，但如果有时间写新测试，最好还是写。完全覆盖并不是真正的目标，但在排查某些错误并确保它们不会再次出现时，测试很有用。

### 内部测试

模块在 `tests/` 文件夹中包含内部测试。目前没有使用测试框架，它们只是通过失败时是否打印错误来运行。

只有向 SCons 命令行传入 `voxel_tests=yes` 参数时，测试才会被编译。
启动 Godot 时传入 `--run_voxel_tests` 命令行参数，测试将在启动时运行。


线程
---------

模块使用多个后台线程处理体素。线程数量可以在项目设置中调整。

![线程示意图](images/threads_schema.webp)

只有一个线程池。这个线程池可以被赋予许多任务，并将它们分发给所有线程。因此可用的线程越多，大量任务完成得越快。任务也会按优先级排序，例如更新玩家附近的网格会先于生成 300 米外体素数据块的任务运行。

某些任务被安排在"串行"组中，这意味着同一时间只有一个任务运行（尽管任何线程都可以运行它们）。这是为了避免所有线程都锁定共享资源而陷入等待任务，从而阻塞全部线程。这用于 I/O，例如从磁盘加载和保存。

线程池位于 [VoxelEngine](api/VoxelEngine.md)。

注意：此任务系统不考虑"帧"。任务可以在任何时间运行，耗时可以小于或大于主线程的一帧。


代码规范<span id="code-guidelines"></span>
-----------------

### 语法

大多数情况下，使用 `clang-format` 并遵循大部分 Godot 约定。

- 类和结构体名称 `PascalCase`
- 常量、枚举和宏 `CAPSLOCK_CASE`
- 其他名称 `snake_case`
- 全局变量以 `g_` 为前缀
- 静态变量以 `s_` 为前缀
- 线程局部变量以 `tls_` 为前缀
- 参数在会产生歧义时以 `p_` 为前缀，推荐用于大型函数。
- 私有和受保护字段以 `_` 为前缀
- 一般来说，函数不应以 `_` 开头，除非出于 API 原因或另有特殊规则。
- 信号处理函数以 `on_` 为前缀，并且最好不要手动调用
- 枚举最好以它们的名称作为前缀。示例：`enum Type { TYPE_ONE, TYPE_TWO }`。对于暴露给 Godot 的枚举是强制性的。
- 左花括号放在行末，右花括号在下一行
- 永远不要省略花括号
- 二元运算符和控制流之间要有空格：`if (a + b == 42)`
- 使用制表符缩进
- 可以使用私有包装函数来适配 Godot 脚本 API，它们以 `_b_` 为前缀。
- 使用 Clang-format 来自动化大部分这些规则（C++ 项目根目录应包含一个配置文件）
- 优先只使用 `//` 注释
- 包装类的一些虚函数以 `_voxel_` 为前缀，以封装签名差异。

### 文件结构

- 使用 `.h` 作为头文件，`.cpp` 作为实现文件。
- 文件名使用 `snake_case`。
- 构造函数和析构函数放在顶部
- 公共 API 放在顶部，私有内容放在下面
- 绑定放在底部。
- 避免过长的行。首选的最大行长为 120 个字符。不要把太多操作放在同一行，使用局部变量。
- 如果是内部使用的类型或函数，定义在 `.cpp` 中可能比在头文件中更有利于编译时间。
- 当一行太长而无法容纳函数签名、函数调用或列表时，将元素按列书写。

### C++ 特性

- 不要使用 `auto`，除非类型无法表达或是很长的模板（如 STL 模板）。IDE 不是必然可用的（Github 审查和 diff）
- 适度使用 lambda 和函子是可以的。不要用 `std::function`。
- lambda 捕获应显式定义（尽量减少 `[=]` 或 `[&]` 的使用）
- 如果 STL 在性能上可度量地优于 Godot 替代方案，那么使用 STL 是可以的。
- 在声明旁边初始化变量
- 避免使用宏来定义逻辑或常量。优先使用 `static const`、`constexpr` 和 `inline` 函数。
- 对于初始化后不会改变的变量，优先添加 `const`
- 当存在显式替代方案时，不要利用布尔化。示例：使用 `if (a == nullptr)` 而不是 `if (!a)`
- 如果可能，避免使用像 `int a[42]` 这样的普通数组。调试器无法捕获它们的越界。优先使用诸如 `std::array`、`FixedArray` 和 `Span` 这样的包装器。
- 在整数大小重要的情况下使用 `uint32_t`、`uint16_t`、`uint8_t`。
- 如果可能，在头文件中使用前向声明而不是包含文件
- `#include` 你使用的东西，不要假设某个头文件会传递性地包含所需内容。这一点长期以来被广泛忽略，但新代码应尽量遵守。`util/godot` 微型头文件是例外。
- 不要在头文件中使用 `using namespace`（使用 `godot::` 时除外，它仅用于镜像代码库自身的命名空间约定）。
- `mutable` 必须**只**用于线程同步原语。不要用它配合"缓存数据"来让 getter 变成 `const`，因为这在多线程上下文中可能产生误导。
- 对不派生自 Godot `Object` 的类型，使用 `VOXEL_NEW` 和 `VOXEL_DELETE` 而不是 `new` 和 `delete`。这是为可能独立于 Godot、但在使用时需要被 Godot 默认分配器跟踪的代码而设计的。
- 使用 `VOXEL_ALLOC` 和 `VOXEL_FREE` 而不是 `malloc` 和 `free`。这是为可能独立于 Godot、但在使用时需要被 Godot 默认分配器跟踪的代码而设计的。
- 使用标准库容器时，优先使用 `util/containers/` 中的别名，例如 `StdVector`。它们使用 Godot 的分配函数，因此内存会被跟踪。

### 错误处理

- 不使用异常。
- 检查不变量，尽早失败。在调试模式下使用 `CRASH_COND` 或 `VOXEL_ASSERT`，以确保状态符合预期，即使它们不会立即造成伤害。
- 崩溃对用户不友好，因此在面向用户的代码（脚本）中，对可从错误中恢复的代码，或为防止触发内部断言，使用 `ERR_FAIL_COND` 或 `VOXEL_ASSERT_RETURN` 宏。
- 以 `VOXEL_` 为前缀的宏与 Godot 无关，可用于不太依赖 Godot 的区域以实现可移植性。

### 性能

在运行频繁、对性能至关重要的区域：

- 避免分配。使用内存池、`ObjectPool`、固定大小数组复用内存，或使用 `std::vector` 的容量。
- 避免 `virtual`、`Ref<T>`、`String`
- 不要调整 `PoolVectors` 或 `Vector<T>` 的大小，如果需要请一次性完成
- 注意哪些是线程安全的，哪些不是。本模块的一些主要区域在线程中工作。
- 将互斥锁的使用减到最少，并避免长时间锁定。
- 使用最适合长期最频繁使用方式的数据结构（通常会是数组、vector 或哈希映射）。
- 如果开销可以忽略不计，考虑跟踪调试统计信息。这有助于用户即使在发布构建中也能监控模块的性能表现。
- 在发布模式下对你的代码进行性能分析。本模块对 Tracy 友好，参见 `util/profiling.hpp`。
- 制作数据结构时注意对齐。例如，将小于 4 字节的字段打包，以便更好地利用空间

### Godot API

- 在性能重要的区域，使用最直接的 API 来完成任务。尤其是不要使用节点。参见 `RenderingServer` 和 `PhysicsServer`。
- 只有当一个函数使用起来安全，并能保证在相当长的时间内保持存在时，才将其暴露给脚本 API
- 对派生自 Godot `Object` 的类型，使用 `memnew` 和 `memdelete` 而不是 `new` 和 `delete`
- 不要留下随意的打印语句。对于详细模式，你也可以使用 `VOXEL_PRINT_VERBOSE()` 而不是 `print_verbose()`。
- 对于暴露给脚本的函数，如果不需要超过 2^31，即使它们永远不会为负，也使用 `int` 作为参数，这样用户出错时错误信息会更清晰
- 如果可能，尽量少用 Godot，使代码更具可移植性，有时也更快。某些区域使用 `util/` 中定义的自定义等价物。

编译涉及一些限制：

- 不要直接包含 Godot 头文件。使用 `util/godot` 中的头文件。

### 命名空间

预期的命名空间是 `voxel::` 作为主要命名空间，`voxel::` 用于体素相关内容。模块的不同部分可能还有其他命名空间。

注册的类也会被放入命名空间以防止冲突。命名空间不会出现在 Godot 的 ClassDB 中，因此体素相关类也以 `Voxel` 为前缀。其他更通用的类以 `VOXEL_` 为前缀。

如果一个注册的类需要与内部类同名，可以把它放在 `::godot` 子命名空间中。另一方面，内部类也可以加上 `Internal` 后缀。

### 版本控制

- 优先将逻辑变更的提交与代码格式化的提交分开
- 做 PR 时，优先把 WIP 提交合并（squash）


调试
----------

### 命令行参数

当你启动 Godot 时，默认会启动项目管理器。当你从中选择一个项目时，它会重新启动自身，但这会断开调试器的连接。因此建议使用命令行参数直接以你想要的模式和项目启动 Godot。

首先，确保 Godot 是在你项目的工作目录中启动的。

- 要调试游戏，不带参数启动 Godot，它将从主场景开始。
- 要调试项目的特定场景，将场景的相对路径作为命令行参数启动 Godot
- 要调试编辑器，添加 `-e` 参数。

在 Windows 上 VSCode `launch.json` 中选项设置示例：

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "(Windows) Launch",
            "type": "cppvsdbg", // For MSVC
            //"type": "cppdbg", // For GDB
            "request": "launch",
            "program": "${workspaceFolder}/bin/godot.windows.editor.dev.x86_64.exe", // Dev build (old target=debug)
            //"program": "${workspaceFolder}/bin/godot.windows.editor.x86_64.exe", // Non-dev build (old target=release_debug)
            "args": [
                "-v", // Verbose output
                
                //"-e", // Editor mode
                
                //"--debug-collisions",
                
                // Run a specific scene
                //"local_tests/sqlite/test_sqlite.tscn",
                //"local_tests/texturing/test_textured_terrain.tscn"
                //"local_tests/texturing/test_texturing.tscn"
            ],
            "stopAtEntry": false,
            "cwd": "D:/PROJETS/INFO/GODOT/Games/SolarSystem/Project",
            "environment": [],
            "visualizerFile": "${workspaceFolder}/modules/voxel/misc/voxel.natvis"
        }
    ]
}
```

### 错误断点

建议使用调试器，以便在发生错误或崩溃时获得更好的信息。可以打开 `core/error/error_macros.cpp`（在 Godot 4.x 中）并在 `_err_print_error` 中留下一个断点，这样每次发生错误时，调试器都会在那里中断，为你提供实时的调用栈和变量状态以供检查。

如果你调试编辑器，Godot 往往会打印更多非关键的错误，例如在脚本编辑器中犯的临时错误，或者在资源管理器坞中尝试索引资源文件并因某种原因失败。在这种情况下，你可能需要干净专用的测试项目，或者在启动后再设置断点。

### 调试打印

Godot：

```cpp
#include <core/string/print_string.h>

print_line(String("Hello {0}, my age is {1}").format(varray(name, age)));
```

非 Godot：

```cpp
#include "util/io/log.h"
#include "util/string/format.h"

println(format("Hello {}, my age is {}", name, age));
```

### 美化打印

Godot 和体素模块除了使用 STL 的容器类型外，都使用各自的容器类型。调试器通常无法检查它们。例如，Godot 的 `Vector<T>` 类与 `std::vector<T>` 类似，但调试器无法让你检查其中的内容。

要解决这个问题，通常可以给调试器提供一个文件，列出检查这些类型的特殊模式，使其更友好。

在 VSCode 中，cpp-tools 扩展支持 Natvis 文件。Godot 自带一个这样的文件，位于 `platform/windows/godot.natvis`。要为 Godot 类型获得美化打印，请在 `launch.json` 文件中添加以下行：
```json
            "visualizerFile": "${workspaceFolder}/platform/windows/godot.natvis"
```

遗憾的是，目前只能提供一个文件。[有一个 issue](https://github.com/Microsoft/vscode-cpptools/issues/925) 请求支持多个文件。
这意味着如果你还想让体素模块的结构获得美化打印，就必须把 natvis 路径替换为以下内容：
```json
            "visualizerFile": "${workspaceFolder}/modules/voxel/misc/voxel.natvis"
```


使用 Tracy 进行性能分析
-------------------

本模块包含用于对特定代码段进行性能分析的宏。默认情况下，这些宏展开为 [Tracy Profiler](https://github.com/wolfpld/tracy) 的 zone。它可以检查代码运行需要多长时间，并以时间线的形式显示。

它已用 [Tracy 0.10](https://github.com/wolfpld/tracy/releases/tag/v0.10) 测试过，但更新的版本很可能也能工作。

![Tracy 截图](images/tracy.webp)

[Godot 文档](https://docs.godotengine.org/en/stable/engine_details/development/debugging/using_cpp_profilers.html#doc-using-cpp-profilers) 中也提到了其他性能分析器。它们分析所有内容，似乎基于 CPU 采样，而 Tracy 是一种插桩式分析器，可在时间线上提供具体的、实时的结果。

一个典型的工作流程是：启动 Tracy，开始连接，然后启动游戏，游戏将建立连接并记录所有事件。Tracy 也可以在游戏之后再启动并连接，在这种情况下数据会在游戏内累积。

### 启用 Tracy 的构建

!!! note
    这些构建是实验性的，当 Godot 4.6 发布并提供更好的 Tracy 支持后，它们将被重新设计。

在 Github Actions 上提供集成了该模块和 Tracy 的 Godot 构建，适用于 Windows。要下载的文件名称中会包含 `tracy`。注意，你需要一个 Github 帐户才能下载。

!!! warning
    这些构建在启动时立即开始记录数据。*包括项目管理器和编辑器*。这会占用大量内存（仅启动编辑器就 2 Gb）。如果你只想对游戏进行性能分析，请[使用命令行](https://docs.godotengine.org/en/stable/tutorials/editor/command_line_tutorial.html#command-line-tutorial)直接使用你的游戏启动该 Godot 构建，或者直接把可执行文件放到你项目的根目录并启动它。

除此之外，你必须自己编译以获得 Tracy 支持。

完成性能分析后，别忘了切换回正常构建，否则性能分析数据会在内存中累积而不会被取走。

### 添加 Tracy

#### 在 Godot 4.6 及更高版本中

从 Godot 4.6 开始，Tracy 作为 Godot 构建系统的一个选项得到原生支持。你需要用额外的 SCons 选项重新编译引擎：`profiler=tracy profiler_path=<path to the public folder in Tracy>`。

#### 在 Godot 4.5 及更早版本中

要添加 Tracy 支持，请将它克隆到 `thirdparty/tracy` 下（Godot 的 `thirdparty` 文件夹，不是模块的）。
然后在 SCons 命令行中包含 `tracy=yes` 来编译引擎。

这样一来，其中一些工作实际上是在体素模块的构建脚本中完成的。

!!! note
    Tracy 有一个帧标记（frame mark）的概念，通常由应用程序提供，用于告诉分析器每帧何时开始。Godot 没有提供钩子供我们在正确的时间插入该调用，因此帧标记被硬塞进了 `VoxelEngine` 的处理函数中。这样可以在时间线上看到主线程的帧，但它们可能与其真实的开始时间存在偏移。


### 如何添加性能分析作用域

如果现有的插桩还不够，你可以通过编辑代码来添加更多。

一个性能分析作用域界定了一段代码。它记录之前的时间和之后的时间，并将其记录到时间线中。在 C++ 中，我们可以使用 RAII 在退出函数或代码块时自动关闭作用域，因此通常只需要在被分析的区域开头放置一个宏。模块中已经有很多这样的宏，但如果你需要更多洞察，也可以添加你自己的。

这些宏与具体分析器无关，因此如果你想使用其他分析器，可以修改它们。

你需要包含 `util/profiling.h` 才能访问这些宏。

对整个函数进行性能分析：
```cpp
void some_function() {
    VOXEL_PROFILE_SCOPE();
    //……
}
```

对函数的一部分进行性能分析：
```cpp
void some_function() {
    // 一些代码……

    // 可以是 `if`、`for`、`while`，或像这里一样的简单代码块
    {
        VOXEL_PROFILE_SCOPE();
        // 被分析的代码……
    }

    //……
}
```

默认情况下，作用域采用函数名，或文件名和行号，但你也可以使用 `VOXEL_PROFILE_SCOPE_NAMED("Hello")` 显式指定名称。只支持编译期字符串，不要使用 `String` 或 `std::string`。

也可以绘制数值，使它们也显示在时间线上：

```cpp
void process_every_frame() {
    // 一些代码……

    VOXEL_PROFILE_PLOT("Bunnies", bunnies.size());
}
```

编译标志和宏
-------------------------------

模块有一些预处理器宏，可以定义它们来关闭部分代码的编译。
其中一些可以通过 SCons 命令行参数指定。

### 特性<span id="features"></span>

默认情况下，除非另有指定，所有特性都是启用的。要关闭某个特性，请在 SCons 命令行中指定 `flag_name=no`。

SCons 标志              | C++ 宏                       | 描述
------------------------ | ------------------------------- | -------------------------------------------------------------
`voxel_fast_noise_2`     | `VOXEL_ENABLE_FAST_NOISE_2`     | 使用 FastNoise2 集成支持 SIMD CPU 噪声。它是可选的，以防在某些编译器或平台（仅 x86）上造成问题。
`voxel_tests`            | `VOXEL_TESTS`                   | 单元测试。如果传入 `--run_voxel_tests` 命令行参数，或调用 `VoxelEngine.run_tests()`，它们将在启动时运行。
`voxel_smooth_meshing`   | `VOXEL_ENABLE_SMOOTH_MESHING`   | 平滑体素网格化器及一些相关特性。关闭它也会关闭依赖它的修改器。
`voxel_modifiers`        | `VOXEL_ENABLE_MODIFIERS`        | `VoxelModifier` 实验性特性支持。
`voxel_sqlite`           | `VOXEL_ENABLE_SQLITE`           | `VoxelStreamSQLite`，它还捆绑了 SQLite3 库。
`voxel_instancer`        | `VOXEL_ENABLE_INSTANCER`        | `VoxelInstancer` 支持
`voxel_gpu`              | `VOXEL_ENABLE_GPU`              | GPU 计算支持（即使关闭此选项，GPU 仍会被使用，只是不使用计算着色器）
`voxel_basic_generators` | `VOXEL_ENABLE_BASIC_GENERATORS` | 包含可用于测试的基础生成器。
`voxel_mesh_sdf`         | `VOXEL_ENABLE_MESH_SDF`         | 支持使用 `VoxelMeshSDF` 进行体素化网格。关闭它也会关闭依赖它的修改器。
`voxel_vox`              | `VOXEL_ENABLE_VOX`              | 加载 `.vox` MagicaVoxel 文件的能力。

!!! warning
    随着时间的推移测试这些标志的组合非常耗时，而且大多数人都不会编译自定义构建来关闭它们。因此项目有可能无法用某个特定子集编译。如果你发现问题，可以报告它和/或提交 PR 来修复。


### 其他宏

- `MESHOPTIMIZER_VOXEL_WRAP_LIBRARY_IN_NAMESPACE`：必须定义此宏以防止与 Godot 自带的 MeshOptimizer 版本冲突。参见 [https://github.com/zeux/meshoptimizer/issues/311#issuecomment-955750624](https://github.com/zeux/meshoptimizer/issues/311#issuecomment-955750624)


着色器
---------

模块为其部分特性包含着色器，主要是计算着色器。它们位于 `shaders/dev/` 文件夹下。

`shaders/dev` 包含一个 Godot 项目。该项目的主要目的是快速测试着色器是否能正确编译，并最终用简单的场景和 GDScript 代码测试它们。

着色器的编写方式有几种：

- Plain：普通着色器，将按原样使用。
- Templates：这些包含 `<PLACEHOLDER>` 段，引擎将用生成的代码替换它们。这些段内的代码将被替换，并且仅用于让着色器在测试项目中能够编译。
- Snippets：这些包含 `<SNIPPET>` 段，它们将被插入到模板或其他生成的代码中。这些段外的代码不会被使用，并且仅用于让着色器在测试项目中能够编译。

以模块形式编译时，随附外部文件很不方便，因此它们被直接嵌入到 C++ 中，方式与 Godot 类似。可以执行一个脚本来更新那些生成的文件。你必须在 `shaders/` 文件夹内打开命令行并运行 `python text2cpp.py`。

目前，生成着色器的 C++ 代码与这些着色器的内容交织在一起。例如，代码生成中的 C++ 字符串可能包含 GLSL 文件中出现的变量名，因此你最好同时打开两者以理解上下文。


从另一个模块使用本模块
----------------------------------------

直接在 Godot 中编写自定义 C++ 模块是直接访问 Godot 和体素引擎特性的最简单方式。如果你想创建自定义生成器、网格化器、数据流，或只是使用模块的组件而不直接修改模块，你也可以这样做。

你可以通过在 include 中使用 `modules/voxel/` 来包含体素模块的文件：

```cpp
#include <modules/voxel/storage/voxel_buffer.h>
```

!!! note
    虽然 API 文档涵盖了你在 C++ 中也会找到的函数，但内部代码有时只有注释。它们没有在外部记录，也没有计划这样做。建议查看头文件，了解暴露了什么、使用哪些命名空间等。你也可以阅读 `.cpp` 文件中的现有代码，看看某些东西是如何使用的。
