#ifndef VOXEL_LOD_OCTREE_H
#define VOXEL_LOD_OCTREE_H

#include "../../util/containers/std_vector.h"
#include "../../util/math/box3i.h"

namespace voxel {

// 用于处理细节层级（LOD）的八叉树。
class LodOctree {
public:
	static const unsigned int NO_CHILDREN = -1;
	static const unsigned int ROOT_INDEX = -1; // 根节点不存储在池中

	struct NodeData {
		uint32_t state = 0;
	};

	struct Node {
		// 节点池中第一个子节点的索引。
		// 接下来 7 个索引是其余子节点。
		// 若节点未细分，则设置为 NO_CHILDREN。
		// 本可使用指针，但索引已足够，占用一半内存且不受 realloc 影响
		unsigned int first_child;

		NodeData data;

		// 节点位置为节省内存而即时计算，
		// 并按当前 LOD 的数据块大小相除，
		// 因此在每个 LOD 内是连续的，可用于网格存储

		Node() {
			init();
		}

		inline bool has_children() const {
			return first_child != NO_CHILDREN;
		}

		inline void init() {
			first_child = NO_CHILDREN;
		}
	};

	struct NoDestroyAction {
		inline void operator()(Vector3i node_pos, unsigned int lod) {}
	};

	template <typename DestroyAction_T>
	void clear(DestroyAction_T &destroy_action) {
		join_all_recursively(&_root, Vector3i(), _max_depth, destroy_action);
		_is_root_created = false;
		_max_depth = 0;
	}

	void clear() {
		_pool.clear();
		_root.init();
		_is_root_created = false;
		_max_depth = 0;
	}

	template <typename DestroyAction_T>
	void create(unsigned int lod_count, DestroyAction_T &destroy_action) {
		clear(destroy_action);
		CRASH_COND(lod_count == 0);
		_max_depth = lod_count - 1;
	}

	void create(unsigned int lod_count) {
		clear();
		CRASH_COND(lod_count == 0);
		_max_depth = lod_count - 1;
	}

	unsigned int get_lod_count() const {
		return _max_depth + 1;
	}

	// 签名示例
	struct DefaultUpdateActions {
		void create_child(Vector3i node_pos, unsigned int lod, NodeData &data) {} // 在分裂时发生
		void destroy_child(Vector3i node_pos, unsigned int lod) {} // 在合并时发生
		void show_parent(Vector3i node_pos, unsigned int lod) {} // 在合并时发生
		void hide_parent(Vector3i node_pos, unsigned int lod) {} // 在分裂时发生
		bool can_create_root(int lod) {
			return true;
		}
		bool can_split(Vector3i node_pos, unsigned int lod_index, NodeData &data) {
			return true;
		}
		bool can_join(Vector3i node_pos, unsigned int lod) {
			return true;
		}
	};

	// TODO 提供一个完全递归工作的 `update` 版本。
	// 即一次调用就足以得到目标形状

	// 通过满足 `can_split` 谓词时分裂节点、满足 `can_join` 谓词时合并节点来适配八叉树。
	// 这并非完全递归。预期在多帧内调用，
	// 因此形状是逐步得到的。
	template <typename UpdateActions_T>
	void update(UpdateActions_T &actions) {
		if (_is_root_created || _root.has_children()) {
			update(ROOT_INDEX, Vector3i(), _max_depth, actions);
		} else {
			// TODO 我不太喜欢这样
			// 首次以略微不同的方式处理根节点。
			if (actions.can_create_root(_max_depth)) {
				actions.create_child(Vector3i(), _max_depth, _root.data);
				_is_root_created = true;

				update(ROOT_INDEX, Vector3i(), _max_depth, actions);
			}
		}
	}

	static inline Vector3i get_child_position(Vector3i parent_position, unsigned int i) {
		return Vector3i( //
				parent_position.x * 2 + (i & 1), //
				parent_position.y * 2 + ((i >> 1) & 1), //
				parent_position.z * 2 + ((i >> 2) & 1));
	}

	const Node *get_root() const {
		return &_root;
	}

	const Node *get_child(const Node *node, unsigned int i) const {
		ERR_FAIL_COND_V(node == nullptr, nullptr);
		VOXEL_ASSERT_RETURN_V(i < 8, nullptr);
		return get_node(node->first_child + i);
	}

