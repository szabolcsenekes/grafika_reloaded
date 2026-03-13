#include "model.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct
{
    float x, y, z;
} V3;
typedef struct
{
    float u, v;
} V2;

static void aabb_init_empty(AABB *b)
{
    b->minx = b->miny = b->minz = 1e30f;
    b->maxx = b->maxy = b->maxz = -1e30f;
}

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

static int fix_index(int idx, int count)
{
    if (idx > 0)
        return idx - 1;
    if (idx < 0)
        return count + idx;
    return -1;
}

static V3 v3_sub(V3 a, V3 b)
{
    V3 r = {a.x - b.x, a.y - b.y, a.z - b.z};
    return r;
}

static V3 v3_cross(V3 a, V3 b)
{
    V3 r = {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x};
    return r;
}

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

bool model_load_obj_with_ao(Model *out, const char *obj_path, const char *tex_path, const char *ao_path)
{

    memset(out, 0, sizeof(*out));

    FILE *f = fopen(obj_path, "rb");
    if (!f)
    {
        printf("OBJ open failed: %s\n", obj_path);
        return false;
    }

    V3 *positions = NULL;
    int pos_count = 0, pos_cap = 0;
    V3 *normals = NULL;
    int nrm_count = 0, nrm_cap = 0;
    V2 *uvs = NULL;
    int uv_count = 0, uv_cap = 0;

    ModelVertex *verts = NULL;
    int vert_count = 0;
    int vert_cap = 0;

    char line[1024];

    while (fgets(line, sizeof(line), f))
    {

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

            if (nrm_count == 0)
            {

                V3 p0 = positions[v0];
                V3 p1 = positions[v1];
                V3 p2 = positions[v2];

                face_n = v3_norm(v3_cross(v3_sub(p1, p0), v3_sub(p2, p0)));
            }

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

    AABB b;
    aabb_init_empty(&b);

    for (int i = 0; i < vert_count; i++)
        aabb_grow(&b, verts[i].x, verts[i].y, verts[i].z);

    float cx = (b.minx + b.maxx) * 0.5f;
    float cy = (b.miny + b.maxy) * 0.5f;
    float cz = (b.minz + b.maxz) * 0.5f;

    for (int i = 0; i < vert_count; i++)
    {
        verts[i].x -= cx;
        verts[i].y -= cy;
        verts[i].z -= cz;
    }

    AABB cb;
    aabb_init_empty(&cb);

    for (int i = 0; i < vert_count; i++)
        aabb_grow(&cb, verts[i].x, verts[i].y, verts[i].z);

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

    if (tex_path)
        texture_load(&out->texture, tex_path);

    if (ao_path)
        texture_load(&out->ao_texture, ao_path);

    return true;
}

bool model_load_obj(Model *out, const char *obj, const char *tex)
{
    return model_load_obj_with_ao(out, obj, tex, NULL);
}

void model_free(Model *m)
{

    if (!m)
        return;

    free(m->verts);
    m->verts = NULL;

    texture_free(&m->texture);
    texture_free(&m->ao_texture);
}

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