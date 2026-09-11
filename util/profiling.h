#include <core/version.h>
#ifndef VOXEL_PROFILING_H
#define VOXEL_PROFILING_H


// Godot 支持 Tracy，但没有定义全局预处理宏来让我们检测它，而是在
// 生成的头文件中定义。这是为了避免重新编译整个引擎，因为并非每个文件都用到它。但这样一来，我们不得不
// 包含那个头文件，无论是否启用了性能分析。


#if defined(TRACY_ENABLE)

#include <tracy/Tracy.hpp>

#define VOXEL_PROFILER_ENABLED

#define VOXEL_PROFILE_SCOPE() ZoneScoped
#define VOXEL_PROFILE_SCOPE_NAMED(name) ZoneScopedN(name)

#ifdef GODOT_USE_TRACY
#define VOXEL_PROFILE_MARK_FRAME()
#else
// 仅当 Tracy 由我们自己的集成（而非 Godot 的）启用时，才定义我们自己的帧跟踪。
#define VOXEL_PROFILE_MARK_FRAME() FrameMark
#endif

#define VOXEL_PROFILE_SET_THREAD_NAME(name) tracy::SetThreadName(name)
#define VOXEL_PROFILE_PLOT(name, number) TracyPlot(name, number)
#define VOXEL_PROFILE_MESSAGE(message) TracyMessageL(message)
#define VOXEL_PROFILE_MESSAGE_DYN(message, size) TracyMessage(message, size)

#else

#define VOXEL_PROFILE_SCOPE()
// 名称必须是 static const char*（通常是字符串字面量）
#define VOXEL_PROFILE_SCOPE_NAMED(name)
#define VOXEL_PROFILE_MARK_FRAME()
#define VOXEL_PROFILE_PLOT(name, number)
#define VOXEL_PROFILE_MESSAGE(message)
// 名称必须是 const char*。内部会创建一份副本，因此它可以作为临时值使用。
// 大小不包含终止字符。
#define VOXEL_PROFILE_MESSAGE_DYN(message, size)
// 名称必须是 const char*。内部会创建一份副本，因此它可以作为临时值使用。
#define VOXEL_PROFILE_SET_THREAD_NAME(name)

#endif

/*
要添加 Tracy 支持，请将其克隆到 thirdparty/tracy 下，并在 core/SCsub 中添加以下行：

```
# tracy library
env.Append(CPPDEFINES="TRACY_ENABLE")
env_thirdparty.Append(CPPDEFINES="TRACY_ENABLE")
env_thirdparty.add_source_files(env.core_sources, ["#thirdparty/tracy/TracyClient.cpp"])
```
*/

#endif // VOXEL_PROFILING_H
