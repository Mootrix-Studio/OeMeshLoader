#define OEMESH_IMPLEMENTATION
#include "oemesh/oemesh.h"
#include <stdio.h>

int main(void) {
    OeMeshModel model;
    oemesh_load_model_from_file(&model, "./example/data/snowman.oemesh");
    oemesh_unload_model(&model);
    return 0;
}
