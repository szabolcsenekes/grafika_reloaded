#ifndef MODEL_H
#define MODEL_H

#include <stdbool.h>
#include <GL/gl.h>

#include "geom.h"
#include "texture.h"

/*
 * One vertex of a 3D model.
 *
 * x, y, z       - vertex position
 * nx, ny, nz    - vertex normal
 * u, v          - texture coordinates
 */
typedef struct ModelVertex
{
    float x, y, z;
    float nx, ny, nz;
    float u, v;
} ModelVertex;

/*
 * Stores the full data of a loaded 3D model.
 *
 * verts         - dynamic array of model vertices
 * vert_count    - number of vertices
 *
 * has_normals   - true if the source model contains normals
 * has_uvs       - true if the source model contains texture coordinates
 *
 * local_bounds  - local-space axis-aligned bounding box
 * radius_xy     - approximate radius in the X-Y plane
 *
 * texture       - main texture of the model
 * ao_texture    - optional ambient occlusion texture
 */
typedef struct Model
{
    ModelVertex *verts;
    int vert_count;

    bool has_normals;
    bool has_uvs;

    AABB local_bounds;
    float radius_xy;

    Texture2D texture;
    Texture2D ao_texture;
} Model;

/*
 * Load an OBJ model and its main texture.
 * Returns true if loading was successful.
 */
bool model_load_obj(Model *out_model, const char *obj_path, const char *tex_path);

/*
 * Load an OBJ model with both a main texture and an ambient occlusion texture.
 * Returns true if loading was successful.
 */
bool model_load_obj_with_ao(Model *out_model, const char *obj_path, const char *tex_path, const char *ao_path);

/*
 * Free all memory and textures used by the model.
 */
void model_free(Model *model);

/*
 * Render the model using OpenGL.
 */
void model_draw(const Model *model);

#endif // MODEL_H