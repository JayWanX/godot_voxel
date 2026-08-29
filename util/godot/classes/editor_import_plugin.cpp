#include "editor_import_plugin.h"
#include "../../errors.h"

namespace voxel::godot {

#if defined(VOXEL_GODOT)

String Voxel_EditorImportPlugin::get_importer_name() const {
	return _voxel_get_importer_name();
}

String Voxel_EditorImportPlugin::get_visible_name() const {
	return _voxel_get_visible_name();
}

void Voxel_EditorImportPlugin::get_recognized_extensions(List<String> *p_extensions) const {
	VOXEL_ASSERT_RETURN(p_extensions != nullptr);
	const PackedStringArray extensions = _voxel_get_recognized_extensions();
	for (const String &extension : extensions) {
		p_extensions->push_back(extension);
	}
}

String Voxel_EditorImportPlugin::get_preset_name(int p_idx) const {
	return _voxel_get_preset_name(p_idx);
}

int Voxel_EditorImportPlugin::get_preset_count() const {
	return _voxel_get_preset_count();
}

String Voxel_EditorImportPlugin::get_save_extension() const {
	return _voxel_get_save_extension();
}

String Voxel_EditorImportPlugin::get_resource_type() const {
	return _voxel_get_resource_type();
}

float Voxel_EditorImportPlugin::get_priority() const {
	return _voxel_get_priority();
}

int Voxel_EditorImportPlugin::get_import_order() const {
	return _voxel_get_import_order();
}

void Voxel_EditorImportPlugin::get_import_options(
		const String &p_path,
		List<ImportOption> *r_options,
		int p_preset
) const {
	VOXEL_ASSERT_RETURN(r_options != nullptr);
	StdVector<ImportOptionWrapper> options;
	_voxel_get_import_options(options, p_path, p_preset);
	for (const ImportOptionWrapper &option : options) {
		ImportOption opt(option.option, option.default_value);
		r_options->push_back(opt);
	}
}

bool Voxel_EditorImportPlugin::get_option_visibility(
		const String &p_path,
		const String &p_option,
		const HashMap<StringName, Variant> &p_options
) const {
	return _voxel_get_option_visibility(p_path, p_option, KeyValueWrapper{ p_options });
}

Error Voxel_EditorImportPlugin::import(
#if GODOT_VERSION_MAJOR == 4 && GODOT_VERSION_MINOR >= 4
		ResourceUID::ID p_source_id,
#endif
		const String &p_source_file,
		const String &p_save_path,
		const HashMap<StringName, Variant> &p_options,
		List<String> *r_platform_variants,
		List<String> *r_gen_files,
		Variant *r_metadata
) {
	VOXEL_ASSERT_RETURN_V(r_platform_variants != nullptr, ERR_BUG);
	VOXEL_ASSERT_RETURN_V(r_gen_files != nullptr, ERR_BUG);
	return _voxel_import(
			p_source_file,
			p_save_path,
			KeyValueWrapper{ p_options },
			StringListWrapper{ *r_platform_variants },
			StringListWrapper{ *r_gen_files }
	);
}

#if GODOT_VERSION_MAJOR == 4 && GODOT_VERSION_MINOR >= 3
bool Voxel_EditorImportPlugin::can_import_threaded() const {
	return _voxel_can_import_threaded();
}
#endif

#endif

String Voxel_EditorImportPlugin::_voxel_get_importer_name() const {
	VOXEL_PRINT_ERROR("Method is not implemented");
	return "<unnamed>";
}

String Voxel_EditorImportPlugin::_voxel_get_visible_name() const {
	VOXEL_PRINT_ERROR("Method is not implemented");
	return "<unnamed>";
}
PackedStringArray Voxel_EditorImportPlugin::_voxel_get_recognized_extensions() const {
	VOXEL_PRINT_ERROR("Method is not implemented");
	return PackedStringArray();
}

String Voxel_EditorImportPlugin::_voxel_get_preset_name(int p_idx) const {
	VOXEL_PRINT_ERROR("Method is not implemented");
	return "<unnamed>";
}

int Voxel_EditorImportPlugin::_voxel_get_preset_count() const {
	VOXEL_PRINT_ERROR("Method is not implemented");
	return 0;
}

String Voxel_EditorImportPlugin::_voxel_get_save_extension() const {
	VOXEL_PRINT_ERROR("Method is not implemented");
	return "";
}

String Voxel_EditorImportPlugin::_voxel_get_resource_type() const {
	VOXEL_PRINT_ERROR("Method is not implemented");
	return "";
}

float Voxel_EditorImportPlugin::_voxel_get_priority() const {
	return 1.0;
}

int Voxel_EditorImportPlugin::_voxel_get_import_order() const {
	return IMPORT_ORDER_DEFAULT;
}

void Voxel_EditorImportPlugin::_voxel_get_import_options(
		StdVector<ImportOptionWrapper> &p_out_options,
		const String &p_path,
		int p_preset_index
) const {
	VOXEL_PRINT_ERROR("Method is not implemented");
}

bool Voxel_EditorImportPlugin::_voxel_get_option_visibility(
		const String &p_path,
		const StringName &p_option_name,
		const KeyValueWrapper p_options
) const {
	VOXEL_PRINT_ERROR("Method is not implemented");
	return false;
}

Error Voxel_EditorImportPlugin::_voxel_import(
		const String &p_source_file,
		const String &p_save_path,
		const KeyValueWrapper p_options,
		StringListWrapper p_out_platform_variants,
		StringListWrapper p_out_gen_files
) const {
	VOXEL_PRINT_ERROR("Method is not implemented");
	return ERR_METHOD_NOT_FOUND;
}

bool Voxel_EditorImportPlugin::_voxel_can_import_threaded() const {
	// 根据文档
	// https://docs.godotengine.org/en/stable/classes/class_editorimportplugin.html#class-editorimportplugin-private-method-can-import-threaded
	return true;
}

} // namespace voxel::godot
