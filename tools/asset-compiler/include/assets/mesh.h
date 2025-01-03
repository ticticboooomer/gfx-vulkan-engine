#ifndef ASSETS_MESH_H
#define ASSETS_MESH_H
#include <core/defines.h>
#include <core/handle.h>

int mesh_load(const char *filename, handle_t* handle);

void mesh_save(const char *filename, handle_t *handle);

void mesh_cleanup();

#endif
