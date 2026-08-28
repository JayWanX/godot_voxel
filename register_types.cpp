#ifdef VOXEL_GODOT
// Module specific
#include "register_types.h"
#endif

#include "constants/voxel_string_names.h"
#include "edition/voxel_tool.h"
#include "edition/voxel_tool_buffer.h"
#include "edition/voxel_tool_lod_terrain.h"
#include "edition/voxel_tool_terrain.h"
#include "engine/voxel_engine_gd.h"
#include "generators/graph/node_type_db.h"
#include "generators/graph/voxel_generator_graph.h"
#include "generators/multipass/voxel_generator_multipass_cb.h"
#include "generators/voxel_generator_script.h"
#include "meshers/blocky/types/voxel_blocky_attribute_axis.h"
#include "meshers/blocky/types/voxel_blocky_attribute_custom.h"
#include "meshers/blocky/types/voxel_blocky_attribute_direction.h"
#include "meshers/blocky/types/voxel_blocky_attribute_rotation.h"
#include "meshers/blocky/types/voxel_blocky_type_library.h"
#include "meshers/blocky/voxel_blocky_fluid.h"
#include "meshers/blocky/voxel_blocky_library.h"
#include "meshers/blocky/voxel_blocky_model_cube.h"
#include "meshers/blocky/voxel_blocky_model_empty.h"
#include "meshers/blocky/voxel_blocky_model_fluid.h"
#include "meshers/blocky/voxel_blocky_model_mesh.h"
#include "meshers/blocky/voxel_mesher_blocky.h"
#include "meshers/cubes/voxel_mesher_cubes.h"
#include "storage/metadata/voxel_metadata_factory.h"
#include "storage/metadata/voxel_metadata_variant.h"
#include "storage/voxel_buffer_gd.h"
#include "storage/voxel_format_gd.h"
#include "storage/voxel_memory_pool.h"
#include "streams/region/voxel_stream_region_files.h"
#include "streams/voxel_block_serializer_gd.h"
#include "streams/voxel_stream_memory.h"
#include "streams/voxel_stream_script.h"
#include "terrain/fixed_lod/voxel_box_mover.h"
#include "terrain/fixed_lod/voxel_terrain.h"
#include "terrain/fixed_lod/voxel_terrain_multiplayer_synchronizer.h"
#include "terrain/variable_lod/voxel_lod_terrain.h"
#include "terrain/voxel_a_star_grid_3d.h"
#include "terrain/voxel_mesh_block.h"
#include "terrain/voxel_save_completion_tracker.h"
#include "terrain/voxel_viewer.h"
#include "util/godot/check_ref_ownership.h"
#include "util/godot/string_names.h"
#include "util/macros.h"
#include "util/noise/fast_noise_lite/fast_noise_lite.h"
#include "util/noise/fast_noise_lite/fast_noise_lite_gradient.h"
#include "util/noise/spot_noise_gd.h"
#include "util/string/format.h"
#include "util/tasks/async_dependency_tracker.h"
#include "util/tasks/godot/threaded_task_gd.h"

#ifdef VOXEL_ENABLE_SMOOTH_MESHING
#include "meshers/transvoxel/voxel_mesher_transvoxel.h"
#endif

#ifdef VOXEL_ENABLE_MODIFIERS
#include "modifiers/godot/voxel_modifier_gd.h"
#include "modifiers/godot/voxel_modifier_mesh_gd.h"
#include "modifiers/godot/voxel_modifier_sphere_gd.h"
#endif

#ifdef VOXEL_ENABLE_SQLITE
#include "streams/sqlite/voxel_stream_sqlite.h"
#endif

#ifdef VOXEL_ENABLE_INSTANCER
#include "terrain/instancing/voxel_instance_component.h"
#include "terrain/instancing/voxel_instance_library.h"
#include "terrain/instancing/voxel_instance_library_multimesh_item.h"
#include "terrain/instancing/voxel_instance_library_scene_item.h"
#include "terrain/instancing/voxel_instancer.h"
#include "terrain/instancing/voxel_instancer_rigidbody.h"
#endif

