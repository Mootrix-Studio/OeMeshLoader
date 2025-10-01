#ifndef OEMESH_H
#define OEMESH_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define OEMESH_MALLOC(size) malloc(size)
#define OEMESH_FREE(ptr) free(ptr)
#define OEMESH_FUNC_DECL static
#define OEMESH_FUNC_IMPL static
#define OEMESH_PRIV static

#define OEMESH_MESH_FACES_COUNT 6

typedef struct OeMeshColor {
    uint8_t r, g, b, a;
} OeMeshColor;

typedef struct OeMeshVec3i {
    int x, y, z;
} OeMeshVec3i;

typedef struct OeMeshVec3 {
    float x, y, z;
} OeMeshVec3;

typedef struct OeMeshQuat {
    float x, y, z, w;
} OeMeshQuat;

typedef struct OeMeshVec2i {
    int x, y;
} OeMeshVec2i;

typedef struct OeMeshTexture {
    int32_t w, h;
    OeMeshColor *pixels;
} OeMeshTexture;

typedef struct OeMeshTransform {
    OeMeshVec3 translation;
    OeMeshQuat rotation;
    OeMeshVec3 scale;
} OeMeshTransform;

typedef struct OeMeshCoordChange {
    bool change;
    int x, y;
} OeMeshCoordChange;

typedef struct OeMeshFrameMesh {
    struct OeMeshFrameMesh *mesh;
    OeMeshTransform transform;
    OeMeshCoordChange coordChange[OEMESH_MESH_FACES_COUNT];
} OeMeshFrameMesh;

typedef struct OeMeshFrame {
    float duration;
    uint32_t meshes_count;
    OeMeshFrameMesh *meshes;
} OeMeshFrame;

typedef struct OeAnim {
    const char *name;
    OeMeshFrame *frames;
    unsigned int frames_count;
} OeAnim;

struct OeMeshMesh;

typedef struct OeMeshMesh {
    struct OeMeshMesh *parent;
    const char *name;
    OeMeshTransform transform;
    OeMeshVec3 origin;
    OeMeshVec3i pixel_size;
    OeMeshVec2i tex_coords[OEMESH_MESH_FACES_COUNT];
} OeMeshMesh;

typedef struct OeMeshModel {
    OeMeshTexture texture;
    unsigned int meshes_count;
    OeMeshMesh *meshes;

    OeAnim *animations;
    unsigned animations_count;
} OeMeshModel;

OEMESH_FUNC_DECL bool oemesh_load_model_from_file(OeMeshModel *model, const char *filename);

OEMESH_FUNC_DECL bool oemesh_load_model_from_memory(OeMeshModel *model, const char *data, size_t size);

OEMESH_FUNC_DECL void oemesh_unload_model(OeMeshModel *model);

#ifdef OEMESH_IMPLEMENTATION

#include <stdio.h>

typedef struct {
    size_t current;
    size_t size;
    const void *data;
} OeMeshPrivBuffer;

OEMESH_PRIV bool oemesh_priv_read(OeMeshPrivBuffer *buffer, void *container, size_t size) {
    if (buffer->current + size < buffer->size) {
        memcpy(container, buffer->data + buffer->current, size);
        buffer->current += size;
        return true;
    }
    return false;
}

OEMESH_PRIV bool oemesh_priv_read_str(OeMeshPrivBuffer *buffer, const char **container) {
    uint32_t str_size;
    if (oemesh_priv_read(buffer, &str_size, sizeof(str_size))) {
        char *result = OEMESH_MALLOC(str_size + 1);
        if (oemesh_priv_read(buffer, result, str_size)) {
            result[str_size] = 0;
            *container = result;
            return true;
        }
    }
    return false;
}

OEMESH_FUNC_IMPL bool oemesh_load_model_from_file(OeMeshModel *model, const char *filename) {
    bool result = false;
    FILE *f = fopen(filename, "rb");
    if (!f) {
        return false;
    }

    // determine file size
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (file_size <= 0) {
        fclose(f);
        return false;
    }

    char *buffer = (char *) OEMESH_MALLOC(file_size);
    if (!buffer) {
        fclose(f);
        return false;
    }

    size_t read_bytes = fread(buffer, 1, file_size, f);
    fclose(f);

    if (read_bytes != (size_t) file_size) {
        OEMESH_FREE(buffer);
        return false;
    }

    result = oemesh_load_model_from_memory(model, buffer, (size_t) file_size);

    OEMESH_FREE(buffer);

    return result;
}

