#ifndef MODEL_H
#define MODEL_H

#include <stdbool.h>
#include <GL/gl.h>

#include "geom.h"
#include "texture.h"

typedef struct ModelVertex {
    float x, y, z;
    float nx, ny, nz;
    float u, v;
} ModelVertex;

typedef struct Model {
    ModelVertex* verts;
    int vert_count;

    bool has_normals;
    bool has_uvs;

    AABB local_bounds;
    float radius_xy;

    Texture2D texture;
    Texture2D ao_texture;
} Model;

bool model_load_obj(Model* out_model, const char* obj_path, const char* tex_path);

bool model_load_obj_with_ao(Model* out_model, const char* obj_path, const char* tex_path, const char* ao_path);

void model_free(Model* model);

void model_draw(const Model* model);

#endif //MODEL_H