#ifdef VOXEL_ENABLE_BASIC_GENERATORS
#include "generators/simple/voxel_generator_flat.h"
#include "generators/simple/voxel_generator_heightmap.h"
#include "generators/simple/voxel_generator_image.h"
#include "generators/simple/voxel_generator_noise.h"
#include "generators/simple/voxel_generator_noise_2d.h"
#include "generators/simple/voxel_generator_waves.h"
#endif


#ifdef VOXEL_ENABLE_FAST_NOISE_2
#include "util/noise/fast_noise_2.h"
#endif

#ifdef VOXEL_ENABLE_MESH_SDF
#include "edition/voxel_mesh_sdf_gd.h"
#endif

#ifdef VOXEL_ENABLE_VOX
#include "streams/vox/vox_loader.h"
#endif

#include "util/godot/classes/engine.h"
#include "util/godot/classes/os.h" // for get_command_line_arguments
#include "util/godot/classes/project_settings.h"
#include "util/godot/core/class_db.h"
// Just for size reminders
#include "util/godot/classes/control.h"
#include "util/godot/classes/mesh_instance_3d.h"
#include "util/godot/classes/sprite_2d.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef TOOLS_ENABLED

#include "editor/blocky_library/voxel_blocky_library_editor_plugin.h"
#include "editor/fast_noise_lite/fast_noise_lite_editor_plugin.h"
#include "editor/graph/graph_nodes_doc_tool.h"
#include "editor/graph/voxel_graph_editor_node_preview.h"
#include "editor/graph/voxel_graph_editor_plugin.h"
#include "editor/instance_library/control_sizer.h"

#include "editor/instancer/voxel_instancer_editor_plugin.h"
#include "editor/multipass/voxel_generator_multipass_editor_plugin.h"
#include "editor/spot_noise/spot_noise_editor_plugin.h"
#include "editor/terrain/voxel_terrain_editor_plugin.h"
#include "editor/vox/vox_editor_plugin.h"
#include "util/godot/classes/os.h"

#ifdef VOXEL_ENABLE_FAST_NOISE_2
#include "editor/fast_noise_2/fast_noise_2_editor_plugin.h"
#endif

#ifdef VOXEL_ENABLE_INSTANCER
#include "editor/instance_library/voxel_instance_library_editor_plugin.h"
#include "editor/instance_library/voxel_instance_library_list_editor.h"
#include "editor/instance_library/voxel_instance_library_multimesh_item_editor_plugin.h"
#endif

#ifdef VOXEL_ENABLE_MESH_SDF
#include "editor/mesh_sdf/voxel_mesh_sdf_editor_plugin.h"
#endif


#endif // TOOLS_ENABLED

