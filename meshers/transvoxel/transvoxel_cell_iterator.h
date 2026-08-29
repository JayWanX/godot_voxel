#ifndef VOXEL_TRANSVOXEL_CELL_ITERATOR_H
#define VOXEL_TRANSVOXEL_CELL_ITERATOR_H

#include "../../engine/detail_rendering/detail_rendering.h"
#include "../../util/containers/std_vector.h"
#include "transvoxel.h"

namespace voxel {

// 实现通用接口以迭代体素网格单元，可用于计算虚拟纹理。
// 该实现针对收集 Transvoxel 网格生成器的结果进行了优化，其开箱即用地附带单元信息。
class TransvoxelCellIterator : public ICellIterator {
public:
	TransvoxelCellIterator(Span<const transvoxel::CellInfo> p_cell_infos) :
			_current_index(0), _triangle_begin_index(0) {
		// 制作副本
		_cell_infos.resize(p_cell_infos.size());
		for (unsigned int i = 0; i < p_cell_infos.size(); ++i) {
			_cell_infos[i] = p_cell_infos[i];
		}
	}

	unsigned int get_count() const override {
		return _cell_infos.size();
	}

	bool next(CurrentCellInfo &current) override {
		if (_current_index < _cell_infos.size()) {
			const transvoxel::CellInfo &cell = _cell_infos[_current_index];
			current.position = cell.position;
			current.triangle_count = cell.triangle_count;

			for (unsigned int i = 0; i < cell.triangle_count; ++i) {
				current.triangle_begin_indices[i] = _triangle_begin_index;
				_triangle_begin_index += 3;
			}

			++_current_index;
			return true;

		} else {
			return false;
		}
	}

	void rewind() override {
		_current_index = 0;
		_triangle_begin_index = 0;
	}

private:
	StdVector<transvoxel::CellInfo> _cell_infos;
	unsigned int _current_index;
	unsigned int _triangle_begin_index;
};

} // namespace voxel

#endif // VOXEL_TRANSVOXEL_CELL_ITERATOR_H
