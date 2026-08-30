#ifndef VOXEL_STREAM_SQLITE_H
#define VOXEL_STREAM_SQLITE_H

#include "../../util/containers/std_vector.h"
#include "../../util/string/std_string.h"
#include "../../util/thread/mutex.h"
#include "../voxel_block_serializer.h"
#include "../voxel_stream.h"
#include "../voxel_stream_cache.h"
#include "block_key_cache.h"

namespace voxel::sqlite {
class Connection;
}

namespace voxel {

// 将体素数据保存到单个 SQLite 数据库文件中。
class VoxelStreamSQLite : public VoxelStream {
	GDCLASS(VoxelStreamSQLite, VoxelStream)
public:
	static const unsigned int CACHE_SIZE = 64;

	VoxelStreamSQLite();
	~VoxelStreamSQLite();

	// 警告：在多线程上下文中，把此路径从一个有效值更改到另一个并不总是安全的。
	// 如果线程正要写入数据库 A，但路径却被改为数据库 B，
	// 那么剩余的数据将会被写入数据库 B。
	// 典型的用例是在游戏启动时设置此路径，并在本次会话结束前不再更改它。
	void set_database_path(String path);
	// 获取数据库文件路径
	String get_database_path() const;

	// 加载单个体素数据块
	void load_voxel_block(VoxelStream::VoxelQueryData &q) override;
	// 保存单个体素数据块
	void save_voxel_block(VoxelStream::VoxelQueryData &q) override;

	// 批量加载体素数据块
	void load_voxel_blocks(Span<VoxelStream::VoxelQueryData> p_blocks) override;
	// 批量保存体素数据块
	void save_voxel_blocks(Span<VoxelStream::VoxelQueryData> p_blocks) override;

#ifdef VOXEL_ENABLE_INSTANCER
	// 是否支持实例块数据
	bool supports_instance_blocks() const override;
	// 批量加载实例块数据
	void load_instance_blocks(Span<VoxelStream::InstancesQueryData> out_blocks) override;
	// 批量保存实例块数据
	void save_instance_blocks(Span<VoxelStream::InstancesQueryData> p_blocks) override;
#endif

	// 是否支持一次性加载所有数据块
	bool supports_loading_all_blocks() const override {
		return true;
	}
	// 一次性加载所有数据块
	void load_all_blocks(FullLoadingResult &result) override;

	// 获取此数据流中可用的通道掩码
	int get_used_channels_mask() const override;

	// 强制将待处理数据写入数据库
	void flush() override;
	// 刷新缓存，若刷新未完成则返回 false。此时，如果事务无法启动，缓存的数据块会被保留，
	// 但如果提交本身失败，它们则会丢失。
	bool flush_cache();

	// 设置键缓存是否启用。如果保存的数据非常稀疏（例如只保存被编辑过的数据块），这可能会改善查询性能。
	void set_key_cache_enabled(bool enable);
	// 获取键缓存是否启用
	bool is_key_cache_enabled() const;

	// 获取此数据流支持的数据块坐标范围
	Box3i get_supported_block_range() const override;
	// 获取数据块的细节层级（LOD）数量
	int get_lod_count() const override;

	enum CoordinateFormat {
		COORDINATE_FORMAT_INT64_X16_Y16_Z16_L16 = 0,
		COORDINATE_FORMAT_INT64_X19_Y19_Z19_L7,
		COORDINATE_FORMAT_STRING_CSD,
		COORDINATE_FORMAT_BLOB80_X25_Y25_Z25_L5,
		COORDINATE_FORMAT_COUNT
	};

	// 设置新建数据库时优先使用的坐标格式
	void set_preferred_coordinate_format(CoordinateFormat format);
	// 获取新建数据库时优先使用的坐标格式
	CoordinateFormat get_preferred_coordinate_format() const;

	// 获取现有数据库当前实际使用的坐标格式
	CoordinateFormat get_current_coordinate_format();