#ifdef VOXEL_TESTS
#include "tests/tests.h"
#include "util/testing/test_options.h"
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// This is used to have an idea of the memory footprint of various objects as Godot and Voxel development progresses.
void print_size_reminders() {
	using namespace voxel;
	using namespace voxel;

	// Note, this only logs the base size each of these classes. They can often have a bigger memory
	// footprint due to dynamically-allocated members (arrays, dictionaries, RIDs referring to even more data in
	// RenderingServer...)

	VOXEL_PRINT_VERBOSE(format("Size of Variant: {}", sizeof(Variant)));
	VOXEL_PRINT_VERBOSE(format("Size of Object: {}", sizeof(Object)));
	VOXEL_PRINT_VERBOSE(format("Size of RefCounted: {}", sizeof(RefCounted)));
	VOXEL_PRINT_VERBOSE(format("Size of Node: {}", sizeof(Node)));
	VOXEL_PRINT_VERBOSE(format("Size of Node3D: {}", sizeof(Node3D)));
	VOXEL_PRINT_VERBOSE(format("Size of MeshInstance3D: {}", sizeof(MeshInstance3D)));
	VOXEL_PRINT_VERBOSE(format("Size of GeometryInstance3D: {}", sizeof(MeshInstance3D)));
	VOXEL_PRINT_VERBOSE(format("Size of Resource: {}", sizeof(Resource)));
	VOXEL_PRINT_VERBOSE(format("Size of Mesh: {}", sizeof(Mesh)));
	VOXEL_PRINT_VERBOSE(format("Size of ArrayMesh: {}", sizeof(ArrayMesh)));

	VOXEL_PRINT_VERBOSE(format("Size of CanvasItem: {}", sizeof(CanvasItem)));
	VOXEL_PRINT_VERBOSE(format("Size of Node2D: {}", sizeof(Node2D)));
	VOXEL_PRINT_VERBOSE(format("Size of Sprite2D: {}", sizeof(Sprite2D)));
	VOXEL_PRINT_VERBOSE(format("Size of Control: {}", sizeof(Control)));

	VOXEL_PRINT_VERBOSE(format("Size of RWLock: {}", sizeof(voxel::RWLock)));
	VOXEL_PRINT_VERBOSE(format("Size of Mutex: {}", sizeof(voxel::Mutex)));
	VOXEL_PRINT_VERBOSE(format("Size of BinaryMutex: {}", sizeof(voxel::BinaryMutex)));

	VOXEL_PRINT_VERBOSE(format("Size of godot::VoxelBuffer: {}", sizeof(voxel::godot::VoxelBuffer)));
	VOXEL_PRINT_VERBOSE(format("Size of VoxelBuffer: {}", sizeof(VoxelBuffer)));
	VOXEL_PRINT_VERBOSE(format("Size of VoxelMeshBlock: {}", sizeof(VoxelMeshBlock)));
	VOXEL_PRINT_VERBOSE(format("Size of VoxelTerrain: {}", sizeof(VoxelTerrain)));
	VOXEL_PRINT_VERBOSE(format("Size of VoxelLodTerrain: {}", sizeof(VoxelLodTerrain)));
#ifdef VOXEL_ENABLE_INSTANCER
	VOXEL_PRINT_VERBOSE(format("Size of VoxelInstancer: {}", sizeof(VoxelInstancer)));
#endif
	VOXEL_PRINT_VERBOSE(format("Size of VoxelDataMap: {}", sizeof(VoxelDataMap)));
	VOXEL_PRINT_VERBOSE(format("Size of VoxelData: {}", sizeof(VoxelData)));
	VOXEL_PRINT_VERBOSE(format("Size of VoxelMesher::Output: {}", sizeof(VoxelMesher::Output)));
	VOXEL_PRINT_VERBOSE(format("Size of VoxelEngine::BlockMeshOutput: {}", sizeof(VoxelEngine::BlockMeshOutput)));
#ifdef VOXEL_ENABLE_MODIFIERS
	VOXEL_PRINT_VERBOSE(format("Size of VoxelModifierStack: {}", sizeof(VoxelModifierStack)));
#endif
	VOXEL_PRINT_VERBOSE(format("Size of AsyncDependencyTracker: {}", sizeof(AsyncDependencyTracker)));
}