	// 对与盒子相交的所有叶节点运行谓词，一旦为真即停止。
	// 盒子以八叉树的单位坐标给出（1 单位等于最大深度处叶节点的大小）。
	// 若谓词匹配到任意节点则返回 true，否则返回 false。
	// predicate: `bool is_match(Vector3i node_pos, int lod_index, const NodeData &data)`
	template <typename Predicate_T>
	bool find_in_box(Box3i box, Predicate_T predicate) const {
		Box3i root_box(Vector3i(), Vector3iUtil::create(1 << _max_depth));
		box.clip(root_box);
		return find_in_box_recursive(box, Vector3i(), ROOT_INDEX, _max_depth, predicate);
	}

	// 在与盒子相交的所有叶节点上执行函数。
	// f: `void f(Vector3i node_pos, int lod_index, NodeData &data)`
	template <typename F>
	void for_leaves_in_box(Box3i box, F f) {
		Box3i root_box(Vector3i(), Vector3iUtil::create(1 << _max_depth));
		box.clip(root_box);
		return for_leaves_in_box_recursive(box, Vector3i(), ROOT_INDEX, _max_depth, f);
	}

	// 在八叉树的所有叶节点上执行函数。
	// f: `void f(Vector3i node_pos, int lod_index, const NodeData &data)`
	template <typename F>
	void for_each_leaf(F f) const {
		return for_each_leaf_recursive(Vector3i(), ROOT_INDEX, _max_depth, f);
	}

	unsigned int get_node_count() const {
		return get_node_count_recursive(ROOT_INDEX);
	}

	struct SubdivideActionsDefault {
		bool can_split(Vector3i node_pos, unsigned int lod_index, const NodeData &node_data) {
			return true;
		}
		void create_child(Vector3i node_pos, unsigned int lod_index, NodeData &node_data) {}
	};

	// 仅根据 `can_split` 递归细分八叉树。
	// 不会对已有节点进行取消细分。
	template <typename Actions_T>
	void subdivide(Actions_T &actions) {
		if (!_is_root_created && actions.can_split(Vector3i(), _max_depth, _root.data)) {
			actions.create_child(Vector3i(), _max_depth, _root.data);
			_is_root_created = true;
		} else {
			return;
		}
		subdivide_recursively(ROOT_INDEX, Vector3i(), _max_depth, actions);
	}

	// 获取节点在 LOD0 坐标系中的包围盒
	// （即叶节点始终为 1x1x1，LOD1 节点为 2x2x2 等）
	static inline Box3i get_node_box(Vector3i pos_within_lod, unsigned int lod_index) {
		return Box3i(pos_within_lod << lod_index, Vector3iUtil::create(1 << lod_index));
	}

	// 用于 UpdateActions::can_split 的便捷方法。
	// 坐标位于八叉树空间（其中 1 单位 = 叶节点的大小）
	static bool is_below_split_distance(Vector3i node_pos, unsigned int lod, Vector3 view_pos, float lod_distance) {
		const unsigned int lod_factor = 1 << lod;
		const Vector3 world_center = static_cast<real_t>(lod_factor) * (Vector3(node_pos) + Vector3(0.5, 0.5, 0.5));
		const float split_distance_sq = math::squared(lod_distance * lod_factor);
		return world_center.distance_squared_to(view_pos) < split_distance_sq;
	}

	// 创建具有正确深度的八叉树的辅助函数
	static int compute_lod_count(unsigned int base_size, unsigned int full_size) {
		unsigned int po = 0;
		while (full_size > base_size) {
			full_size = full_size >> 1;
			po += 1;
		}
		return po;
	}

	static inline unsigned int get_octree_size_po2(unsigned int block_size_po2, unsigned int lod_count) {
		return block_size_po2 + lod_count - 1;
	}

private:
	// 此池将节点按 8 个一组处理，因此只需知道第一个子节点即可寻址
	class NodePool {
	public:
		// 警告：返回的指针之后可能被 `allocate_children` 失效。请谨慎使用。
		inline Node *get_node(unsigned int i) {
			CRASH_COND(i >= _nodes.size());
			CRASH_COND(i == ROOT_INDEX);
			return &_nodes[i];
		}

