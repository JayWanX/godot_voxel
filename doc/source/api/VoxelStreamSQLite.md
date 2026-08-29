# VoxelStreamSQLite

继承自：[VoxelStream](VoxelStream.md)

将体素数据保存到单个 SQLite 数据库文件中。

## 属性：


类型                                                                          | 名称                                                             | 默认值                              
--------------------------------------------------------------------------- | -------------------------------------------------------------- | ---------------------------------
[String](https://docs.godotengine.org/en/stable/classes/class_string.html)  | [database_path](#i_database_path)                              | ""                               
[CoordinateFormat](VoxelStreamSQLite.md#enumerations)                       | [preferred_coordinate_format](#i_preferred_coordinate_format)  | COORDINATE_FORMAT_STRING_CSD (2) 
<p></p>

## 方法：


返回值                                                                     | 函数签名                                                                                                                                  
----------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)  | [is_key_cache_enabled](#i_is_key_cache_enabled) ( ) const                                                                             
[void](#)                                                               | [set_key_cache_enabled](#i_set_key_cache_enabled) ( [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html) enabled )  
<p></p>

## 枚举：<span id="enumerations"></span>

枚举 **CoordinateFormat**：

- <span id="i_COORDINATE_FORMAT_INT64_X16_Y16_Z16_L16"></span>**COORDINATE_FORMAT_INT64_X16_Y16_Z16_L16** = **0** --- 坐标存储在 64 位整数键中，其中 X、Y、Z 和 LOD 为 16 位有符号整数。
- <span id="i_COORDINATE_FORMAT_INT64_X19_Y19_Z19_L7"></span>**COORDINATE_FORMAT_INT64_X19_Y19_Z19_L7** = **1** --- 坐标存储在 64 位整数键中，其中 X、Y 和 Z 为 19 位有符号整数，LOD 为 7 位无符号整数。
- <span id="i_COORDINATE_FORMAT_STRING_CSD"></span>**COORDINATE_FORMAT_STRING_CSD** = **2** --- 坐标存储在由逗号分隔的十进制数字字符串中，格式为“X,Y,Z,LOD”。
- <span id="i_COORDINATE_FORMAT_BLOB80_X25_Y25_Z25_L5"></span>**COORDINATE_FORMAT_BLOB80_X25_Y25_Z25_L5** = **3** --- 坐标存储在 80 位二进制大对象中，其中 X、Y 和 Z 为 25 位有符号整数，LOD 为 5 位无符号整数。
- <span id="i_COORDINATE_FORMAT_COUNT"></span>**COORDINATE_FORMAT_COUNT** = **4**


## 属性描述

### [String](https://docs.godotengine.org/en/stable/classes/class_string.html)<span id="i_database_path"></span> **database_path** = ""

数据库文件的路径。`res://` 和 `user://` 应该可用，但 `res://` 在导出后将无法工作（原因参见 [此处](https://docs.godotengine.org/en/stable/tutorials/io/data_paths.html#accessing-persistent-user-data-user)）。路径可以相对于游戏的可执行文件。路径中的目录必须存在。如果文件不存在，它将被创建。

### [CoordinateFormat](VoxelStreamSQLite.md#enumerations)<span id="i_preferred_coordinate_format"></span> **preferred_coordinate_format** = COORDINATE_FORMAT_STRING_CSD (2)

设置创建新数据库时将使用的数据块坐标格式。这会影响支持的坐标范围以及 SQLite 执行查询的速度（影响较小）。打开现有数据库时，此设置将被忽略，而使用数据库本身的格式。目前无法更改现有数据库的格式，可能需要使用脚本从某个数据流加载各个数据块并将其保存到新的数据库中。

## 方法描述

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_is_key_cache_enabled"></span> **is_key_cache_enabled**( ) 

*(此方法暂无文档)*

### [void](#)<span id="i_set_key_cache_enabled"></span> **set_key_cache_enabled**( [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html) enabled ) 

启用数据库键的缓存，以加快在只保存稀疏编辑数据块的地形中的加载查询。如果你的地形保存了其所有数据块（例如保存了生成器的输出），则这不会带来任何好处。

必须在任何对 `load_voxel_block` 的调用之前（即在地形开始使用它之前）调用此方法，否则将无法正常工作。你可以使用脚本来完成此操作。

_生成于 2026-08-28_
