#ifndef VOX_DATA_H
#define VOX_DATA_H

#include "../../util/containers/fixed_array.h"
#include "../../util/containers/std_unordered_map.h"
#include "../../util/containers/std_vector.h"
#include "../../util/godot/core/basis.h"
#include "../../util/godot/core/string.h"
#include "../../util/math/color8.h"
#include "../../util/math/vector3i.h"
#include "../../util/memory/memory.h"


namespace voxel::magica {

struct Model {
	Vector3i size;
	// TODO 优化：实现懒加载/流式传输以减少中间内存分配？
	// 加载完整的 256^3 模型需要 16 兆字节，但很多区域实际上可能是均匀的，
	// 而且我们可能并不立即需要实际的模型
	StdVector<uint8_t> color_indexes;
};

struct Node {
	enum Type { //
		TYPE_TRANSFORM = 0,
		TYPE_GROUP,
		TYPE_SHAPE
	};

	int id;
	// 根据类型不同，节点指针可以被转换为不同的结构体
	const Type type;
	StdUnorderedMap<String, String> attributes;

	Node(Type p_type) : type(p_type) {}

	virtual ~Node() {}
};

struct Rotation {
	uint8_t data;
	Basis basis;
};

struct TransformNode : public Node {
	int child_node_id;
	int layer_id;
	// 轴点位置，在 MagicaVoxel 中正好位于中心
	Vector3i position;
	Rotation rotation;
	String name;
	bool hidden;

	TransformNode() : Node(Node::TYPE_TRANSFORM) {}
};

struct GroupNode : public Node {
	StdVector<int> child_node_ids;

	GroupNode() : Node(Node::TYPE_GROUP) {}
};

struct ShapeNode : public Node {
	int model_id; // 对应模型数组中的索引
	StdUnorderedMap<String, String> model_attributes;

	ShapeNode() : Node(Node::TYPE_SHAPE) {}
};

struct Layer {
	int id;
	StdUnorderedMap<String, String> attributes;
	String name;
	bool hidden;
};

struct Material {
	enum Type { //
		TYPE_DIFFUSE,
		TYPE_METAL,
		TYPE_GLASS,
		TYPE_EMIT
	};
	int id;
	Type type = TYPE_DIFFUSE;
	float weight = 0.f;
	float roughness = 1.f;
	float specular = 0.f;
	float ior = 1.f; // refraction
	float flux = 0.f;
	// TODO 我不知道 `_att` 是什么意思
	float att = 0.f;
	// TODO `_plastic` 到底是什么？
};

class Data {
public:
	void clear();
	Error load_from_file(String fpath);

	unsigned int get_model_count() const;
	const Model &get_model(unsigned int index) const;

	// 如果没有场景图，可以返回 -1
	int get_root_node_id() const;
	const Node *get_node(int id) const;

	unsigned int get_layer_count() const;
	const Layer &get_layer_by_index(unsigned int index) const;

	int get_material_id_for_palette_index(unsigned int palette_index) const;
	const Material &get_material_by_id(int id) const;

	inline const FixedArray<Color8, 256> &get_palette() const {
		return _palette;
	}

private:
	Error _load_from_file(String fpath);

	StdVector<UniquePtr<Model>> _models;
	StdVector<UniquePtr<Layer>> _layers;
	StdUnorderedMap<int, UniquePtr<Node>> _scene_graph;
	// 材质 ID 据称与调色板索引相关联
	StdUnorderedMap<int, UniquePtr<Material>> _materials;
	int _root_node_id = -1;
	FixedArray<Color8, 256> _palette;
};

} // namespace voxel::magica

#endif // VOX_DATA_H
