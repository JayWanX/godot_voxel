# VoxelDataBlockEnterInfo

继承自：[Object](https://docs.godotengine.org/en/stable/classes/class_object.html)

当地形的一个数据块进入 [VoxelViewer](VoxelViewer.md) 的区域时，由地形发送的信息。

## 描述：

当地形的一个数据块进入 [VoxelViewer](VoxelViewer.md) 的区域时，由地形发送的信息。参见 [VoxelTerrain._on_data_block_entered](VoxelTerrain.md#i__on_data_block_entered)。

此类实例不可被存储，因为在产生它们的调用之后，它们将变为无效。

## 方法：


返回值                                                                             | 函数签名                                                    
------------------------------------------------------------------------------- | --------------------------------------------------------
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)          | [are_voxels_edited](#i_are_voxels_edited) ( ) const     
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)            | [get_lod_index](#i_get_lod_index) ( ) const             
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)            | [get_network_peer_id](#i_get_network_peer_id) ( ) const 
[Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html)  | [get_position](#i_get_position) ( ) const               
[VoxelBuffer](VoxelBuffer.md)                                                   | [get_voxels](#i_get_voxels) ( ) const                   
<p></p>

## 方法描述

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_are_voxels_edited"></span> **are_voxels_edited**( ) 

指示数据块中的体素是否曾被编辑过。如果没有，则意味着可以通过运行生成器获得相同的数据。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_get_lod_index"></span> **get_lod_index**( ) 

获取数据块所在的 LOD 索引。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_get_network_peer_id"></span> **get_network_peer_id**( ) 

获取导致该数据块被引用的 [VoxelViewer](VoxelViewer.md) 的网络对端 ID。

### [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html)<span id="i_get_position"></span> **get_position**( ) 

获取数据块的位置，坐标为数据块坐标（体素坐标由这些坐标乘以数据块大小得到）。

### [VoxelBuffer](VoxelBuffer.md)<span id="i_get_voxels"></span> **get_voxels**( ) 

获取对数据块内体素的访问。

_生成于 2026-09-12_
