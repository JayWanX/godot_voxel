#include "test_container_funcs.h"
#include "../../util/containers/container_funcs.h"
#include "../../util/containers/std_vector.h"
#include "../../util/testing/test_macros.h"

namespace voxel::tests {

void test_unordered_remove_if() {
	struct L {
		static unsigned int count(const StdVector<int> &vec, int v) {
			unsigned int n = 0;
			for (size_t i = 0; i < vec.size(); ++i) {
				if (vec[i] == v) {
					++n;
				}
			}
			return n;
		}
	};
	// 移除开头的一个元素
	{
		StdVector<int> vec;
		vec.push_back(0);
		vec.push_back(1);
		vec.push_back(2);
		vec.push_back(3);

		unordered_remove_if(vec, [](int v) { return v == 0; });

		VOXEL_TEST_ASSERT(vec.size() == 3);
		VOXEL_TEST_ASSERT(
				L::count(vec, 0) == 0 && L::count(vec, 1) == 1 && L::count(vec, 2) == 1 && L::count(vec, 3) == 1
		);
	}
	// 移除中间的一个元素
	{
		StdVector<int> vec;
		vec.push_back(0);
		vec.push_back(1);
		vec.push_back(2);
		vec.push_back(3);

		unordered_remove_if(vec, [](int v) { return v == 2; });

		VOXEL_TEST_ASSERT(vec.size() == 3);
		VOXEL_TEST_ASSERT(
				L::count(vec, 0) == 1 && L::count(vec, 1) == 1 && L::count(vec, 2) == 0 && L::count(vec, 3) == 1
		);
	}
	// 移除末尾的一个元素
	{
		StdVector<int> vec;
		vec.push_back(0);
		vec.push_back(1);
		vec.push_back(2);
		vec.push_back(3);

		unordered_remove_if(vec, [](int v) { return v == 3; });

		VOXEL_TEST_ASSERT(vec.size() == 3);
		VOXEL_TEST_ASSERT(
				L::count(vec, 0) == 1 && L::count(vec, 1) == 1 && L::count(vec, 2) == 1 && L::count(vec, 3) == 0
		);
	}
	// 移除多个元素
	{
		StdVector<int> vec;
		vec.push_back(0);
		vec.push_back(1);
		vec.push_back(2);
		vec.push_back(3);

		unordered_remove_if(vec, [](int v) { return v == 1 || v == 2; });

		VOXEL_TEST_ASSERT(vec.size() == 2);
		VOXEL_TEST_ASSERT(
				L::count(vec, 0) == 1 && L::count(vec, 1) == 0 && L::count(vec, 2) == 0 && L::count(vec, 3) == 1
		);
	}
	// 移除仅剩的最后一个元素
	{
		StdVector<int> vec;
		vec.push_back(0);

		unordered_remove_if(vec, [](int v) { return v == 0; });

		VOXEL_TEST_ASSERT(vec.size() == 0);
	}
}

} // namespace voxel::tests
