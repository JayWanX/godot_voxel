#include "voxel_stream_sqlite.h"
#include "../../util/godot/classes/project_settings.h"
#include "../../util/godot/core/string.h"
#include "../../util/profiling.h"
#include "../../util/string/format.h"
#include "../../util/string/std_string.h"
#include "../compressed_data.h"
#include "connection.h"
#include <core/object/class_db.h>


#include <string_view>
#include <unordered_set>

namespace voxel {

using namespace sqlite;

namespace {
StdVector<uint8_t> &get_tls_temp_block_data() {
	thread_local StdVector<uint8_t> tls_temp_block_data;
	return tls_temp_block_data;
}
StdVector<uint8_t> &get_tls_temp_compressed_block_data() {
	thread_local StdVector<uint8_t> tls_temp_compressed_block_data;
	return tls_temp_compressed_block_data;
}

BlockLocation::CoordinateFormat to_internal_coordinate_format(VoxelStreamSQLite::CoordinateFormat format) {
	return static_cast<BlockLocation::CoordinateFormat>(format);
}

VoxelStreamSQLite::CoordinateFormat to_exposed_coordinate_format(BlockLocation::CoordinateFormat format) {
	return static_cast<VoxelStreamSQLite::CoordinateFormat>(format);
}

// 在事务失败后把连接恢复到可用状态，并告知它是否可以复用。
// 失败的 COMMIT 尤其会把事务留在打开状态，而 SQLite 没有嵌套事务，因此没有此处理时，该连接上后续每次
// `begin_transaction` 都会以 "cannot start a transaction within a transaction" 失败。
bool recover_after_failed_transaction(sqlite::Connection &con) {
	if (con.rollback_transaction()) {
		VOXEL_PRINT_VERBOSE("VoxelStreamSQLite: recovered connection after a failed transaction");
		return true;
	}
	VOXEL_PRINT_ERROR("VoxelStreamSQLite: could not recover connection after a failed transaction, dropping it");
	return false;
}

bool validate_range(Vector3i pos, unsigned int lod_index, const Box3i coordinate_range, unsigned int lod_count) {
	if (!coordinate_range.contains(pos)) {
		VOXEL_PRINT_ERROR(format("Block position {} is outside of supported range {}", pos, coordinate_range));
		return false;
	}
	if (lod_index >= lod_count) {
		VOXEL_PRINT_ERROR(format("Block LOD {} is outside of supported range [0..{})", lod_index, lod_count));
		return false;
	}
	return true;
}

} // namespace

VoxelStreamSQLite::VoxelStreamSQLite() {}

VoxelStreamSQLite::~VoxelStreamSQLite() {
	VOXEL_PRINT_VERBOSE("~VoxelStreamSQLite");
	if (!_globalized_connection_path.empty() && _cache.get_indicative_block_count() > 0) {
		VOXEL_PRINT_VERBOSE("~VoxelStreamSQLite flushy flushy");
		if (!flush_cache()) {
			// 保存该数据的最后机会：过了这个点数据就会丢失。
			VOXEL_PRINT_ERROR("VoxelStreamSQLite: final flush failed in destructor, unsaved cached data was lost");
		}
		VOXEL_PRINT_VERBOSE("~VoxelStreamSQLite flushy done");
	}
	for (auto it = _connection_pool.begin(); it != _connection_pool.end(); ++it) {
		delete *it;
	}
	_connection_pool.clear();
	VOXEL_PRINT_VERBOSE("~VoxelStreamSQLite done");
}

void VoxelStreamSQLite::set_database_path(String path) {
	MutexLock lock(_connection_mutex);
	if (path == _user_specified_connection_path) {
		return;
	}
	if (!_globalized_connection_path.empty() && _cache.get_indicative_block_count() > 0) {
		// 在更改路径之前保存缓存的数据。
		// 不使用 get_connection()，因为它会加锁，而我们已经处于上锁状态。
		sqlite::Connection con;
		// 注意，路径可能是无效的，
		// 因为 Godot 会在检查器中为每次键入的字符都设置该属性。
		// 因此如果你输入它，编辑器中可能会出现大量错误。
		if (con.open(_globalized_connection_path.data(), to_internal_coordinate_format(_preferred_coordinate_format))) {
			if (!flush_cache_to_connection(&con)) {
				// 该连接是局部的，随后立即被销毁，所以没有需要恢复的东西，但
				// 在切换到新数据库之前，数据未能保存到之前的数据库中。
				VOXEL_PRINT_ERROR(
						"VoxelStreamSQLite: failed to save cached data to the previous database before "
						"changing the path"
				);
			}
		}
	}
	for (auto it = _connection_pool.begin(); it != _connection_pool.end(); ++it) {
		delete *it;
	}
	_block_keys_cache.clear();
	_connection_pool.clear();

	_user_specified_connection_path = path;
	// To support Godot shortcuts like `user://` and `res://` (though the latter won't work on exported builds)
	_globalized_connection_path = voxel::godot::to_std_string(ProjectSettings::get_singleton()->globalize_path(path));

	// 这里实际上不打开任何东西。我们只在必要时才打开
}

String VoxelStreamSQLite::get_database_path() const {
	MutexLock lock(_connection_mutex);
	return _user_specified_connection_path;
}

void VoxelStreamSQLite::load_voxel_block(VoxelStream::VoxelQueryData &q) {
	load_voxel_blocks(Span<VoxelStream::VoxelQueryData>(&q, 1));
}

void VoxelStreamSQLite::save_voxel_block(VoxelStream::VoxelQueryData &q) {
	save_voxel_blocks(Span<VoxelStream::VoxelQueryData>(&q, 1));
}

static void set_result_codes(Span<VoxelStream::VoxelQueryData> p_blocks, VoxelStream::ResultCode code) {
	for (VoxelStream::VoxelQueryData &q : p_blocks) {
		q.result = code;
	}
}

#ifdef VOXEL_ENABLE_INSTANCER
static void set_result_codes(Span<VoxelStream::InstancesQueryData> p_blocks, VoxelStream::ResultCode code) {
	for (VoxelStream::InstancesQueryData &q : p_blocks) {
		q.result = code;
	}
}
#endif

// 只设置给定的子集，保持已从缓存解析出的数据块不变。
template <typename TBlockQueryData>
static void set_result_codes(
		Span<TBlockQueryData> p_blocks,
		const StdVector<unsigned int> &p_indices,
		const VoxelStream::ResultCode code
) {
	for (const unsigned int i : p_indices) {
		p_blocks[i].result = code;
	}
}

void VoxelStreamSQLite::load_voxel_blocks(Span<VoxelStream::VoxelQueryData> p_blocks) {
	VOXEL_PROFILE_SCOPE();

	// 先获取连接，以便在启用时让键缓存加载。
	// 首次调用之后这应该很快，因为连接已被缓存。
	const ConnectionResult con_res = get_connection();

	switch (con_res.code) {
		case ConnectionResult::SUCCESS:
			break;
		case ConnectionResult::NOT_CONFIGURED:
			set_result_codes(p_blocks, VoxelStream::RESULT_BLOCK_NOT_FOUND);
			return;
		default:
			set_result_codes(p_blocks, VoxelStream::RESULT_ERROR);
			return;
	}

	sqlite::Connection *con = con_res.connection;

	ScopeRecycle con_scope(this, con);

	// 先检查缓存
	StdVector<unsigned int> blocks_to_load;
	for (unsigned int i = 0; i < p_blocks.size(); ++i) {
		VoxelStream::VoxelQueryData &q = p_blocks[i];
		const Vector3i pos = q.position_in_blocks;

		if (_block_keys_cache_enabled && !_block_keys_cache.contains(pos, q.lod_index)) {
			q.result = RESULT_BLOCK_NOT_FOUND;
			continue;
		}

		if (_cache.load_voxel_block(pos, q.lod_index, q.voxel_buffer)) {
			q.result = RESULT_BLOCK_FOUND;

		} else {
			blocks_to_load.push_back(i);
		}
	}

	if (blocks_to_load.size() == 0) {
		// 所有内容都已缓存，无需查询数据库
		return;
	}

	if (con->begin_transaction() == false) {
		VOXEL_PRINT_ERROR("VoxelStreamSQLite: failed to begin transaction, blocks were not loaded");
		set_result_codes(p_blocks, blocks_to_load, RESULT_ERROR);
		con_scope.broken = !recover_after_failed_transaction(*con);
		return;
	}

	for (unsigned int i = 0; i < blocks_to_load.size(); ++i) {
		const unsigned int ri = blocks_to_load[i];
		VoxelStream::VoxelQueryData &q = p_blocks[ri];

		BlockLocation loc;
		loc.position = q.position_in_blocks;
		loc.lod = q.lod_index;

		StdVector<uint8_t> &temp_block_data = get_tls_temp_block_data();

		const ResultCode res = con->load_block(loc, temp_block_data, sqlite::Connection::VOXELS);

		if (res == RESULT_BLOCK_FOUND) {
			// TODO 不确定我们是否真的应期待非空。可能存在合法“未找到”的数据块。
			BlockSerializer::decompress_and_deserialize(to_span_const(temp_block_data), q.voxel_buffer);
		}

		q.result = res;
	}

	if (con->end_transaction() == false) {
		// 该事务只读取数据，结果已在上面复制出来，因此它们仍然有效。
		// 只有连接需要关注。
		VOXEL_PRINT_ERROR("VoxelStreamSQLite: failed to end read transaction, recovering the connection");
		con_scope.broken = !recover_after_failed_transaction(*con);
	}
}

void VoxelStreamSQLite::save_voxel_blocks(Span<VoxelStream::VoxelQueryData> p_blocks) {
	const ConnectionResult con_res = get_connection();
	switch (con_res.code) {
		case ConnectionResult::SUCCESS:
			break;
		case ConnectionResult::NOT_CONFIGURED:
			return;
		default:
			return;
	}

	sqlite::Connection *con = con_res.connection;

	const BlockLocation::CoordinateFormat coordinate_format = con->get_meta().coordinate_format;
	recycle_connection(con);

	const Box3i coordinate_range = BlockLocation::get_coordinate_range(coordinate_format);
	const unsigned int lod_count = BlockLocation::get_lod_count(coordinate_format);

	// 先放入缓存
	for (unsigned int i = 0; i < p_blocks.size(); ++i) {
		VoxelStream::VoxelQueryData &q = p_blocks[i];
		const Vector3i pos = q.position_in_blocks;

		if (!validate_range(pos, q.lod_index, coordinate_range, lod_count)) {
			continue;
		}

		_cache.save_voxel_block(pos, q.lod_index, q.voxel_buffer);
		if (_block_keys_cache_enabled) {
			_block_keys_cache.add(pos, q.lod_index);
		}
	}

	// TODO 我们应该考虑使用序列化缓存，并测量以字节为单位的阈值
	if (_cache.get_indicative_block_count() >= CACHE_SIZE) {
		if (!flush_cache()) {
			// 可恢复：block 保持缓存状态（除非提交本身失败，如上所述），并且
			// 下一次使缓存增长超过阈值的保存会重试。
			VOXEL_PRINT_WARNING("VoxelStreamSQLite: automatic cache flush did not complete, will retry later");
		}
	}
}

#ifdef VOXEL_ENABLE_INSTANCER

bool VoxelStreamSQLite::supports_instance_blocks() const {
	return true;
}

void VoxelStreamSQLite::load_instance_blocks(Span<VoxelStream::InstancesQueryData> out_blocks) {
	VOXEL_PROFILE_SCOPE();

	// TODO 从数据库获取数据块尺寸
	// const int bs_po2 = constants::DEFAULT_BLOCK_SIZE_PO2;

	// 先检查缓存
	StdVector<unsigned int> blocks_to_load;
	for (size_t i = 0; i < out_blocks.size(); ++i) {
		VoxelStream::InstancesQueryData &q = out_blocks[i];

		if (_cache.load_instance_block(q.position_in_blocks, q.lod_index, q.data)) {
			q.result = RESULT_BLOCK_FOUND;

		} else {
			blocks_to_load.push_back(i);
		}
	}

	if (blocks_to_load.size() == 0) {
		// 所有内容都已缓存，无需查询数据库
		return;
	}

	ConnectionResult con_res = get_connection();
	switch (con_res.code) {
		case ConnectionResult::SUCCESS:
			break;
		case ConnectionResult::NOT_CONFIGURED:
			set_result_codes(out_blocks, VoxelStream::RESULT_BLOCK_NOT_FOUND);
			return;
		default:
			set_result_codes(out_blocks, VoxelStream::RESULT_ERROR);
			return;
	}

	sqlite::Connection *con = con_res.connection;
	ScopeRecycle con_scope(this, con);

	if (con->begin_transaction() == false) {
		VOXEL_PRINT_ERROR("VoxelStreamSQLite: failed to begin transaction, instance blocks were not loaded");
		set_result_codes(out_blocks, blocks_to_load, RESULT_ERROR);
		con_scope.broken = !recover_after_failed_transaction(*con);
		return;
	}

	for (unsigned int i = 0; i < blocks_to_load.size(); ++i) {
		const unsigned int ri = blocks_to_load[i];
		VoxelStream::InstancesQueryData &q = out_blocks[ri];

		BlockLocation loc;
		loc.position = q.position_in_blocks;
		loc.lod = q.lod_index;

		StdVector<uint8_t> &temp_compressed_block_data = get_tls_temp_compressed_block_data();

		const ResultCode res = con->load_block(loc, temp_compressed_block_data, sqlite::Connection::INSTANCES);

		if (res == RESULT_BLOCK_FOUND) {
			StdVector<uint8_t> &temp_block_data = get_tls_temp_block_data();

			if (!CompressedData::decompress(to_span_const(temp_compressed_block_data), temp_block_data)) {
				ERR_PRINT("Failed to decompress instance block");
				q.result = RESULT_ERROR;
				continue;
			}
			q.data = make_unique_instance<InstanceBlockData>();
			if (!deserialize_instance_block_data(*q.data, to_span_const(temp_block_data))) {
				ERR_PRINT("Failed to deserialize instance block");
				q.result = RESULT_ERROR;
				continue;
			}
		}

		q.result = res;
	}

	if (con->end_transaction() == false) {
		// 该事务只读取数据，结果已在上面复制出来，因此它们仍然有效。
		// 只有连接需要关注。
		VOXEL_PRINT_ERROR("VoxelStreamSQLite: failed to end read transaction, recovering the connection");
		con_scope.broken = !recover_after_failed_transaction(*con);
	}
}

void VoxelStreamSQLite::save_instance_blocks(Span<VoxelStream::InstancesQueryData> p_blocks) {
	ConnectionResult con_res = get_connection();
	switch (con_res.code) {
		case ConnectionResult::SUCCESS:
			break;
		case ConnectionResult::NOT_CONFIGURED:
			return;
		default:
			return;
	}

	sqlite::Connection *con = con_res.connection;
	const BlockLocation::CoordinateFormat coordinate_format = con->get_meta().coordinate_format;
	recycle_connection(con);

	const Box3i coordinate_range = BlockLocation::get_coordinate_range(coordinate_format);
	const unsigned int lod_count = BlockLocation::get_lod_count(coordinate_format);

	// 先放入缓存
	for (size_t i = 0; i < p_blocks.size(); ++i) {
		VoxelStream::InstancesQueryData &q = p_blocks[i];

		if (!validate_range(q.position_in_blocks, q.lod_index, coordinate_range, lod_count)) {
			continue;
		}

		_cache.save_instance_block(q.position_in_blocks, q.lod_index, std::move(q.data));
		if (_block_keys_cache_enabled) {
			_block_keys_cache.add(q.position_in_blocks, q.lod_index);
		}
	}

	// TODO 优化：我们应该考虑使用序列化缓存，并测量以字节为单位的阈值
	if (_cache.get_indicative_block_count() >= CACHE_SIZE) {
		if (!flush_cache()) {
			// 可恢复：block 保持缓存状态（除非提交本身失败，如上所述），并且
			// 下一次使缓存增长超过阈值的保存会重试。
			VOXEL_PRINT_WARNING("VoxelStreamSQLite: automatic cache flush did not complete, will retry later");
		}
	}
}

#endif

void VoxelStreamSQLite::load_all_blocks(FullLoadingResult &result) {
	VOXEL_PROFILE_SCOPE();

	const ConnectionResult con_res = get_connection();

	switch (con_res.code) {
		case ConnectionResult::SUCCESS:
			break;
		case ConnectionResult::NOT_CONFIGURED:
			return;
		default:
			return;
	}

	sqlite::Connection *con = con_res.connection;

	const ScopeRecycle con_scope(this, con);

	struct Context {
		FullLoadingResult &result;
	};

	// 诚然，由于一个相当愚蠢的原因而使用局部函数而不是 lambda：
	// Godot 的 clang-format 不允许把函数参数写在一列上，
	// 这会使 lambda 超出行长限制。
	struct L {
		static void process_block_func(
				void *callback_data,
				const BlockLocation location,
				Span<const uint8_t> voxel_data,
				Span<const uint8_t> instances_data
		) {
			Context *ctx = reinterpret_cast<Context *>(callback_data);

			if (voxel_data.size() == 0 && instances_data.size() == 0) {
				VOXEL_PRINT_VERBOSE(format(
						"Unexpected empty voxel data and instances data at {} lod {}", location.position, location.lod
				));
				return;
			}

			FullLoadingResult::Block result_block;
			result_block.position = location.position;
			result_block.lod = location.lod;

			if (voxel_data.size() > 0) {
				std::shared_ptr<VoxelBuffer> voxels = make_shared_instance<VoxelBuffer>(VoxelBuffer::ALLOCATOR_POOL);
				ERR_FAIL_COND(!BlockSerializer::decompress_and_deserialize(voxel_data, *voxels));
				result_block.voxels = voxels;
			}

#ifdef VOXEL_ENABLE_INSTANCER
			if (instances_data.size() > 0) {
				StdVector<uint8_t> &temp_block_data = get_tls_temp_block_data();
				if (!CompressedData::decompress(instances_data, temp_block_data)) {
					ERR_PRINT("Failed to decompress instance block");
					return;
				}
				result_block.instances_data = make_unique_instance<InstanceBlockData>();
				if (!deserialize_instance_block_data(*result_block.instances_data, to_span_const(temp_block_data))) {
					ERR_PRINT("Failed to deserialize instance block");
					return;
				}
			}
#endif

			ctx->result.blocks.push_back(std::move(result_block));
		}
	};

	// 不得不添加 `_outer` 后缀，
	// 否则 GCC 认为它会遮蔽局部函数/无捕获 lambda 内部的某个变量
	Context ctx_outer{ result };
	const bool request_result = con->load_all_blocks(&ctx_outer, L::process_block_func);
	ERR_FAIL_COND(request_result == false);
}

int VoxelStreamSQLite::get_used_channels_mask() const {
	// 假定包含全部，因为该流可以存储任何内容。
	return VoxelBuffer::ALL_CHANNELS_MASK;
}

bool VoxelStreamSQLite::flush_cache() {
	const ConnectionResult con_res = get_connection();
	switch (con_res.code) {
		case ConnectionResult::SUCCESS:
			break;
		case ConnectionResult::NOT_CONFIGURED:
			return false;
		default:
			return false;
	}
	sqlite::Connection *con = con_res.connection;

	ScopeRecycle con_scope(this, con);
	const bool success = flush_cache_to_connection(con);
	if (!success) {
		con_scope.broken = !recover_after_failed_transaction(*con);
	}
	return success;
}

void VoxelStreamSQLite::flush() {
	if (!flush_cache()) {
		VOXEL_PRINT_ERROR("VoxelStreamSQLite: flush did not complete");
	}
}

// 此函数不为内部用途锁定任何互斥锁。
// 若事务失败则返回 false，此时连接在复用前可能需要恢复（参见
// `recover_after_failed_transaction`）。
bool VoxelStreamSQLite::flush_cache_to_connection(sqlite::Connection *p_connection) {
	VOXEL_PROFILE_SCOPE();
	VOXEL_PRINT_VERBOSE(format("VoxelStreamSQLite: Flushing cache ({} elements)", _cache.get_indicative_block_count()));

	ERR_FAIL_COND_V(p_connection == nullptr, false);
	if (p_connection->begin_transaction() == false) {
		// 此时没有任何东西被写入或丢弃：缓存的数据块被保留，因此后续的刷新会重试
		// 它们。详细的报告留给调用方处理，因为它们了解自身的上下文。
		VOXEL_PRINT_VERBOSE("VoxelStreamSQLite: could not begin flush transaction, keeping cached blocks");
		return false;
	}

#ifdef VOXEL_ENABLE_INSTANCER
	StdVector<uint8_t> &temp_data = get_tls_temp_block_data();
#endif
	StdVector<uint8_t> &temp_compressed_data = get_tls_temp_compressed_block_data();

	const BlockLocation::CoordinateFormat coordinate_format = p_connection->get_meta().coordinate_format;
	const Box3i coordinate_range = BlockLocation::get_coordinate_range(coordinate_format);
	const unsigned int lod_count = BlockLocation::get_lod_count(coordinate_format);

	const CompressedData::Compression compression_mode = _compression_mode;

	// TODO 需要更好的错误回滚处理
	_cache.flush([p_connection,
#ifdef VOXEL_ENABLE_INSTANCER
				  &temp_data,
#endif
				  &temp_compressed_data,
				  coordinate_range,
				  compression_mode,
				  lod_count](VoxelStreamCache::Block &block) {
		VOXEL_ASSERT_RETURN(validate_range(block.position, block.lod, coordinate_range, lod_count));

		BlockLocation loc;
		loc.position = block.position;
		loc.lod = block.lod;

		// 保存体素
		if (block.has_voxels) {
			if (block.voxels_deleted) {
				p_connection->save_block(loc, Span<const uint8_t>(), sqlite::Connection::VOXELS);
			} else {
				BlockSerializer::SerializeResult res =
						BlockSerializer::serialize_and_compress(block.voxels, compression_mode);
				ERR_FAIL_COND(!res.success);
				p_connection->save_block(loc, to_span(res.data), sqlite::Connection::VOXELS);
			}
		}

		// 保存实例
		temp_compressed_data.clear();
#ifdef VOXEL_ENABLE_INSTANCER
		if (block.instances != nullptr) {
			temp_data.clear();

			ERR_FAIL_COND(!serialize_instance_block_data(*block.instances, temp_data));

			ERR_FAIL_COND(!CompressedData::compress(
					to_span_const(temp_data), temp_compressed_data, CompressedData::COMPRESSION_NONE
			));
		}
#endif
		p_connection->save_block(loc, to_span(temp_compressed_data), sqlite::Connection::INSTANCES);

		// TODO 优化：增加一个可同时更新两者的查询版本
	});

	if (p_connection->end_transaction() == false) {
		// 缓存已被排空到该事务中，因此这些数据块在不保存的情况下被丢弃。
		// 与上面失败的 begin 不同，这里是会丢失数据的情况。
		VOXEL_PRINT_ERROR("VoxelStreamSQLite: failed to commit flush transaction, unsaved cached blocks were dropped");
		return false;
	}

	return true;
}

VoxelStreamSQLite::ConnectionResult VoxelStreamSQLite::get_connection() {
	StdString fpath;
	CoordinateFormat preferred_coordinate_format;
	{
		MutexLock mlock(_connection_mutex);

		if (_globalized_connection_path.empty()) {
			VOXEL_PRINT_WARNING_ONCE("The database path hasn't been set.")
			return { nullptr, ConnectionResult::NOT_CONFIGURED };
		}
		if (_connection_pool.size() != 0) {
			sqlite::Connection *existing_connection = _connection_pool.back();
			_connection_pool.pop_back();
			return { existing_connection, ConnectionResult::SUCCESS };
		}
		// 自我们设置数据库路径以来获取的第一个连接
		fpath = _globalized_connection_path;
		preferred_coordinate_format = _preferred_coordinate_format;
	}

	if (fpath.empty()) {
		VOXEL_PRINT_WARNING_ONCE("The database path hasn't been set.")
		return { nullptr, ConnectionResult::NOT_CONFIGURED };
	}
	sqlite::Connection *con = new sqlite::Connection();
	if (!con->open(fpath.data(), to_internal_coordinate_format(preferred_coordinate_format))) {
		delete con;
		return { nullptr, ConnectionResult::ERROR };
	}
	if (_block_keys_cache_enabled) {
		RWLockWrite wlock(_block_keys_cache.rw_lock);
		con->load_all_block_keys(&_block_keys_cache, [](void *ctx, BlockLocation loc) {
			BlockKeysCache *cache = static_cast<BlockKeysCache *>(ctx);
			cache->add_no_lock(loc.position, loc.lod);
		});
	}
	return { con, ConnectionResult::SUCCESS };
}

void VoxelStreamSQLite::recycle_connection(sqlite::Connection *con) {
	const char *con_path = con->get_opened_file_path();
	// 如果连接路径没有改变，则放回连接池
	{
		MutexLock mlock(_connection_mutex);
		if (_globalized_connection_path == con_path) {
			_connection_pool.push_back(con);
			return;
		}
	}
	delete con;
}

void VoxelStreamSQLite::destroy_connection(sqlite::Connection *con) {
	// 在这里定义而非在头文件中内联，因为头文件中 `Connection` 仅被前置声明，其析构函数
	// 因此不会运行。
	delete con;
}

void VoxelStreamSQLite::set_key_cache_enabled(bool enable) {
	_block_keys_cache_enabled = enable;
}

bool VoxelStreamSQLite::is_key_cache_enabled() const {
	return _block_keys_cache_enabled;
}

Box3i VoxelStreamSQLite::get_supported_block_range() const {
	// const Connection *con = get_connection();
	// const CoordinateFormat format = con != nullptr ? con->get_meta().coordinate_format :
	// _preferred_coordinate_format;
	const CoordinateFormat format = _preferred_coordinate_format;
	return BlockLocation::get_coordinate_range(to_internal_coordinate_format(format));
}

int VoxelStreamSQLite::get_lod_count() const {
	// const Connection *con = get_connection();
	// const CoordinateFormat format = con != nullptr ? con->get_meta().coordinate_format :
	// _preferred_coordinate_format;
	const CoordinateFormat format = _preferred_coordinate_format;
	return BlockLocation::get_lod_count(to_internal_coordinate_format(format));
}

void VoxelStreamSQLite::set_preferred_coordinate_format(CoordinateFormat format) {
	VOXEL_ASSERT_RETURN(format >= 0 && format < COORDINATE_FORMAT_COUNT);
	_preferred_coordinate_format = format;
}

VoxelStreamSQLite::CoordinateFormat VoxelStreamSQLite::get_preferred_coordinate_format() const {
	return _preferred_coordinate_format;
}

VoxelStreamSQLite::CoordinateFormat VoxelStreamSQLite::get_current_coordinate_format() {
	const ConnectionResult con_res = get_connection();
	sqlite::Connection *con = con_res.connection;
	if (con == nullptr) {
		return get_preferred_coordinate_format();
	}
	const ScopeRecycle con_scope(this, con);
	return to_exposed_coordinate_format(con->get_meta().coordinate_format);
}

bool VoxelStreamSQLite::copy_blocks_to_other_sqlite_stream(Ref<VoxelStreamSQLite> dst_stream) {
	// 当旧存档格式需要改变时，此函数可作为把旧存档迁移到新存档的通用方法。如果只是版本变化，
	// 或许可以在原地完成，但诸如坐标格式之类
	// 的变更会影响主键，因此不原地处理会更容易。

	VOXEL_ASSERT_RETURN_V(dst_stream.is_valid(), false);
	VOXEL_ASSERT_RETURN_V(dst_stream.ptr() != this, false);

	VOXEL_ASSERT_RETURN_V(dst_stream->get_database_path() != get_database_path(), false);

	VOXEL_ASSERT_RETURN_V_MSG(
			dst_stream->get_block_size_po2() != get_block_size_po2(),
			false,
			"Copying between streams of different block sizes is not supported"
	);

	sqlite::Connection *src_con = get_connection().connection;
	VOXEL_ASSERT_RETURN_V(src_con != nullptr, false);
	const ScopeRecycle src_con_scope(this, src_con);

	// 我们可以跳过反序列化而直接拷贝数据块。
	// 我们也不使用缓存。

	struct Context {
		sqlite::Connection *dst_con;

		static void save(
				void *cb_data,
				BlockLocation location,
				Span<const uint8_t> voxel_data,
				Span<const uint8_t> instances_data
		) {
			Context *ctx = static_cast<Context *>(cb_data);
			ctx->dst_con->save_block(location, voxel_data, sqlite::Connection::VOXELS);
			ctx->dst_con->save_block(location, instances_data, sqlite::Connection::INSTANCES);
		}
	};

	Context context;
	context.dst_con = dst_stream->get_connection().connection;
	VOXEL_ASSERT_RETURN_V(context.dst_con != nullptr, false);
	const ScopeRecycle dst_con_scope(dst_stream.ptr(), context.dst_con);

	const bool success = src_con->load_all_blocks(&context, Context::save);

	return success;
}

void VoxelStreamSQLite::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_database_path", "path"), &VoxelStreamSQLite::set_database_path);
	ClassDB::bind_method(D_METHOD("get_database_path"), &VoxelStreamSQLite::get_database_path);

