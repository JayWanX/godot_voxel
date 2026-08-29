#include "transvoxel.h"
#include "../../constants/cube_tables.h"
#include "../../storage/mixel4.h"
#include "../../util/godot/core/sort_array.h"
#include "../../util/math/conv.h"
#include "../../util/math/funcs.h"
#include "../../util/profiling.h"
#include "transvoxel_materials_mixel4.h"
#include "transvoxel_materials_null.h"
#include "transvoxel_materials_single_s4.h"
#include "transvoxel_tables.cpp"

// 质量不佳，因此目前不可用。
#ifdef VOXEL_ENABLE_TRANSVOXEL_MATERIAL_SINGLE_S2
#include "transvoxel_materials_single_s2.h"
#endif

// #define VOXEL_TRANSVOXEL_REUSE_VERTEX_ON_COINCIDENT_CASES

namespace voxel::transvoxel {

static const float TRANSITION_CELL_SCALE = 0.25;

// 在此算法中，视为负的 SDF 值其符号位为 1
inline uint8_t sign_f(float v) {
	return v < 0.f;
}

Vector3f get_border_offset(const Vector3f pos_scaled, const int lod_index, const Vector3i block_size_non_scaled) {
	// 当在具有不同 LOD 的数据块之间插入过渡网格时，我们需要为其留出空间。
	// 次级顶点位置可以通过对边界单元内的位置进行线性变换来计算，
	// 使全尺寸单元缩放到较小的尺寸，从而根据其在整个数据块边缘和角落的位置，
	// 为一到三个过渡单元留出必要的空间。这可以通过为任意边界单元中的坐标 (x, y, z)
	// 计算偏移量 (Δx, Δy, Δz) 来实现。

	Vector3f delta;

	const float p2k = 1 << lod_index; // 2 ^ lod
	const float p2mk = 1.f / p2k; // 2 ^ (-lod)

	const float wk = TRANSITION_CELL_SCALE * p2k; // 2 ^ (lod - 2)，如果缩放比例为 0.25

	for (unsigned int i = 0; i < Vector3iUtil::AXIS_COUNT; ++i) {
		const float p = pos_scaled[i];
		const float s = block_size_non_scaled[i];

		if (p < p2k) {
			// 顶点位于最小单元内部。
			delta[i] = (1.0f - p2mk * p) * wk;

		} else if (p > (p2k * (s - 1))) {
			// 顶点位于最大单元内部。
			delta[i] = (s - 1.0f - p2mk * p) * wk;
		}
	}

	return delta;
}

inline Vector3f project_border_offset(Vector3f delta, Vector3f normal) {
	// 次级位置可通过以下公式计算：
	//
	// | x |   | 1 - nx²   ,  -nx * ny  ,  -nx * nz |   | Δx |
	// | y | + | -nx * ny  ,  1 - ny²   ,  -ny * nz | * | Δy |
	// | z |   | -nx * nz  ,  -ny * nz  ,  1 - nz²  |   | Δz |
	//
	// clang-format off
	return Vector3f(
		(1 - normal.x * normal.x) * delta.x -      normal.y * normal.x  * delta.y -      normal.z * normal.x  * delta.z,
		    -normal.x * normal.y  * delta.x + (1 - normal.y * normal.y) * delta.y -      normal.z * normal.y  * delta.z,
		    -normal.x * normal.z  * delta.x -      normal.y * normal.z  * delta.y + (1 - normal.z * normal.z) * delta.z
	);
	// clang-format on
}

inline Vector3f get_secondary_position(
		const Vector3f primary,
		const Vector3f normal,
		const int lod_index,
		const Vector3i block_size_non_scaled
) {
	Vector3f delta = get_border_offset(primary, lod_index, block_size_non_scaled);
	delta = project_border_offset(delta, normal);

	// 在非常低的 LOD 级别下，误差可能很大，使次级位置偏离很远。
	// 在 LOD 8 左右出现过这种情况。这并不能消除它们，但应该使它们不那么明显。
	const float p2k = 1 << lod_index;
	delta = math::clamp(delta, Vector3f(-p2k), Vector3f(p2k));

	return primary + delta;
}

inline uint8_t get_border_mask(const Vector3i &pos, const Vector3i &block_size) {
	uint8_t mask = 0;

	//  1: -X
	//  2: +X
	//  4: -Y
	//  8: +Y
	// 16: -Z
	// 32: +Z

	for (unsigned int i = 0; i < Vector3iUtil::AXIS_COUNT; i++) {
		// 靠近负方向的面。
		if (pos[i] == 0) {
			mask |= (1 << (i * 2));
		}
		// 靠近正方向的面。
		if (pos[i] == block_size[i]) {
			mask |= (1 << (i * 2 + 1));
		}
	}

	return mask;
}

inline Vector3f normalized_not_null(Vector3f n) {
	const float lengthsq = math::length_squared(n);
	if (lengthsq == 0) {
		return Vector3f(0, 1, 0);
	} else {
		const float length = Math::sqrt(lengthsq);
		return Vector3f(n.x / length, n.y / length, n.z / length);
	}
}

inline Vector3i dir_to_prev_vec(uint8_t dir) {
	// return g_corner_dirs[mask] - Vector3f(1,1,1);
	return Vector3i(-(dir & 1), -((dir >> 1) & 1), -((dir >> 2) & 1));
}

inline float sdf_as_float(int8_t v) {
	return -s8_to_snorm_noclamp(v);
}

inline float sdf_as_float(int16_t v) {
	return -s16_to_snorm_noclamp(v);
}

inline float sdf_as_float(float v) {
	return -v;
}

inline float sdf_as_float(double v) {
	return -v;
}

template <typename Sdf_T>
inline Vector3f get_corner_gradient(unsigned int data_index, Span<const Sdf_T> sdf_data, const Vector3i block_size) {
	const unsigned int n010 = 1; // Y+1
	const unsigned int n100 = block_size.y; // X+1
	const unsigned int n001 = block_size.y * block_size.x; // Z+1

	const float nx = sdf_as_float(sdf_data[data_index - n100]);
	const float px = sdf_as_float(sdf_data[data_index + n100]);
	const float ny = sdf_as_float(sdf_data[data_index - n010]);
	const float py = sdf_as_float(sdf_data[data_index + n010]);
	const float nz = sdf_as_float(sdf_data[data_index - n001]);
	const float pz = sdf_as_float(sdf_data[data_index + n001]);

	// get_gradient_normal(nx, px, ny, py, nz, pz, cell_samples[i]);
	return Vector3f(nx - px, ny - py, nz - pz);
}

template <typename Sdf_T>
inline Sdf_T get_isolevel() = delete;

template <>
inline int8_t get_isolevel<int8_t>() {
	return 0;
}

template <>
inline int16_t get_isolevel<int16_t>() {
	return 0;
}

template <>
inline float get_isolevel<float>() {
	return 0.f;
}

// 该函数是模板，这样在采样体素时就可以避免分支和检查
template <typename TSdf, typename TMaterialProcessor>
void build_regular_mesh(
		Span<const TSdf> sdf_data,
		TMaterialProcessor material_processor,
		const Vector3i block_size_with_padding,
		uint32_t lod_index,
		Cache &cache,
		MeshArrays &output,
		StdVector<CellInfo> *cell_info,
		const float edge_clamp_margin
) {
	VOXEL_PROFILE_SCOPE();

	const float edge_clamp_margin_max = 1.f - edge_clamp_margin;

	// 该函数中的一些注释引自 Transvoxel 论文。

	const Vector3i block_size = block_size_with_padding - Vector3iUtil::create(MIN_PADDING + MAX_PADDING);
	const Vector3i block_size_scaled = block_size << lod_index;

	// 准备顶点复用缓存
	cache.reset_reuse_cells(block_size_with_padding);

	// 我们遍历 2x2x2 的体素组，论文中称之为“单元”（cell）。
	// 我们还会多取一个体素来计算法线，因此需要调整遍历区域
	const Vector3i min_pos = Vector3iUtil::create(MIN_PADDING);
	const Vector3i max_pos = block_size_with_padding - Vector3iUtil::create(MAX_PADDING);

	// 在数据数组中推进多少以获取相邻体素
	const unsigned int n010 = 1; // Y+1
	const unsigned int n100 = block_size_with_padding.y; // X+1
	const unsigned int n001 = block_size_with_padding.y * block_size_with_padding.x; // Z+1
	const unsigned int n110 = n010 + n100;
	const unsigned int n101 = n100 + n001;
	const unsigned int n011 = n010 + n001;
	const unsigned int n111 = n100 + n010 + n001;

	// 获取等值面的直接表示（由于我们尚未使用有符号整数，所以并不总是零）
	const TSdf isolevel = get_isolevel<TSdf>();

	// 遍历所有带填充的单元（预期是相邻单元）
	Vector3i pos;
	for (pos.z = min_pos.z; pos.z < max_pos.z; ++pos.z) {
		for (pos.y = min_pos.y; pos.y < max_pos.y; ++pos.y) {
			// TODO 优化：是否将迭代改为 ZXY 顺序？（数据以 Y 作为最深坐标布局）
			unsigned int data_index =
					Vector3iUtil::get_zxy_index(Vector3i(min_pos.x, pos.y, pos.z), block_size_with_padding);

			for (pos.x = min_pos.x; pos.x < max_pos.x; ++pos.x, data_index += block_size_with_padding.y) {
				{
					// 这里选择的比较方式非常重要。这与 4 个样本等于等值面而另外 4 个在上或下的情况选择有关：
					// 在这两种情况下，必须提取出一个表面，否则当表面恰好与整数坐标对齐时将不允许出现。
					// 如果我们使用 `<` 而不是 `>`，看起来可行，但会破坏这些边界情况。
					// 选择 `>` 是因为它必须与我们在情况选择中进行的比较保持一致（在 Transvoxel 中是相反的）。
					const bool s = sdf_data[data_index] > isolevel;

					if ( //
							(sdf_data[data_index + n010] > isolevel) == s &&
							(sdf_data[data_index + n100] > isolevel) == s &&
							(sdf_data[data_index + n110] > isolevel) == s &&
							(sdf_data[data_index + n001] > isolevel) == s &&
							(sdf_data[data_index + n011] > isolevel) == s &&
							(sdf_data[data_index + n101] > isolevel) == s &&
							(sdf_data[data_index + n111] > isolevel) == s) {
						// 未跨越等值面，此单元不会产生任何几何体。
						// 我们必须尽可能快地判断这一点，因为这种情况会经常发生。
						continue;
					}
				}

				//    6-------7
				//   /|      /|
				//  / |     / |  Corners
				// 4-------5  |
				// |  2----|--3
				// | /     | /   z y
				// |/      |/    |/
				// 0-------1     o--x

				FixedArray<unsigned int, 8> corner_data_indices;
				corner_data_indices[0] = data_index;
				corner_data_indices[1] = data_index + n100;
				corner_data_indices[2] = data_index + n010;
				corner_data_indices[3] = data_index + n110;
				corner_data_indices[4] = data_index + n001;
				corner_data_indices[5] = data_index + n101;
				corner_data_indices[6] = data_index + n011;
				corner_data_indices[7] = data_index + n111;

				FixedArray<float, 8> cell_samples_sdf;
				for (unsigned int i = 0; i < corner_data_indices.size(); ++i) {
					cell_samples_sdf[i] = sdf_as_float(sdf_data[corner_data_indices[i]]);
				}

				// 拼接单元值的符号以得到情况代码（case code）。
				// 索引 0 是最低有效位，索引 7 是最高有效位。
				uint8_t case_code = sign_f(cell_samples_sdf[0]);
				case_code |= (sign_f(cell_samples_sdf[1]) << 1);
				case_code |= (sign_f(cell_samples_sdf[2]) << 2);
				case_code |= (sign_f(cell_samples_sdf[3]) << 3);
				case_code |= (sign_f(cell_samples_sdf[4]) << 4);
				case_code |= (sign_f(cell_samples_sdf[5]) << 5);
				case_code |= (sign_f(cell_samples_sdf[6]) << 6);
				case_code |= (sign_f(cell_samples_sdf[7]) << 7);

				// TODO 既然我们已经提前检查过等值面，这个真的还需要吗？
				if (case_code == 0 || case_code == 255) {
					// 如果 case_code 为 0 或 255，则无需进行三角剖分。
					continue;
				}

				ReuseCell &current_reuse_cell = cache.get_reuse_cell(pos);

#if DEBUG_ENABLED
				VOXEL_ASSERT(case_code <= 255);
#endif

				FixedArray<Vector3i, 8> padded_corner_positions;
				padded_corner_positions[0] = Vector3i(pos.x, pos.y, pos.z);
				padded_corner_positions[1] = Vector3i(pos.x + 1, pos.y, pos.z);
				padded_corner_positions[2] = Vector3i(pos.x, pos.y + 1, pos.z);
				padded_corner_positions[3] = Vector3i(pos.x + 1, pos.y + 1, pos.z);
				padded_corner_positions[4] = Vector3i(pos.x, pos.y, pos.z + 1);
				padded_corner_positions[5] = Vector3i(pos.x + 1, pos.y, pos.z + 1);
				padded_corner_positions[6] = Vector3i(pos.x, pos.y + 1, pos.z + 1);
				padded_corner_positions[7] = Vector3i(pos.x + 1, pos.y + 1, pos.z + 1);

				current_reuse_cell.packed_texture_indices = material_processor.on_cell(corner_data_indices, case_code);

				FixedArray<Vector3i, 8> corner_positions;
				for (unsigned int i = 0; i < padded_corner_positions.size(); ++i) {
					const Vector3i p = padded_corner_positions[i];
					// 在此撤销填充。从这一点开始，角点位置即为实际位置。
					corner_positions[i] = (p - min_pos) << lod_index;
				}

				// 对于沿数据块最小边界出现的单元，
				// 顶点复用所需的前一单元可能不存在。
				// 在这些情况下，我们允许在单元的附加边上创建新顶点。
				// 在遍历数据块中的单元时，会维护一个 3 位掩码，其位指示
				// 方向代码中的对应位是否有效
				const uint8_t direction_validity_mask = (pos.x > min_pos.x ? 1 : 0) |
						((pos.y > min_pos.y ? 1 : 0) << 1) | ((pos.z > min_pos.z ? 1 : 0) << 2);

				const uint8_t regular_cell_class_index = tables::get_regular_cell_class(case_code);
				const tables::RegularCellData &regular_cell_data =
						tables::get_regular_cell_data(regular_cell_class_index);
				const uint8_t triangle_count = regular_cell_data.geometryCounts & 0x0f;
				const uint8_t vertex_count = (regular_cell_data.geometryCounts & 0xf0) >> 4;

				FixedArray<int, 12> cell_vertex_indices;
				fill(cell_vertex_indices, -1);

				// TODO 不使用 LOD 时，这并非必需
				const uint8_t cell_border_mask = get_border_mask(pos - min_pos, block_size - Vector3i(1, 1, 1));

				// 对情况中的每个顶点
				for (unsigned int vertex_index = 0; vertex_index < vertex_count; ++vertex_index) {
					// 情况索引映射到一个 16 位代码列表，提供顶点所在边的信息。
					// 每个 16 位代码的低字节各用半个字节包含边的端点角点索引，
					// 高字节包含图 3.8(b) 所示的映射代码
					const unsigned short rvd = tables::get_regular_vertex_data(case_code, vertex_index);
					const uint8_t edge_code_low = rvd & 0xff;
					const uint8_t edge_code_high = (rvd >> 8) & 0xff;

					// 获取低半个字节中的角点索引（总是排序为较高的在后）
					const uint8_t v0 = (edge_code_low >> 4) & 0xf;
					const uint8_t v1 = edge_code_low & 0xf;

#ifdef DEBUG_ENABLED
					VOXEL_ASSERT_RETURN(v1 > v0);
#endif

					// 获取角点处的体素值
					const float sample0 = cell_samples_sdf[v0]; // 论文中称为 d0
					const float sample1 = cell_samples_sdf[v1]; // 论文中称为 d1

#ifdef DEBUG_ENABLED
					// TODO 论文中没有提到除零问题？？（不过从未发生过）
					VOXEL_ASSERT_RETURN(sample1 != sample0);
					VOXEL_ASSERT_RETURN(sample1 != 0 || sample0 != 0);
#endif

					// 获取插值位置
					// 我们使用 8 位小数，当包含两个端点时，允许新顶点位于边的 257 个
					// 可能位置之一。
					// const int t = (sample1 << 8) / (sample1 - sample0);
					const float t =
							math::clamp(sample1 / (sample1 - sample0), edge_clamp_margin, edge_clamp_margin_max);

					const Vector3i p0 = corner_positions[v0];
					const Vector3i p1 = corner_positions[v1];

					if (t > 0.f && t < 1.f) {
						// 顶点位于 p0 和 p1 之间（在边内部）

						// 单元的每条边都分配了一个 8 位代码，如图 3.8(b) 所示，
						// 它提供到前一单元以及该前一单元上允许创建新顶点的重合边的映射。
						// 该代码的高半个字节指示要到达正确的前一单元需要沿哪个方向走。
						// 该半字节中的位值 1、2 和 4 分别表示必须从 x、y 和/或 z 坐标减一。
						const uint8_t reuse_dir = (edge_code_high >> 4) & 0xf;
						const uint8_t reuse_vertex_index = edge_code_high & 0xf;

						// TODO 在数据块的负方向一侧会错过一些复用机会，
						// 但我真的不知道如何修复……
						// 你可以通过在着色器中根据顶点索引随机“抖动”每个顶点来检查，
						// 你会看到接触数据块 -X、-Y 或 -Z 面的顶点没有连接起来

						const bool present = (reuse_dir & direction_validity_mask) == reuse_dir;

						if (present) {
							const Vector3i cache_pos = pos + dir_to_prev_vec(reuse_dir);
							const ReuseCell &prev_cell = cache.get_reuse_cell(cache_pos);
							if (prev_cell.packed_texture_indices == current_reuse_cell.packed_texture_indices) {
								// 将复用先前的顶点
								cell_vertex_indices[vertex_index] = prev_cell.vertices[reuse_vertex_index];
							}
						}

						if (!present || cell_vertex_indices[vertex_index] == -1) {
							// 创建新顶点

							const float t0 = t; // static_cast<float>(t) / 256.f;
							const float t1 = 1.f - t; // static_cast<float>(0x100 - t) / 256.f;
							// const int ti0 = t;
							// const int ti1 = 0x100 - t;
							// const Vector3i primary = p0 * ti0 + p1 * ti1;

							const Vector3f primaryf = to_vec3f(p0) * t0 + to_vec3f(p1) * t1;
							// TODO 二分查找能给出更好的位置结果，但并不能改善法线。
							// 我不确定如何克服这个问题，因为如果我们采样低细节法线，会因 SDF 裁剪
							// 而得到“块状”结果。如果采样高细节梯度，会得到细节，
							// 但如果细节凹凸不平，我们也会得到噪点较多的结果。
							const Vector3f cg0 = get_corner_gradient<TSdf>(
									corner_data_indices[v0], sdf_data, block_size_with_padding
							);
							const Vector3f cg1 = get_corner_gradient<TSdf>(
									corner_data_indices[v1], sdf_data, block_size_with_padding
							);
							const Vector3f normal = normalized_not_null(cg0 * t0 + cg1 * t1);

							Vector3f secondary;

							uint8_t vertex_border_mask = 0;
							if (cell_border_mask > 0) {
								secondary = get_secondary_position(primaryf, normal, lod_index, block_size);
								vertex_border_mask =
										(get_border_mask(p0, block_size_scaled) & get_border_mask(p1, block_size_scaled)
										);
							}

							cell_vertex_indices[vertex_index] = output.add_vertex(
									primaryf, normal, cell_border_mask, vertex_border_mask, 0, secondary
							);

							material_processor.on_vertex(v0, v1, t1);

							if (reuse_dir & 8) {
								// 存储生成的顶点，以便其他单元可以复用。
								current_reuse_cell.vertices[reuse_vertex_index] = cell_vertex_indices[vertex_index];
							}
						}

					} else if (t == 0 && v1 == 7) {
						// t == 0：顶点位于 p1 上
						// v1 == 7：p1 位于单元的最大角
						// 此单元拥有该顶点，因此应创建它。

						const Vector3i primary = p1;
						const Vector3f primaryf = to_vec3f(primary);
						const Vector3f cg1 =
								get_corner_gradient<TSdf>(corner_data_indices[v1], sdf_data, block_size_with_padding);
						const Vector3f normal = normalized_not_null(cg1);

						Vector3f secondary;

						uint8_t vertex_border_mask = 0;
						if (cell_border_mask > 0) {
							secondary = get_secondary_position(primaryf, normal, lod_index, block_size);
							vertex_border_mask = get_border_mask(p1, block_size_scaled);
						}

						cell_vertex_indices[vertex_index] =
								output.add_vertex(primaryf, normal, cell_border_mask, vertex_border_mask, 0, secondary);

						material_processor.on_vertex(v0, v1, 1.f);

						current_reuse_cell.vertices[0] = cell_vertex_indices[vertex_index];

					} else {
						// 顶点要么位于 p0 上，要么位于 p1 上。
						// 原始 Transvoxel 在这些情况下会尝试复用先前顶点，
						// 但这里我们不做，因为一些模糊情况会导致伪影出现。
						// 这不是常见情况，所以应该不会太糟
						// （除非你大量使用网格对齐的形状？）。

						// 如果我们要复用的顶点位于一个没有三角剖分的单元上，该怎么办？
						// 前一单元可能所有角点符号相同，只有一个为 0。
						// 强制 `present=false` 似乎可以修复由此造成的空洞情况。
						// 在处理每一层之前重置缓存也能解决，但会稍慢一些。
						// 否则代码会尝试复用一个未被标记为可复用的顶点，
						// 从而拾取到先前层留下的一些垃圾数据。
#ifdef VOXEL_TRANSVOXEL_REUSE_VERTEX_ON_COINCIDENT_CASES
						// 通过反转 3 位角点索引（按位与数字 7 进行异或），可以轻松获得
						// 指向正确单元的 3 位方向代码。
						// 角点索引取决于 t 的值，t = 0 表示我们位于编号较高的端点。
						const uint8_t reuse_dir = (t == 0 ? v1 ^ 7 : v0 ^ 7);
						const bool present = (reuse_dir & direction_validity_mask) == reuse_dir;

						// 注意：与上面类似代码的唯一区别是我们在 `else` 中取顶点 0
						if (present) {
							const Vector3i cache_pos = pos + dir_to_prev_vec(reuse_dir);
							const ReuseCell &prev_cell = cache.get_reuse_cell(cache_pos);
							cell_vertex_indices[vertex_index] = prev_cell.vertices[0];
						}

						if (!present || cell_vertex_indices[vertex_index] == -1)
#endif
						{
							// 创建新顶点

							const unsigned int vi = t == 0 ? v1 : v0;

							const Vector3i primary = t == 0 ? p1 : p0;
							const Vector3f primaryf = to_vec3f(primary);
							const Vector3f cg = get_corner_gradient<TSdf>(
									corner_data_indices[vi], sdf_data, block_size_with_padding
							);
							const Vector3f normal = normalized_not_null(cg);

							// TODO 这段代码重复了几次，是否要提取出来？
							Vector3f secondary;

							uint8_t vertex_border_mask = 0;
							if (cell_border_mask > 0) {
								secondary = get_secondary_position(primaryf, normal, lod_index, block_size);
								vertex_border_mask = get_border_mask(primary, block_size_scaled);
							}

							cell_vertex_indices[vertex_index] = output.add_vertex(
									primaryf, normal, cell_border_mask, vertex_border_mask, 0, secondary
							);

							material_processor.on_vertex(v0, v1, 1.f - t);
						}
					}

				} // 对每个单元顶点

				const uint32_t effective_triangle_count = triangle_count;

				for (int t = 0; t < triangle_count; ++t) {
					const int t0 = t * 3;

					const int i0 = cell_vertex_indices[regular_cell_data.get_vertex_index(t0)];
					const int i1 = cell_vertex_indices[regular_cell_data.get_vertex_index(t0 + 1)];
					const int i2 = cell_vertex_indices[regular_cell_data.get_vertex_index(t0 + 2)];

					{
						// Transvoxel 论文：
						// 当单元的一个或多个角点样本值为零时，可能会生成面积为零的三角形。
						// 例如，当我们对一个角点样本值为零、其余七个角点样本值为负的单元进行三角剖分时，
						// 会生成等价类 #1 的单个三角形（见表 3.2）。然而，三个顶点
						// 都恰好位于样本值为零的角点上。通过简单的面积计算发现它们是退化三角形后，
						// 这些三角形会被消除。
						//
						// 实际上，不修复这一点以前也能正常工作，但 Jolt 物理集成使其
						// 成为问题。Jolt 会检查退化三角形，但不是直接跳过它们，而是
						// 抛出错误。此外，Godot 强制将去索引的网格传递给 Physics3DServer，
						// 这要求 Jolt 重新索引它。这不仅浪费时间，
						// 而且 Jolt 在这个过程中会消除位于同一位置或低于硬编码阈值的顶点。
						// 这反过来又引起更多问题，因为退化或微小的三角形会导致
						// 同样的错误，而不是被忽略。因此，一种变通方法是在这里主动移除这些
						// 三角形，代价是额外的 CPU 开销。
						//
						// 注意，这种变通方法意味着最终网格中可能存在未使用的顶点。
						// 另一种变通方法可能是修改 SDF 使其永远不会为 0，但那无法
						// 覆盖三角形过薄的情况。
						//
						// 性能分析结果（每个 16^3 数据块的平均耗时）。
						// 启用时：75 us
						// 禁用时：65 us
						// 所以修复 0.5% 包含至少一个退化/超薄三角形的网格并不是无足轻重的事，
						// 不幸的是。
						//
						// 关于 Jolt 重新索引网格，请参见 PR（已放弃？）：
						// https://github.com/godotengine/godot/pull/72868
						//
						// const Vector3f p0 = output.vertices[i0];
						// const Vector3f p1 = output.vertices[i1];
						// const Vector3f p2 = output.vertices[i2];
						// if (math::is_triangle_degenerate_approx(p0, p1, p2, 0.000001f)) {
						// 	--effective_triangle_count;
						// 	continue;
						// }
					}

					output.indices.push_back(i0);
					output.indices.push_back(i1);
					output.indices.push_back(i2);
				}

				if (cell_info != nullptr) {
					cell_info->push_back(CellInfo{ pos - min_pos, static_cast<uint8_t>(effective_triangle_count) });
				}

			} // x
		} // y
	} // z
}

//    y            y
//    |            | z
//    |            |/     OpenGL axis convention
//    o---x    x---o
//   /
//  z

// 将面空间坐标转换为数据块空间坐标，需考虑当前处理的面。
inline Vector3i face_to_block(int x, int y, int z, int dir, const Vector3i &bs) {
	// 这个问题有多种可能的解法，因为我们可以旋转坐标轴。
	// 我们采用 XY 映射到不同坐标轴且保持相同相对方向的配置，
	// 因此只有一半情况下 Z 会翻转。
	switch (dir) {
		case Cube::SIDE_NEGATIVE_X:
			return Vector3i(z, x, y);

		case Cube::SIDE_POSITIVE_X:
			return Vector3i(bs.x - 1 - z, y, x);

		case Cube::SIDE_NEGATIVE_Y:
			return Vector3i(y, z, x);

		case Cube::SIDE_POSITIVE_Y:
			return Vector3i(x, bs.y - 1 - z, y);

		case Cube::SIDE_NEGATIVE_Z:
			return Vector3i(x, y, z);

		case Cube::SIDE_POSITIVE_Z:
			return Vector3i(y, x, bs.z - 1 - z);

		default:
			CRASH_NOW();
			return Vector3i();
	}
}

// 我选择了支持非立方体区域，所以...
inline void get_face_axes(int &ax, int &ay, int dir) {
	switch (dir) {
		case Cube::SIDE_NEGATIVE_X:
			ax = Vector3i::AXIS_Y;
			ay = Vector3i::AXIS_Z;
			break;

		case Cube::SIDE_POSITIVE_X:
			ax = Vector3i::AXIS_Z;
			ay = Vector3i::AXIS_Y;
			break;

		case Cube::SIDE_NEGATIVE_Y:
			ax = Vector3i::AXIS_Z;
			ay = Vector3i::AXIS_X;
			break;

		case Cube::SIDE_POSITIVE_Y:
			ax = Vector3i::AXIS_X;
			ay = Vector3i::AXIS_Z;
			break;

		case Cube::SIDE_NEGATIVE_Z:
			ax = Vector3i::AXIS_X;
			ay = Vector3i::AXIS_Y;
			break;

		case Cube::SIDE_POSITIVE_Z:
			ax = Vector3i::AXIS_Y;
			ay = Vector3i::AXIS_X;
			break;

		default:
			VOXEL_CRASH();
	}
}

// TODO Cube::Side 有一个遗留问题：Y 轴与其他轴相比是反的
inline uint8_t get_face_index(int cube_dir) {
	switch (cube_dir) {
		case Cube::SIDE_NEGATIVE_X:
			return 0;

		case Cube::SIDE_POSITIVE_X:
			return 1;

		case Cube::SIDE_NEGATIVE_Y:
			return 2;

		case Cube::SIDE_POSITIVE_Y:
			return 3;

		case Cube::SIDE_NEGATIVE_Z:
			return 4;

		case Cube::SIDE_POSITIVE_Z:
			return 5;

		default:
			VOXEL_CRASH();
			return 0;
	}
}

template <typename TSdf, typename TMaterialProcessor>
void build_transition_mesh(
		Span<const TSdf> sdf_data,
		TMaterialProcessor material_processor,
		const Vector3i block_size_with_padding,
		const int direction,
		const int lod_index,
		Cache &cache,
		MeshArrays &output,
		const float edge_clamp_margin
) {
	// 从这里开始，我们期望缓冲区包含已分配的数据。
	// 该函数中的一些注释引自 Transvoxel 论文。

	const float edge_clamp_margin_max = 1.f - edge_clamp_margin;

	const Vector3i block_size_without_padding =
			block_size_with_padding - Vector3iUtil::create(MIN_PADDING + MAX_PADDING);
	const Vector3i block_size_scaled = block_size_without_padding << lod_index;

	VOXEL_ASSERT_RETURN(block_size_with_padding.x >= 3);
	VOXEL_ASSERT_RETURN(block_size_with_padding.y >= 3);
	VOXEL_ASSERT_RETURN(block_size_with_padding.z >= 3);

	cache.reset_reuse_cells_2d(block_size_with_padding);

	// 这部分在“面空间”中工作，该空间沿局部 X 和 Y 轴是二维的。
	// 在此空间中，-Z 指向半分辨率单元，而 +Z 指向全分辨率单元。
	// 通过方向枚举将该空间映射到数据块空间。

	// 注意：与论文相比，我做了几处改动。
	// 我没有让过渡网格从低分辨率数据块延伸到高分辨率数据块，
	// 而是反其道而行之，从高分辨率延伸到低分辨率。这样更容易，因为半分辨率体素可以免费获得，
	// 只要我们在常规网格之后用相同的体素数据计算过渡网格即可。
	// TODO 然而，这种改动的一个问题是网格密度的“凸起”可能很明显。

	// 这表示我们实际处理的体素盒子。
	// 它也代表了可生成的最小和最大顶点的位置。
	// 存在填充是为了允许向外多访问 1 个体素以计算法线
	const Vector3i min_pos = Vector3iUtil::create(MIN_PADDING);
	const Vector3i max_pos = block_size_with_padding - Vector3iUtil::create(MAX_PADDING);

	int axis_x, axis_y;
	get_face_axes(axis_x, axis_y, direction);
	const int min_fpos_x = min_pos[axis_x];
	const int min_fpos_y = min_pos[axis_y];
	const int max_fpos_x = max_pos[axis_x] - 1; // 这里再减 1，因为 2D 核是 3x3
	const int max_fpos_y = max_pos[axis_y] - 1;

	// 在数据数组中推进多少以获取相邻体素
	const unsigned int n010 = 1; // Y+1
	const unsigned int n100 = block_size_with_padding.y; // X+1
	const unsigned int n001 = block_size_with_padding.y * block_size_with_padding.x; // Z+1
	// const unsigned int n110 = n010 + n100;
	// const unsigned int n101 = n100 + n001;
	// const unsigned int n011 = n010 + n001;
	// const unsigned int n111 = n100 + n010 + n001;

	// 使用临时局部变量，否则 clang-format 会使其难以阅读
	const Vector3i ftb_000 = face_to_block(0, 0, 0, direction, block_size_with_padding);
	const Vector3i ftb_x00 = face_to_block(1, 0, 0, direction, block_size_with_padding);
	const Vector3i ftb_0y0 = face_to_block(0, 1, 0, direction, block_size_with_padding);
	// 使用面坐标在数据数组中前进多少以获取相邻体素
	const int fn00 = Vector3iUtil::get_zxy_index(ftb_000, block_size_with_padding);
	const int fn10 = Vector3iUtil::get_zxy_index(ftb_x00, block_size_with_padding) - fn00;
	const int fn01 = Vector3iUtil::get_zxy_index(ftb_0y0, block_size_with_padding) - fn00;
	const int fn11 = fn10 + fn01;
	const int fn21 = 2 * fn10 + fn01;
	const int fn22 = 2 * fn10 + 2 * fn01;
	const int fn12 = fn10 + 2 * fn01;
	const int fn20 = 2 * fn10;
	const int fn02 = 2 * fn01;

	FixedArray<Vector3i, 13> cell_positions;
	const int fz = MIN_PADDING;

	const TSdf isolevel = get_isolevel<TSdf>();

	const uint8_t transition_hint_mask = 1 << get_face_index(direction);

	// 在面空间中迭代
	for (int fy = min_fpos_y; fy < max_fpos_y; fy += 2) {
		for (int fx = min_fpos_x; fx < max_fpos_x; fx += 2) {
			// 数据块空间中的单元位置
			// 警告：暂时包含填充，稍后会被撤销。
			cell_positions[0] = face_to_block(fx, fy, fz, direction, block_size_with_padding);

			const int data_index = Vector3iUtil::get_zxy_index(cell_positions[0], block_size_with_padding);

			{
				const bool s = sdf_data[data_index] > isolevel;

				// `//` 可防止 clang-format 打乱格式
				if ( //
						(sdf_data[data_index + fn10] > isolevel) == s && //
						(sdf_data[data_index + fn20] > isolevel) == s && //
						(sdf_data[data_index + fn01] > isolevel) == s && //
						(sdf_data[data_index + fn11] > isolevel) == s && //
						(sdf_data[data_index + fn21] > isolevel) == s && //
						(sdf_data[data_index + fn02] > isolevel) == s && //
						(sdf_data[data_index + fn12] > isolevel) == s && //
						(sdf_data[data_index + fn22] > isolevel) == s) {
					// 未跨越等值面，此单元不会产生任何几何体。
					// 我们必须尽快判断这一点，因为这种情况会经常发生。
					continue;
				}
			}

			FixedArray<unsigned int, 9> cell_data_indices;
			cell_data_indices[0] = data_index;
			cell_data_indices[1] = data_index + fn10;
			cell_data_indices[2] = data_index + fn20;
			cell_data_indices[3] = data_index + fn01;
			cell_data_indices[4] = data_index + fn11;
			cell_data_indices[5] = data_index + fn21;
			cell_data_indices[6] = data_index + fn02;
			cell_data_indices[7] = data_index + fn12;
			cell_data_indices[8] = data_index + fn22;

			//  6---7---8
			//  |   |   |
			//  3---4---5
			//  |   |   |
			//  0---1---2

			// 全分辨率样本 0..8
			FixedArray<float, 13> cell_samples;
			for (unsigned int i = 0; i < 9; ++i) {
				cell_samples[i] = sdf_as_float(sdf_data[cell_data_indices[i]]);
			}

			//  B-------C
			//  |       |
			//  |       |
			//  |       |
			//  9-------A

			// 半分辨率样本 9..C：它们与对应角点相同
			cell_samples[0x9] = cell_samples[0];
			cell_samples[0xA] = cell_samples[2];
			cell_samples[0xB] = cell_samples[6];
			cell_samples[0xC] = cell_samples[8];

			// 注意，由于查找表的制作方式，这里的 case code 顺序与采样顺序不同（按 Transvoxel 论文）
			uint16_t case_code = sign_f(cell_samples[0]);
			case_code |= (sign_f(cell_samples[1]) << 1);
			case_code |= (sign_f(cell_samples[2]) << 2);
			case_code |= (sign_f(cell_samples[5]) << 3);
			case_code |= (sign_f(cell_samples[8]) << 4);
			case_code |= (sign_f(cell_samples[7]) << 5);
			case_code |= (sign_f(cell_samples[6]) << 6);
			case_code |= (sign_f(cell_samples[3]) << 7);
			case_code |= (sign_f(cell_samples[4]) << 8);

			if (case_code == 0 || case_code == 511) {
				// 该单元不包含三角形。
				continue;
			}

			ReuseTransitionCell &current_reuse_cell = cache.get_reuse_cell_2d(fx, fy);
			current_reuse_cell.packed_texture_indices =
					material_processor.on_transition_cell(cell_data_indices, case_code);

			VOXEL_ASSERT(case_code <= 511);

			// TODO 我们可能并不需要全部！
			FixedArray<Vector3f, 13> cell_gradients;
			for (unsigned int i = 0; i < 9; ++i) {
				const unsigned int di = cell_data_indices[i];

				const float nx = sdf_as_float(sdf_data[di - n100]);
				const float ny = sdf_as_float(sdf_data[di - n010]);
				const float nz = sdf_as_float(sdf_data[di - n001]);
				const float px = sdf_as_float(sdf_data[di + n100]);
				const float py = sdf_as_float(sdf_data[di + n010]);
				const float pz = sdf_as_float(sdf_data[di + n001]);

				cell_gradients[i] = Vector3f(nx - px, ny - py, nz - pz);
			}
			cell_gradients[0x9] = cell_gradients[0];
			cell_gradients[0xA] = cell_gradients[2];
			cell_gradients[0xB] = cell_gradients[6];
			cell_gradients[0xC] = cell_gradients[8];

			// TODO 优化：去掉 face_to_block 中涉及的条件分支
			cell_positions[1] = face_to_block(fx + 1, fy + 0, fz, direction, block_size_with_padding);
			cell_positions[2] = face_to_block(fx + 2, fy + 0, fz, direction, block_size_with_padding);
			cell_positions[3] = face_to_block(fx + 0, fy + 1, fz, direction, block_size_with_padding);
			cell_positions[4] = face_to_block(fx + 1, fy + 1, fz, direction, block_size_with_padding);
			cell_positions[5] = face_to_block(fx + 2, fy + 1, fz, direction, block_size_with_padding);
			cell_positions[6] = face_to_block(fx + 0, fy + 2, fz, direction, block_size_with_padding);
			cell_positions[7] = face_to_block(fx + 1, fy + 2, fz, direction, block_size_with_padding);
			cell_positions[8] = face_to_block(fx + 2, fy + 2, fz, direction, block_size_with_padding);
			for (unsigned int i = 0; i < 9; ++i) {
				cell_positions[i] = (cell_positions[i] - min_pos) << lod_index;
			}
			cell_positions[0x9] = cell_positions[0];
			cell_positions[0xA] = cell_positions[2];
			cell_positions[0xB] = cell_positions[6];
			cell_positions[0xC] = cell_positions[8];

			const uint8_t cell_class = tables::get_transition_cell_class(case_code);

			CRASH_COND((cell_class & 0x7f) > 55);

			const tables::TransitionCellData cell_data = tables::get_transition_cell_data(cell_class & 0x7f);
			const bool flip_triangles = ((cell_class & 128) != 0);

			const unsigned int vertex_count = cell_data.GetVertexCount();
			FixedArray<int, 12> cell_vertex_indices;
			fill(cell_vertex_indices, -1);
			CRASH_COND(vertex_count > cell_vertex_indices.size());

			const uint8_t direction_validity_mask = (fx > min_fpos_x ? 1 : 0) | ((fy > min_fpos_y ? 1 : 0) << 1);

			const uint8_t cell_border_mask = get_border_mask(cell_positions[0], block_size_scaled);

			for (unsigned int vertex_index = 0; vertex_index < vertex_count; ++vertex_index) {
				const uint16_t edge_code = tables::get_transition_vertex_data(case_code, vertex_index);
				const uint8_t index_vertex_a = (edge_code >> 4) & 0xf;
				const uint8_t index_vertex_b = (edge_code & 0xf);

				const float sample_a = cell_samples[index_vertex_a]; // 论文中的 d0 和 d1
				const float sample_b = cell_samples[index_vertex_b];
				// TODO 论文中未提及除零问题？？
				VOXEL_ASSERT_RETURN(sample_a != sample_b);
				VOXEL_ASSERT_RETURN(sample_a != 0 || sample_b != 0);

				// 获取插值位置
				// 我们使用 8 位分数，当两个端点都包含在内时，
				// 允许新顶点位于边上 257 个可能位置之一。
				// const int t = (sample_b << 8) / (sample_b - sample_a);
				const float t = math::clamp(sample_b / (sample_b - sample_a), edge_clamp_margin, edge_clamp_margin_max);

				const float t0 = t; // static_cast<float>(t) / 256.f;
				const float t1 = 1.f - t; // static_cast<float>(0x100 - t) / 256.f;
				// const int ti0 = t;
				// const int ti1 = 0x100 - t;

				if (t > 0.f && t < 1.f) {
					// 顶点位于边的内部。
					// （即 t 为 0 或 257 时，意味着它直接位于顶点 a 或顶点 b 上）

					const uint8_t vertex_index_to_reuse_or_create = (edge_code >> 8) & 0xf;

					// 该半字节中的位值 1 和 2 表示我们必须分别从 x 或 y
					// 坐标中减一，且这两位永远不会同时设置。
					// 位值 4 表示要在内部边上创建新顶点，
					// 该顶点无法复用；位值 8 表示要在最大边上创建新顶点，
					// 该顶点可以被复用。
					//
					// Bit 0 (0x1): 需要从 X 减一
					// Bit 1 (0x2): 需要从 Y 减一
					// Bit 2 (0x4): 顶点位于内部边上，不会被复用
					// Bit 3 (0x8): 顶点位于最大边上，可以被复用
					const uint8_t reuse_direction = (edge_code >> 12);

					const bool present = (reuse_direction & direction_validity_mask) == reuse_direction;

					if (present) {
						// 前一个单元可用。获取缓存的单元，
						// 从中取得要复用的顶点索引。
						const ReuseTransitionCell &prev =
								cache.get_reuse_cell_2d(fx - (reuse_direction & 1), fy - ((reuse_direction >> 1) & 1));
						if (prev.packed_texture_indices == current_reuse_cell.packed_texture_indices) {
							// 复用前一个单元的顶点索引。
							cell_vertex_indices[vertex_index] = prev.vertices[vertex_index_to_reuse_or_create];
						}
					}

					if (!present || cell_vertex_indices[vertex_index] == -1) {
						// 即将创建新顶点

						const Vector3i p0 = cell_positions[index_vertex_a];
						const Vector3i p1 = cell_positions[index_vertex_b];

						const Vector3f n0 = cell_gradients[index_vertex_a];
						const Vector3f n1 = cell_gradients[index_vertex_b];

						// Vector3i primary = p0 * ti0 + p1 * ti1;
						const Vector3f primaryf = to_vec3f(p0) * t0 + to_vec3f(p1) * t1;
						const Vector3f normal = normalized_not_null(n0 * t0 + n1 * t1);

						const bool fullres_side = (index_vertex_a < 9 || index_vertex_b < 9);

						Vector3f secondary;
						uint8_t cell_border_mask2 = cell_border_mask;
						uint8_t vertex_border_mask = 0;
						if (fullres_side) {
							secondary = get_secondary_position(primaryf, normal, lod_index, block_size_without_padding);
							vertex_border_mask =
									(get_border_mask(p0, block_size_scaled) & get_border_mask(p1, block_size_scaled));
						} else {
							// 如果顶点位于半分辨率一侧（在我们的实现中，
							// 即数据块的一侧），那么我们将掩码设为 0，使该顶点永远不会被移动。
							// 我们只移动全分辨率一侧以连接常规网格，
							// 常规网格也会移动相同的量以适配过渡网格。
							cell_border_mask2 = 0;
						}

						cell_vertex_indices[vertex_index] = output.add_vertex(
								primaryf, normal, cell_border_mask2, vertex_border_mask, transition_hint_mask, secondary
						);

						material_processor.on_vertex(index_vertex_a, index_vertex_b, t1);

						if (reuse_direction & 0x8) {
							// 该顶点稍后可被复用
							ReuseTransitionCell &r = cache.get_reuse_cell_2d(fx, fy);
							r.vertices[vertex_index_to_reuse_or_create] = cell_vertex_indices[vertex_index];
						}
					}

				} else {
					// 顶点正好位于其中一个边端点上。
					// 尝试复用前一个单元的角点顶点。
					// 使用 transitionCornerData 中的复用信息。

					const uint8_t cell_index = (t == 0 ? index_vertex_b : index_vertex_a);
					CRASH_COND(cell_index >= 13);
					const uint8_t corner_data = tables::get_transition_corner_data(cell_index);
					const uint8_t vertex_index_to_reuse_or_create = (corner_data & 0xf);
					const uint8_t reuse_direction = ((corner_data >> 4) & 0xf);

					const bool present = (reuse_direction & direction_validity_mask) == reuse_direction;

					if (present) {
						// 前一个单元可用。获取缓存的单元，
						// 从中取得要复用的顶点索引。
						const ReuseTransitionCell &prev =
								cache.get_reuse_cell_2d(fx - (reuse_direction & 1), fy - ((reuse_direction >> 1) & 1));
						// 复用前一个单元的顶点索引。
						cell_vertex_indices[vertex_index] = prev.vertices[vertex_index_to_reuse_or_create];
					}

					if (!present || cell_vertex_indices[vertex_index] == -1) {
						// 即将创建新顶点

						const Vector3i primary = cell_positions[cell_index];
						const Vector3f primaryf = to_vec3f(primary);
						const Vector3f normal = normalized_not_null(cell_gradients[cell_index]);

						const bool fullres_side = (cell_index < 9);

						Vector3f secondary;
						uint8_t vertex_border_mask = 0;
						uint8_t cell_border_mask2 = cell_border_mask;
						if (fullres_side) {
							secondary = get_secondary_position(primaryf, normal, lod_index, block_size_without_padding);
							vertex_border_mask = get_border_mask(primary, block_size_scaled);
						} else {
							cell_border_mask2 = 0;
						}

						cell_vertex_indices[vertex_index] = output.add_vertex(
								primaryf, normal, cell_border_mask2, vertex_border_mask, transition_hint_mask, secondary
						);

						material_processor.on_vertex(index_vertex_a, index_vertex_b, 1.f - t);

						// 我们位于角点上，因此该顶点之后可复用
						ReuseTransitionCell &r = cache.get_reuse_cell_2d(fx, fy);
						r.vertices[vertex_index_to_reuse_or_create] = cell_vertex_indices[vertex_index];
					}
				}

			} // for 循环：vertex

			const unsigned int triangle_count = cell_data.GetTriangleCount();

			for (unsigned int ti = 0; ti < triangle_count; ++ti) {
				if (flip_triangles) {
					output.indices.push_back(cell_vertex_indices[cell_data.get_vertex_index(ti * 3)]);
					output.indices.push_back(cell_vertex_indices[cell_data.get_vertex_index(ti * 3 + 1)]);
					output.indices.push_back(cell_vertex_indices[cell_data.get_vertex_index(ti * 3 + 2)]);
				} else {
					output.indices.push_back(cell_vertex_indices[cell_data.get_vertex_index(ti * 3 + 2)]);
					output.indices.push_back(cell_vertex_indices[cell_data.get_vertex_index(ti * 3 + 1)]);
					output.indices.push_back(cell_vertex_indices[cell_data.get_vertex_index(ti * 3)]);
				}
			}

		} // for 循环：x
	} // for 循环：y
}

template <typename T>
Span<const T> get_or_decompress_channel(const VoxelBuffer &voxels, StdVector<T> &backing_buffer, unsigned int channel) {
	//
	VOXEL_ASSERT_RETURN_V(
			voxels.get_channel_depth(channel) == VoxelBuffer::get_depth_from_size(sizeof(T)), Span<const T>()
	);

	if (voxels.get_channel_compression(channel) == VoxelBuffer::COMPRESSION_UNIFORM) {
		backing_buffer.resize(Vector3iUtil::get_volume_u64(voxels.get_size()));
		const T v = voxels.get_voxel(Vector3i(), channel);
		// TODO 是否可以使用 8 字节块或内建函数进行快速填充？
		for (unsigned int i = 0; i < backing_buffer.size(); ++i) {
			backing_buffer[i] = v;
		}
		return to_span_const(backing_buffer);

	} else {
		Span<const uint8_t> data_bytes;
		VOXEL_ASSERT(voxels.get_channel_as_bytes_read_only(channel, data_bytes) == true);
		return data_bytes.reinterpret_cast_to<const T>();
	}
}

// 当精度不足时（8 位、缩放 SDF 或缓慢的梯度），样本中出现零值的情况会更频繁。
// 这会导致两种症状：
// - 退化三角形。对之后使用该网格的系统（MeshOptimizer、物理引擎）可能不利
// - 出错的三角形。错误的顶点被复用。
//   需要更深入调查以确定原因，可能与 case 选择有关
//
// 另请参见 https://github.com/zeux/meshoptimizer/issues/312
//
// 一个快速修复方法是对等于等值面的值添加一个微小偏移。
// 必须在整个缓冲区上执行以确保一致性（而不能在提前剔除单元之后执行），
// 否则会在最终网格中产生裂缝。
//
// 目前不再使用，但如果再次遇到此问题，我们可能需要进行调查。
//
// 2023/12/28 更新：
// Jolt 物理引擎非常不喜欢退化网格并会抛出错误，这与 GodotPhysics 和 Bullet 不同。整个网格
// 都可能如此，因为可能存在一个体素缓冲区，其 SDF 全部 > 0，只有一个为零，而由于零
// 被视为“内部”，case 223 触发并产生一堆位于完全相同位置的顶点。
// 目前通过消除三角形来修复。
//
/*template <typename Sdf_T>
Span<const Sdf_T> apply_zero_sdf_fix(Span<const Sdf_T> p_sdf_data) {
	VOXEL_PROFILE_SCOPE();

	static thread_local StdVector<Sdf_T> s_sdf_backing_buffer;
	StdVector<Sdf_T> &sdf_data = s_sdf_backing_buffer;

	sdf_data.resize(p_sdf_data.size());
	memcpy(sdf_data.data(), p_sdf_data.data(), p_sdf_data.size());

	for (auto it = sdf_data.begin(); it != sdf_data.end(); ++it) {
		if (*it == get_isolevel<Sdf_T>()) {
			// 假设 Sdf_T 是整数。对于浮点数可能并不需要。
			*it += 1;
		}
	}
	return to_span_const(sdf_data);
}*/

// TODO 可考虑使用临时分配器
StdVector<uint16_t> &get_tls_weights_backing_buffer_u16() {
	thread_local StdVector<uint16_t> tls_weights_backing_buffer_u16;
	return tls_weights_backing_buffer_u16;
}

// TODO 可考虑使用临时分配器
StdVector<uint8_t> &get_tls_u8_conversion_buffer() {
	static thread_local StdVector<uint8_t> tls_conversion_backing_buffer;
	return tls_conversion_backing_buffer;
}

template <typename TMaterialProcessor>
inline void build_regular_mesh_dispatch_sd(
		const VoxelBuffer &voxels,
		const unsigned int sdf_channel,
		TMaterialProcessor material_processor,
		const uint32_t lod_index,
		Cache &cache,
		MeshArrays &output,
		StdVector<CellInfo> *cell_infos,
		const float edge_clamp_margin
) {
	Span<const uint8_t> sdf_data_raw;
	VOXEL_ASSERT(voxels.get_channel_as_bytes_read_only(sdf_channel, sdf_data_raw) == true);

	// 我们预先确定数据类型，从而去掉抽象层和条件分支，
	// 否则它们会在紧凑的迭代中损害性能
	switch (voxels.get_channel_depth(sdf_channel)) {
		case VoxelBuffer::DEPTH_8_BIT: {
			Span<const int8_t> sdf_data = sdf_data_raw.reinterpret_cast_to<const int8_t>();
			build_regular_mesh<int8_t>(
					sdf_data,
					material_processor,
					voxels.get_size(),
					lod_index,
					cache,
					output,
					cell_infos,
					edge_clamp_margin
			);
		} break;

		case VoxelBuffer::DEPTH_16_BIT: {
			Span<const int16_t> sdf_data = sdf_data_raw.reinterpret_cast_to<const int16_t>();
			build_regular_mesh<int16_t>(
					sdf_data,
					material_processor,
					voxels.get_size(),
					lod_index,
					cache,
					output,
					cell_infos,
					edge_clamp_margin
			);
		} break;

		// TODO 在 Transvoxel 中移除对 32 位 SDF 的支持？
		// 我认为不值得保留。而且它可能会显著减小可执行文件大小
		// （仅 transvoxel.cpp 的优化后 obj 大小在 Windows 上为 1.2 Mb）
		case VoxelBuffer::DEPTH_32_BIT: {
			Span<const float> sdf_data = sdf_data_raw.reinterpret_cast_to<const float>();
			build_regular_mesh<float>(
					sdf_data,
					material_processor,
					voxels.get_size(),
					lod_index,
					cache,
					output,
					cell_infos,
					edge_clamp_margin
			);
		} break;

		case VoxelBuffer::DEPTH_64_BIT: {
			static bool s_once = false;
			if (s_once == false) {
				s_once = true;
				VOXEL_PRINT_ERROR("Double-precision SDF channel is not supported");
				// 不值得为相对无意义的双精度 SDF 增加可执行文件大小
			}
		} break;

		default:
			VOXEL_PRINT_ERROR("Invalid channel");
			break;
	}
}

DefaultTextureIndicesData build_regular_mesh(
		const VoxelBuffer &voxels,
		const unsigned int sdf_channel,
		const uint32_t lod_index,
		const TexturingMode texturing_mode,
		Cache &cache,
		MeshArrays &output,
		StdVector<CellInfo> *cell_infos,
		const float edge_clamp_margin,
		const bool textures_ignore_air_voxels
) {
	VOXEL_PROFILE_SCOPE();
	// 从这里开始，我们期望缓冲区在相关通道中包含已分配的数据。

	const unsigned int voxels_count = Vector3iUtil::get_volume_u64(voxels.get_size());

	output.clear();

	DefaultTextureIndicesData default_texture_indices;
	default_texture_indices.use = false;

	switch (texturing_mode) {
		case TEXTURES_NONE:
			build_regular_mesh_dispatch_sd(
					voxels,
					sdf_channel,
					materials::NullProcessor{},
					lod_index,
					cache,
					output,
					cell_infos,
					edge_clamp_margin
			);
			break;

		case TEXTURES_MIXEL4_S4: {
			materials::mixel4::TextureIndicesData voxel_material_indices;
			materials::mixel4::WeightSamplerPackedU16 voxel_material_weights;
			{
				VOXEL_PROFILE_SCOPE_NAMED("Prepare material info");

				// 从这里开始我们已知 SDF 不是均匀的，因此它有已分配的缓冲区，
				// 但索引或权重可能是均匀的，所以我们需要确保存在后备缓冲区。
				voxel_material_indices = materials::mixel4::get_texture_indices_data(
						voxels, VoxelBuffer::CHANNEL_INDICES, default_texture_indices
				);
				voxel_material_weights.u16_data = get_or_decompress_channel(
						voxels,
						// TODO 可考虑使用临时分配器
						get_tls_weights_backing_buffer_u16(),
						VoxelBuffer::CHANNEL_WEIGHTS
				);
				VOXEL_ASSERT_RETURN_V(voxel_material_weights.u16_data.size() == voxels_count, default_texture_indices);
			}
			build_regular_mesh_dispatch_sd(
					voxels,
					sdf_channel,
					materials::mixel4::Processor<8>(
							voxel_material_indices,
							voxel_material_weights,
							output.texturing_data_2f32,
							textures_ignore_air_voxels
					),
					lod_index,
					cache,
					output,
					cell_infos,
					edge_clamp_margin
			);
		} break;

		case TEXTURES_SINGLE_S4: {
			const materials::single::VoxelMaterialIndices voxel_material_indices =
					materials::single::get_material_indices_from_vb(
							voxels, VoxelBuffer::CHANNEL_INDICES, get_tls_u8_conversion_buffer()
					);
			if (voxel_material_indices.is_uniform) {
				default_texture_indices.indices[0] = voxel_material_indices.uniform_value;
				default_texture_indices.use = true;
			}
			build_regular_mesh_dispatch_sd(
					voxels,
					sdf_channel,
					materials::single::s4::Processor<8>(voxel_material_indices.to_span(), output.texturing_data_2f32),
					lod_index,
					cache,
					output,
					cell_infos,
					edge_clamp_margin
			);
		} break;

#ifdef VOXEL_ENABLE_TRANSVOXEL_MATERIAL_SINGLE_S2
		case TEXTURES_SINGLE_S2: {
			const materials::single::VoxelMaterialIndices voxel_material_indices =
					materials::single::get_material_indices_from_vb(
							voxels, VoxelBuffer::CHANNEL_INDICES, get_tls_u8_conversion_buffer()
					);
			if (voxel_material_indices.is_uniform) {
				default_texture_indices.indices[0] = voxel_material_indices.uniform_value;
				default_texture_indices.use = true;
			}
			build_regular_mesh_dispatch_sd(
					voxels,
					sdf_channel,
					materials::single::s2::Processor<8>(voxel_material_indices.to_span(), output.texturing_data_1f32),
					lod_index,
					cache,
					output,
					cell_infos,
					edge_clamp_margin
			);
		} break;
#endif

		default:
			VOXEL_PRINT_ERROR("Invalid material mode");
			break;
	}

	return default_texture_indices;
}

template <typename TMaterialProcessor>
inline void build_transition_mesh_dispatch_sd(
		const VoxelBuffer &voxels,
		const unsigned int sdf_channel,
		const TMaterialProcessor material_processor,
		const int direction,
		const uint32_t lod_index,
		Cache &cache,
		MeshArrays &output,
		const float edge_clamp_margin
) {
	Span<const uint8_t> sdf_data_raw;
	VOXEL_ASSERT(voxels.get_channel_as_bytes_read_only(sdf_channel, sdf_data_raw) == true);

	switch (voxels.get_channel_depth(sdf_channel)) {
		case VoxelBuffer::DEPTH_8_BIT: {
			Span<const int8_t> sdf_data = sdf_data_raw.reinterpret_cast_to<const int8_t>();
			build_transition_mesh<int8_t>(
					sdf_data,
					material_processor,
					voxels.get_size(),
					direction,
					lod_index,
					cache,
					output,
					edge_clamp_margin
			);
		} break;

		case VoxelBuffer::DEPTH_16_BIT: {
			Span<const int16_t> sdf_data = sdf_data_raw.reinterpret_cast_to<const int16_t>();
			build_transition_mesh<int16_t>(
					sdf_data,
					material_processor,
					voxels.get_size(),
					direction,
					lod_index,
					cache,
					output,
					edge_clamp_margin
			);
		} break;

		case VoxelBuffer::DEPTH_32_BIT: {
			Span<const float> sdf_data = sdf_data_raw.reinterpret_cast_to<const float>();
			build_transition_mesh<float>(
					sdf_data,
					material_processor,
					voxels.get_size(),
					direction,
					lod_index,
					cache,
					output,
					edge_clamp_margin
			);
		} break;

		case VoxelBuffer::DEPTH_64_BIT:
			VOXEL_PRINT_ERROR("Double-precision SDF channel is not supported");
			// 不值得为相对无意义的双精度 SDF 增加可执行文件大小
			break;

		default:
			VOXEL_PRINT_ERROR("Invalid channel");
			break;
	}
}

void build_transition_mesh(
		const VoxelBuffer &voxels,
		const unsigned int sdf_channel,
		const int direction,
		const uint32_t lod_index,
		const TexturingMode texturing_mode,
		Cache &cache,
		MeshArrays &output,
		DefaultTextureIndicesData default_texture_indices_data,
		const float edge_clamp_margin,
		const bool textures_ignore_air_voxels
) {
	VOXEL_PROFILE_SCOPE();
	// 从这里开始，我们期望缓冲区在相关通道中包含已分配的数据。

	const unsigned int voxels_count = Vector3iUtil::get_volume_u64(voxels.get_size());

	switch (texturing_mode) {
		case TEXTURES_NONE:
			build_transition_mesh_dispatch_sd(
					voxels,
					sdf_channel,
					materials::NullProcessor{},
					direction,
					lod_index,
					cache,
					output,
					edge_clamp_margin
			);
			break;

		case TEXTURES_MIXEL4_S4: {
			materials::mixel4::TextureIndicesData indices_data;
			materials::mixel4::WeightSamplerPackedU16 weights_data;

			if (default_texture_indices_data.use) {
				indices_data.default_indices = default_texture_indices_data.indices;
				indices_data.packed_default_indices = default_texture_indices_data.packed_indices;
			} else {
				// 从这里开始我们已知 SDF 不是均匀的，因此它有已分配的缓冲区，
				// 但索引或权重可能是均匀的，所以我们需要确保存在后备缓冲区。
				// TODO 在网格化过程中改用条件判断是否值得？
				indices_data = materials::mixel4::get_texture_indices_data(
						voxels, VoxelBuffer::CHANNEL_INDICES, default_texture_indices_data
				);
			}
			weights_data.u16_data = get_or_decompress_channel(
					voxels,
					// TODO 可考虑使用临时分配器
					get_tls_weights_backing_buffer_u16(),
					VoxelBuffer::CHANNEL_WEIGHTS
			);
			VOXEL_ASSERT_RETURN(weights_data.u16_data.size() == voxels_count);

			build_transition_mesh_dispatch_sd(
					voxels,
					sdf_channel,
					materials::mixel4::Processor<13>(
							indices_data, weights_data, output.texturing_data_2f32, textures_ignore_air_voxels
					),
					direction,
					lod_index,
					cache,
					output,
					edge_clamp_margin
			);
		} break;

		case TEXTURES_SINGLE_S4: {
			materials::single::VoxelMaterialIndices voxel_material_indices;
			if (default_texture_indices_data.use) {
				voxel_material_indices.is_uniform = true;
				voxel_material_indices.uniform_value = default_texture_indices_data.indices[0];
			} else {
				voxel_material_indices = materials::single::get_material_indices_from_vb(
						voxels, VoxelBuffer::CHANNEL_INDICES, get_tls_u8_conversion_buffer()
				);
			}
			build_transition_mesh_dispatch_sd(
					voxels,
					sdf_channel,
					materials::single::s4::Processor<13>(voxel_material_indices.to_span(), output.texturing_data_2f32),
					direction,
					lod_index,
					cache,
					output,
					edge_clamp_margin
			);
		} break;

#ifdef VOXEL_ENABLE_TRANSVOXEL_MATERIAL_SINGLE_S2
		case TEXTURES_SINGLE_S2: {
			materials::single::VoxelMaterialIndices voxel_material_indices;
			if (default_texture_indices_data.use) {
				voxel_material_indices.is_uniform = true;
				voxel_material_indices.uniform_value = default_texture_indices_data.indices[0];
			} else {
				voxel_material_indices = materials::single::get_material_indices_from_vb(
						voxels, VoxelBuffer::CHANNEL_INDICES, get_tls_u8_conversion_buffer()
				);
			}
			build_transition_mesh_dispatch_sd(
					voxels,
					sdf_channel,
					materials::single::s2::Processor<13>(voxel_material_indices.to_span(), output.texturing_data_1f32),
					direction,
					lod_index,
					cache,
					output,
					edge_clamp_margin
			);
		} break;
#endif

		default:
			VOXEL_PRINT_ERROR("Invalid material mode");
			break;
	}
}

} // namespace voxel::transvoxel
