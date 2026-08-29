#ifndef VOXEL_GRAPH_NODE_INSPECTOR_WRAPPER_H
#define VOXEL_GRAPH_NODE_INSPECTOR_WRAPPER_H

#include "../../generators/graph/voxel_generator_graph.h"
#include "../../util/godot/classes/ref_counted.h"

VOXEL_GODOT_FORWARD_DECLARE(class EditorUndoRedoManager)

namespace voxel {

class VoxelGraphEditor;

// 节点不是资源，因此这里将它们转换为检查器能理解的形式。
// 这使支持撤销/重做和子资源变得更容易。
// 警告：`AnimationPlayer` 允许对属性做关键帧，但实际上这里并不支持。
class VoxelGraphNodeInspectorWrapper : public RefCounted {
	GDCLASS(VoxelGraphNodeInspectorWrapper, RefCounted)
public:
	void setup(uint32_t p_node_id, VoxelGraphEditor *ed);

	// 可能在图形编辑器被销毁时调用。这可以防止在插件被移除后，
	// 万一 UndoRedo 从该编辑器调用函数时访问到悬空指针。
	void detach_from_graph_editor();

	inline Ref<pg::VoxelGraphFunction> get_graph() const {
		return _graph;
	}
	inline Ref<VoxelGeneratorGraph> get_generator() const {
		return _generator;
	}

protected:
	void _get_property_list(List<PropertyInfo> *p_list) const;
	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;
	bool _dont_undo_redo() const;

private:
	static void _bind_methods();

	Ref<pg::VoxelGraphFunction> _graph;
	Ref<VoxelGeneratorGraph> _generator;
	uint32_t _node_id = ProgramGraph::NULL_ID;
	VoxelGraphEditor *_graph_editor = nullptr;
};

} // namespace voxel

#endif // VOXEL_GRAPH_NODE_INSPECTOR_WRAPPER_H