void initialize_voxel_module(ModuleInitializationLevel p_level) {
	using namespace voxel;
	using namespace voxel::godot;
	using namespace voxel;

	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
#ifdef VOXEL_DEBUG_LOG_FILE_ENABLED
		open_log_file();
#endif

		// TODO Enhancement: can I prevent users from instancing `VoxelEngine`?
		// This class is used as a singleton so it's not really abstract.
		// Should I use `register_abstract_class` anyways?
		ClassDB::register_class<voxel::godot::VoxelEngine>();

		// Misc

		// Should be abstract, but isn't for compatibility with old versions that didn't have separate VoxelBlockyModel
		// classes
		ClassDB::register_class<VoxelBlockyModel>();

		ClassDB::register_class<VoxelBlockyModelCube>();
		ClassDB::register_class<VoxelBlockyModelMesh>();
		ClassDB::register_class<VoxelBlockyModelEmpty>();
		ClassDB::register_class<VoxelBlockyModelFluid>();
		ClassDB::register_class<VoxelBlockyFluid>();
		ClassDB::register_abstract_class<VoxelBlockyLibraryBase>();
		ClassDB::register_class<VoxelBlockyLibrary>();
		ClassDB::register_abstract_class<VoxelBlockyAttribute>();
		ClassDB::register_class<VoxelBlockyAttributeAxis>();
		ClassDB::register_class<VoxelBlockyAttributeDirection>();
		ClassDB::register_class<VoxelBlockyAttributeRotation>();
		ClassDB::register_class<VoxelBlockyAttributeCustom>();
		ClassDB::register_class<VoxelBlockyType>();
		ClassDB::register_class<VoxelBlockyTypeLibrary>();

		ClassDB::register_class<VoxelColorPalette>();
		ClassDB::register_class<VoxelDataBlockEnterInfo>();
		ClassDB::register_class<VoxelSaveCompletionTracker>();
		ClassDB::register_class<pg::VoxelGraphFunction>();

		// Storage
		ClassDB::register_class<voxel::godot::VoxelBuffer>();
		ClassDB::register_class<voxel::godot::VoxelFormat>();

		// Nodes
		ClassDB::register_abstract_class<VoxelNode>();
		ClassDB::register_class<VoxelTerrain>();
		ClassDB::register_class<VoxelLodTerrain>();
		ClassDB::register_class<VoxelViewer>();

		// Streams
		ClassDB::register_abstract_class<VoxelStream>();
		ClassDB::register_class<VoxelStreamRegionFiles>();
		ClassDB::register_class<VoxelStreamScript>();
		ClassDB::register_class<VoxelStreamMemory>();

		// Generators
		ClassDB::register_abstract_class<VoxelGenerator>();
		ClassDB::register_class<VoxelGeneratorGraph>();
		ClassDB::register_class<VoxelGeneratorScript>();
		ClassDB::register_class<VoxelGeneratorMultipassCB>();

		// Utilities
		ClassDB::register_class<VoxelBoxMover>();
		ClassDB::register_class<VoxelRaycastResult>();
		ClassDB::register_abstract_class<VoxelTool>();
		ClassDB::register_abstract_class<VoxelToolTerrain>();
		ClassDB::register_abstract_class<VoxelToolLodTerrain>();
		// I had to bind this one despite it being useless as-is because otherwise Godot lazily initializes its class.
		// And this can happen in a thread, causing crashes due to the concurrent access
		ClassDB::register_abstract_class<VoxelToolBuffer>();
		ClassDB::register_abstract_class<VoxelToolMultipassGenerator>();
		ClassDB::register_class<voxel::godot::VoxelBlockSerializer>();
		ClassDB::register_class<VOXEL_FastNoiseLite>();
		ClassDB::register_class<VOXEL_FastNoiseLiteGradient>();
		ClassDB::register_class<VOXEL_SpotNoise>();
		ClassDB::register_class<VOXEL_ThreadedTask>();
		ClassDB::register_class<VoxelTerrainMultiplayerSynchronizer>();
		ClassDB::register_class<VoxelAStarGrid3D>();

		// Meshers
		ClassDB::register_abstract_class<VoxelMesher>();
		ClassDB::register_class<VoxelMesherBlocky>();
		ClassDB::register_class<VoxelMesherCubes>();

		// See SCsub
#ifdef VOXEL_ENABLE_FAST_NOISE_2
		ClassDB::register_class<FastNoise2>();
#endif

#ifdef VOXEL_ENABLE_SMOOTH_MESHING
		ClassDB::register_class<VoxelMesherTransvoxel>();
#endif

#ifdef VOXEL_ENABLE_MODIFIERS
		ClassDB::register_abstract_class<voxel::godot::VoxelModifier>();
		ClassDB::register_class<voxel::godot::VoxelModifierSphere>();
		ClassDB::register_class<voxel::godot::VoxelModifierMesh>();
#endif

#ifdef VOXEL_ENABLE_SQLITE
		ClassDB::register_class<VoxelStreamSQLite>();
#endif

#ifdef VOXEL_ENABLE_INSTANCER
		ClassDB::register_class<VoxelInstanceLibrary>();
		ClassDB::register_abstract_class<VoxelInstanceLibraryItem>();
		ClassDB::register_class<VoxelInstanceLibraryMultiMeshItem>();
		ClassDB::register_class<VoxelInstanceLibrarySceneItem>();
		ClassDB::register_class<VoxelInstanceGenerator>();

		ClassDB::register_class<VoxelInstancer>();
		ClassDB::register_class<VoxelInstanceComponent>();
		ClassDB::register_abstract_class<VoxelInstancerRigidBody>();
#endif

#ifdef VOXEL_ENABLE_BASIC_GENERATORS
		ClassDB::register_class<VoxelGeneratorFlat>();
		ClassDB::register_abstract_class<VoxelGeneratorHeightmap>();
		ClassDB::register_class<VoxelGeneratorWaves>();
		ClassDB::register_class<VoxelGeneratorImage>();
		ClassDB::register_class<VoxelGeneratorNoise2D>();
		ClassDB::register_class<VoxelGeneratorNoise>();
#endif

#ifdef VOXEL_ENABLE_MESH_SDF
		ClassDB::register_class<VoxelMeshSDF>();
#endif

#ifdef VOXEL_ENABLE_VOX
		ClassDB::register_class<VoxelVoxLoader>();
#endif


		print_size_reminders();

#ifdef VOXEL_GODOT
		if (RenderingDevice::get_singleton() != nullptr) {
			VOXEL_PRINT_VERBOSE(
					format("TextureArray max layers: {}",
						   RenderingDevice::get_singleton()->limit_get(RenderingDevice::LIMIT_MAX_TEXTURE_ARRAY_LAYERS))
			);
		}
#endif

#ifdef VOXEL_GODOT
		// Compatibility with older version
		// ClassDB::add_compatibility_class("VoxelLibrary", "VoxelBlockyLibrary");
		// ClassDB::add_compatibility_class("Voxel", "VoxelBlockyModel");
		ClassDB::add_compatibility_class("VoxelInstanceLibraryItem", "VoxelInstanceLibraryMultiMeshItem");
		// Not possible to add a compat class for this one because the new name is indistinguishable from an old one.
		// However this is an abstract class so it should not be found in resources hopefully
		// ClassDB::add_compatibility_class("VoxelInstanceLibraryItemBase", "VoxelInstanceLibraryItem");
#endif
		// Setup engine after classes are registered.

		voxel::godot::StringNames::create_singleton();
		VoxelMemoryPool::create_singleton();
		VoxelStringNames::create_singleton();
		pg::NodeTypeDB::create_singleton();

		const voxel::godot::VoxelEngine::Config config =
				voxel::godot::VoxelEngine::get_config_from_godot();
#ifdef TOOLS_ENABLED
		CheckRefCountDoesNotChange::set_enabled(config.ownership_checks);
#endif
		voxel::VoxelEngine::create_singleton(config.inner);

		voxel::godot::VoxelEngine::create_singleton();
		voxel::godot::add_singleton("VoxelEngine", voxel::godot::VoxelEngine::get_singleton());

		VoxelMetadataFactory::get_singleton().add_constructor_by_type<voxel::godot::VoxelMetadataVariant>(
				voxel::godot::METADATA_TYPE_VARIANT
		);

#ifdef VOXEL_ENABLE_SMOOTH_MESHING
		VoxelMesherTransvoxel::load_static_resources();
#endif

#ifdef VOXEL_TESTS
		const PackedStringArray command_line_arguments = voxel::godot::get_command_line_arguments();
		const String tests_cmd = "--run_voxel_tests";

		for (int i = 0; i < command_line_arguments.size(); ++i) {
			const String arg = command_line_arguments[i];
			if (arg == tests_cmd) {
				voxel::tests::run_voxel_tests(voxel::testing::TestOptions());
				break;
			}
		}
#endif
	}

