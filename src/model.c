#include "model.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/*
 * Temporary 3D vector used while parsing OBJ data.
 */
typedef struct
{
    float x, y, z;
} V3;

/*
 * Temporary 2D vector used for texture coordinates.
 */
typedef struct
{
    float u, v;
} V2;

/*
 * Initialize an AABB to an "empty" state,
 * so later points can properly expand it.
 */
static void aabb_init_empty(AABB *b)
{
    b->minx = b->miny = b->minz = 1e30f;
    b->maxx = b->maxy = b->maxz = -1e30f;
}

/*
 * Expand an AABB so that it includes the given point.
 */
static void aabb_grow(AABB *b, float x, float y, float z)
{
    if (x < b->minx)
        b->minx = x;
    if (y < b->miny)
        b->miny = y;
    if (z < b->minz)
        b->minz = z;
    if (x > b->maxx)
        b->maxx = x;
    if (y > b->maxy)
        b->maxy = y;
    if (z > b->maxz)
        b->maxz = z;
}

/*
 * Parse one OBJ face token.
 *
 * Supported formats:
 *   v
 *   v/vt
 *   v//vn
 *   v/vt/vn
 *
 * Returns 1 on success, 0 on failure.
 */
static int parse_face_index(const char *t, int *v, int *vt, int *vn)
{
    *v = *vt = *vn = 0;
    int a = 0, b = 0, c = 0;

    if (sscanf(t, "%d/%d/%d", &a, &b, &c) == 3)
    {
        *v = a;
        *vt = b;
        *vn = c;
        return 1;
    }
    if (sscanf(t, "%d//%d", &a, &c) == 2)
    {
        *v = a;
        *vn = c;
        return 1;
    }
    if (sscanf(t, "%d/%d", &a, &b) == 2)
    {
        *v = a;
        *vt = b;
        return 1;
    }
    if (sscanf(t, "%d", &a) == 1)
    {
        *v = a;
        return 1;
    }

    return 0;
}

/*
 * Convert OBJ indices to zero-based C array indices.
 * OBJ uses 1-based indexing and also supports negative indices.
 */
static int fix_index(int idx, int count)
{
    if (idx > 0)
        return idx - 1;
    if (idx < 0)
        return count + idx;
    return -1;
}

/*
 * Subtract two 3D vectors.
 */
static V3 v3_sub(V3 a, V3 b)
{
    V3 r = {a.x - b.x, a.y - b.y, a.z - b.z};
    return r;
}

/*
 * Compute the cross product of two 3D vectors.
 */
static V3 v3_cross(V3 a, V3 b)
{
    V3 r = {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x};
    return r;
}

/*
 * Normalize a 3D vector.
 * If the vector is too small, return the default up vector.
 */
static V3 v3_norm(V3 a)
{
    float l2 = a.x * a.x + a.y * a.y + a.z * a.z;
    if (l2 < 1e-12f)
    {
        V3 z = {0, 0, 1};
        return z;
    }
    float inv = 1.0f / sqrtf(l2);
    V3 r = {a.x * inv, a.y * inv, a.z * inv};
    return r;
}

/*
 * Load an OBJ model, optionally with a main texture and an AO texture.
 *
 * The loader:
 * - parses vertex positions, normals and UVs
 * - triangulates polygon faces
 * - generates face normals if missing
 * - centers the model around the origin
 * - normalizes it to a consistent size
 * - computes bounds and an approximate XY radius
 */
