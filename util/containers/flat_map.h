#ifndef VOXEL_FLAT_MAP_H
#define VOXEL_FLAT_MAP_H

#include "container_funcs.h"
#include "span.h"
#include "std_vector.h"
#include <algorithm>

namespace voxel {

template <typename T>
struct FlatMapDefaultComparator {
	static inline bool less_than(const T &a, const T &b) {
		return a < b;
	}
};

// 基于有序内部向量的关联容器。
// 在少量唯一元素、需要按键查找又希望保持快速迭代时，是一个不错的折中。
// 不允许出现两个相同键的元素。
// 键和值的地址不保证稳定。
// 参见 https://riptutorial.com/cplusplus/example/7270/using-a-sorted-vector-for-fast-element-lookup
template <typename K, typename T, typename KComp = FlatMapDefaultComparator<K>>
class FlatMap {
public:
	struct Pair {
		K key;
		T value;

		// 用于 std::sort
		inline bool operator<(const Pair &other) const {
			return KComp::less_than(key, other.key);
		}

		// 用于 std::lower_bound
		inline bool operator<(const K &other_key) const {
			return KComp::less_than(key, other_key);
		}
	};

	// 若键已存在，则不插入该项并返回 false。
	// 若插入成功，返回 true。
	bool insert(K key, T value) {
		typename StdVector<Pair>::const_iterator it = std::lower_bound(_items.begin(), _items.end(), key);
		if (it != _items.end() && it->key == key) {
			// 项已存在
			return false;
		}
		_items.insert(it, Pair{ key, value });
		return true;
	}

	// 若键已存在，该项会替换之前的值。
	T &insert_or_assign(K key, T value) {
		typename StdVector<Pair>::iterator it = std::lower_bound(_items.begin(), _items.end(), key);
		if (it != _items.end() && it->key == key) {
			// 项已存在，直接赋值
			it->value = value;
		} else {
			// 项不存在，插入它
			it = _items.insert(it, Pair{ key, value });
		}
		return it->value;
	}

	// 从元素集合初始化。
	// 比逐项插入更快。
	void clear_and_insert(Span<Pair> pairs) {
		clear();
		_items.resize(pairs.size());
		for (size_t i = 0; i < pairs.size(); ++i) {
			_items[i] = pairs[i];
		}
		std::sort(_items.begin(), _items.end());
	}

	const T *find(K key) const {
		typename StdVector<Pair>::const_iterator it = std::lower_bound(_items.begin(), _items.end(), key);
		if (it != _items.end() && it->key == key) {
			return &it->value;
		}
		return nullptr;
	}

	T *find(K key) {
		typename StdVector<Pair>::iterator it = std::lower_bound(_items.begin(), _items.end(), key);
		if (it != _items.end() && it->key == key) {
			return &it->value;
		}
		return nullptr;
	}

	bool has(K key) const {
		// 使用 std::binary_search 非常麻烦。
		// 首先，我们不想传入 Pair，因为那需要构造一个 T。
		// "仅用键作为待搜索的"value"无法编译，因为 Pair 只提供了与 K 作为第二
		// 参数的比较。
		// 指定比较 lambda 也不可行，因为两个参数都需能转换为 K。
		// 要让它工作，需要传入一个带两个 operators()、参数顺序相反的结构体……
		// return std::binary_search(_items.cbegin(), _items.cend(), key);

		typename StdVector<Pair>::const_iterator it = std::lower_bound(_items.begin(), _items.end(), key);
		return it != _items.end() && it->key == key;
	}

	bool erase(K key) {
		typename StdVector<Pair>::iterator it = std::lower_bound(_items.begin(), _items.end(), key);
		if (it != _items.end() && it->key == key) {
			_items.erase(it);
			return true;
		}
		return false;
	}

	inline size_t size() const {
		return _items.size();
	}

	void clear() {
		_items.clear();
	}

	// template <typename F>
	// inline void for_each(F f) {
	// 	for (auto it = _items.begin(); it != _items.end(); ++it) {
	// 		// 不暴露修改键（key）的可能性
	// 		const K key = it->key;
	// 		f(key, it->value);
	// 	}
	// }

	// template <typename F>
	// inline void for_each_const(F f) const {
	// 	for (auto it = _items.begin(); it != _items.end(); ++it) {
	// 		// 不暴露修改键（key）的可能性
	// 		const K key = it->key;
	// 		f(key, it->value);
	// 	}
	// }

	// `bool predicate(FlatMap<K, T>::Pair)`
	template <typename F>
	inline void remove_if(F predicate) {
		_items.erase(std::remove_if(_items.begin(), _items.end(), predicate), _items.end());
	}

	void operator=(const FlatMap<K, T> &other) {
		_items = other._items;
	}

	bool operator==(const FlatMap<K, T> &other) const {
		return _items == other._items;
	}

	class ConstIterator {
	public:
		ConstIterator(const Pair *p) : _current(p) {}

		inline const Pair &operator*() {
#ifdef DEBUG_ENABLED
			VOXEL_ASSERT(_current != nullptr);
#endif
			return *_current;
		}

		inline const Pair *operator->() {
#ifdef DEBUG_ENABLED
			VOXEL_ASSERT(_current != nullptr);
#endif
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
		const Pair *_current;
	};

	inline ConstIterator begin() const {
		return ConstIterator(_items.empty() ? nullptr : &_items[0]);
	}