#ifdef TOOLS_ENABLED
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		VoxelGraphEditorNodePreview::load_resources();


		EditorPlugins::add_by_type<VoxelGraphEditorPlugin>();
		EditorPlugins::add_by_type<VoxelTerrainEditorPlugin>();
		EditorPlugins::add_by_type<VOXEL_FastNoiseLiteEditorPlugin>();
		EditorPlugins::add_by_type<VOXEL_SpotNoiseEditorPlugin>();
		EditorPlugins::add_by_type<VoxelBlockyLibraryEditorPlugin>();
		EditorPlugins::add_by_type<VoxelGeneratorMultipassEditorPlugin>();

#ifdef VOXEL_ENABLE_MESH_SDF
		EditorPlugins::add_by_type<VoxelMeshSDFEditorPlugin>();
#endif

#ifdef VOXEL_ENABLE_INSTANCER
		EditorPlugins::add_by_type<VoxelInstancerEditorPlugin>();
		EditorPlugins::add_by_type<VoxelInstanceLibraryEditorPlugin>();
		EditorPlugins::add_by_type<VoxelInstanceLibraryMultiMeshItemEditorPlugin>();
#endif

#ifdef VOXEL_ENABLE_FAST_NOISE_2
		EditorPlugins::add_by_type<FastNoise2EditorPlugin>();
