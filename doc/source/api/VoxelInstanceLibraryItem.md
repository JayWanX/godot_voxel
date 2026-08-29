# VoxelInstanceLibraryItem

继承自：[Resource](https://docs.godotengine.org/en/stable/classes/class_resource.html)

派生：[VoxelInstanceLibraryMultiMeshItem](VoxelInstanceLibraryMultiMeshItem.md), [VoxelInstanceLibrarySceneItem](VoxelInstanceLibrarySceneItem.md)

供 [VoxelInstancer](VoxelInstancer.md) 使用的模型的设置。

## 属性：


类型                                                                          | 名称                                                                       | 默认值   
--------------------------------------------------------------------------- | ------------------------------------------------------------------------ | ------
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)    | [floating_sdf_offset_along_normal](#i_floating_sdf_offset_along_normal)  | -0.1  
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)    | [floating_sdf_threshold](#i_floating_sdf_threshold)                      | 0.0   
[VoxelInstanceGenerator](VoxelInstanceGenerator.md)                         | [generator](#i_generator)                                                |       
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)        | [lod_index](#i_lod_index)                                                | 0     
[String](https://docs.godotengine.org/en/stable/classes/class_string.html)  | [name](#i_name)                                                          | ""    
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)      | [persistent](#i_persistent)                                              | false 
<p></p>

## 属性描述

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_floating_sdf_offset_along_normal"></span> **floating_sdf_offset_along_normal** = -0.1

在检测对象是否漂浮时，沿实例向上轴应用的偏移。

如果实例原点处的 SDF 大于 0，则该实例会被判定为漂浮。然而在实际中，SDF 可能不精确，而且实例生成在网格三角形上的方式意味着它们位置的 SDF 总会围绕 0 上下小幅波动。这可能导致在周围挖掘后实例被过度移除，或移除不足。你可以尝试将此偏移增大到很小的值来修复，这样会改而检测实例略下方（通常在地下）的 SDF。负值会检测下方，正值会检测上方。请务必测试最终的行为。

警告：将此值设置得过高会完全破坏自动移除功能，但相比 [floating_sdf_threshold](VoxelInstanceLibraryItem.md#i_floating_sdf_threshold) 相对更安全。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_floating_sdf_threshold"></span> **floating_sdf_threshold** = 0.0

用于判断 SDF 是否为空气的阈值。如果采样位置的 SDF 更大，则该实例被视为漂浮。更多信息请参见 [floating_sdf_offset_along_normal](VoxelInstanceLibraryItem.md#i_floating_sdf_offset_along_normal)。

警告：将此值设置得过高会完全破坏自动移除功能。虽然这可以在挖掘地面后“关闭”移除，但当实例没有碰撞体时不建议这样做，因为那样就完全没有移除它们的方法了。

### [VoxelInstanceGenerator](VoxelInstanceGenerator.md)<span id="i_generator"></span> **generator**

用于挑选条目生成位置的生成器。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_lod_index"></span> **lod_index** = 0

该条目将生成的地形区块的 LOD 索引。索引越高，在观察者周围生成的区域越广，但由于使用更低细节层级的网格，精度也会更低。建议将较大的对象（大树、巨石）放在较高的 LOD 索引上生成，将较小的对象（草、小石头）放在较低的 LOD 索引上生成。

### [String](https://docs.godotengine.org/en/stable/classes/class_string.html)<span id="i_name"></span> **name** = ""

*(此属性暂无文档)*

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_persistent"></span> **persistent** = false

若未启用，只要生成器条件满足，条目就总会生成：例如，如果你挖了一个洞，它会移除地表的草，但如果你离开该区域再回来，草会在洞内重新生成。

若启用，且地形具有支持实例的 [VoxelStream](VoxelStream.md)，则该条目将保存在其实例被修改的区块中（与体素修改遵循相同的事件），因此如果你离开再回来，该条目的实例将保持你离开时的状态。仅保存条目的变换。

注意：保存时依赖于使用 [VoxelInstanceLibrary](VoxelInstanceLibrary.md) 中给出的相同编号来在存档文件中标识该条目。移除条目或更改其 ID 可能导致存档错误地加载该条目。

另请参见 [https://voxel-tools.readthedocs.io/en/latest/instancing/#persistence](https://voxel-tools.readthedocs.io/en/latest/instancing/#persistence)

_生成于 2026-08-28_
