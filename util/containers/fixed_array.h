#ifndef VOXEL_FIXED_ARRAY_H
#define VOXEL_FIXED_ARRAY_H

#include "../errors.h"
#include "span.h"

namespace voxel {

// TODO 本可使用 std::array，但由于 Godot 的编译方式，
// 我找不到在不导致模块与 Godot 其余部分链接失败的前提下启用边界检查的方法……
// 参见 https://github.com/godotengine/godot/issues/31608
template <typename T, unsigned int N>
class FixedArray {
public:
	// TODO 优化：移动语义

	inline T &operator[](unsigned int i) {
#ifdef DEBUG_ENABLED
		VOXEL_ASSERT(i < N);
#endif
		return _data[i];
	}

	inline const T &operator[](unsigned int i) const {
#ifdef DEBUG_ENABLED
		VOXEL_ASSERT(i < N);
#endif
		return _data[i];
	}

	inline bool equals(const FixedArray<T, N> &other) const {
		for (unsigned int i = 0; i < N; ++i) {
			if (_data[i] != other._data[i]) {
				return false;
			}
		}
		return true;
	}

	inline bool operator==(const FixedArray<T, N> &other) const {
		return equals(other);
	}

	inline bool operator!=(const FixedArray<T, N> &other) const {
		return !equals(other);
	}

	inline constexpr T *data() {
		return _data;
	}

	inline constexpr const T *data() const {
		return _data;
	}

#if defined(__GNUC__)
	// 告诉 GCC 该函数仅依赖于其参数（无）且不访问 `this`。
	// 规避对 `size()` 的调用有时产生的过于严格的 `maybe-uninitialized` 警告，
	// 尽管其实无需初始化，且根本未访问 `this`。
	__attribute__((const))
#endif
	inline constexpr unsigned int
	size() const {
		return N;
	}

	class ConstIterator {
	public:
		inline ConstIterator(const T *p) : _current(p) {}

		inline const T &operator*() {
			return *_current;
		}

		inline const T *operator->() {
			return _current;
		}

		inline ConstIterator &operator++() {
			++_current;
			return *this;
		}

		inline bool operator==(const ConstIterator other) const {
			return _current == other._current;
		}

		inline bool operator!=(const ConstIterator other) const {
			return _current != other._current;
		}

	private:
		const T *_current;
	};

	inline ConstIterator begin() const {
		return ConstIterator(_data);
	}

	inline ConstIterator end() const {
		return ConstIterator(_data + N);
	}

	class Iterator {
	public:
		inline Iterator(T *p) : _current(p) {}

		inline T &operator*() {
			return *_current;
		}

		inline T *operator->() {
			return _current;
		}

		inline Iterator &operator++() {
			++_current;
			return *this;
		}

		inline bool operator==(Iterator other) const {
			return _current == other._current;
		}

		inline bool operator!=(Iterator other) const {
			return _current != other._current;
		}

	private:
		T *_current;
	};

	inline Iterator begin() {
		return Iterator(_data);
	}

	inline Iterator end() {
		return Iterator(_data + N);
	}

private:
	T _data[N];
};

// 用相同的值填充数组。
// 不是成员函数，否则对不可拷贝类型无法编译。
template <typename T, unsigned int N>
inline void fill(FixedArray<T, N> &dst, const T v) {
	for (unsigned int i = 0; i < dst.size(); ++i) {
		dst[i] = v;
	}
}

template <typename T, unsigned int N>
inline bool find(const FixedArray<T, N> &a, const T &v, unsigned int &out_index) {
	for (unsigned int i = 0; i < a.size(); ++i) {
		if (a[i] == v) {
			out_index = i;
			return true;
		}
	}
	return false;
}

template <typename T, unsigned int N>
inline bool contains(const FixedArray<T, N> &a, const T &value_to_search) {
	for (const T &v : a) {
		if (v == value_to_search) {
			return true;
		}
	}
	return false;
}

template <typename T, unsigned int N>
Span<T> to_span(FixedArray<T, N> &a) {
	return Span<T>(a.data(), a.size());
}

template <typename T, unsigned int N>
Span<const T> to_span(const FixedArray<T, N> &a) {
	return Span<const T>(a.data(), a.size());
}

template <typename T, unsigned int N>
Span<T> to_span(FixedArray<T, N> &a, unsigned int count) {
	VOXEL_ASSERT(count <= a.size());
	return Span<T>(a.data(), count);
}

// TODO 弃用，现在 Span 拥有转换构造函数可以实现这一点
template <typename T, unsigned int N>
Span<const T> to_span_const(const FixedArray<T, N> &a, unsigned int count) {
	VOXEL_ASSERT(count <= a.size());
	return Span<const T>(a.data(), count);
}

// TODO 弃用，现在 Span 拥有转换构造函数可以实现这一点
template <typename T, unsigned int N>
Span<const T> to_span_const(const FixedArray<T, N> &a) {
	return Span<const T>(a.data(), 0, a.size());
}

} // namespace voxel

#endif // VOXEL_FIXED_ARRAY_H
