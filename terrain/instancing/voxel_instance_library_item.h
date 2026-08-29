#ifndef VOXEL_INSTANCE_LIBRARY_ITEM_H
#define VOXEL_INSTANCE_LIBRARY_ITEM_H

#include "../../util/containers/fixed_array.h"
#include "../../util/containers/std_vector.h"
#include "instance_library_item_listener.h"
#include "voxel_instance_generator.h"

namespace voxel {

class VoxelInstanceLibraryItem : public Resource {
	GDCLASS(VoxelInstanceLibraryItem, Resource)
public:
	void set_item_name(String p_name);
	String get_item_name() const;

	void set_lod_index(int lod);
	int get_lod_index() const;

	void set_generator(Ref<VoxelInstanceGenerator> generator);
	Ref<VoxelInstanceGenerator> get_generator() const;

	void set_persistent(bool persistent);
	bool is_persistent() const;

	float get_floating_sdf_threshold() const;
	void set_floating_sdf_threshold(const float new_threshold);

	float get_floating_sdf_offset_along_normal() const;
	void set_floating_sdf_offset_along_normal(const float new_offset);

	// 内部

	void add_listener(IInstanceLibraryItemListener *listener, int id);
	void remove_listener(IInstanceLibraryItemListener *listener, int id);

#ifdef TOOLS_ENABLED
	virtual void get_configuration_warnings(PackedStringArray &warnings) const;
#endif

protected:
	void notify_listeners(IInstanceLibraryItemListener::ChangeType change);

private:
	void _on_generator_changed();

	static void _bind_methods();

	// 供用户使用，引擎不使用
	String _name;

	// 如果图层是持久的，则只要体积有支持实例的流，对其实例的任何更改都会被保存。
	// 它也不会在已修改的表面之上生成。
	// 如果图层不是持久的，更改不会被保存，并且它会在所有符合要求的表面上持续生成。
	bool _persistent = false;

	// 该模型将生成到八叉树的哪个 LOD 中。
	// 数值越大表示距离越远，但精度和密度越低
	int _lod_index = 0;

	Ref<VoxelInstanceGenerator> _generator;

	struct ListenerSlot {
		IInstanceLibraryItemListener *listener;
		int id;

		inline bool operator==(const ListenerSlot &other) const {
			return listener == other.listener && id == other.id;
		}
	};

	StdVector<ListenerSlot> _listeners;
	float _floating_sdf_threshold = 0.0f;
	float _floating_sdf_offset_along_normal = -0.1f;
};

} // namespace voxel

#endif // VOXEL_INSTANCE_LIBRARY_ITEM_H