	inline ConstIterator end() const {
		return ConstIterator(_items.empty() ? nullptr : (&_items[0] + _items.size()));
	}

private:
	// 按键排序
	StdVector<Pair> _items;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// template <typename T>
// void insert_default(StdVector<T> &vec, size_t pi) {
// 	VOXEL_ASSERT(pi <= vec.size());
// 	const size_t prev_size = vec.size();
// 	vec.resize(vec.size() + 1);
// 	for (size_t i = pi; i < prev_size; ++i) {
// 		vec[i + 1] = std::move(vec[i]);
// 	}
// }

// FlatMap 的特化版本，其中 `T` 不可拷贝，仅可移动
template <typename K, typename T, typename KComp = FlatMapDefaultComparator<K>>
class FlatMapMoveOnly {
public:
	struct Pair {
		K key;
		T value;

		// 用于 std::sort
		inline bool operator<(const Pair &other) const {
			return KComp::less_than(key, other.key);
		}

		// 用于 std::lower_bound
		inline bool operator<(const K &other_key) const {
			return KComp::less_than(key, other_key);
		}

		Pair() {}

		Pair(const K &p_key, T &&p_value) {
			key = p_key;
			value = std::move(p_value);
		}

		Pair(Pair &&other) {
			key = other.key;
			value = std::move(other.value);
		}

		void operator=(Pair &&other) {
			key = other.key;
			value = std::move(other.value);
		}
	};

	inline const K &get_key_at_index(const unsigned int i) const {
		return _items[i].key;
	}

	inline const T &get_value_at_index(const unsigned int i) const {
		return _items[i].value;
	}

	// 若键已存在，则不插入该项并返回 false。
	// 若插入成功，返回 true。
	bool insert(K key, T &&value) {
		typename StdVector<Pair>::const_iterator it = std::lower_bound(_items.begin(), _items.end(), key);
		if (it != _items.end() && it->key == key) {
			// 项已存在
			return false;
		}
		_items.insert(it, std::move(Pair(key, std::move(value))));
		return true;
	}

	// 若键已存在，该项会替换之前的值。
	T &insert_or_assign(K key, T &&value) {
		typename StdVector<Pair>::iterator it = std::lower_bound(_items.begin(), _items.end(), key);
		if (it != _items.end() && it->key == key) {
			// 项已存在，直接赋值
			it->value = std::move(value);
		} else {
			// 项不存在，插入它
			it = _items.insert(it, std::move(Pair(key, std::move(value))));
		}
		return it->value;
	}

	// 从元素集合初始化。
	// 比逐项插入更快。
	void clear_and_insert(Span<Pair> pairs) {
		clear();
		_items.resize(pairs.size());
		for (size_t i = 0; i < pairs.size(); ++i) {
			_items[i] = std::move(pairs[i]);
		}
		std::sort(_items.begin(), _items.end());
	}

	const T *find(K key) const {
		typename StdVector<Pair>::const_iterator it = std::lower_bound(_items.begin(), _items.end(), key);
		if (it != _items.end() && it->key == key) {
			return &it->value;
		}
		return nullptr;
	}

	T *find(K key) {
		typename StdVector<Pair>::iterator it = std::lower_bound(_items.begin(), _items.end(), key);
		if (it != _items.end() && it->key == key) {
			return &it->value;
		}
		return nullptr;
	}

	bool has(K key) const {
		// 使用 std::binary_search 非常麻烦。
		// 首先，我们不想传入 Pair，因为那需要构造一个 T。
		// "仅用键作为待搜索的"value"无法编译，因为 Pair 只提供了与 K 作为第二
		// 参数的比较。
		// 指定比较 lambda 也不可行，因为两个参数都需能转换为 K。
		// 要让它工作，需要传入一个带两个 operators()、参数顺序相反的结构体……
		// return std::binary_search(_items.cbegin(), _items.cend(), key);

		typename StdVector<Pair>::const_iterator it = std::lower_bound(_items.begin(), _items.end(), key);
		return it != _items.end() && it->key == key;
	}

	bool erase(K key) {
		typename StdVector<Pair>::iterator it = std::lower_bound(_items.begin(), _items.end(), key);
		if (it != _items.end() && it->key == key) {
			_items.erase(it);
			return true;
		}
		return false;
	}

	inline size_t size() const {
		return _items.size();
	}

	void clear() {
		_items.clear();
	}

	// `bool predicate(FlatMap<K, T>::Pair)`
	template <typename F>
	inline void remove_if(F predicate) {
		_items.erase(std::remove_if(_items.begin(), _items.end(), predicate), _items.end());
	}

	template <typename F>
	void remap_keys_unchecked(F modifier) {
		for (Pair &p : _items) {
			p.key = modifier(p.key);
		}
#ifdef DEV_ENABLED
		VOXEL_ASSERT(!has_duplicate_f(to_span_const(_items), [](const Pair &a, const Pair &b) { return a.key == b.key; }));
#endif
		std::sort(_items.begin(), _items.end());
	}

	void operator=(const FlatMap<K, T> &other) {
		_items = other._items;
	}

	bool operator==(const FlatMap<K, T> &other) const {
		return _items == other._items;
	}

	class ConstIterator {
	public:
		ConstIterator(const Pair *p) : _current(p) {}

		inline const Pair &operator*() {
#ifdef DEBUG_ENABLED
			VOXEL_ASSERT(_current != nullptr);
#endif
			return *_current;
		}

		inline const Pair *operator->() {
#ifdef DEBUG_ENABLED
			VOXEL_ASSERT(_current != nullptr);
#endif
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
		const Pair *_current;
	};

	inline ConstIterator begin() const {
		return ConstIterator(_items.empty() ? nullptr : &_items[0]);
	}

	inline ConstIterator end() const {
		return ConstIterator(_items.empty() ? nullptr : (&_items[0] + _items.size()));
	}

private:
	// 按键排序
	StdVector<Pair> _items;
};

} // namespace voxel

#endif // VOXEL_FLAT_MAP_H
