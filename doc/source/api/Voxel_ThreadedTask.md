# Voxel_ThreadedTask

继承自：[RefCounted](https://docs.godotengine.org/en/stable/classes/class_refcounted.html)

允许在体素引擎线程池中运行自定义任务的抽象任务。

## 描述：

此类允许你将自定义任务调度到体素引擎的线程池中运行。继承此类并实现 [_run](Voxel_ThreadedTask.md#i__run)（必须）、可选的 [_get_priority](Voxel_ThreadedTask.md#i__get_priority) 和 [_is_cancelled](Voxel_ThreadedTask.md#i__is_cancelled)。任务完成后会发出 [Voxel_ThreadedTask.completed](Voxel_ThreadedTask.md#signals) 信号。此类为实验性质，其 API 可能会在未来的版本中发生变化。

## 方法：


返回值                                                                     | 函数签名                                                                                                          
----------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)    | [_get_priority](#i__get_priority) ( ) virtual                                                                 
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)  | [_is_cancelled](#i__is_cancelled) ( ) virtual                                                                 
[void](#)                                                               | [_run](#i__run) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) thread_index ) virtual 
<p></p>

## 信号：<span id="signals"></span>

### completed( ) 

任务完成时发出。

## 方法描述

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i__get_priority"></span> **_get_priority**( ) 

获取任务的优先级。值越高，任务在队列中被提前执行的可能性越大。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i__is_cancelled"></span> **_is_cancelled**( ) 

在任务执行期间调用，用于检查任务是否已被取消。

### [void](#)<span id="i__run"></span> **_run**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) thread_index ) 

在后台线程中运行任务逻辑。必须实现此方法。

_生成于 2026-09-12_
