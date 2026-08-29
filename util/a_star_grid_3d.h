#ifndef VOXEL_ASTAR_GRID_3D_H
#define VOXEL_ASTAR_GRID_3D_H

#include "../util/containers/std_unordered_map.h"
#include "../util/containers/std_vector.h"
#include "../util/godot/core/sort_array.h"
#include "../util/math/box3i.h"
#include "../util/math/vector3f.h"
#include <limits>
#include <unordered_map>

namespace voxel {

// AStar 的变体，专门用于三维网格。
// 该实现可以逐步执行，便于调试或将开销分摊到时间内。
class AStarGrid3D {
public:
	AStarGrid3D();
	virtual ~AStarGrid3D() {}

	void set_agent_size(Vector3f size);
	Vector3f get_agent_size() const {
		return _agent_size;
	}

	void set_max_fall_height(int h);
	int get_max_fall_height() const;

	void set_max_path_cost(float cost);
	float get_max_path_cost() const;

	void set_region(Box3i region);
	const Box3i &get_region() const {
		return _region;
	}

	void start(Vector3i from_position, Vector3i target_position);
	void step();
	bool is_running() const;
	Span<const Vector3i> get_path() const;
	void clear();

	// Debug

	void debug_get_visited_points(StdVector<Vector3i> &out_positions) const;
	bool debug_get_next_step_point(Vector3i &out_pos) const;

protected:
	virtual bool is_solid(Vector3i pos);

private:
	float evaluate_heuristic(Vector3i pos, Vector3i target_pos) const;
	void reconstruct_path(uint32_t end_point_index);
	void get_neighbor_positions(Vector3i pos, StdVector<Vector3i> &out_positions);
	bool is_ground_close_enough(Vector3i pos);
	bool fits(Vector3f pos, Vector3f agent_extents);

	struct Point {
		static const uint32_t NO_CAME_FROM = std::numeric_limits<uint32_t>::max();

		Vector3i position;

		// 沿目前最优路径，到达该点前经过每个点的所有代价之和
		float gscore;

		// 从该点到目标的估计代价
		float fscore;

		// 目前最优路径上的前一个点
		uint32_t came_from_point_index;

		bool in_open_set;
	};

	struct ComparePoints {
		StdVector<Point> *pool;

		inline bool operator()(uint32_t ai, uint32_t bi) const {
			const Point &a = (*pool)[ai];
			const Point &b = (*pool)[bi];
			return a.fscore > b.fscore;

			// "下面这段代码会反复报"bad comparison function"，不知原因，尽管它正是 AStarGrid2D 的工作方式。"
			// 把 >= 替换为 > 后该问题消失，但那样逻辑就不同了。
			// 从我目前的性能分析来看，这也没有区别
			// if (a.fscore < b.fscore) {
			// 	return true;
			// }
			// if (a.fscore > b.fscore) {
			// 	return false;
			// }
			// // 如果 fscores 相同，则优先考虑离起点更远的点。
			// return a.gscore >= b.gscore;
		}
	};

	struct PriorityQueue {
		SortArray<uint32_t, ComparePoints> sorter;
		StdVector<uint32_t> items;

		inline uint32_t peek() const {
			return items[0];
		}

		inline void pop() {
			// 将当前点从开放列表中移除。
			sorter.pop_heap(0, items.size(), items.data());
			items.pop_back();
		}

		inline void push(uint32_t v) {
			items.push_back(v);
			sorter.push_heap(0, items.size() - 1, 0, v, items.data());
		}

		inline unsigned int size() const {
			return items.size();
		}

		inline void clear() {
			items.clear();
		}

		void update_priority(uint32_t v) {
			for (unsigned int i = 0; i < items.size(); ++i) {
				// 这通常需要一个自定义相等比较的方法，但在我们的使用场景中这样做是可行的
				if (items[i] == v) {
					sorter.push_heap(0, i, 0, v, items.data());
					break;
				}
			}
		}
	};

	uint32_t _start_point_index;
	Vector3i _target_position;
	bool _is_running = false;
	// 智能体尺寸，默认相当于一个高 2 体素、宽 1 体素的玩家。应略小于一个体素，以
	// 留出一点余量。
	// TODO 手动指定智能体原点，因为当它大于一个格时，检查是否适配某个格会比较麻烦；
	// 该算法只经过格的中心，因此根据智能体原点的不同，某些路径将永远
	// 不会被采用，即使它在格间移动时本可以。通常较好的做法是将智能体
	// 划分为可容纳其身体各部分的网格格，并将其原点放在左下角的中心。这需要在
	// 读取最终路径时加以考虑。
	Vector3f _agent_size = Vector3f(0.8f, 1.8f, 0.8f);
	Vector3f _fitting_offset;
	int _max_fall_height = 3;

	// 累积边代价大于此值的节点将被忽略。
	// 除区域检查外，这也限制了路径的有效代价上限。
	// 默认情况下，边代价即距离，因此它相当于最大路径长度。
	float _max_path_cost = 1000.f;

	Box3i _region;
	StdVector<Point> _points_pool;
	PriorityQueue _open_list;

	// 只有被访问过的点才会进入该映射。相比建立一个大的三维点网格，它应占用更少内存，因为
	// 实际上我们可能只访问其中的一小部分。
	// 最终若更快的话，我们可以尝试分块网格？
	StdUnorderedMap<Vector3i, uint32_t> _points_map;

	StdVector<Vector3i> _path;
	StdVector<Vector3i> _neighbor_positions;
};

} // namespace voxel

#endif // VOXEL_ASTAR_GRID_3D_H
