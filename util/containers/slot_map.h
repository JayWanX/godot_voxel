#ifndef VOXEL_SLOT_MAP_H
#define VOXEL_SLOT_MAP_H

#include "../errors.h"
#include "std_vector.h"
#include <cstdint>
#include <limits>

namespace voxel {

// SlotMap 键的版本部分。最后一位用于表示未使用槽位的版本。
template <typename TVersion>
struct SlotMapVersion {
	static const uint32_t UNUSED_BIT = 1 << (8 * sizeof(TVersion) - 1);
	static const uint32_t MASK = UNUSED_BIT - 1;
	static const uint32_t MAX_VALUE = MASK;

	TVersion value = 0;

	inline bool operator==(const SlotMapVersion &other) const {
		return value == other.value;
	}

	inline bool operator!=(const SlotMapVersion &other) const {
		return value != other.value;
	}

	inline bool is_valid() const {
		return (value & UNUSED_BIT) == 0;
	}

	inline bool is_invalid() const {
		return (value & UNUSED_BIT) != 0;
	}

	inline void make_valid() {
		value &= MASK;
	}

	inline void make_invalid() {
		VOXEL_ASSERT(is_valid());
#if DEBUG_ENABLED
		if (value == MAX_VALUE) {
			VOXEL_PRINT_WARNING("SlotMapVersion overflow");
		}
#endif
		value = (value + 1) | UNUSED_BIT;
	}
};

// 索引与版本的组合，用于标识槽位的一次具体占用。
template <typename TIndex, typename TVersion>
struct SlotMapKey {
	// 槽位数组中的索引
	TIndex index = 0;
	// 槽位必须拥有的版本，以与键匹配
	SlotMapVersion<TVersion> version;

	inline bool operator==(const SlotMapKey &other) const {
		return index == other.index && version == other.version;
	}

	inline bool operator!=(const SlotMapKey &other) const {
		return index != other.index || version != other.version;
	}
};

// 在 O(1) 访问的容器中存储具有唯一标识的值。
// 若要在某处存储引用，务必使用 ID。值的地址并不稳定。
// ID 通过代际（generation）系统保证唯一，且永远不会被复用。
// 该概念与 https://docs.rs/slotmap/latest/slotmap/ 类似
template <typename T, typename TIndex = uint32_t, typename TVersion = uint32_t>
class SlotMap {
private:
	struct Slot {
		T value;
		SlotMapVersion<TVersion> version;
	};

public:
	typedef SlotMapKey<TIndex, TVersion> Key;

	Key add(T value) {
		if (_free_list.size() > 0) {
			const TIndex i = _free_list.back();
			_free_list.pop_back();
			Slot &slot = _slots[i];
			slot.value = value;
			slot.version.make_valid();
			++_count;
			return Key{ i, slot.version };
		} else {
			const TIndex i = _slots.size();
			// 版本从 1 开始，这样可以用 0 表示无效
			const uint32_t v = 1;
			_slots.push_back(Slot{ value, { v } });
			++_count;
			return Key{ i, { v } };
		}
	}

	T *try_get(Key key) {
		VOXEL_ASSERT_RETURN_V(key.version.is_valid(), nullptr);
		if (key.index >= _slots.size()) {
			return nullptr;
		}
		Slot &slot = _slots[key.index];
		if (slot.version != key.version) {
			return nullptr;
		}
		return &slot.value;
	}

	const T *try_get(Key key) const {
		VOXEL_ASSERT_RETURN_V(key.version.is_valid(), nullptr);
		if (key.index >= _slots.size()) {
			return nullptr;
		}
		const Slot &slot = _slots[key.index];
		if (slot.version != key.version) {
			return nullptr;
		}
		return &slot.value;
	}

	bool exists(Key key) const {
		VOXEL_ASSERT_RETURN_V(key.version.is_valid(), false);
		if (key.index >= _slots.size()) {
			return false;
		}
		const Slot &slot = _slots[key.index];
		if (slot.version != key.version) {
			return false;
		}
		return true;
	}

	bool try_remove(Key key) {
		VOXEL_ASSERT_RETURN_V(key.version.is_valid(), false);
		if (key.index >= _slots.size()) {
			return false;
		}
		Slot &slot = _slots[key.index];
		if (slot.version != key.version) {
			return false;
		}
		slot.value = T();
		slot.version.make_invalid();
		VOXEL_ASSERT(_count > 0);
		--_count;
		_free_list.push_back(key.index);
		return true;
	}

	T &get(Key key) {
		T *v = try_get(key);
		VOXEL_ASSERT(v != nullptr);
		return *v;
	}

	const T &get(Key key) const {
		const T *v = try_get(key);
		VOXEL_ASSERT(v != nullptr);
		return *v;
	}

	void remove(Key key) {
		VOXEL_ASSERT(try_remove(key));
	}

	void clear() {
		_slots.clear();
		_free_list.clear();
		_count = 0;
	}

	uint32_t count() const {
		return _count;
	}

	template <typename F>
	void for_each_value(F f) {
		for (Slot &slot : _slots) {
			if (slot.version.is_valid()) {
				f(slot.value);
			}
		}
	}

	template <typename F>
	void for_each_value(F f) const {
		for (const Slot &slot : _slots) {
			if (slot.version.is_valid()) {
				f(slot.value);
			}
		}
	}

	template <typename F>
	void for_each_key_value(F f) {
		TIndex i = 0;
		for (Slot &slot : _slots) {
			if (slot.version.is_valid()) {
				f(Key{ i, slot.version }, slot.value);
			}
			++i;
		}
	}

	template <typename F>
	void for_each_key_value(F f) const {
		TIndex i = 0;
		for (const Slot &slot : _slots) {
			if (slot.version.is_valid()) {
				f(Key{ i, slot.version }, slot.value);
			}
			++i;
		}
	}

private:
	StdVector<Slot> _slots;
	// TODO 空闲链表可以不借助 vector 更高效地实现，但会带来一些复杂度。
	// 如果每个槽位中的值是 TIndex 与 T 的联合体，无效槽位可将该值解释为指向下
	// 一个空闲槽位的 TIndex。但当 T 具有非平凡构造/析构函数时，无法将其放入联合体。
	StdVector<TIndex> _free_list;
	uint32_t _count = 0;
};

} // namespace voxel

#endif // VOXEL_SLOT_MAP_H
