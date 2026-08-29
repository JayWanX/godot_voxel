#include "shader_material_pool.h"
#include "../errors.h"
#include "../profiling.h"
#include "classes/rendering_server.h"

namespace voxel::godot {

void ShaderMaterialPool::set_template(Ref<ShaderMaterial> tpl) {
	_template_material = tpl;
	_materials.clear();
	_shader_params_cache.clear();

	if (_template_material.is_valid()) {
		Ref<Shader> shader = _template_material->get_shader();

		if (shader.is_valid()) {
			StdVector<godot::ShaderParameterInfo> params;
			get_shader_parameter_list(shader->get_rid(), params);

			for (const godot::ShaderParameterInfo &pi : params) {
				_shader_params_cache.push_back(pi.name);
			}
		}
	}
}

Ref<ShaderMaterial> ShaderMaterialPool::get_template() const {
	return _template_material;
}

Ref<ShaderMaterial> ShaderMaterialPool::allocate() {
	if (_template_material.is_null() || _template_material->get_shader().is_null()) {
		return Ref<ShaderMaterial>();
	}
	if (!_materials.empty()) {
		Ref<ShaderMaterial> material = _materials.back();
		_materials.pop_back();
		return material;
	}
	VOXEL_PROFILE_SCOPE();
	Ref<ShaderMaterial> material;
	material.instantiate();
	material->set_shader(_template_material->get_shader());
	for (const StringName &name : _shader_params_cache) {
		// 注意，我不需要复制纹理。它们是共享的（至少来自模板材质的那部分是共享的）。
		material->set_shader_parameter(name, _template_material->get_shader_parameter(name));
	}
	return material;
}

void ShaderMaterialPool::recycle(Ref<ShaderMaterial> material) {
	VOXEL_ASSERT_RETURN(material.is_valid());
	VOXEL_ASSERT_RETURN(_template_material.is_valid());
	VOXEL_ASSERT_RETURN(material->get_shader() == _template_material->get_shader());
	_materials.push_back(material);
}

Span<const StringName> ShaderMaterialPool::get_cached_shader_uniforms() const {
	return to_span(_shader_params_cache);
}

void copy_shader_params(const ShaderMaterial &src, ShaderMaterial &dst, Span<const StringName> params) {
	// Ref<Shader> shader = src.get_shader();
	// VOXEL_ASSERT_RETURN(shader.is_valid());
	// 不使用 `Shader::get_param_list()`，因为它没有暴露给脚本/扩展 API，而且它会给每个参数名加上
	// `shader_params/` 前缀，这很慢且不适用于我们的场景。
	// 说实话 List 也很慢，我不知道为什么 Godot 用链表来存着色器参数列表。
	// List<PropertyInfo> properties;
	// RenderingServer::get_singleton()->shader_get_shader_uniform_list(shader->get_rid(), &properties);
	// for (const PropertyInfo &property : properties) {
	// 	dst.set_shader_uniform(property.name, src.get_shader_uniform(property.name));
	// }
	for (unsigned int i = 0; i < params.size(); ++i) {
		const StringName &name = params[i];
		dst.set_shader_parameter(name, src.get_shader_parameter(name));
	}
}

} // namespace voxel::godot
