#ifndef VOXEL_SHADER_MATERIAL_POOL_H
#define VOXEL_SHADER_MATERIAL_POOL_H

#include "../containers/span.h"
#include "../containers/std_vector.h"
#include "classes/shader_material.h"

namespace voxel::godot {

// 池化同一 ShaderMaterial 的众多副本的原因：
// - 在编辑器中，即使材质不可编辑，Shader 的 `changed` 信号也会被连接，导致 shader 维护一个
//   庞大的"监听"材质连接列表，使插入/移除变得极慢。
// - 通用的 `Resource.duplicate()` 行为极慢。95% 的时间花在非设置 shader 参数上
//   （获取链表形式的属性列表，其中很多要回退到"生成的"属性、分配内存、
//   用 variant `set` 函数解析赋值……）。
// - 仅分配对象本身就需要一点时间
// TODO 下一步可以做一个轻量封装，直接使用 RenderingServer？
class ShaderMaterialPool {
public:
	void set_template(Ref<ShaderMaterial> tpl);
	Ref<ShaderMaterial> get_template() const;

	Ref<ShaderMaterial> allocate();
	void recycle(Ref<ShaderMaterial> material);

	// 材质本身也有缓存，但这个是更直接的
	Span<const StringName> get_cached_shader_uniforms() const;

private:
	Ref<ShaderMaterial> _template_material;
	StdVector<StringName> _shader_params_cache;
	StdVector<Ref<ShaderMaterial>> _materials;
};

void copy_shader_params(const ShaderMaterial &src, ShaderMaterial &dst, Span<const StringName> params);

} // namespace voxel::godot

#endif // VOXEL_SHADER_MATERIAL_POOL_H
