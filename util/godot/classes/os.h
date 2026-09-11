#ifndef VOXEL_GODOT_OS_H
#define VOXEL_GODOT_OS_H

#include <core/os/os.h>

namespace voxel::godot {

inline PackedStringArray get_command_line_arguments() {
	List<String> args_list = OS::get_singleton()->get_cmdline_args();
	PackedStringArray args;
	for (const String &arg : args_list) {
		args.push_back(arg);
	}
	return args;

}

} // namespace voxel::godot

#endif // VOXEL_GODOT_OS_H
