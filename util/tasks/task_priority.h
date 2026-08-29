#ifndef VOXEL_TASK_PRIORITY_H
#define VOXEL_TASK_PRIORITY_H

#include <cstdint>

namespace voxel {

// 表示任务的优先级，可快速地与另一个进行比较。
struct TaskPriority {
	static const uint8_t BAND_MAX = 255;

	union {
		struct {
			uint8_t band0; // 数值越大表示优先级越高（显而易见）
			uint8_t band1; // 优先于 band0
			uint8_t band2; // 优先于 band1
			uint8_t band3; // 优先于 band2
		};
		uint32_t whole;
	};

	TaskPriority() : whole(0) {}

	TaskPriority(uint8_t p_band0, uint8_t p_band1, uint8_t p_band2, uint8_t p_band3) :
			band0(p_band0), band1(p_band1), band2(p_band2), band3(p_band3) {}

	// 若左侧优先级低于右侧，返回 `true`。
	// 意味着右侧任务应优先运行。
	inline bool operator<(const TaskPriority &other) const {
		return whole < other.whole;
	}

	// 若左侧优先级高于右侧，返回 `true`。
	// 意味着左侧任务应优先运行。
	inline bool operator>(const TaskPriority &other) const {
		return whole > other.whole;
	}

	inline bool operator==(const TaskPriority &other) const {
		return whole == other.whole;
	}

	static inline TaskPriority min() {
		TaskPriority p;
		p.whole = 0;
		return p;
	}

	static inline TaskPriority max() {
		TaskPriority p;
		p.whole = 0xffffffff;
		return p;
	}
};

} // namespace voxel

#endif // VOXEL_TASK_PRIORITY_H
