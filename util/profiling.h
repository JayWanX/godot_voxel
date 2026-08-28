#ifndef VOXEL_PROFILING_H
#define VOXEL_PROFILING_H

#if defined(VOXEL_GODOT)
#include "godot/core/version.h"

#if GODOT_VERSION_MAJOR >= 4 && GODOT_VERSION_MINOR >= 6
// Godot supports Tracy, but doesn't define global preprocessor symbols to let us detect it, instead it defines them in
// generated headers. This is to avoid recompiling the whole engine, as not every file uses it. But then, we have to
// include that header, regardless of profiling being enabled or not.
#include "core/profiling/profiling.h"
#endif

#endif

#if defined(TRACY_ENABLE)

#include <tracy/Tracy.hpp>

#define VOXEL_PROFILER_ENABLED

#define VOXEL_PROFILE_SCOPE() ZoneScoped
#define VOXEL_PROFILE_SCOPE_NAMED(name) ZoneScopedN(name)

#ifdef GODOT_USE_TRACY
#define VOXEL_PROFILE_MARK_FRAME()
#else
// Only define our own frame tracking when Tracy is enabled from our own integration instead of Godot's.
#define VOXEL_PROFILE_MARK_FRAME() FrameMark
#endif

#define VOXEL_PROFILE_SET_THREAD_NAME(name) tracy::SetThreadName(name)
#define VOXEL_PROFILE_PLOT(name, number) TracyPlot(name, number)
#define VOXEL_PROFILE_MESSAGE(message) TracyMessageL(message)
#define VOXEL_PROFILE_MESSAGE_DYN(message, size) TracyMessage(message, size)

#else

#define VOXEL_PROFILE_SCOPE()
// Name must be static const char* (usually string litteral)
#define VOXEL_PROFILE_SCOPE_NAMED(name)
#define VOXEL_PROFILE_MARK_FRAME()
#define VOXEL_PROFILE_PLOT(name, number)
#define VOXEL_PROFILE_MESSAGE(message)
// Name must be const char*. An internal copy will be made so it can be temporary.
// Size does not include the terminating character.
#define VOXEL_PROFILE_MESSAGE_DYN(message, size)
// Name must be const char*. An internal copy will be made so it can be temporary.
#define VOXEL_PROFILE_SET_THREAD_NAME(name)

#endif

/*
To add Tracy support, clone it under thirdparty/tracy, and add the following lines in core/SCsub:

```
# tracy library
env.Append(CPPDEFINES="TRACY_ENABLE")
env_thirdparty.Append(CPPDEFINES="TRACY_ENABLE")
env_thirdparty.add_source_files(env.core_sources, ["#thirdparty/tracy/TracyClient.cpp"])
```
*/

#endif // VOXEL_PROFILING_H