		inline const Node *get_node(unsigned int i) const {
			CRASH_COND(i >= _nodes.size());
			CRASH_COND(i == ROOT_INDEX);
			return &_nodes[i];
		}

		unsigned int allocate_children() {
			if (_free_indexes.size() == 0) {
				unsigned int i0 = _nodes.size();
				_nodes.resize(i0 + 8);
				return i0;
			} else {
				unsigned int i0 = _free_indexes[_free_indexes.size() - 1];
				_free_indexes.pop_back();
				return i0;
			}
		}

		// 警告：这不是递归的。请正确使用。
		void recycle_children(unsigned int i0) {
			// 调试检查，回收非第一个子节点的节点没有应用场景
			CRASH_COND(i0 % 8 != 0);

			for (unsigned int i = 0; i < 8; ++i) {
				_nodes[i0 + i].init();
			}

			_free_indexes.push_back(i0);
		}

		void clear() {
			_nodes.clear();
			_free_indexes.clear();
		}

	private:
		// TODO 若增长过大，也许可以实现分页向量来对抗碎片化。
		// 若这样做，由于页面保持稳定，或许也能解决指针失效问题
		StdVector<Node> _nodes;
		StdVector<unsigned int> _free_indexes;
	};

	inline Node *get_node(unsigned int index) {
		if (index == ROOT_INDEX) {
			return &_root;
		} else {
			return _pool.get_node(index);
		}
	}

	inline const Node *get_node(unsigned int index) const {
		if (index == ROOT_INDEX) {
			return &_root;
		} else {
			return _pool.get_node(index);
		}
	}

	template <typename UpdateActions_T>
	void update(unsigned int node_index, Vector3i node_pos, unsigned int lod, UpdateActions_T &actions) {
		// 此函数应在各帧之间定期调用。
		Node *node = get_node(node_index);

		if (!node->has_children()) {
			// 若不是最后一层 LOD，且距离足够近并满足自定义条件
			if (lod > 0 && actions.can_split(node_pos, lod, node->data)) {
				// 分裂
				const unsigned int first_child = _pool.allocate_children();
				// 重新获取节点，因为 `allocate_children` 可能使指针失效
				node = get_node(node_index);
				node->first_child = first_child;

				for (unsigned int i = 0; i < 8; ++i) {
					const Vector3i child_pos = get_child_position(node_pos, i);
					const unsigned int child_lod = lod - 1;
					const unsigned int child_index = first_child + i;

					Node *child = get_node(child_index);
					actions.create_child(child_pos, child_lod, child->data);

					update(child_index, child_pos, child_lod, actions);
				}

				actions.hide_parent(node_pos, lod);
			}

		} else {
			// `node` 有子节点

			bool has_split_child = false;
			const unsigned int first_child = node->first_child;

			for (unsigned int i = 0; i < 8; ++i) {
				const unsigned int child_index = first_child + i;
				update(child_index, get_child_position(node_pos, i), lod - 1, actions);
				has_split_child |= _pool.get_node(child_index)->has_children();
			}

			if (!has_split_child && actions.can_join(node_pos, lod)) {
				// 重新获取节点，因为 `update` 可能使指针失效
				node = get_node(node_index);

				// 合并
				for (unsigned int i = 0; i < 8; ++i) {
					actions.destroy_child(get_child_position(node_pos, i), lod - 1);
				}

				_pool.recycle_children(first_child);
				node->first_child = NO_CHILDREN;

				actions.show_parent(node_pos, lod);
			}
		}
	}

	template <typename DestroyAction_T>
	void join_all_recursively(Node *node, Vector3i node_pos, unsigned int lod, DestroyAction_T &destroy_action) {
		// 这里可以使用指针，因为我们不会分配新节点，
		// 也不会缩小节点池

		if (node->has_children()) {
			unsigned int first_child = node->first_child;

			for (unsigned int i = 0; i < 8; ++i) {
				Node *child = _pool.get_node(first_child + i);
				join_all_recursively(child, get_child_position(node_pos, i), lod - 1, destroy_action);
			}

			_pool.recycle_children(first_child);
			node->first_child = NO_CHILDREN;
		}

		// 销毁自身
		destroy_action(node_pos, lod);
	}

