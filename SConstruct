#!/usr/bin/env python

# 这是以 GDExtension 方式构建此引擎的 SCons 入口脚本。
# 若要以引擎内置"模块"方式构建，参见 `SCsub`。

import os
import sys

import common
import voxel_version

# 动态库基础名称，最终产物如 libvoxel.<平台>.<目标>.<架构>.<后缀>。
LIB_NAME = "libvoxel"
# 产物输出目录，同时也是 Godot 插件(GDExtension)的存放位置。
BIN_FOLDER = "project/addons/voxel/bin"


def is_using_clang(env):
    # 通过编译器可执行文件名判断当前是否使用 Clang。
    return "clang" in os.path.basename(env["CC"])


def is_using_gcc(env):
    # 通过编译器可执行文件名判断当前是否使用 GCC。
    return "gcc" in os.path.basename(env["CC"])


# GodotCpp 未提供配置警告级别的辅助函数，这里自行处理。
def configure_warnings(env):
    if env.get("is_msvc", False):
        # 按编译器警告编号关闭特定警告，均为已知的不安全起见的处理。
        disabled_warnings = [
            "/wd4100",  # C4100（未引用的形参）：与多态配合不佳。
            "/wd4127",  # C4127（条件表达式为常量）
            "/wd4201",  # C4201（非标准的匿名 struct/union）：仅针对 C89。
            "/wd4244",  # C4244 C4245 C4267（缩窄转换）：此规模下难以避免。
            "/wd4245",
            "/wd4267",
            "/wd4305",  # C4305（截断）：double/float/real_t 间的转换，难以避免。
            "/wd4324",  # C4324（因对齐说明符导致结构体被填充）
            "/wd4514",  # C4514（未引用的内联函数被移除）
            "/wd4714",  # C4714（标记为 __forceinline 的函数未被内联）
            "/wd4820",  # C4820（构造后结构体尾部被填充）
        ]

        env.Append(CXXFLAGS=["/W3"])
        # C4458 相当于 -Wshadow，属于 /W4 级别，这里在默认 /W3 下也一并开启。
        env.AppendUnique(CXXFLAGS=["/w34458"] + disabled_warnings)

    else:  # GCC、Clang
        common_warnings = []

        if is_using_gcc(env):
            common_warnings += ["-Wshadow", "-Wno-misleading-indentation"]

        elif is_using_clang(env):
            common_warnings += ["-Wshadow-field-in-constructor", "-Wshadow-uncaptured-local"]
            # 为了将指针结构体放入 `Set`/`Map`，我们常为其实现 `operator<`。
            # 这里并不关心其排序是否可靠，因此关闭该警告。
            common_warnings += ["-Wno-ordered-compare-function-pointers"]

        env.AppendUnique(CXXFLAGS=["-Wall"] + common_warnings)

        # if env["werror"]:
        #     env.AppendUnique(CXXFLAGS=["-Werror"])

# 生成版本头文件（传入 False 表示以"GDExtension"版本号生成，见 voxel_version.py）。
voxel_version.generate_version_header(False)

# TODO 增强：由于环境是通过运行 GodotCpp 得到的，暂时不知道如何以 SCons 选项形式提供 GodotCpp 源码路径。
# env_vars.Add(PathVariable("godot_cpp_path", "GodotCpp 库源码路径", None, PathVariable.PathIsDir))
# TODO GDX：未来希望把 GodotCpp 放入 thirdparty/ 下。
godot_cpp_path = os.environ.get("GODOT_CPP_PATH", "D:/PROJETS/INFO/GODOT/Engine/godot_cpp_fork")

# 依赖 GodotCpp，复用其跨平台的编译配置。
# TODO GDX：确认这里配置量是否过多？
env = SConscript(godot_cpp_path + "/SConstruct", {"api_version": "4.5"})

# TODO GDX：传入本项目自定义变量会产生一条警告。
# "提示：传入的未知 SCons 变量将被忽略"
# 该提示由 GodotCpp 的 SConstruct 打印，它并不认识这些变量。
# 不过变量仍会被本项目 `SConstruct` 正常读取。
# 若需对这种检查做处理，应放在本文件，而非 GodotCpp 内。

# 注册本项目在命令行可用的 SCons 选项（voxel_sqlite、voxel_werror 等）。
common.register_scons_options(env, True)

env.Append(CPPDEFINES=[
	# 告诉与引擎无关的通用代码：我们正以 Godot"扩展"(GDExtension)形式编译。
	"VOXEL_GODOT_EXTENSION"
])

# 第三方库单独克隆一份环境，不为其开启警告。
thirdparty_env = env.Clone()

# 仅为本项目代码开启警告级别配置。
configure_warnings(env)

# 是否为编辑器构建目标，决定是否引入编辑器专属源码。
is_editor_build = (env["target"] == "editor")

# 收集需要编译的源文件路径。
sources = common.get_sources(env, is_editor_build)

if env["voxel_sqlite"]:
    # TODO 增强：SQLite 在 Godot 与 GodotCpp 两种目标里的集成方式不应重复。
    # 无法放进 common 脚本……
    # 因为在 `warnings=extra` 下 SQLite 会产生警告，因此必须仅对 SQLite 关闭警告。
    # 但这需要借助 Godot 构建系统专属代码，尚不清楚 GodotCpp 侧该用什么写法。
    # 理想情况是两种目标共享同一套代码。
    # FastNoise2 也存在同类问题！
    sources += [
        "thirdparty/sqlite/sqlite3.c"
    ]

sources += [
	"util/thread/godot_thread_helper.cpp",
]

sources += [
	# GodotCpp 未随附 RandomPCG，这里手动编译其源码。
	thirdparty_env.SharedObject("util/godot/core/pcg.cpp"),
	thirdparty_env.SharedObject("util/godot/core/random_pcg.cpp")
]

if is_editor_build:
    sources += [
        "util/godot/editor_scale.cpp"
    ]

    try:
        # 由 doc/classes/*.xml 生成类参考，供编辑器使用。
        doc_data = env.GodotCPPDocData("doc_data.gen.cpp", source=Glob("doc/classes/*.xml"))
        sources.append(doc_data)
    except AttributeError:
        # 目标 Godot 基线低于 4.3 时无此接口，则跳过类参考生成。
        print("Not including class reference as we're targeting a pre-4.3 baseline.")


if env["platform"] == "macos":
    # macOS 使用 framework 形态（目录内含同名可执行文件）。
    library = env.SharedLibrary(
        "{}/{}{}.framework/{}{}".format(
            BIN_FOLDER,
            LIB_NAME,
            env['suffix'],
            LIB_NAME,
            env['suffix']
        ),
        source = sources
    )
else:
    # 其余平台直接生成动态库文件，后缀由环境按平台目标拼接。
    library = env.SharedLibrary(
        "{}/{}{}{}".format(
            BIN_FOLDER,
            LIB_NAME,
            env["suffix"],
            env["SHLIBSUFFIX"]
        ),
        source = sources
    )

# 将动态库设置为默认构建目标，直接执行 scons 即生成该库。
Default(library)
