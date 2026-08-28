#ifndef VOXEL_GODOT_OS_H
#define VOXEL_GODOT_OS_H

#if defined(VOXEL_GODOT)
#include <core/os/os.h>
#endif

namespace voxel::godot {

inline PackedStringArray get_command_line_arguments() {
#if defined(VOXEL_GODOT)
	List<String> args_list = OS::get_singleton()->get_cmdline_args();
	PackedStringArray args;
	for (const String &arg : args_list) {
		args.push_back(arg);
	}
	return args;

#endif
}

} // namespace voxel::godot

#endif // VOXEL_GODOT_OS_H
