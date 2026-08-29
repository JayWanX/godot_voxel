#ifndef VOXEL_SMALL_VECTOR_H
#define VOXEL_SMALL_VECTOR_H

#include "../errors.h"
#include "span.h"
#include <cstdint>
#include <memory>
#include <new>
#include <type_traits>

namespace voxel {

// 使用固定容量的动态元素序列。
// 用于元素数量较少、且最大数量已知且较小的情况。
// 出于性能考虑可优先于动态容量向量使用，因为它不发生堆分配。它并非可直接
// 替换的等价物，因为它不支持在超出容量时添加元素。
template <typename T, unsigned int N>
class SmallVector {
public:
	// 最初为 natvis 添加，不知为何 (T*) 无法工作
	using ValueType = T;

	// TODO 许多功能未支持，目前也用不到。
	// 由于 `reinterpret_cast`，无法使用 constexpr。不过目前我们也用不到。

	SmallVector() = default;

	SmallVector(const SmallVector<T, N> &other) {
		while (_size < other._size) {
			::new (&_items[_size]) T(other[_size]);
			++_size;
		}
	}

	~SmallVector() {
		for (unsigned int i = 0; i < _size; ++i) {
			std::destroy_at(std::launder(reinterpret_cast<T *>(&_items[i])));
		}
	}

	inline void push_back(const T &v) {
		VOXEL_ASSERT(_size < capacity());
		::new (&_items[_size]) T(v);
		++_size;
	}

	inline void clear() {
		// 销毁所有元素
		for (unsigned int i = 0; i < _size; ++i) {
			std::destroy_at(std::launder(reinterpret_cast<T *>(&_items[i])));
		}
		_size = 0;
	}

	void resize(unsigned int new_size) {
		if (_size == new_size) {
			return;
		}

		VOXEL_ASSERT(new_size <= capacity());

		// 默认构造新元素
		for (; _size < new_size; ++_size) {
			::new (&_items[_size]) T();
		}

		// 销毁多余的元素
		for (; _size > new_size; --_size) {
			std::destroy_at(std::launder(reinterpret_cast<T *>(&_items[_size])));
		}
	}

	void resize(unsigned int new_size, const T &default_value) {
		if (_size == new_size) {
			return;
		}

		VOXEL_ASSERT(new_size <= capacity());

		// 拷贝构造新元素
		for (; _size < new_size; ++_size) {
			::new (&_items[_size]) T(default_value);
		}

		// 销毁多余的元素
		for (; _size > new_size; --_size) {
			std::destroy_at(std::launder(reinterpret_cast<T *>(&_items[_size])));
		}
	}

	inline unsigned int size() const {
		return _size;
	}

	inline constexpr unsigned int capacity() const {
		return N;
	}

	inline const T *data() const {
		return &(*this)[0];
	}

	inline T &operator[](unsigned int i) {
#ifdef DEBUG_ENABLED
		VOXEL_ASSERT(i < N);
#endif
		return *std::launder(reinterpret_cast<T *>(&_items[i]));
	}

	inline const T &operator[](unsigned int i) const {
#ifdef DEBUG_ENABLED
		VOXEL_ASSERT(i < N);
#endif
		return *std::launder(reinterpret_cast<const T *>(&_items[i]));
	}

	SmallVector<T, N> &operator=(const SmallVector<T, N> &other) {
		if ((void *)this != (void *)&other) {
			clear();

			for (; _size < other.size(); ++_size) {
				::new (&_items[_size]) T(other[_size]);
			}
		}

		return *this;
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
		return Iterator(std::launder(reinterpret_cast<T *>(&_items[0])));
	}

	inline Iterator end() {
		return Iterator(std::launder(reinterpret_cast<T *>(&_items[_size])));
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
		return ConstIterator(std::launder(reinterpret_cast<const T *>(&_items[0])));
	}

	inline ConstIterator end() const {
		return ConstIterator(std::launder(reinterpret_cast<const T *>(&_items[_size])));
	}

private:
	// 该参考资料给出了使用 std::aligned_storage 实现 small vector 的方式：
	// https://en.cppreference.com/w/cpp/types/aligned_storage
	// 但它将在 C++23 中被弃用
	// https://stackoverflow.com/questions/71828288/why-is-stdaligned-storage-to-be-deprecated-in-c23-and-what-to-use-instead

	struct alignas(T) Item {
		uint8_t data[sizeof(T)];
	};

	static_assert(sizeof(T) == sizeof(Item), "Mismatch in size");
	static_assert(alignof(T) == alignof(Item), "Mismatch in alignment");

	// 逻辑元素数量。小于或等于 N。
	// 当 N 足够小时，是否值得改用 uint16_t 或 uint8_t？
	unsigned int _size = 0;

	Item _items[N];
};

template <typename T, unsigned int N>
Span<const T> to_span(const SmallVector<T, N> &v) {
	return Span<const T>(v.data(), v.size());
}

} // namespace voxel

#endif // VOXEL_SMALL_VECTOR_H
