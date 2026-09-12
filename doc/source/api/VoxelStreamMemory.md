# VoxelStreamMemory

继承自：[VoxelStream](VoxelStream.md)

将数据保存在内存中，而不是写入磁盘。

## 描述：

该数据流主要用于测试目的。它不应被用作正式的保存系统。

## 属性：


类型                                                                    | 名称                                                               | 默认值 
--------------------------------------------------------------------- | ---------------------------------------------------------------- | ----
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)  | [artificial_save_latency_usec](#i_artificial_save_latency_usec)  | 0   
<p></p>

## 属性描述

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_artificial_save_latency_usec"></span> **artificial_save_latency_usec** = 0

通过让调用线程休眠一段时间来模拟长时间保存。

_生成于 2026-09-12_