bool model_load_obj_with_ao(Model *out, const char *obj_path, const char *tex_path, const char *ao_path)
{
    memset(out, 0, sizeof(*out));

    FILE *f = fopen(obj_path, "rb");
    if (!f)
    {
        printf("OBJ open failed: %s\n", obj_path);
        return false;
    }

    /*
     * Temporary arrays for raw OBJ data.
     */
    V3 *positions = NULL;
    int pos_count = 0, pos_cap = 0;

    V3 *normals = NULL;
    int nrm_count = 0, nrm_cap = 0;

    V2 *uvs = NULL;
    int uv_count = 0, uv_cap = 0;

    /*
     * Final expanded triangle vertex array.
     */
    ModelVertex *verts = NULL;
    int vert_count = 0;
    int vert_cap = 0;

    char line[1024];

    while (fgets(line, sizeof(line), f))
    {
        /*
         * Vertex position: "v x y z"
         */
        if (strncmp(line, "v ", 2) == 0)
        {
            V3 p;
            if (sscanf(line + 2, "%f %f %f", &p.x, &p.y, &p.z) == 3)
            {
                if (pos_count >= pos_cap)
                {
                    pos_cap = pos_cap ? pos_cap * 2 : 256;
                    positions = realloc(positions, pos_cap * sizeof(V3));
                }

                positions[pos_count++] = p;
            }
        }
        /*
         * Vertex normal: "vn x y z"
         */
        else if (strncmp(line, "vn ", 3) == 0)
        {
            V3 n;
            if (sscanf(line + 3, "%f %f %f", &n.x, &n.y, &n.z) == 3)
            {
                if (nrm_count >= nrm_cap)
                {
                    nrm_cap = nrm_cap ? nrm_cap * 2 : 256;
                    normals = realloc(normals, nrm_cap * sizeof(V3));
                }

                normals[nrm_count++] = n;
            }
        }
        /*
         * Texture coordinate: "vt u v"
         */
        else if (strncmp(line, "vt ", 3) == 0)
        {
            V2 t;
            if (sscanf(line + 3, "%f %f", &t.u, &t.v) >= 2)
            {
                if (uv_count >= uv_cap)
                {
                    uv_cap = uv_cap ? uv_cap * 2 : 256;
                    uvs = realloc(uvs, uv_cap * sizeof(V2));
                }

                uvs[uv_count++] = t;
            }
        }
        /*
         * Face definition: "f ..."
         * Faces with more than 3 vertices are triangulated as a fan.
         */
        else if (strncmp(line, "f ", 2) == 0)
        {
            char *p = line + 2;

            int fv[32], fvt[32], fvn[32];
            int fcount = 0;

            while (*p && fcount < 32)
            {
                while (*p == ' ' || *p == '\t')
                    p++;
                if (*p == '\0' || *p == '\n')
                    break;

                char tok[128];
                int ti = 0;

                while (*p && *p != ' ' && *p != '\t' && *p != '\n')
                {
                    tok[ti++] = *p++;
                }

                tok[ti] = 0;

                int iv, iuv, in;
                if (!parse_face_index(tok, &iv, &iuv, &in))
                    break;

                fv[fcount] = iv;
                fvt[fcount] = iuv;
                fvn[fcount] = in;

                fcount++;
            }

            if (fcount < 3)
                continue;

            int v0 = fix_index(fv[0], pos_count);
            int v1 = fix_index(fv[1], pos_count);
            int v2 = fix_index(fv[2], pos_count);

            V3 face_n = {0, 0, 1};

            /*
             * If the OBJ has no normals, compute a face normal
             * from the first triangle of the polygon.
             */
            if (nrm_count == 0)
            {
                V3 p0 = positions[v0];
                V3 p1 = positions[v1];
                V3 p2 = positions[v2];

                face_n = v3_norm(v3_cross(v3_sub(p1, p0), v3_sub(p2, p0)));
            }

            /*
             * Triangulate the face as:
             * (0,1,2), (0,2,3), (0,3,4), ...
             */
            for (int i = 1; i + 1 < fcount; i++)
            {
                int idx[3] = {0, i, i + 1};

                for (int k = 0; k < 3; k++)
                {
                    int fi = idx[k];

                    int pv = fix_index(fv[fi], pos_count);
                    int puv = fix_index(fvt[fi], uv_count);
                    int pvn = fix_index(fvn[fi], nrm_count);

                    if (vert_count >= vert_cap)
                    {
                        vert_cap = vert_cap ? vert_cap * 2 : 1024;
                        verts = realloc(verts, vert_cap * sizeof(ModelVertex));
                    }

                    ModelVertex mv;

                    V3 ppos = positions[pv];

                    mv.x = ppos.x;
                    mv.y = ppos.y;
                    mv.z = ppos.z;

                    /*
                     * Use source normal if available,
                     * otherwise use the generated face normal.
                     */
                    if (pvn >= 0 && nrm_count > 0)
                    {
                        V3 nn = normals[pvn];
                        mv.nx = nn.x;
                        mv.ny = nn.y;
                        mv.nz = nn.z;

                        out->has_normals = true;
                    }
                    else
                    {
                        mv.nx = face_n.x;
                        mv.ny = face_n.y;
                        mv.nz = face_n.z;
                    }

                    /*
                     * Use source texture coordinates if available.
                     */
                    if (puv >= 0 && uv_count > 0)
                    {
                        V2 tt = uvs[puv];
                        mv.u = tt.u;
                        mv.v = tt.v;

                        out->has_uvs = true;
                    }
                    else
                    {
                        mv.u = 0;
                        mv.v = 0;
                    }

                    verts[vert_count++] = mv;
                }
            }
        }
    }

    fclose(f);

    free(positions);
    free(normals);
    free(uvs);

    if (vert_count == 0)
        return false;

    /*
     * Compute the original bounding box.
     */
    AABB b;
    aabb_init_empty(&b);

    for (int i = 0; i < vert_count; i++)
        aabb_grow(&b, verts[i].x, verts[i].y, verts[i].z);

    /*
     * Center the model around the origin.
     */
    float cx = (b.minx + b.maxx) * 0.5f;
    float cy = (b.miny + b.maxy) * 0.5f;
    float cz = (b.minz + b.maxz) * 0.5f;

    for (int i = 0; i < vert_count; i++)
    {
        verts[i].x -= cx;
        verts[i].y -= cy;
        verts[i].z -= cz;
    }

    /*
     * Recompute bounds after centering.
     */
    AABB cb;
    aabb_init_empty(&cb);

    for (int i = 0; i < vert_count; i++)
        aabb_grow(&cb, verts[i].x, verts[i].y, verts[i].z);

    /*
     * Normalize the model so its largest dimension becomes 1.0.
     */
    float sx = cb.maxx - cb.minx;
    float sy = cb.maxy - cb.miny;
    float sz = cb.maxz - cb.minz;

    float max_dim = sx;
    if (sy > max_dim)
        max_dim = sy;
    if (sz > max_dim)
        max_dim = sz;

    float inv = 1.0f / max_dim;

    for (int i = 0; i < vert_count; i++)
    {
        verts[i].x *= inv;
        verts[i].y *= inv;
        verts[i].z *= inv;
    }

    cb.minx *= inv;
    cb.maxx *= inv;
    cb.miny *= inv;
    cb.maxy *= inv;
    cb.minz *= inv;
    cb.maxz *= inv;

    /*
     * Compute an approximate radius in the X-Y plane
     * using the 2D corners of the bounding box.
     */
    float max_r2 = 0;

    float corners[4][2] = {
        {cb.minx, cb.miny},
        {cb.minx, cb.maxy},
        {cb.maxx, cb.miny},
        {cb.maxx, cb.maxy}};

    for (int i = 0; i < 4; i++)
    {
        float x = corners[i][0];
        float y = corners[i][1];
        float r2 = x * x + y * y;
        if (r2 > max_r2)
            max_r2 = r2;
    }

    out->radius_xy = sqrtf(max_r2);
    out->local_bounds = cb;

    out->verts = verts;
    out->vert_count = vert_count;

    /*
     * Load textures if file paths were provided.
     */
    if (tex_path)
        texture_load(&out->texture, tex_path);

    if (ao_path)
        texture_load(&out->ao_texture, ao_path);

    return true;
}

