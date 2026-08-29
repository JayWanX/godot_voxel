# VoxelEngine

继承自：[Object](https://docs.godotengine.org/en/stable/classes/class_object.html)

保存通用设置并在后台线程中处理体素处理任务的单例。

## 描述：

体素引擎的单例。它在后台线程中运行体素处理任务（例如流式传输、网格化和生成），并保存通用设置，例如用于 `ThreadedTaskRunner` 的线程数。共享的线程池与内存池也由此管理。

## 方法：


返回值                                                                                 | 函数签名                                                                                                                      
----------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------
[Dictionary](https://docs.godotengine.org/en/stable/classes/class_dictionary.html)  | [get_stats](#i_get_stats) ( ) const                                                                                       
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)                | [get_thread_count](#i_get_thread_count) ( ) const                                                                         
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)              | [get_threaded_graphics_resource_building_enabled](#i_get_threaded_graphics_resource_building_enabled) ( ) const           
[String](https://docs.godotengine.org/en/stable/classes/class_string.html)          | [get_version_edition](#i_get_version_edition) ( ) const                                                                   
[String](https://docs.godotengine.org/en/stable/classes/class_string.html)          | [get_version_git_hash](#i_get_version_git_hash) ( ) const                                                                 
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)                | [get_version_major](#i_get_version_major) ( ) const                                                                       
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)                | [get_version_minor](#i_get_version_minor) ( ) const                                                                       
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)                | [get_version_patch](#i_get_version_patch) ( ) const                                                                       
[String](https://docs.godotengine.org/en/stable/classes/class_string.html)          | [get_version_status](#i_get_version_status) ( ) const                                                                     
[Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html)      | [get_version_v](#i_get_version_v) ( ) const                                                                               
[void](#)                                                                           | [run_tests](#i_run_tests) ( [Dictionary](https://docs.godotengine.org/en/stable/classes/class_dictionary.html) options )  
[void](#)                                                                           | [set_thread_count](#i_set_thread_count) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) count )    
<p></p>

## 方法描述

### [Dictionary](https://docs.godotengine.org/en/stable/classes/class_dictionary.html)<span id="i_get_stats"></span> **get_stats**( ) 

获取有关共享体素处理的调试信息。

返回的字典具有以下结构：

```
{
	"thread_pools": {
		"general": {
			"tasks": int,
			"active_threads": int,
			"thread_count": int,
			"task_names": PackedStringArray
		}
	},
	"tasks": {
		"streaming": int,
		"meshing": int,
		"generation": int,
		"main_thread": int,
		"gpu": int
	},
	"memory_pools": {
		"voxel_used": int,
		"voxel_total": int,
		"block_count": int,
		"std_allocated": int,
		"std_deallocated": int,
		"std_current": int
	}
}
```

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_get_thread_count"></span> **get_thread_count**( ) 

返回 `ThreadedTaskRunner` 当前在内部使用的线程数。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_get_threaded_graphics_resource_building_enabled"></span> **get_threaded_graphics_resource_building_enabled**( ) 

指示体素引擎是否能够从不同线程创建图形资源。如果当前渲染器的线程模型是安全的或多线程的，这通常为 true，但如果渲染器从中获益甚微（例如旧版 OpenGL），则可能为 false。

### [String](https://docs.godotengine.org/en/stable/classes/class_string.html)<span id="i_get_version_edition"></span> **get_version_edition**( ) 

告知体素引擎的版本类型，为以下之一：`module`、`extension`

### [String](https://docs.godotengine.org/en/stable/classes/class_string.html)<span id="i_get_version_git_hash"></span> **get_version_git_hash**( ) 

获取用于编译体素引擎的 Git 哈希。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_get_version_major"></span> **get_version_major**( ) 

获取体素引擎的主版本号。例如，在 `1.2.0` 中，`1` 是主版本号。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_get_version_minor"></span> **get_version_minor**( ) 

获取体素引擎的次版本号。例如，在 `1.2.0` 中，`2` 是次版本号。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_get_version_patch"></span> **get_version_patch**( ) 

获取体素引擎的修订版本号。例如，在 `1.2.0` 中，`0` 是修订版本号。

### [String](https://docs.godotengine.org/en/stable/classes/class_string.html)<span id="i_get_version_status"></span> **get_version_status**( ) 

获取版本状态，可能为以下之一：`dev`、`release`

### [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html)<span id="i_get_version_v"></span> **get_version_v**( ) 

将体素引擎的主版本号 (x)、次版本号 (y) 和修订版本号 (z) 作为一个向量获取。可用于版本比较。

### [void](#)<span id="i_run_tests"></span> **run_tests**( [Dictionary](https://docs.godotengine.org/en/stable/classes/class_dictionary.html) options ) 

运行内部单元测试。仅当体素引擎以 `voxel_tests=true` 编译时，此函数才可用。

### [void](#)<span id="i_set_thread_count"></span> **set_thread_count**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) count ) 

设置 `ThreadedTaskRunner` 在内部要使用的线程数。设置此值可能导致卡顿，并且可能需要一段时间，线程数才会真正与给定值一致。

_生成于 2026-08-28_
