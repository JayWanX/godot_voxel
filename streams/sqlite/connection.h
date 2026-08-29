#ifndef VOXEL_STREAM_SQLITE_CONNECTION_H
#define VOXEL_STREAM_SQLITE_CONNECTION_H

#include "../../storage/voxel_buffer.h"
#include "../../util/string/std_string.h"
#include "../voxel_stream.h"
#include "block_location.h"

struct sqlite3;
struct sqlite3_stmt;

namespace voxel::sqlite {

// 一个连接到数据库的连接，带有我们的预编译语句
class Connection {
public:
	static constexpr int VERSION_V0 = 0;
	static constexpr int VERSION_V1 = 1;
	static constexpr int VERSION_LATEST = VERSION_V1;

	struct Meta {
		int version = -1;
		int block_size_po2 = 0;
		BlockLocation::CoordinateFormat coordinate_format =
				// Default as of V0
				BlockLocation::FORMAT_INT64_X16_Y16_Z16_L16;

		struct Channel {
			VoxelBuffer::Depth depth;
			bool used = false;
		};

		FixedArray<Channel, VoxelBuffer::MAX_CHANNELS> channels;
	};

	enum BlockType { //
		VOXELS,
		INSTANCES
	};

	Connection();
	~Connection();

	bool open(const char *fpath, const BlockLocation::CoordinateFormat preferred_coordinate_format);
	void close();

	bool is_open() const {
		return _db != nullptr;
	}

	// 从 SQLite 返回文件路径
	const char *get_file_path() const;

	// 返回用于打开该连接的文件路径。
	// 如果你想要确定性，可以使用这个，因为 SQLite 似乎会全局化其路径。
	const char *get_opened_file_path() const {
		return _opened_path.c_str();
	}

	bool begin_transaction();
	bool end_transaction();

	// 回滚可能仍在此连接上活跃的事务，并清除
	// 事务语句上的残留错误状态。在 `begin_transaction` 或 `end_transaction`
	// 失败后，可用此方法恢复连接以便复用。若连接无法恢复则返回 false，此时不得
	// 复用它。在没有活动事务时调用也是安全的。
	bool rollback_transaction();

	bool save_block(const BlockLocation loc, const Span<const uint8_t> block_data, const BlockType type);

	VoxelStream::ResultCode load_block(
			const BlockLocation loc,
			StdVector<uint8_t> &out_block_data,
			const BlockType type
	);

	bool load_all_blocks(
			void *callback_data,
			void (*process_block_func)(
					void *callback_data,
					BlockLocation location,
					Span<const uint8_t> voxel_data,
					Span<const uint8_t> instances_data
			)
	);

	bool load_all_block_keys(
			void *callback_data,
			void (*process_block_func)(void *callback_data, BlockLocation location)
	);

	const Meta &get_meta() const {
		return _meta;
	}

	void migrate_to_latest_version();

private:
	int load_version();
	Meta load_meta();
	void save_meta(Meta meta);
	bool migrate_to_next_version();
	bool migrate_from_v0_to_v1();

	StdString _opened_path;
	Meta _meta;
	sqlite3 *_db = nullptr;
	sqlite3_stmt *_load_version_statement = nullptr;
	sqlite3_stmt *_begin_statement = nullptr;
	sqlite3_stmt *_end_statement = nullptr;
	sqlite3_stmt *_rollback_statement = nullptr;
	sqlite3_stmt *_update_voxel_block_statement = nullptr;
	sqlite3_stmt *_get_voxel_block_statement = nullptr;
	sqlite3_stmt *_update_instance_block_statement = nullptr;
	sqlite3_stmt *_get_instance_block_statement = nullptr;
	sqlite3_stmt *_load_meta_statement = nullptr;
	sqlite3_stmt *_save_meta_statement = nullptr;
	sqlite3_stmt *_load_channels_statement = nullptr;
	sqlite3_stmt *_save_channel_statement = nullptr;
	sqlite3_stmt *_load_all_blocks_statement = nullptr;
	sqlite3_stmt *_load_all_block_keys_statement = nullptr;
};

} // namespace voxel::sqlite

#endif // VOXEL_STREAM_SQLITE_CONNECTION_H
