SQLite 格式 v1
================

本页描述 `VoxelStreamSQLite` 使用的数据库模式。


相对版本 0 的变化
-------------------------

向 `meta` 表添加了 `coordinate_format` 列，并为 `blocks` 表提供了多种表示坐标的方式。


模式
--------

### `meta`

```
meta {
    - version: INTEGER
    - block_size_po2: INTEGER
    - coordinate_format: INTEGER
}
```

包含关于体量的通用信息。其中只有一行记录。

- `version` 是模式的版本。当前为 `1`。
- `block_size_po2` 是数据块以 2 的幂表示的尺寸。它们预期始终保持一致。默认值为 `4`（用于 16x16x16 的数据块）。
- `coordinate_format` 指定数据块坐标的存储方式。

通常，一旦数据库建立，这一行记录就不应再被修改。如果由于某些原因必须更改，更改方式必须保证数据库保持一致（可能需要重新处理所有数据块）。新建一个数据库并转换过去可能比就地修改更可取。


### `blocks`

```
blocks {
    if meta.coordinate_format is:
        0 or 1
            - loc: INT64 PRIMARY KEY
        2:
            - loc: TEXT PRIMARY KEY

    - vb: BLOB
    - instances: BLOB
}
```

包含体量中的每一个数据块。可能有成千上万个。

- `loc` 是标识数据块的键，通常由其坐标构成。其编码取决于 `meta.coordinate_format`。
- `vb` 包含使用[数据块格式](block_format_v4.md)压缩的体素数据。
- `instances` 包含使用[实例格式](instances_format_v1.md)压缩的实例数据。

#### 坐标格式

在所有情况下，坐标等于数据块在体素中的原点，按欧几里得除法除以数据块尺寸 + LOD 索引（`coord >> (block_size_po2 + lod_index)`）。
根据 `meta.coordinate_format`，该列的解释方式不同：

- `0`：64 位小端整数，打包数据块的坐标和 LOD 索引。XYZ 是 16 位有符号整数，LOD 是 8 位无符号整数：`0LXXYYZZ`。这是 v0 中的默认格式。
- `1`：64 位小端整数，打包数据块的坐标和 LOD 索引。XYZ 是 19 位有符号整数，LOD 是 7 位无符号整数：`lllllllx xxxxxxxx xxxxxxxx xxyyyyyy yyyyyyyy yyyyyzzz zzzzzzzz zzzzzzzz`（最高有效位在左侧）。
- `2`：以逗号分隔、基数为 10 的坐标，以纯文本形式存储，不含空格。
- `3`：80 位 blob，打包坐标和 LOD 索引。XYZ 是 25 位有符号整数，LOD 是 5 位无符号整数。

格式 `3` 可以这样表示：
```
Byte |   9        8        7        6        5        4        3        2        1        0
-----|--------|--------|--------|--------|--------|--------|--------|--------|--------|--------
Bits |lllllzzz zzzzzzzz zzzzzzzz zzzzzzyy yyyyyyyy yyyyyyyy yyyyyyyx xxxxxxxx xxxxxxxx xxxxxxxx
```
其中每个位簇（对应每个坐标）在读取时最高有效位在左侧，与打印出来时一致。注意字节顺序是反的。



### `channels`

```
channels {
    - idx: INTEGER PRIMARY KEY
    - depth: INTEGER
}
```

包含关于体量中应期望哪些通道格式的通用信息。每个使用的通道有一行记录。

!!! warning
    目前这张表实际上未被使用，因为引擎在整体上仍需要完善格式管理。目前数据库接受任何格式的数据块，因为自版本 3 起它们就是独立自足的，但理想情况下它们必须保持一致。