	ClassDB::bind_method(D_METHOD("set_key_cache_enabled", "enabled"), &VoxelStreamSQLite::set_key_cache_enabled);
	ClassDB::bind_method(D_METHOD("is_key_cache_enabled"), &VoxelStreamSQLite::is_key_cache_enabled);

	ClassDB::bind_method(
			D_METHOD("set_preferred_coordinate_format", "format"), &VoxelStreamSQLite::set_preferred_coordinate_format
	);
	ClassDB::bind_method(
			D_METHOD("get_preferred_coordinate_format"), &VoxelStreamSQLite::get_preferred_coordinate_format
	);

	BIND_ENUM_CONSTANT(COORDINATE_FORMAT_INT64_X16_Y16_Z16_L16);
	BIND_ENUM_CONSTANT(COORDINATE_FORMAT_INT64_X19_Y19_Z19_L7);
	BIND_ENUM_CONSTANT(COORDINATE_FORMAT_STRING_CSD);
	BIND_ENUM_CONSTANT(COORDINATE_FORMAT_BLOB80_X25_Y25_Z25_L5);
	BIND_ENUM_CONSTANT(COORDINATE_FORMAT_COUNT);

	ADD_PROPERTY(
			PropertyInfo(Variant::STRING, "database_path", PROPERTY_HINT_FILE), "set_database_path", "get_database_path"
	);

	ADD_PROPERTY(
			PropertyInfo(
					Variant::INT,
					"preferred_coordinate_format",
					PROPERTY_HINT_ENUM,
					"Int64_X16_Y16_Z16_LOD16,Int64_X19_Y19_Z19_LOD7,String_CSD,Blob80_X25_Y25_Z25_LOD5"
			),
			"set_preferred_coordinate_format",
			"get_preferred_coordinate_format"
	);
}

} // namespace voxel
