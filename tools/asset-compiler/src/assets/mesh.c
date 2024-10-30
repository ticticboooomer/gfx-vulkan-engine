#include "assets/mesh.h"

#include <stdlib.h>
#include <core/arrays.h>

#include "assimp/cimport.h"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

#define ARRAY_START_LEN 8

typedef struct {
    float *vertices;
    u32 *indices;
} mesh_t;

static u64 meshes_count = 0;
static u64 meshes_capacity = ARRAY_START_LEN;
static mesh_t *meshes = malloc(ARRAY_START_LEN * sizeof(mesh_t));

int mesh_load(const char *filename, long *handle) {
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
    ensure_capacity_overalloc(&meshes, &meshes_capacity, meshes_count, sizeof(mesh_t));

    mesh_t out_mesh = {
        .vertices = vertices,
        .indices = indices
    };

    meshes[meshes_count] = out_mesh;

    aiReleaseImport(scene);
    return 0;
}

int mesh_save(const char *filename, long handle) {
}

void cleanup() {
    for (u64 i = 0; i < meshes_count; i++) {
        free(meshes[i].vertices);
        free(meshes[i].indices);
    }
    free(meshes);
}
