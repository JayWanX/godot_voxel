#ifndef VOXEL_TEST_OPTIONS_H
#define VOXEL_TEST_OPTIONS_H

#include "../containers/std_vector.h"
#include "../godot/core/dictionary.h"
#include "../string/std_string.h"

namespace voxel::testing {

class TestOptions {
public:
	TestOptions() {}
	TestOptions(const Dictionary &options_dict);

	bool can_run_print(const char *test_name) const;

private:
	StdVector<StdString> _excludes;
	StdVector<StdString> _includes;
};

} // namespace voxel::testing

#endif // VOXEL_TEST_OPTIONS_H
