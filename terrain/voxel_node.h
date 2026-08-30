#ifndef VOXEL_NODE_H
#define VOXEL_NODE_H

#include "../engine/ids.h"
#include "../engine/streaming_dependency.h"
#include "../generators/voxel_generator.h"
#include "../meshers/voxel_mesher.h"
#include "../storage/voxel_format_gd.h"
#include "../streams/voxel_stream.h"
#include "../util/godot/classes/geometry_instance_3d.h"
#include "../util/godot/classes/node_3d.h"

#ifdef TOOLS_ENABLED
#include "../util/godot/core/version.h"
#endif

namespace voxel {

class VoxelTool;
class VoxelData;

// 体素体积的基类
class VoxelNode : public Node3D {
	GDCLASS(VoxelNode, Node3D)
public:
	// 网格器：负责将体素数据转换为网格
	virtual void set_mesher(Ref<VoxelMesher> mesher);
	virtual Ref<VoxelMesher> get_mesher() const;

	// 流：负责数据的加载与保存
	virtual void set_stream(Ref<VoxelStream> stream);
	virtual Ref<VoxelStream> get_stream() const;

	// 生成器：负责生成地形数据
	virtual void set_generator(Ref<VoxelGenerator> generator);
	virtual Ref<VoxelGenerator> get_generator() const;

	// 获取地形数据存储
	virtual VoxelData &get_storage() const;

	// 体素数据格式
	void set_format(Ref<godot::VoxelFormat> format);
	Ref<godot::VoxelFormat> get_format() const;
	virtual void on_format_changed();

	// 全局光照模式
	void set_gi_mode(GeometryInstance3D::GIMode mode);
	GeometryInstance3D::GIMode get_gi_mode() const;

	// 阴影投射设置
	void set_shadow_casting(GeometryInstance3D::ShadowCastingSetting setting);
	GeometryInstance3D::ShadowCastingSetting get_shadow_casting() const;

	// 渲染层掩码
	void set_render_layers_mask(int mask);
	int get_render_layers_mask() const;

	// 重启流，重新网格化所有块
	virtual void restart_stream();
	virtual void remesh_all_blocks();

	// 体积 ID 与流依赖
	virtual VolumeID get_volume_id() const;
	virtual std::shared_ptr<StreamingDependency> get_streaming_dependency() const;

	// 获取用于编辑地形的工具
	virtual Ref<VoxelTool> get_voxel_tool();

	enum NodeConversionFlags {
		NODE_CONVERSION_INCLUDE_INSTANCER = 1 << 0,
		NODE_CONVERSION_INCLUDE_INVISIBLE_BLOCKS = 1 << 1,
		NODE_CONVERSION_INCLUDE_MATERIAL_OVERRIDES = 1 << 2
	};

	// 将地形转换为普通节点
	virtual Node3D *convert_to_nodes(const BitField<NodeConversionFlags> flags) const;

#ifdef TOOLS_ENABLED
#if defined(VOXEL_GODOT)
	PackedStringArray get_configuration_warnings() const override;
#endif
	virtual void get_configuration_warnings(PackedStringArray &warnings) const;
#endif

protected:
	int get_used_channels_mask() const;

	virtual void _on_gi_mode_changed() {}
	virtual void _on_shadow_casting_changed() {}
	virtual void _on_render_layers_mask_changed() {}

	VoxelFormat get_internal_format() const;

private:
	Ref<VoxelMesher> _b_get_mesher() {
		return get_mesher();
	}
	void _b_set_mesher(Ref<VoxelMesher> mesher) {
		set_mesher(mesher);
	}

	Ref<VoxelStream> _b_get_stream() {
		return get_stream();
	}
	void _b_set_stream(Ref<VoxelStream> stream) {
		set_stream(stream);
	}

	Ref<VoxelGenerator> _b_get_generator() {
		return get_generator();
	}
	void _b_set_generator(Ref<VoxelGenerator> g) {
		set_generator(g);
	}

	void _b_on_format_changed();

	static void _bind_methods();

	GeometryInstance3D::GIMode _gi_mode = GeometryInstance3D::GI_MODE_DISABLED;
	GeometryInstance3D::ShadowCastingSetting _shadow_casting = GeometryInstance3D::SHADOW_CASTING_SETTING_ON;
	int _render_layers_mask = 1;
	Ref<godot::VoxelFormat> _format;
};

} // namespace voxel

VARIANT_BITFIELD_CAST(voxel::VoxelNode::NodeConversionFlags);

#endif // VOXEL_NODE_H
