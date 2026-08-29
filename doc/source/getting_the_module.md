获取 Voxel Tools
=====================

本项目是一个 [模块](#module)，会被打包进 Godot Engine 的自定义构建和自定义导出模板中。

模块<span id="module"></span>
--------

以下章节适用于 Voxel Tools 的模块版。

### 预编译版本

#### 正式版

版本可在 GitHub Releases 页面获取。
模块版本通常以 `Godot 4.x.x + Voxel Tools 1.x.x` 为前缀命名。

本项目遵循持续开发周期，因此“正式版”只是开发版本的快照。由于从 Github Actions 下载最新开发版本需要 Github 账号，为了方便起见发布了正式版。

引擎非常庞大且面向众多平台，相比之下我们的模块很小，而且我们没有专用的构建容器，因此并非所有编辑器与导出模板的组合都可用。你可以在主要桌面平台上用编辑器开发和测试游戏，但如果某个平台/选项组合未提供，你就需要自行构建。

#### 开发版

开发版包含最新的功能和错误修复（尽管也可能包含未知的 bug）。它们可在 Github Actions 上获取。

!!! note
	你需要一个 Github 账号才能从 Github Actions 下载构建产物。否则链接将无法使用。

然后点击带有绿色勾选标记的最新成功构建：

![构建列表截图，最新的成功构建已用绿色圆圈标出](images/ci_builds_latest_link.webp)

然后滚动到底部，你应该能看到下载链接：

![Github Actions 截图](images/github_actions_windows_artifacts.webp)

如果有多个可下载的构建产物，名称中包含 `editor` 的即为编辑器版本。

这些版本对应于更新日志中描述的 `master` 版本。
除非另有说明，它们使用 Godot 最新的稳定版本分支（例如撰写本文时的 `4.7`）而非 `master` 构建。
每次提交推送到主分支时都会构建新版本，其他开发者提交 Pull Request 时也会构建，所以要留意你选择的是哪一个。

!!! note
	Mono 构建（C# 支持）也有提供，但它们不再开箱即用。更多信息请参阅 [C# 支持](#c-and-module-defined-classes)。


### 自行构建

参见 [作为模块构建](development.md#building)


### 导出

!!! note 
	如果你想将游戏导出为可执行文件，则需要阅读本节。

#### 支持的平台

本模块支持 Godot 所支持的所有具备线程能力的平台。

某些功能可能并非始终可用：

- 使用 FastNoise2 0.10 的 SIMD 噪声只能从 x86 CPU 中受益，否则会回退到标量计算，速度较慢
- GPU 功能需要计算着色器支持（Forward+ 渲染器）
- Web 导出的线程在部分浏览器上可能无法工作

#### 获取模板

在 Godot Engine 中，将游戏导出为目标平台的可执行文件需要一个“模板”。模板是不包含编辑器内容的 Godot Engine 优化构建。Godot 会将你的项目文件与该模板组合，生成最终的可执行文件。

如果你只下载包含模块的 Godot 编辑器，它可以让你开发和测试游戏，但如果你不做任何其他设置就进行导出，Godot 会尝试使用不包含该模块的原版模板。因此，某些场景将无法打开。

如前文所述，你可以为某些平台和配置获取预构建的模板。

如果你的平台没有可用的预构建模板，你可以自行构建。这与使用模块构建 Godot 相同，只是选项不同。更多细节请参阅 [Godot 文档](https://docs.godotengine.org/en/latest/development/compiling/index.html) 中你所针对平台下的“构建导出模板”分类。

#### 使用模板

拿到模板构建后，在导出配置中告诉 Godot 使用它。在“Custom Template”部分填写自定义模板的路径：

![Godot 导出配置窗口截图，为 Windows 指定了自定义模板](images/export_template_window.webp)


C# 支持
--------------

在 Godot 中，C# 有点特殊，尤其是在插件方面。它的配置需要额外的工作。

### 模块

可用的构建过去可以在 Github Actions 上获取（称为“Mono Builds”）。不幸的是，Godot 4 改用 Nuget 包管理器来集成 C#，这使得模块开发者更难提供开箱即用的可执行文件，对用户来说也更难：

- 当你在 Godot C# 中创建项目时，它会从 Nuget 获取“原版”Godot SDK，但它仅适用于官方稳定版本，因此你无法使用基于 Godot 最新开发版本的引擎 CI 构建。
- 模块会向 API 添加官方 SDK 中不存在的新类。这需要为你想使用的每种模块组合创建 SDK 并上传到 Nuget，这并不实际。
- 你可以回退到 Nuget 上可用的最新官方 SDK，但要访问模块 API，就必须在代码中使用 `obj.Get(string)`、`Set(string)` 和 `Call(string, args)` 之类的变通方法，这些方法难以使用、效率低下且极难维护。

要获得可用的版本，你必须自行生成 SDK，并使用本地 Nuget 仓库代替官方仓库。请按照 [Godot C# 文档](https://docs.godotengine.org/en/stable/engine_details/development/compiling/compiling_with_dotnet.html) 中描述的步骤操作。


### 所有权检查

Voxel Tools 在运行某些虚方法（如自定义生成器）时会做一些健全性检查。这些检查涉及引用计数。然而，这在 C# 中无法正常工作，因为 C# 是垃圾回收语言：超出作用域的 `RefCounted` 对象在垃圾回收器运行之前并不会真正释放。这可能会导致误报错误。

你可以在项目设置中关闭这些检查：`voxel/ownership_checks`


### C# 与模块定义的类<span id="c-and-module-defined-classes"></span>

目前，对用 C++ 实现的扩展的 C# 支持尚未明确定义。

问题在于，C# 可以使用的 Godot API（俗称“glue”）是在 Godot 本身构建时生成的，因此它只包含核心的原版类。其他一切（扩展、GDScript）都不在其中，因此需要使用 Godot 的反射方法。
理论上，C++ 扩展可以提供强类型 API，因为它们拥有可绑定到 C# 的函数指针，但 Godot 至今尚未实现这一点。

因此，从 C# 与扩展定义的类交互的唯一方式是使用以下方法：

- 调用方法：[call](https://docs.godotengine.org/en/stable/classes/class_object.html#class-object-method-call)
- 获取或设置属性：[get](https://docs.godotengine.org/en/stable/classes/class_object.html#class-object-method-get) 和 [set](https://docs.godotengine.org/en/stable/classes/class_object.html#class-object-method-set) 
- 创建新实例：[ClassDB.instantiate](https://docs.godotengine.org/en/stable/classes/class_classdb.html#class-classdb-method-instantiate)。

```cs
// /!\ 伪代码，未测试
GodotObject model = Godot.ClassDB.Instantiate("VoxelBlockyModelCube");
model.Call("set_tile", Godot.ClassDB.ClassGetIntegerConstant("VoxelBlockyModel", "SIDE_NEGATIVE_X"), new Vector2I(1, 1))
model.Set("atlas_size_in_tiles", new Vector2I(8, 8));
```

然而，这种方法开销很大，影响性能，而且使用起来很繁琐。

Godot 文档中的[跨语言脚本](https://docs.godotengine.org/en/stable/tutorials/scripting/cross_language_scripting.html)也介绍了类似的情况。