OEMESH_FUNC_IMPL bool oemesh_load_model_from_memory(OeMeshModel *model, const char *data, size_t size) {
    OeMeshPrivBuffer buffer;
    buffer.current = 0;
    buffer.size = size;
    buffer.data = data;

    const char *header;
    if (oemesh_priv_read_str(&buffer, &header) && strcmp(header, "oemesh") == 0) {
        uint8_t version;
        if (!oemesh_priv_read(&buffer, &version, sizeof(version))) {
            return false;
        }

        // texture reading
        if (!oemesh_priv_read(&buffer, &model->texture.w, sizeof(int32_t))) {
            return false;
        }

        if (!oemesh_priv_read(&buffer, &model->texture.h, sizeof(int32_t))) {
            return false;
        }

        const size_t pixels_count = model->texture.w * model->texture.h;
        model->texture.pixels = OEMESH_MALLOC(pixels_count * sizeof(OeMeshTexture));
        if (!oemesh_priv_read(&buffer, model->texture.pixels, pixels_count * sizeof(OeMeshColor))) {
            return false;
        }

        uint32_t meshes_count;
        if (!oemesh_priv_read(&buffer, &meshes_count, sizeof(meshes_count))) {
            return false;
        }

        model->meshes_count = meshes_count;
        model->meshes = OEMESH_MALLOC(meshes_count * sizeof(OeMeshMesh));
        for (uint32_t i = 0; i < meshes_count; i++) {
            int32_t parent;
            if (!oemesh_priv_read(&buffer, &parent, sizeof(parent))) {
                return false;
            }
            if (parent >= 0) {
                model->meshes[i].parent = NULL;
            } else {
                model->meshes[i].parent = &model->meshes[i];
            }

            if (!oemesh_priv_read(&buffer, &model->meshes[i].transform, sizeof(OeMeshTransform))) {
                return false;
            }

            if (!oemesh_priv_read(&buffer, &model->meshes[i].origin, sizeof(OeMeshVec3))) {
                return false;
            }

            if (!oemesh_priv_read_str(&buffer, &model->meshes[i].name)) {
                return false;
            }

            if (!oemesh_priv_read(&buffer, &model->meshes[i].pixel_size, sizeof(OeMeshVec3i))) {
                return false;
            }

            for (int j = 0; j < OEMESH_MESH_FACES_COUNT; ++j) {
                if (!oemesh_priv_read(&buffer, &model->meshes[i].tex_coords[j], sizeof(OeMeshVec2i))) {
                    return false;
                }
            }
        }

        uint32_t animations_count;
        if (!oemesh_priv_read(&buffer, &animations_count, sizeof(animations_count))) {
            return false;
        }
        model->animations_count = animations_count;
        model->animations = OEMESH_MALLOC(animations_count * sizeof(OeAnim));
        for (uint32_t i = 0; i < animations_count; i++) {
            if (!oemesh_priv_read_str(&buffer, &model->animations[i].name)) {
                return false;
            }
            uint32_t frames_count;
            if (!oemesh_priv_read(&buffer, &frames_count, sizeof(frames_count))) {
                return false;
            }
            model->animations[i].frames_count = frames_count;
            model->animations[i].frames = OEMESH_MALLOC(frames_count * sizeof(OeMeshFrame));
            for (uint32_t j = 0; j < frames_count; j++) {

                if (!oemesh_priv_read(&buffer, &model->animations[i].frames[j].duration, sizeof(float))) {
                    return false;
                }

                uint32_t meshes_animated_count;
                if (!oemesh_priv_read(&buffer, &meshes_animated_count, sizeof(uint32_t))) {
                    return false;
                }

                model->animations[i].frames[j].meshes_count = meshes_animated_count;
                model->animations[i].frames[j].meshes = OEMESH_MALLOC(meshes_animated_count * sizeof(OeMeshFrameMesh));

                for (uint32_t k = 0; k < meshes_animated_count; k++) {
                    if (!oemesh_priv_read(&buffer, &model->animations[i].frames[j].meshes[k].transform,
                                          sizeof(OeMeshTransform))) {
                        return false;
                    }

                    for (int l = 0; l < OEMESH_MESH_FACES_COUNT; ++l) {
                        bool has_change;
                        if (!oemesh_priv_read(&buffer, &has_change, sizeof(has_change))) {
                            return false;
                        }

                        model->animations[i].frames[j].meshes[k].coordChange[l].change = has_change;
                        if (has_change) {
                            if (!oemesh_priv_read(
                                &buffer, &model->animations[i].frames[j].meshes[k].coordChange[l].change,
                                sizeof(OeMeshCoordChange))) {
                                return false;
                            }
                        }
                    }
                }
            }
        }
    } else {
        return false;
    }

    return true;
}

OEMESH_FUNC_IMPL void oemesh_unload_model(OeMeshModel *model) {
    OEMESH_FREE(model->animations);
    OEMESH_FREE(model->meshes);
}

#endif

#endif //OEMESH_H
