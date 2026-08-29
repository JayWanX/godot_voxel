#ifndef VOXEL_GRAPH_COMPILER_H
#define VOXEL_GRAPH_COMPILER_H

#include "../../util/containers/std_vector.h"
#include "voxel_graph_function.h"
#include "voxel_graph_runtime.h"
#include <type_traits>

namespace voxel::pg {

struct PortRemap {
	ProgramGraph::PortLocation original;
	// ID 可为 null，表示该端口在展开后没有对应内容。
	ProgramGraph::PortLocation expanded;
};

struct ExpandedNodeRemap {
	uint32_t expanded_node_id;
	uint32_t original_node_id;
};

struct GraphRemappingInfo {
	// 用于编辑器中的调试输出预览
	StdVector<PortRemap> user_to_expanded_ports;
	// 用于错误报告
	StdVector<ExpandedNodeRemap> expanded_to_user_node_ids;
};

// 预处理图并在主编译阶段之前应用一些优化。
// 这可能导致某些节点被移除或用新节点替换。
CompilationResult expand_graph(
		const ProgramGraph &graph,
		ProgramGraph &expanded_graph,
		Span<const VoxelGraphFunction::Port> input_defs,
		StdVector<uint32_t> *input_node_ids,
		const NodeTypeDB &type_db,
		GraphRemappingInfo *remap_info,
		const bool debug
);

// 编译阶段可供节点实现使用的函数
class CompileContext {
public:
	CompileContext(
			/*const ProgramGraph::Node &node,*/ StdVector<uint16_t> &program,
			StdVector<Runtime::HeapResource> &heap_resources,
			Span<const Variant> params
	) :
			/*_node(node),*/ _program(program), _heap_resources(heap_resources), _params(params) {}

	const Variant &get_param(size_t i) const {
		VOXEL_ASSERT(i < _params.size());
		return _params[i];
	}

	// 存储节点所需的编译期参数。T 必须是 POD 结构体。
	template <typename T>
	void set_params(T params) {
		static_assert(std::is_standard_layout_v<T> == true);
		static_assert(std::is_trivial_v<T> == true);

		// 每个节点只能调用一次
		CRASH_COND(_params_added);
		// 我们需要对齐内存，因此结构体不会立即存储在这里。
		// 取而代之的是放入一个头部，它告诉我们前进多少才能到达结构体的开头，
		// 那将是已对齐的位置。
		// 我们在结构体与存储在程序缓冲区中的字类型（即 uint16）之间取最大对齐。
		const size_t params_alignment = math::max(alignof(T), alignof(uint16_t));
		const size_t params_offset_index = _program.size();
		// 预留空间以存储偏移量（至少为 1，因为该头部是一个字）
		_program.push_back(1);
		// 为结构体对齐内存。
		// 注意：我们按字而不是按字节索引。
		const size_t struct_offset =
				math::alignup(_program.size() * sizeof(uint16_t), params_alignment) / sizeof(uint16_t);
		if (struct_offset > _program.size()) {
			_program.resize(struct_offset);
		}
		// 在头部写入偏移量
		_program[params_offset_index] = struct_offset - params_offset_index;
		// 为结构体分配空间。它以字为单位，因此可能最多多出 1 个字节。
		_params_size_in_words = (sizeof(T) + sizeof(uint16_t) - 1) / sizeof(uint16_t);
		_program.resize(_program.size() + _params_size_in_words);
		// 写入结构体
		T &p = *reinterpret_cast<T *>(&_program[struct_offset]);
		p = params;

		_params_added = true;
	}

	// 当编译步骤产生需要删除的资源时使用
	template <typename T>
	void add_delete_cleanup(T *ptr) {
		static_assert(!std::is_base_of<Object, T>::value);
		Runtime::HeapResource hr;
		hr.ptr = ptr;
		hr.deleter = [](void *p) {
			T *tp = reinterpret_cast<T *>(p);
			VOXEL_DELETE(tp);
		};
		_heap_resources.push_back(hr);
	}

	void make_error(String message) {
		_error_message = message;
		_has_error = true;
	}

	bool has_error() const {
		return _has_error;
	}

	const String &get_error_message() const {
		return _error_message;
	}

	size_t get_params_size_in_words() const {
		return _params_size_in_words;
	}

private:
	// const ProgramGraph::Node &_node;
	StdVector<uint16_t> &_program;
	StdVector<Runtime::HeapResource> &_heap_resources;
	Span<const Variant> _params;
	String _error_message;
	size_t _params_size_in_words = 0;
	bool _has_error = false;
	bool _params_added = false;
};

typedef void (*CompileFunc)(CompileContext &);

} // namespace voxel::pg

#endif // VOXEL_GRAPH_COMPILER_H
