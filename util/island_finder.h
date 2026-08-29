#ifndef ISLAND_FINDER_H
#define ISLAND_FINDER_H

#include "containers/fixed_array.h"
#include "containers/span.h"
#include "godot/core/sort_array.h"
#include "math/box3i.h"

namespace voxel {

// 扫描一个二值网格并返回另一个网格，
// 其中所有相连的连通块都被标记为一个唯一的 ID。
// 它基于连通分量标记（Connected-Component-Labeling）的两遍算法。
//
// 在第一遍中，我们扫描网格，通过为它们分配临时 ID 来识别相连的区块，
// 如果两个区块相接触，则将它们标记为等价。
// 在第二遍中，我们将 ID 替换为从 1 开始的连续编号，这样使用起来更方便。
//
// 参见 https://en.wikipedia.org/wiki/Connected-component_labeling
//
class IslandFinder {
public:
	static const int MAX_ISLANDS = 256;

	template <typename VolumePredicate_F>
	void scan_3d(Box3i box, VolumePredicate_F volume_predicate_func, Span<uint8_t> output, unsigned int *out_count) {
		const size_t volume = Vector3iUtil::get_volume_u64(box.size);
		CRASH_COND(output.size() != volume);
		memset(output.data(), 0, volume * sizeof(uint8_t));

		memset(_equivalences.data(), 0, MAX_ISLANDS * sizeof(uint8_t));

		int top_label = 0;
		int left_label = 0;
		int back_label = 0;
		int next_unique_label = 1;

		Vector3i pos;
		for (pos.z = 0; pos.z < box.size.z; ++pos.z) {
			for (pos.x = 0; pos.x < box.size.x; ++pos.x) {
				// TODO 我最初按 ZYX 顺序编写该算法，移植到 C++ 时改成了 ZXY。
				// `left` 表示 `top`，而 `top` 表示 `left`。
				left_label = 0;

				for (pos.y = 0; pos.y < box.size.y; ++pos.y) {
					int label = 0;

					if (volume_predicate_func(box.position + pos)) {
						if (pos.z > 0) {
							back_label =
									output[Vector3iUtil::get_zxy_index(Vector3i(pos.x, pos.y, pos.z - 1), box.size)];
						} else {
							back_label = 0;
						}

						if (pos.x > 0) {
							top_label =
									output[Vector3iUtil::get_zxy_index(Vector3i(pos.x - 1, pos.y, pos.z), box.size)];
						} else {
							top_label = 0;
						}

						// TODO 这一堆 if 是我第一个能用的写法，但一定有办法可以简化

						if (left_label == 0 && top_label == 0 && back_label == 0) {
							// TODO 让算法改为返回值，否则调用方很难处理
							CRASH_COND(next_unique_label >= MAX_ISLANDS);
							_equivalences[next_unique_label] = 0;
							label = next_unique_label;
							++next_unique_label;

						} else if (left_label == 0 && top_label == 0) {
							label = back_label;

						} else if (left_label == 0 && back_label == 0) {
							label = top_label;

						} else if (top_label == 0 && back_label == 0) {
							label = left_label;

						} else if (left_label == 0 || //
								   (top_label != 0 && back_label != 0 &&
									(left_label == top_label || left_label == back_label))) {
							if (top_label == back_label) {
								label = back_label;

							} else if (top_label < back_label) {
								label = top_label;
								add_equivalence(back_label, top_label);

							} else {
								label = back_label;
								add_equivalence(top_label, back_label);
							}

						} else if (top_label == 0 || //
								   (left_label != 0 && back_label != 0 &&
									(top_label == left_label || top_label == back_label))) {
							if (left_label == back_label) {
								label = back_label;

							} else if (left_label < back_label) {
								label = left_label;
								add_equivalence(back_label, left_label);

							} else {
								label = back_label;
								add_equivalence(left_label, back_label);
							}

						} else if (back_label == 0 || //
								   (left_label != 0 && top_label != 0 &&
									(back_label == left_label || back_label == top_label))) {
							if (left_label == top_label) {
								label = top_label;

							} else if (left_label < top_label) {
								label = left_label;
								add_equivalence(top_label, left_label);

							} else {
								label = top_label;
								add_equivalence(left_label, top_label);
							}

						} else {
							int a[3] = { left_label, top_label, back_label };
							SortArray<int> sa;
							sa.sort(a, 3);
							label = a[0];
							add_equivalence(a[1], a[0]);
							add_equivalence(a[2], a[1]);
						}

						output[Vector3iUtil::get_zxy_index(pos, box.size)] = label;
					}

					left_label = label;
				}
			}
		}

		flatten_equivalences();
		int count = compact_labels(next_unique_label);

		if (out_count != nullptr) {
			*out_count = count;
		}

		for (unsigned int i = 0; i < output.size(); ++i) {
			uint8_t &c = output[i];
			uint8_t e = _equivalences[c];
			if (e != 0) {
				c = e;
			}
		}
	}

private:
	void add_equivalence(int upper, int lower) {
		CRASH_COND(upper <= lower);
		int prev_lower = _equivalences[upper];

		if (prev_lower == 0) {
			_equivalences[upper] = lower;

		} else if (prev_lower > lower) {
			_equivalences[upper] = lower;
			add_equivalence(prev_lower, lower);

		} else if (prev_lower < lower) {
			add_equivalence(lower, prev_lower);
		}
	}

	// 确保等价关系直接指向标签，而不经传递链接
	void flatten_equivalences() {
		for (int i = 1; i < MAX_ISLANDS; ++i) {
			int e = _equivalences[i];
			if (e == 0) {
				continue;
			}
			int e2 = _equivalences[e];
			while (e2 != 0) {
				e = e2;
				e2 = _equivalences[e];
			}
			_equivalences[i] = e;
		}
	}

	// 确保从等价关系中得到的标签是连续的，并从 1 开始。
	// 返回标签总数。
	int compact_labels(int equivalences_count) {
		int next_label = 1;
		for (int i = 1; i < equivalences_count; ++i) {
			const int e = _equivalences[i];
			if (e == 0) {
				// 该标签无等价项，为其分配一个索引
				_equivalences[i] = next_label;
				next_label += 1;
			} else {
				// 该标签有等价项，改为分配那个索引
				int e2 = _equivalences[e];
				_equivalences[i] = e2;
			}
		}
		// 我们从 1 开始，但结束于本应是下一个 ID 的值，因此减去 1 以得到总数
		return next_label - 1;
	}

private:
	FixedArray<uint8_t, MAX_ISLANDS> _equivalences;
};

} // namespace voxel

#endif // ISLAND_FINDER_H
