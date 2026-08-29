#ifndef VOXEL_TEST_DIRECTORY_H
#define VOXEL_TEST_DIRECTORY_H

#include "../godot/core/string.h"

namespace voxel::testing {

// 创建本类实例时创建一个临时目录，使用完毕后将其移除
class TestDirectory {
public:
	TestDirectory();
	~TestDirectory();

	bool is_valid() const {
		return _valid;
	}

	String get_path() const;

private:
	bool _valid = false;
};

} // namespace voxel::testing

#endif // VOXEL_TEST_DIRECTORY_H
