import common


def can_build(env, platform):
    return True


def configure(env):
    common.register_scons_options(env)


def get_icons_path():
    return "project/addons/voxel/editor/icons"


def get_doc_classes():
    return [
        "FastNoise2",
        "VoxelAStarGrid3D",
        "VoxelBlockSerializer",
        "VoxelBlockyAttribute",
        "VoxelBlockyAttributeAxis",
        "VoxelBlockyAttributeCustom",
        "VoxelBlockyAttributeDirection",
        "VoxelBlockyAttributeRotation",
        "VoxelBlockyLibrary",
        "VoxelBlockyLibraryBase",
        "VoxelBlockyModel",
        "VoxelBlockyModelCube",
        "VoxelBlockyModelEmpty",
        "VoxelBlockyModelFluid",
        "VoxelBlockyModelMesh",
        "VoxelBlockyFluid",
        "VoxelBlockyType",
        "VoxelBlockyTypeLibrary",
        "VoxelBoxMover",
        "VoxelBuffer",
        "VoxelColorPalette",
        "VoxelDataBlockEnterInfo",
        "VoxelEngine",
        "VoxelFormat",
        "VoxelGenerator",
        "VoxelGeneratorFlat",
        "VoxelGeneratorGraph",
        "VoxelGeneratorHeightmap",
        "VoxelGeneratorImage",
        "VoxelGeneratorMultipassCB",
        "VoxelGeneratorNoise",
        "VoxelGeneratorNoise2D",
        "VoxelGeneratorScript",
        "VoxelGeneratorWaves",
        "VoxelGraphFunction",
        "VoxelInstanceComponent",
        "VoxelInstanceGenerator",
        "VoxelInstanceLibrary",
        "VoxelInstanceLibraryItem",
        "VoxelInstanceLibraryMultiMeshItem",
        "VoxelInstanceLibrarySceneItem",
        "VoxelInstancer",
        "VoxelInstancerRigidBody",
        "VoxelLodTerrain",
        "VoxelMesher",
        "VoxelMesherBlocky",
        "VoxelMesherCubes",
        "VoxelMesherTransvoxel",
        "VoxelMeshSDF",
        "VoxelModifier",
        "VoxelModifierMesh",
        "VoxelModifierSphere",
        "VoxelNode",
        "VoxelRaycastResult",
        "VoxelSaveCompletionTracker",
        "VoxelStream",
        "VoxelStreamMemory",
        "VoxelStreamRegionFiles",
        "VoxelStreamScript",
        "VoxelStreamSQLite",
        "VoxelTerrain",
        "VoxelTerrainMultiplayerSynchronizer",
        "VoxelTool",
        "VoxelToolBuffer",
        "VoxelToolLodTerrain",
        "VoxelToolMultipassGenerator",
        "VoxelToolTerrain",
        "VoxelViewer",
        "VoxelVoxLoader",
        "VOXEL_FastNoiseLite",
        "VOXEL_FastNoiseLiteGradient",
        "VOXEL_SpotNoise",
        "VOXEL_ThreadedTask",
    ]


def get_doc_path():
    return "doc/classes"
