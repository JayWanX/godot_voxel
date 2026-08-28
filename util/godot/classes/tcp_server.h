#ifndef VOXEL_GODOT_TCP_SERVER_H
#define VOXEL_GODOT_TCP_SERVER_H

#if defined(VOXEL_GODOT)
#include <core/io/tcp_server.h>
#elif defined(VOXEL_GODOT_EXTENSION)
#include <godot_cpp/classes/tcp_server.hpp>
using namespace godot;
#endif

#endif // VOXEL_GODOT_TCP_SERVER_H
