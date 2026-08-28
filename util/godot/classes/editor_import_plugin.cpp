#include "editor_import_plugin.h"
#include "../../errors.h"

namespace voxel::godot {

#if defined(VOXEL_GODOT)

String VOXEL_EditorImportPlugin::get_importer_name() const {
	return _voxel_get_importer_name();
}

String VOXEL_EditorImportPlugin::get_visible_name() const {
	return _voxel_get_visible_name();
}

void VOXEL_EditorImportPlugin::get_recognized_extensions(List<String> *p_extensions) const {
	VOXEL_ASSERT_RETURN(p_extensions != nullptr);
	const PackedStringArray extensions = _voxel_get_recognized_extensions();
	for (const String &extension : extensions) {
		p_extensions->push_back(extension);
	}
}

String VOXEL_EditorImportPlugin::get_preset_name(int p_idx) const {
	return _voxel_get_preset_name(p_idx);
}

int VOXEL_EditorImportPlugin::get_preset_count() const {
	return _voxel_get_preset_count();
}

String VOXEL_EditorImportPlugin::get_save_extension() const {
	return _voxel_get_save_extension();
}

String VOXEL_EditorImportPlugin::get_resource_type() const {
	return _voxel_get_resource_type();
}

float VOXEL_EditorImportPlugin::get_priority() const {
	return _voxel_get_priority();
}

int VOXEL_EditorImportPlugin::get_import_order() const {
	return _voxel_get_import_order();
}

void VOXEL_EditorImportPlugin::get_import_options(
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

bool VOXEL_EditorImportPlugin::get_option_visibility(
		const String &p_path,
		const String &p_option,
		const HashMap<StringName, Variant> &p_options
) const {
	return _voxel_get_option_visibility(p_path, p_option, KeyValueWrapper{ p_options });
}

Error VOXEL_EditorImportPlugin::import(
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
bool VOXEL_EditorImportPlugin::can_import_threaded() const {
	return _voxel_can_import_threaded();
}
#endif

#elif defined(VOXEL_GODOT_EXTENSION)

String VOXEL_EditorImportPlugin::_get_importer_name() const {
	return _voxel_get_importer_name();
}

String VOXEL_EditorImportPlugin::_get_visible_name() const {
	return _voxel_get_visible_name();
}

PackedStringArray VOXEL_EditorImportPlugin::_get_recognized_extensions() const {
	return _voxel_get_recognized_extensions();
}

String VOXEL_EditorImportPlugin::_get_preset_name(int32_t p_idx) const {
	return _voxel_get_preset_name(p_idx);
}

int32_t VOXEL_EditorImportPlugin::_get_preset_count() const {
	return _voxel_get_preset_count();
}

String VOXEL_EditorImportPlugin::_get_save_extension() const {
	return _voxel_get_save_extension();
}

String VOXEL_EditorImportPlugin::_get_resource_type() const {
	return _voxel_get_resource_type();
}

float VOXEL_EditorImportPlugin::_get_priority() const {
	return _voxel_get_priority();
}

int32_t VOXEL_EditorImportPlugin::_get_import_order() const {
	return _voxel_get_import_order();
}

TypedArray<Dictionary> VOXEL_EditorImportPlugin::_get_import_options(const String &path, int32_t preset_index) const {
	StdVector<ImportOptionWrapper> options;
	_voxel_get_import_options(options, path, preset_index);

	TypedArray<Dictionary> output;

	const String name_key = "name";
	const String property_hint_key = "property_hint";
	const String default_value_key = "default_value";
	const String hint_string_key = "hint_string";
	const String usage_key = "usage";

	for (const ImportOptionWrapper &option : options) {
		Dictionary d;
		d[name_key] = String(option.option.name);
		d[default_value_key] = option.default_value;
		d[property_hint_key] = option.option.hint;
		d[hint_string_key] = String(option.option.hint_string);
		d[usage_key] = option.option.usage;
		output.push_back(d);
	}

	return output;
}

bool VOXEL_EditorImportPlugin::_get_option_visibility(
		const String &path,
		const StringName &option_name,
		const Dictionary &options
) const {
	return _voxel_get_option_visibility(path, option_name, KeyValueWrapper{ options });
}

Error VOXEL_EditorImportPlugin::_import(
		const String &source_file,
		const String &save_path,
		const Dictionary &options,
		const TypedArray<String> &platform_variants,
		const TypedArray<String> &gen_files
) const {
	// TODO GDX: `EditorImportPlugin::_import` is passing constant arrays for parameters that should be writable
	TypedArray<String> &platform_variants_writable = const_cast<TypedArray<String> &>(platform_variants);
	TypedArray<String> &gen_files_writable = const_cast<TypedArray<String> &>(gen_files);
	return _voxel_import(
			source_file,
			save_path,
			KeyValueWrapper{ options },
			StringListWrapper{ platform_variants_writable },
			StringListWrapper{ gen_files_writable }
	);
}

#if GODOT_VERSION_MAJOR == 4 && GODOT_VERSION_MINOR >= 3
bool VOXEL_EditorImportPlugin::_can_import_threaded() const {
	return _voxel_can_import_threaded();
}
#endif

#endif

String VOXEL_EditorImportPlugin::_voxel_get_importer_name() const {
	VOXEL_PRINT_ERROR("Method is not implemented");
	return "<unnamed>";
}

String VOXEL_EditorImportPlugin::_voxel_get_visible_name() const {
	VOXEL_PRINT_ERROR("Method is not implemented");
	return "<unnamed>";
}
PackedStringArray VOXEL_EditorImportPlugin::_voxel_get_recognized_extensions() const {
	VOXEL_PRINT_ERROR("Method is not implemented");
	return PackedStringArray();
}

String VOXEL_EditorImportPlugin::_voxel_get_preset_name(int p_idx) const {
	VOXEL_PRINT_ERROR("Method is not implemented");
	return "<unnamed>";
}

int VOXEL_EditorImportPlugin::_voxel_get_preset_count() const {
	VOXEL_PRINT_ERROR("Method is not implemented");
	return 0;
}

String VOXEL_EditorImportPlugin::_voxel_get_save_extension() const {
	VOXEL_PRINT_ERROR("Method is not implemented");
	return "";
}

String VOXEL_EditorImportPlugin::_voxel_get_resource_type() const {
	VOXEL_PRINT_ERROR("Method is not implemented");
	return "";
}

float VOXEL_EditorImportPlugin::_voxel_get_priority() const {
	return 1.0;
}

int VOXEL_EditorImportPlugin::_voxel_get_import_order() const {
	return IMPORT_ORDER_DEFAULT;
}

void VOXEL_EditorImportPlugin::_voxel_get_import_options(
		StdVector<ImportOptionWrapper> &p_out_options,
		const String &p_path,
		int p_preset_index
) const {
	VOXEL_PRINT_ERROR("Method is not implemented");
}

bool VOXEL_EditorImportPlugin::_voxel_get_option_visibility(
		const String &p_path,
		const StringName &p_option_name,
		const KeyValueWrapper p_options
) const {
	VOXEL_PRINT_ERROR("Method is not implemented");
	return false;
}

Error VOXEL_EditorImportPlugin::_voxel_import(
		const String &p_source_file,
		const String &p_save_path,
		const KeyValueWrapper p_options,
		StringListWrapper p_out_platform_variants,
		StringListWrapper p_out_gen_files
) const {
	VOXEL_PRINT_ERROR("Method is not implemented");
	return ERR_METHOD_NOT_FOUND;
}

bool VOXEL_EditorImportPlugin::_voxel_can_import_threaded() const {
	// According to docs
	// https://docs.godotengine.org/en/stable/classes/class_editorimportplugin.html#class-editorimportplugin-private-method-can-import-threaded
	return true;
}

} // namespace voxel::godot
