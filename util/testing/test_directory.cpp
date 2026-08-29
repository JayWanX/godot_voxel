#include "test_directory.h"
#include "../godot/classes/directory.h"
#include "../godot/classes/file_access.h"

namespace voxel::testing {

const char *DEFAULT_TEST_DATA_DIRECTORY = "voxel_testing_dir";

bool create_empty_file(String fpath) {
	if (!voxel::godot::file_exists(fpath)) {
		Ref<FileAccess> f = FileAccess::open(fpath, FileAccess::WRITE);
		if (f.is_valid()) {
			f->store_line("");
		} else {
			ERR_PRINT("Failed to create file " + fpath);
			return false;
		}
	}
	return true;
}

bool remove_dir_if_exists(const char *p_dirpath) {
	String dirpath = p_dirpath;
	// 如果这是空字符串，我们可能会删掉整个项目，不要冒这个险
	ERR_FAIL_COND_V(dirpath.is_empty(), false);

	Ref<DirAccess> da = DirAccess::open(".");
	if (da->dir_exists(dirpath)) {
		String prev_dir = da->get_current_dir();

		// 注意，这不会改变应用程序的工作目录。
		// 必须先做这一步，非常重要，否则 `erase_contents_recursive` 会擦除整个项目
		const Error cd_err = da->change_dir(dirpath);
		ERR_FAIL_COND_V(cd_err != OK, false);

		// 如果目录非空，`remove` 会失败
		const Error contents_remove_err = voxel::godot::erase_directory_contents_recursive(**da);
		ERR_FAIL_COND_V(contents_remove_err != OK, false);
		// OS::get_singleton()->move_to_trash(dirpath); // ?

		const Error cd_err2 = da->change_dir(prev_dir);
		ERR_FAIL_COND_V(cd_err2 != OK, false);

		// 移除空目录
		const Error remove_err = da->remove(dirpath);
		ERR_FAIL_COND_V(remove_err != OK, false);
	}

	return true;
}

bool create_clean_dir(const char *dirpath) {
	ERR_FAIL_COND_V(dirpath == nullptr, false);
	Ref<DirAccess> da = DirAccess::open(".");
	ERR_FAIL_COND_V(da.is_null(), false);

	ERR_FAIL_COND_V(!remove_dir_if_exists(dirpath), false);

	const Error err = da->make_dir(dirpath);
	ERR_FAIL_COND_V(err != OK, false);

	ERR_FAIL_COND_V(!create_empty_file(String(dirpath).path_join(".gdignore")), false);

	return true;
}

TestDirectory::TestDirectory() {
	ERR_FAIL_COND(!create_clean_dir(DEFAULT_TEST_DATA_DIRECTORY));
	_valid = true;
}

TestDirectory::~TestDirectory() {
	ERR_FAIL_COND(!remove_dir_if_exists(DEFAULT_TEST_DATA_DIRECTORY));
}

String TestDirectory::get_path() const {
	return DEFAULT_TEST_DATA_DIRECTORY;
}

} // namespace voxel::testing
