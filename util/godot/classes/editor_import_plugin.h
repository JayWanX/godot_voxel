#ifndef VOXEL_GODOT_EDITOR_IMPORT_PLUGIN_H
#define VOXEL_GODOT_EDITOR_IMPORT_PLUGIN_H

#if defined(VOXEL_GODOT)
#include <editor/import/editor_import_plugin.h>
#endif

#include "../core/version.h"

#include "../../containers/std_vector.h"

namespace voxel::godot {

struct ImportOptionWrapper {
	PropertyInfo option;
	Variant default_value;

	ImportOptionWrapper(PropertyInfo p_option, Variant p_default_value) :
			option(p_option), default_value(p_default_value) {}
};

// Exposes the same interface for different equivalent dictionary types, depending on the compiling target.
struct KeyValueWrapper {
#if defined(VOXEL_GODOT)

	const HashMap<StringName, Variant> &_map;

	inline bool try_get(const String key, Variant &out_value) const {
		const Variant *vp = _map.getptr(key);
		if (vp != nullptr) {
			out_value = *vp;
			return true;
		}
		return false;
	}

	inline Variant get(const String key) const {
		const Variant *vp = _map.getptr(key);
		if (vp != nullptr) {
			return *vp;
		}
		return Variant();
	}

#endif
};

// Exposes the same interface for different equivalent lists of strings, depending on the compiling target.
struct StringListWrapper {
#if defined(VOXEL_GODOT)
	List<String> &_list;
	inline void append(const String s) {
		_list.push_back(s);
	}
#endif
};

// Wraps EditorImportPlugin to isolate engine API differences.
class Voxel_EditorImportPlugin : public EditorImportPlugin {
	GDCLASS(Voxel_EditorImportPlugin, EditorImportPlugin)
public:
#if defined(VOXEL_GODOT)
	String get_importer_name() const override;
	String get_visible_name() const override;
	void get_recognized_extensions(List<String> *p_extensions) const override;
	String get_preset_name(int p_idx) const override;
	int get_preset_count() const override;
	String get_save_extension() const override;
	String get_resource_type() const override;
	float get_priority() const override;
	int get_import_order() const override;
	void get_import_options(const String &p_path, List<ImportOption> *r_options, int p_preset = 0) const override;
	bool get_option_visibility(
			const String &p_path,
			const String &p_option,
			const HashMap<StringName, Variant> &p_options
	) const override;

	Error import(
#if GODOT_VERSION_MAJOR == 4 && GODOT_VERSION_MINOR >= 4
			ResourceUID::ID p_source_id,
#endif
			const String &p_source_file,
			const String &p_save_path,
			const HashMap<StringName, Variant> &p_options,
			List<String> *r_platform_variants,
			List<String> *r_gen_files,
			Variant *r_metadata = nullptr
	) override;

#if GODOT_VERSION_MAJOR == 4 && GODOT_VERSION_MINOR >= 3
	bool can_import_threaded() const override;
#endif

#endif

protected:
	// These methods can be implemented once, wrappers above take care of converting.

	virtual String _voxel_get_importer_name() const;
	virtual String _voxel_get_visible_name() const;
	virtual PackedStringArray _voxel_get_recognized_extensions() const;
	virtual String _voxel_get_preset_name(int p_idx) const;
	virtual int _voxel_get_preset_count() const;
	virtual String _voxel_get_save_extension() const;
	virtual String _voxel_get_resource_type() const;
	virtual float _voxel_get_priority() const;
	virtual int _voxel_get_import_order() const;
	virtual bool _voxel_can_import_threaded() const;

	virtual void _voxel_get_import_options(
			StdVector<ImportOptionWrapper> &p_out_options,
			const String &p_path,
			int p_preset_index
	) const;

	virtual bool _voxel_get_option_visibility(
			const String &p_path,
			const StringName &p_option_name,
			const KeyValueWrapper p_options
	) const;

	virtual Error _voxel_import(
			const String &p_source_file,
			const String &p_save_path,
			const KeyValueWrapper p_options,
			StringListWrapper p_out_platform_variants,
			StringListWrapper p_out_gen_files
	) const;

private:
	static void _bind_methods() {}
};

} // namespace voxel::godot

#endif // VOXEL_GODOT_EDITOR_IMPORT_PLUGIN_H
