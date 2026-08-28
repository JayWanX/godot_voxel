Editor
============

Previewing in the editor
---------------------------

![Screenshot of the editor](images/editor_preview_smooth_2d_noise_terrain.webp)

### Preview options

Terrains with a generator or valid stream assigned to them are able to show up in the editor by default.

If the generator or stream is providing a type of voxel data which is not supported by the mesher, nothing will show up. This is usually fixed by changing the mesher or its channel option, when available.

The whole terrain can be told to re-mesh or re-load by using one of the options in the `Terrain` menu:

![Re-generate menu](images/menu_regenerate.webp)

#### Tool scripts

!!! warning
    Take extra caution with tool scripts on generators and streams.

If you use a script on either [VoxelGeneratorScript](api/VoxelGeneratorScript.md) or [VoxelStreamScript](api/VoxelStreamScript.md), they will be executed in the editor if they are declared with tool mode (`@tool` in GDScript).
However, alongside risks of tool mode, there is extra danger: if the script gets modified while it is still being run by a background thread in the editor, unpredictable bugs can happen. You have to make sure the script doesn't change while previewing this way, or that terrain finished loading (can be forced to a degree by closing the scene). Therefore tool mode should only be used temporarily during development.
You can always test by running your game instead, with or without tool mode.
This limitation is tracked in [issue177](https://github.com/Voxel/godot_voxel/issues/177).


### Camera options

In the editor, blocks will only load around the node's origin by default. If the volume is very big or uses LOD, it will not load further and concentrate detail at its center. You can override this by going in the `Terrain` menu and enabling `Stream follow camera`. This will make the terrain adapt its level of detail and blocks to be around the editor's camera, and will update as the camera moves. Turning off the option will freeze the terrain.

![Stream follow camera menu](images/menu_stream_follow_camera.webp)

This option exists for large volumes because they need to stream blocks in and out as you move around. While this is often done in a controlled manner in a game, in the editor the camera could be moving very fast without any restriction, which can demand much more work for the CPU.
You can monitor the amount of ongoing tasks in the bottom panel, while the node is selected.

The terrain also needs to be selected, partially because of [this](https://github.com/godotengine/godot-proposals/issues/1302)).

Terrains can be very big, and sometimes Godot might prevent you from zooming out further. You can workaround this by increasing the editor's Camera `far` clip in the `View -> Settings` menu. That might slightly degrade visual quality if set too high, so you can also increase `near` clip to keep it balanced. These two numbers cannot be too far apart due to 32-bit float precision.


Editing
--------

There are no tools to edit voxel volumes destructively in the Godot Editor yet. This feature might be implemented in the future.

It is possible to use non-destructive [modifiers](generators.md#modifiers), but they are limited.

Terrains can be fully edited in-game using scripts and [VoxelTool](scripting.md). It is also possible to create a script editor plugin to implement edition in a similar manner.

