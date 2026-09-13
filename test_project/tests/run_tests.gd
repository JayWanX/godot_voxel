extends Node
## voxel 模块冒烟测试：覆盖体素缓冲与地块生成器的基本数据路径。

func _ready() -> void:
	var runner := TestRunner.new()
	_test_voxel_buffer(runner)
	_test_voxel_generator_flat(runner)
	_test_voxel_mesher(runner)
	runner.report()
	var exit_code: int = 0 if runner.is_pass() else 1
	get_tree().quit(exit_code)

## 测试体素缓冲的分配与读写。
func _test_voxel_buffer(runner: TestRunner) -> void:
	var buffer := VoxelBuffer.new()
	buffer.create(16, 8, 16)
	runner.assert_eq(buffer.get_size(), Vector3i(16, 8, 16), "缓冲区尺寸应正确")
	buffer.set_voxel(3, 1, 1, 1, VoxelBuffer.CHANNEL_TYPE)
	runner.assert_eq(buffer.get_voxel(1, 1, 1, VoxelBuffer.CHANNEL_TYPE), 3, "写入并读回的整数体素应一致")
	buffer.set_voxel_f(0.5, 2, 2, 2, VoxelBuffer.CHANNEL_SDF)
	var sdf_value: float = buffer.get_voxel_f(2, 2, 2, VoxelBuffer.CHANNEL_SDF)
	runner.assert_true(absf(sdf_value - 0.5) < 0.1, "浮点体素在量化容差内应一致")

## 测试平面地块生成器的数据填充。
func _test_voxel_generator_flat(runner: TestRunner) -> void:
	var generator := VoxelGeneratorFlat.new()
	generator.height = 4.0
	var buffer := VoxelBuffer.new()
	buffer.create(16, 16, 16)
	generator.generate_block(buffer, Vector3i(0, 0, 0), 0)
	runner.assert_eq(buffer.get_size(), Vector3i(16, 16, 16), "生成后缓冲区尺寸应不变")
	var below_sdf: float = buffer.get_voxel_f(8, 2, 8, VoxelBuffer.CHANNEL_SDF)
	var above_sdf: float = buffer.get_voxel_f(8, 14, 8, VoxelBuffer.CHANNEL_SDF)
	runner.assert_true(below_sdf < 0.0, "高度以下应处于实心（SDF 为负）")
	runner.assert_true(above_sdf > 0.0, "高度以上应处于空气（SDF 为正）")

## 测试根据像素图生成网格。
func _test_voxel_mesher(runner: TestRunner) -> void:
	var image := Image.create_empty(4, 4, false, Image.FORMAT_RGB8)
	image.fill(Color(0.5, 0.5, 0.5))
	var mesh: Mesh = VoxelMesherCubes.generate_mesh_from_image(image, 1.0)
	runner.assert_true(mesh != null, "根据像素图生成的网格不应为空")