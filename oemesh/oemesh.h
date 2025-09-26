#ifndef OEMESH_H
#define OEMESH_H

#include <stdint.h>
#include <stdbool.h>

#define OE_MALLOC(size) malloc(size)
#define OE_FREE(ptr) free(ptr)

typedef struct OeTransform {
    float x_translation, y_translation, z_translation;
    float x_rotation, y_rotation, z_rotation, w_rotation;
    float x_scale, y_scale, z_scale, w_scale;
} OeTransform;

typedef struct OeFrame {
} OeFrame;

typedef struct OeAnim {
    const char *name;
    OeFrame *frames;
    unsigned int frames_count;
} OeAnim;

typedef struct OeMesh {
    const char *name;
    int parent;
    OeTransform transform;
} OeMesh;

typedef struct OeMeshModel {
    unsigned int meshes_count;
    OeMesh *meshes;

    OeAnim *animations;
    unsigned animations_count;
} OeMeshModel;

bool oe_mesh_load_from_file(OeMeshModel *model, const char *filename);

bool oe_mesh_load_from_memory(OeMeshModel *model, char *data, size_t size);

#ifdef OEMESH_IMPLEMENTATION

#endif

#endif //OEMESH_H
