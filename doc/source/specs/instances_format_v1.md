实例数据块格式 v1
=======================

本页描述该模块用于将实例保存到文件或数据库的二进制格式。

相对版本 0 的变化
-------------------------

- 数据现在使用小端字节序，而非大端字节序。
- 没有其它改动，因此版本 0 很容易转换。


规范
---------------

### 压缩容器

数据块通常被序列化为压缩数据。
规范请参见[压缩容器格式](compressed_container.md)。


### 二进制数据

此数据使用小端字节序。

用伪代码表示：

```cpp
// 根结构
struct InstanceBlockData {
	// 版本标记，以防将来增加更多内容
	uint8_t version = 1;
    // 一个数据块中最多可以有 256 个不同的层
	uint8_t layer_count;
	// 要压缩位置，我们需要知道它们的范围。
	// 它是数据块局部的，因此我们知道它从零开始。
	float position_range;
	LayerData layers[layer_count];
	// 用于标记数据块结束的魔数
	uint32_t control_end = 0x900df00d;
};

struct LayerData {
	uint16_t id; // 标识实例的类型（岩石、草、鹅卵石、灌木等）
	uint16_t count;
	// 要压缩缩放比例，我们必须知道它的范围
	float scale_min;
	float scale_max;
	// 这告诉此层的实例使用哪种格式。目前我始终使用相同的格式，
	// 但也许某些类型的实例会需要更多或更少的数据？
	uint8_t format = 0;
	InstanceData data[count];
};

struct InstanceData {
	// 位置根据数据块的大小进行有损压缩
	uint16_t x;
	uint16_t y;
	uint16_t z;
	// 缩放比例是统一的，并有损压缩到 256 个值
	uint8_t scale;
	// 旋转是压缩后的四元数
	uint8_t x;
	uint8_t y;
	uint8_t z;
	uint8_t w;
};
```
