# VoxelBlockSerializer

继承自：[RefCounted](https://docs.godotengine.org/en/stable/classes/class_refcounted.html)

用于保存和加载 [VoxelBuffer](VoxelBuffer.md) 内数据的底层工具。

## 描述：

用于保存和加载 [VoxelBuffer](VoxelBuffer.md) 内数据的底层工具。可用来通过网络发送数据，或将其存储到文件中。


要存储到文件，先分配 PackedByteArray：

```
# `voxels` 是一个已有的 `VoxelBuffer`
var data := VoxelBlockSerializer.serialize_to_byte_array(voxels, true)
file.store_32(len(data))
file.store_buffer(data)
```
再将其读回：

```
var size := file.get_32()
var data := file.get_buffer(size)
VoxelBlockSerializer.deserialize_from_byte_array(data, voxels, true)
```

通过复用 StreamPeerBuffer 存储到文件：

```
# 注意，如果经常这样做，可以复用该 buffer
var stream_peer_buffer := StreamPeerBuffer.new()
var written_size = VoxelBlockSerializer.serialize_to_stream_peer(stream_peer_buffer, voxels, true)
file.store_32(written_size)
file.store_buffer(stream_peer_buffer.data_array)
```
再将其读回：

```
var size := file.get_32()
var stream_peer_buffer := StreamPeerBuffer.new()
# 不幸的是，Godot 使用此 API 时总会分配内存，这一点无法避免
stream_peer_buffer.data_array = file.get_buffer(size)
VoxelBlockSerializer.deserialize_from_stream_peer(stream_peer_buffer, voxels, size, true)
```

## 方法：


返回值                                                                                           | 函数签名                                                                                                                                                                                                                                                                                                                                                                         
--------------------------------------------------------------------------------------------- | -----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
[void](#)                                                                                     | [deserialize_from_byte_array](#i_deserialize_from_byte_array) ( [PackedByteArray](https://docs.godotengine.org/en/stable/classes/class_packedbytearray.html) bytes, [VoxelBuffer](VoxelBuffer.md) voxel_buffer, [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html) decompress ) static                                                                   
[void](#)                                                                                     | [deserialize_from_stream_peer](#i_deserialize_from_stream_peer) ( [StreamPeer](https://docs.godotengine.org/en/stable/classes/class_streampeer.html) peer, [VoxelBuffer](VoxelBuffer.md) voxel_buffer, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) size, [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html) decompress ) static 
[PackedByteArray](https://docs.godotengine.org/en/stable/classes/class_packedbytearray.html)  | [serialize_to_byte_array](#i_serialize_to_byte_array) ( [VoxelBuffer](VoxelBuffer.md) voxel_buffer, [Compression](VoxelBlockSerializer.md#enumerations) compress ) static                                                                                                                                                                                                    
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)                          | [serialize_to_stream_peer](#i_serialize_to_stream_peer) ( [StreamPeer](https://docs.godotengine.org/en/stable/classes/class_streampeer.html) peer, [VoxelBuffer](VoxelBuffer.md) voxel_buffer, [Compression](VoxelBlockSerializer.md#enumerations) compress ) static                                                                                                         
<p></p>

## 枚举：<span id="enumerations"></span>

枚举 **Compression**：

- <span id="i_COMPRESSION_NONE"></span>**COMPRESSION_NONE** = **0** --- 不压缩数据。
- <span id="i_COMPRESSION_LZ4"></span>**COMPRESSION_LZ4** = **1** --- 使用 LZ4 默认压缩算法。速度快，但压缩率较低。
- <span id="i_COMPRESSION_ZSTD"></span>**COMPRESSION_ZSTD** = **2** --- 使用 Zstandard 默认压缩算法。压缩率较高，但速度较慢。


## 方法描述

### [void](#)<span id="i_deserialize_from_byte_array"></span> **deserialize_from_byte_array**( [PackedByteArray](https://docs.godotengine.org/en/stable/classes/class_packedbytearray.html) bytes, [VoxelBuffer](VoxelBuffer.md) voxel_buffer, [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html) decompress ) 

从 [PackedByteArray](https://docs.godotengine.org/en/stable/classes/class_packedbytearray.html) 读取 [VoxelBuffer](VoxelBuffer.md) 的数据。

### [void](#)<span id="i_deserialize_from_stream_peer"></span> **deserialize_from_stream_peer**( [StreamPeer](https://docs.godotengine.org/en/stable/classes/class_streampeer.html) peer, [VoxelBuffer](VoxelBuffer.md) voxel_buffer, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) size, [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html) decompress ) 

从 [StreamPeer](https://docs.godotengine.org/en/stable/classes/class_streampeer.html) 读取 [VoxelBuffer](VoxelBuffer.md) 的数据。你必须提供要读取的字节数。

### [PackedByteArray](https://docs.godotengine.org/en/stable/classes/class_packedbytearray.html)<span id="i_serialize_to_byte_array"></span> **serialize_to_byte_array**( [VoxelBuffer](VoxelBuffer.md) voxel_buffer, [Compression](VoxelBlockSerializer.md#enumerations) compress ) 

将 [VoxelBuffer](VoxelBuffer.md) 的数据存储到 [PackedByteArray](https://docs.godotengine.org/en/stable/classes/class_packedbytearray.html) 中。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_serialize_to_stream_peer"></span> **serialize_to_stream_peer**( [StreamPeer](https://docs.godotengine.org/en/stable/classes/class_streampeer.html) peer, [VoxelBuffer](VoxelBuffer.md) voxel_buffer, [Compression](VoxelBlockSerializer.md#enumerations) compress ) 

将 [VoxelBuffer](VoxelBuffer.md) 的数据存储到 [StreamPeer](https://docs.godotengine.org/en/stable/classes/class_streampeer.html) 中。返回写入的字节数。

_生成于 2026-08-28_