	template <typename Predicate_T>
	bool find_in_box_recursive(
			Box3i box, Vector3i node_pos, unsigned int node_index, unsigned int depth, Predicate_T predicate) const {
		const Node *node = get_node(node_index);
		const Box3i node_box = get_node_box(node_pos, depth);
		if (!node_box.intersects(box)) {
			return false;
		}
		if (node->has_children()) {
			const unsigned int first_child_index = node->first_child;
			const unsigned int lower_depth = depth - 1;
			// TODO 优化：可以改用广度优先搜索代替深度优先，
			// 因为子节点组在内存中是连续的，有助于预取器
			for (unsigned int ri = 0; ri < 8; ++ri) {
				const bool found = find_in_box_recursive(
						box, get_child_position(node_pos, ri), first_child_index + ri, lower_depth, predicate);
				if (found) {
					return true;
				}
			}
		} else if (predicate(node_pos, depth, node->data)) {
			return true;
		}
		return false;
	}

	template <typename F>
	void for_leaves_in_box_recursive(Box3i box, Vector3i node_pos, unsigned int node_index, unsigned int depth, F f) {
		Node *node = get_node(node_index);
		const Box3i node_box = get_node_box(node_pos, depth);
		if (!node_box.intersects(box)) {
			return;
		}
		if (node->has_children()) {
			const unsigned int first_child_index = node->first_child;
			const unsigned int lower_depth = depth - 1;
			for (int ri = 0; ri < 8; ++ri) {
				for_leaves_in_box_recursive(
						box, get_child_position(node_pos, ri), first_child_index + ri, lower_depth, f);
			}
		} else {
			f(node_pos, depth, node->data);
		}
	}

	unsigned int get_node_count_recursive(unsigned int node_index) const {
		const Node *node = get_node(node_index);
		unsigned int count = 1;
		if (node->has_children()) {
			for (unsigned int i = 0; i < 8; ++i) {
				count += get_node_count_recursive(node->first_child + i);
			}
		}
		return count;
	}

	template <typename F>
	void for_each_leaf_recursive(Vector3i node_pos, unsigned int node_index, unsigned int depth, F f) const {
		const Node *node = get_node(node_index);
		if (node->has_children()) {
			const unsigned int first_child_index = node->first_child;
			const unsigned int lower_depth = depth - 1;
			for (unsigned int ri = 0; ri < 8; ++ri) {
				for_each_leaf_recursive(get_child_position(node_pos, ri), first_child_index + ri, lower_depth, f);
			}
		} else {
			f(node_pos, depth, node->data);
		}
	}

	template <typename Actions_T>
	void subdivide_recursively(unsigned int node_index, Vector3i node_pos, unsigned int lod, Actions_T &actions) {
		Node *node = get_node(node_index);
		if (node->has_children()) {
			if (lod == 1) {
				// 子节点无法分裂
				return;
			}
			// `node` 可能在循环期间失效
			const unsigned int first_child_index = node->first_child;
			for (unsigned int i = 0; i < 8; ++i) {
				subdivide_recursively(first_child_index + i, get_child_position(node_pos, i), lod - 1, actions);
			}

		} else if (lod > 0 && actions.can_split(node_pos, lod, node->data)) {
			// 分裂
			const unsigned int first_child_index = _pool.allocate_children();
			// 重新获取节点，因为 `allocate_children` 可能使指针失效
			node = get_node(node_index);
			node->first_child = first_child_index;
			// `node` 可能在循环期间失效

			for (unsigned int i = 0; i < 8; ++i) {
				const unsigned int child_index = first_child_index + i;
				const Vector3i child_pos = get_child_position(node_pos, i);
				Node *child = get_node(child_index);
				actions.create_child(child_pos, lod - 1, child->data);
				// `child` 可能失效
				subdivide_recursively(child_index, child_pos, lod - 1, actions);
			}

			// 这里本应调用 `hide_parent()`，但目前不需要
		}
	}

	Node _root;
	bool _is_root_created = false;
	unsigned int _max_depth = 0;
	NodePool _pool;
};

} // namespace voxel

// 说明：
// 给定深度的八叉树节点总数，感谢 Sage：
// ((1 << 3 * (depth + 1)) - 1 ) / 7

#endif // VOXEL_LOD_OCTREE_H
