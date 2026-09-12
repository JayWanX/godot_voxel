# VoxelSaveCompletionTracker

继承自：[RefCounted](https://docs.godotengine.org/en/stable/classes/class_refcounted.html)

由某些异步函数返回的对象，用于跟踪进度和完成情况。

## 方法：


返回值                                                                     | 函数签名                                                    
----------------------------------------------------------------------- | --------------------------------------------------------
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)    | [get_remaining_tasks](#i_get_remaining_tasks) ( ) const 
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)    | [get_total_tasks](#i_get_total_tasks) ( ) const         
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)  | [is_aborted](#i_is_aborted) ( ) const                   
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)  | [is_complete](#i_is_complete) ( ) const                 
<p></p>

## 方法描述

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_get_remaining_tasks"></span> **get_remaining_tasks**( ) 

获取剩余任务数。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_get_total_tasks"></span> **get_total_tasks**( ) 

获取任务总数。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_is_aborted"></span> **is_aborted**( ) 

保存过程是否已中止。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_is_complete"></span> **is_complete**( ) 

所有任务是否已完成。

_生成于 2026-09-12_
