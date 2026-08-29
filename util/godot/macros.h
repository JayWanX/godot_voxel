#ifndef VOXEL_GODOT_MACROS_H
#define VOXEL_GODOT_MACROS_H

// 必须在全局作用域中使用。
#if defined(VOXEL_GODOT)
#define VOXEL_GODOT_FORWARD_DECLARE(m_class) m_class;
#endif

// 必须在全局作用域中使用。
#if defined(VOXEL_GODOT)
#define VOXEL_GODOT_NAMESPACE_BEGIN
#define VOXEL_GODOT_NAMESPACE_END
#endif

// TODO 等待修复：Godot 的 Variant() 在 JavaScript 和 OSX 构建下无法从 `size_t` 构造。
// 参见 https://github.com/godotengine/godot/issues/36690
#define VOXEL_SIZE_T_TO_VARIANT(s) static_cast<int64_t>(s)

#endif // VOXEL_GODOT_MACROS_H
