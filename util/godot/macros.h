#ifndef VOXEL_GODOT_MACROS_H
#define VOXEL_GODOT_MACROS_H

// TODO 等待修复：Godot 的 Variant() 在 JavaScript 和 OSX 构建下无法从 `size_t` 构造。
// 参见 https://github.com/godotengine/godot/issues/36690
#define VOXEL_SIZE_T_TO_VARIANT(s) static_cast<int64_t>(s)

#endif // VOXEL_GODOT_MACROS_H