#endif

#ifdef VOXEL_ENABLE_VOX
		EditorPlugins::add_by_type<magica::VoxelVoxEditorPlugin>();
#endif

#ifdef TOOLS_ENABLED
		// TODO Any way to define a custom command line argument that closes Godot afterward?

		const PackedStringArray command_line_arguments = voxel::godot::get_command_line_arguments();
		const String doc_tool_cmd = "--voxel_doc_tool";

		for (int i = 0; i < command_line_arguments.size(); ++i) {
			const String arg = command_line_arguments[i];
			if (arg == doc_tool_cmd) {
				if (i + 2 >= command_line_arguments.size()) {
					ERR_PRINT(String("Expected source and destination file paths after {0}").format(varray(arg)));
					break;
				}
				const String src_path = command_line_arguments[i + 1];
				const String dst_path = command_line_arguments[i + 2];
				run_graph_nodes_doc_tool(src_path, dst_path);
				break;
			}
		}
// run_graph_nodes_doc_tool
#endif
	}
#endif // TOOLS_ENABLED
}

void uninitialize_voxel_module(ModuleInitializationLevel p_level) {
	using namespace voxel;
	using namespace voxel;

	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		voxel::godot::remove_singleton("VoxelEngine");

		// At this point, the GDScript module has nullified GDScriptLanguage::singleton!!
		// That means it's impossible to free scripts still referenced by VoxelEngine. And that can happen, because
		// users can write custom generators, which run inside threads, and these threads are hosted in the engine
		// singleton... See https://github.com/Voxel/godot_voxel/issues/189

#ifdef VOXEL_ENABLE_SMOOTH_MESHING
		VoxelMesherTransvoxel::free_static_resources();
#endif
		VoxelStringNames::destroy_singleton();
		pg::NodeTypeDB::destroy_singleton();
		voxel::godot::VoxelEngine::destroy_singleton();
		VoxelEngine::destroy_singleton();

		// Do this last as VoxelEngine might still be holding some refs to voxel blocks
		VoxelMemoryPool::destroy_singleton();

		voxel::godot::StringNames::destroy_singleton();

#ifdef VOXEL_DEBUG_LOG_FILE_ENABLED
		close_log_file();
#endif
	}

#ifdef TOOLS_ENABLED
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		VoxelGraphEditorNodePreview::unload_resources();

		// Plugins are automatically unregistered since https://github.com/godotengine/godot-cpp/pull/1138
	}
#endif // TOOLS_ENABLED
}

