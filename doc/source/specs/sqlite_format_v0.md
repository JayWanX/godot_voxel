SQLite 格式 v0
================

!!! warning
    本文档描述的是该格式的旧版本。你可以查看最新版本。

本页描述 `VoxelStreamSQLite` 使用的数据库模式。


模式
--------

### `meta`

```
meta {
    - version: INTEGER
    - block_size_po2: INTEGER
}
```

包含关于体量的通用信息。其中只有一行记录。

- `version` 是模式的版本。当前为 `0`。
- `block_size_po2` 是数据块以 2 的幂表示的尺寸。它们预期始终保持一致。默认值为 `4`（用于 16x16x16 的数据块）。


### `blocks`

```
blocks {
    - loc: INT64 PRIMARY KEY
    - vb: BLOB
    - instances: BLOB
}
```

包含体量中的每一个数据块。可能有成千上万个。

- `loc` 是一个 64 位整数，使用小端字节序打包数据块的坐标和 LOD 索引。坐标等于数据块在体素中的原点，按欧几里得除法除以数据块尺寸 + LOD 索引（`coord >> (block_size_po2 + lod_index)`）。XYZ 是 16 位有符号整数，LOD 是 8 位无符号整数：`0LXXYYZZ`
- `vb` 包含使用[数据块格式](block_format_v4.md)压缩的体素数据。
- `instances` 包含使用[实例格式](instances_format_v0.md)压缩的实例数据。


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
