#include "assets/mesh.h"

#include <stdlib.h>

#include "assimp/cimport.h"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

typedef struct {
    float* vertices;
    uint32_t *indices;
} mesh_t;

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

    uint32_t *indices = malloc(3 * mesh[0].mNumFaces * sizeof(uint32_t));
    if (!indices) {
        free(vertices);
        aiReleaseImport(scene);
        return 1;
    }

    uint32_t vertex_count = 0;
    for (unsigned int i = 0; i < mesh[0].mNumVertices; i++) {
        vertices[vertex_count++] = mesh[0].mVertices[i].x;
        vertices[vertex_count++] = mesh[0].mVertices[i].y;
        vertices[vertex_count++] = mesh[0].mVertices[o].z;
    }

    uint32_t index_count = 0;
    for (int i = 0; i < mesh[0].mNumFaces; i++) {
        indices[index_count++] = mesh[0].mFaces[i].mIndices[0];
        indices[index_count++] = mesh[0].mFaces[i].mIndices[1];
        indices[index_count++] = mesh[0].mFaces[i].mIndices[2];
    }

    // cache for later

    free(indices);
    free(vertices);

    aiReleaseImport(scene);
    return 0;
}

int mesh_save(const char *filename, long handle) {
}
