#ifndef VOXEL_GODOT_RESOURCE_H
#define VOXEL_GODOT_RESOURCE_H

#include <core/io/resource.h>

namespace voxel::godot {

// Godot 目前还没有针对资源的配置警告。
// 但当我们添加它们并且警告是嵌套的时，当警告出现在场景树中，很难把它们放到合适的上下文中。
// 这个辅助函数会前置上下文，这样我们就可以在资源中嵌套配置警告。
// 上下文是一个可调用的模板，返回可转换为 String 的内容，所以它只在
// 确实存在警告时才求值。
template <typename TResource, typename FContext>
inline void get_resource_configuration_warnings(
		const TResource &resource,
		PackedStringArray &warnings,
		FContext get_context_string_func
) {
	const int prev_size = warnings.size();

	// 这个方法是我们的，不是 Godot 的。
	resource.get_configuration_warnings(warnings);

	const int current_size = warnings.size();
	if (current_size != prev_size) {
		// 添加了新的警告
		String context = get_context_string_func();
		for (int i = prev_size; i < current_size; ++i) {
			const String w = context + warnings[i];
			warnings.write[i] = w;
		}
	}
}

} // namespace voxel::godot

#endif // VOXEL_GODOT_RESOURCE_H