	// 将本数据流中的数据块复制到另一个 SQLite 数据流
	bool copy_blocks_to_other_sqlite_stream(Ref<VoxelStreamSQLite> dst_stream);

private:
	void rebuild_key_cache();

	// 在串行化模式下，SQlite3 数据库可安全地供多个线程使用，
	// 但用调试器单步查看实现之后，以下是实际发生的情况：
	//
	// 1) 预编译语句可能在多线程下使用是安全的，但最终结果是不安全的。
	//    线程 A 可能绑定一个值，随后在线程 A 执行语句前，线程 B 可能绑定另一个值替换第一个，
	//    因此最终每个线程都应有自己的一组语句。
	//
	// 2) 执行语句会用互斥锁锁住整个数据库。
	//    所以访问确实是被串行化的，也就是说 CPU 工作会串行执行，而不是并行执行。
	//    换句话说，你会失去多线程的速度优势。
	//
	// 正因为如此，在我们的用例中，让 SQLite 保持线程安全模式也许更简单，
	// 然后由我们自己来同步。

	struct ConnectionResult {
		enum Code { SUCCESS, NOT_CONFIGURED, ERROR };

		sqlite::Connection *connection = nullptr;
		Code code = ERROR;
	};

	ConnectionResult get_connection();
	void recycle_connection(sqlite::Connection *con);
	void destroy_connection(sqlite::Connection *con);

	struct ScopeRecycle {
		VoxelStreamSQLite *stream;
		sqlite::Connection *connection;
		// 在事务失败后无法恢复连接时置位。这样的连接在 SQLite 层面可能仍处于
		// 某个事务之中，随后每次调用 `begin_transaction` 都会以
		// "cannot start a transaction within a transaction" 失败，因此它绝不能放回连接池。
		bool broken = false;

		ScopeRecycle(VoxelStreamSQLite *p_stream, sqlite::Connection *p_connection) :
				stream(p_stream), connection(p_connection) {
#ifdef DEV_ENABLED
			VOXEL_ASSERT(stream != nullptr);
			VOXEL_ASSERT(connection != nullptr);
#endif
		}

		~ScopeRecycle() {
			if (broken) {
				stream->destroy_connection(connection);
			} else {
				stream->recycle_connection(connection);
			}
		}
	};

	bool flush_cache_to_connection(sqlite::Connection *p_connection);

	static void _bind_methods();

	String _user_specified_connection_path;
	StdString _globalized_connection_path;
	StdVector<sqlite::Connection *> _connection_pool;
	Mutex _connection_mutex;
	// 此缓存将数据块存储在内存中，当足够大时会刷新到数据库。
	// 这是因为保存类查询的开销更高。
	// 它还能加快对最近保存的数据块的查询速度。
	VoxelStreamCache _cache;
	// 我们当前流式传输数据的方式是查询每个玩家附近的每个数据块位置，以判断是否有数据。
	// 因此测试数据块是否存在，是最常执行的代码路径的开端。
	// 在只保存被编辑数据块的配置中，存入数据库的数据块非常少，
	// 因此缓存键以让该查询快速且可并发是有意义的。
	// 注意：长远来看，对于系统性地保存所有生成内容而非仅保存编辑内容的游戏，
	// 这样的缓存可能会变得相当大。此时我们可以允许关闭它，或改用八叉树。
	BlockKeysCache _block_keys_cache;
	bool _block_keys_cache_enabled = false;
	// 创建新数据库时将使用的格式。也不一定与现有数据库实际使用的格式一致
	// 即现有数据库实际所用的格式。
	CoordinateFormat _preferred_coordinate_format = COORDINATE_FORMAT_STRING_CSD;
};

} // namespace voxel

VARIANT_ENUM_CAST(voxel::VoxelStreamSQLite::CoordinateFormat);

#endif // VOXEL_STREAM_SQLITE_H
