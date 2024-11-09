#include "assets/mesh.h"

#include <stdlib.h>
#include <stdio.h>
#include <core/arrays.h>
#include <core/bitwise.h>

#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#define ARRAY_START_LEN 8


typedef struct {
    b8 indices;
    b8 tex_coords;
} mesh_features_t;

typedef struct {
    f32 *vertices;
    u32 *indices;
    u64 vertices_size;
    u64 indices_size;
    mesh_features_t *features;
} mesh_t;

u32 _make_mesh_features_bitmask(mesh_features_t *features);

static u64 meshes_count = 0;
static u64 meshes_capacity = ARRAY_START_LEN;
static mesh_t**meshes = malloc(ARRAY_START_LEN * sizeof(mesh_t));

int mesh_load(const char *filename, handle_t *handle) {
    const struct aiScene *scene = aiImportFile(filename, aiProcess_Triangulate);
    if (!scene) {
        aiReleaseImport(scene);
        return 1;
    }

    struct aiMesh *mesh = *scene->mMeshes;

    float *vertices = malloc(3 * mesh[0].mNumVertices * sizeof(float));
    if (!vertices) {
        aiReleaseImport(scene);
        return 1;
    }

    u32 *indices = malloc(3 * mesh[0].mNumFaces * sizeof(u32));
    if (!indices) {
        free(vertices);
        aiReleaseImport(scene);
        return 1;
    }

    u32 vertex_count = 0;
    for (unsigned int i = 0; i < mesh[0].mNumVertices; i++) {
        vertices[vertex_count++] = mesh[0].mVertices[i].x;
        vertices[vertex_count++] = mesh[0].mVertices[i].y;
        vertices[vertex_count++] = mesh[0].mVertices[i].z;
    }

    u32 index_count = 0;
    for (int i = 0; i < mesh[0].mNumFaces; i++) {
        indices[index_count++] = mesh[0].mFaces[i].mIndices[0];
        indices[index_count++] = mesh[0].mFaces[i].mIndices[1];
        indices[index_count++] = mesh[0].mFaces[i].mIndices[2];
    }

    meshes_count++;
    ensure_capacity_overalloc((void **) &meshes, &meshes_capacity, meshes_count, sizeof(mesh_t));

    mesh_features_t mesh_features = {
        .indices = true,
        .tex_coords = false,
    };

    mesh_t* out_mesh = malloc(sizeof(mesh_t));
    out_mesh->features = &mesh_features;
    out_mesh->indices = indices;
    out_mesh->vertices = vertices;
    out_mesh->vertices_size = vertex_count;
    out_mesh->indices_size = index_count;

    meshes[meshes_count] = out_mesh;

    *handle = handle_create(meshes_count - 1);

    aiReleaseImport(scene);
    return 0;
}

void mesh_save(const char *filename, handle_t *handle) {
    FILE *f = fopen(filename, "rw");
    u64 mesh_index = handle->index;
    const mesh_t* mesh = meshes[mesh_index];

    const u32 features_bitmask = _make_mesh_features_bitmask(mesh->features);
    fwrite(&features_bitmask, sizeof(u32), 1, f);

    fwrite(&mesh->vertices_size, sizeof(u64), 1, f);
    fwrite(&mesh->indices_size, sizeof(u64), 1, f);

    fwrite(mesh->vertices, sizeof(f32), mesh->vertices_size, f);
    fwrite(mesh->indices, sizeof(f32), mesh->indices_size, f);

    fflush(f);
    fclose(f);
}

void mesh_cleanup() {
    for (u64 i = 0; i < meshes_count; i++) {
        free(meshes[i].vertices);
        free(meshes[i].indices);
    }
    free(meshes);
}

u32 _make_mesh_features_bitmask(mesh_features_t *features) {
    u32 bitmask = 0;
    if (features->indices) {
        bitmask = bitmask_flags_set_on(bitmask, 0);
    }
    else if (features->tex_coords) {
        bitmask = bitmask_flags_set_on(bitmask, 1);
    }
    return bitmask;
}
