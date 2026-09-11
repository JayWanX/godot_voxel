#include <scene/resources/texture.h>
#include <scene/resources/material.h>
#include <scene/gui/box_container.h>
#ifndef VOXEL_GRAPH_EDITOR_NODE_PREVIEW_H
#define VOXEL_GRAPH_EDITOR_NODE_PREVIEW_H

#include "../../generators/graph/voxel_graph_runtime.h"
#include "../../util/godot/classes/image.h"
#include <core/version.h>
#include <scene/resources/image_texture.h>
#include <scene/resources/material.h>
#include <scene/gui/box_container.h>
#include "../../util/macros.h"
#include "../../util/math/vector2f.h"
#include "graph_preview_mode.h"
#include "voxel_graph_editor_node_preview_info.h"

class TextureRect;

namespace voxel {

struct GraphEditorAdapter;

// 显示来自输出端口的 3D 值集合的 2D 切片
class VoxelGraphEditorNodePreview : public VBoxContainer {
	GDCLASS(VoxelGraphEditorNodePreview, VBoxContainer)
public:
	enum PixelMode { //
		PIXEL_MODE_GREYSCALE = 0,
		PIXEL_MODE_SDF = 1,
		PIXEL_MODE_COUNT
	};

	static void load_resources();
	static void unload_resources();

	VoxelGraphEditorNodePreview();

	void update_from_buffer(const pg::Runtime::Buffer &buffer);
	void update_display_settings(const pg::VoxelGraphFunction &graph, uint32_t node_id);

	static void update_previews(
			GraphEditorAdapter &adapter,
			Span<const VoxelGraphEditorNodePreviewInfo> previews,
			const GraphEditorPreview::ViewMode view_mode,
			const float transform_scale,
			const Vector2f transform_offset
	);

private:
	static const int RESOLUTION = 128;

	static void _bind_methods() {}

	TextureRect *_texture_rect = nullptr;
	Ref<ImageTexture> _texture;
	Ref<Image> _image;
	Ref<ShaderMaterial> _material;
};

} // namespace voxel

#endif // VOXEL_GRAPH_EDITOR_NODE_PREVIEW_H