/*
 * Load an OBJ model with only the main texture.
 */
bool model_load_obj(Model *out, const char *obj, const char *tex)
{
    return model_load_obj_with_ao(out, obj, tex, NULL);
}

/*
 * Free all dynamic memory and textures belonging to a model.
 */
void model_free(Model *m)
{
    if (!m)
        return;

    free(m->verts);
    m->verts = NULL;

    texture_free(&m->texture);
    texture_free(&m->ao_texture);
}

/*
 * Draw the model using immediate-mode OpenGL.
 * If a valid texture and UVs are available, the model is textured.
 * Otherwise, it is drawn with a flat fallback color.
 */
void model_draw(const Model *m)
{
    if (!m || !m->verts)
        return;

    bool use_tex = m->texture.valid && m->has_uvs;

    glEnable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);

    glColor3f(1.0f, 1.0f, 1.0f);

    if (use_tex)
    {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, m->texture.id);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }
    else
    {
        glDisable(GL_TEXTURE_2D);
        glColor3f(0.7f, 0.7f, 0.7f);
    }

    glBegin(GL_TRIANGLES);
    for (int i = 0; i < m->vert_count; i++)
    {
        const ModelVertex *v = &m->verts[i];
        glNormal3f(v->nx, v->ny, v->nz);
        if (use_tex)
        {
            glTexCoord2f(v->u, v->v);
        }
        glVertex3f(v->x, v->y, v->z);
    }
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}