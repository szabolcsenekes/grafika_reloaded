#include "scene.h"

#include <GL/gl.h>

#define _USE_MATH_DEFINES
#include <math.h>

static void obs_clear(Scene* scene) {
    scene->obstacle_count = 0;
}

static void obs_add(Scene* scene, AABB b) {
    if (scene->obstacle_count < SCENE_MAX_OBSTACLES) {
        scene->obstacles[scene->obstacle_count++] = b;
    }
}

static float clampf(float v, float a, float b)
{
    return v < a ? a : (v > b ? b : v);
}

static float deg2radf(float d) {
    return d * M_PI / 180.0f;
}

static bool circle_aabb_2d(float cx, float cy, float r, AABB b) {
    float px = (cx < b.minx) ? b.minx : (cx > b.maxx) ? b.maxx : cx;
    float py = (cy < b.miny) ? b.miny : (cy > b.maxy) ? b.maxy : cy;
    float dx = cx - px;
    float dy = cy - py;
    return (dx * dx + dy * dy) <= (r * r);
}

bool scene_collides_circle_2d(const Scene* scene, float cx, float cy, float r) {
    for (int i = 0; i < scene->obstacle_count; i++) {
        if (circle_aabb_2d(cx, cy, r, scene->obstacles[i])) return true;
    }
    return false;
}

static void add_fence_obstacles(Scene* scene, float cx, float cy, float half_size) {
    const float t = 0.25f;
    const float post = 0.35f;
    const float gate_w = 3.0f;

    //+Y
    obs_add(scene, (AABB){
        cx - half_size, cy + half_size - t * 0.5f, 0.0f, cx + half_size, cy + half_size + t * 0.5f, 10.0f
    });

    //+X
    obs_add(scene, (AABB){
        cx + half_size - t * 0.5f, cy - half_size, 0.0f, cx + half_size + t * 0.5f, cy + half_size, 10.0f
    });

    //-X
    obs_add(scene, (AABB){
        cx - half_size - t * 0.5f, cy - half_size, 0.0f, cx - half_size + t * 0.5f, cy + half_size, 10.0f
    });

    //-Y split with gate
    float gap = gate_w + post;
    float total = half_size * 2.0f;
    float side_len = (total - gap) * 0.5f;
    if (side_len < 0.5f) side_len = 0.5f;

    float left_cx = cx - (gap * 0.5f + side_len * 0.5f);
    float right_cx = cx + (gap *0.5f + side_len * 0.5f);

    obs_add(scene, (AABB){
        left_cx - side_len * 0.5f, cy - half_size - t * 0.5f, 0.0f, left_cx + side_len * 0.5f, cy - half_size + t * 0.5f, 10.0f
    });

    obs_add(scene, (AABB){
        right_cx - side_len * 0.5f, cy - half_size - t * 0.5f, 0.0f, right_cx + side_len * 0.5f, cy - half_size + t * 0.5f, 10.0f
    });
}

static void draw_box(float cx, float cy, float cz, float sx, float sy, float sz) {
    float x0 = cx - sx * 0.5f, x1 = cx + sx * 0.5f;
    float y0 = cy - sy * 0.5f, y1 = cy + sy * 0.5f;
    float z0 = cz - sz * 0.5f, z1 = cz + sz * 0.5f;

    glBegin(GL_QUADS);

    //top
    glVertex3f(x0, y0, z1); glVertex3f(x1, y0, z1); glVertex3f(x1, y1, z1); glVertex3f(x0, y1, z1);

    //bottom
    glVertex3f(x0, y0, z0); glVertex3f(x0, y1, z0);
    glVertex3f(x1, y1, z0); glVertex3f(x1, y0, z0);

    //front(y1)
    glVertex3f(x0, y1, z0); glVertex3f(x0, y1, z1); glVertex3f(x1, y1, z1); glVertex3f(x1, y1, z0);

    //back(y0)
    glVertex3f(x0, y0, z0); glVertex3f(x1, y0, z0); glVertex3f(x1, y0, z1); glVertex3f(x0, y0, z1);

    //left(x0)
    glVertex3f(x0, y0, z0); glVertex3f(x0, y0, z1); glVertex3f(x0, y1, z1); glVertex3f(x0, y1, z0);

    //right(x1)
    glVertex3f(x1, y0, z0); glVertex3f(x1, y1, z0);
    glVertex3f(x1, y1, z1); glVertex3f(x1, y0, z1);

    glEnd();
}

static void draw_ground(float half_size, float z) {
    glDisable(GL_LIGHTING);
    glColor3f(0.18f, 0.24f, 0.16f);
    glBegin(GL_QUADS);
    glVertex3f(-half_size, -half_size, z);
    glVertex3f(half_size, -half_size, z);
    glVertex3f(half_size, half_size, z);
    glVertex3f(-half_size, half_size, z);
    glEnd();
}

static void draw_fence_visual(float half_size, float wall_height) {
    glDisable(GL_LIGHTING);

    const float t = 0.25f;
    const float post = 0.35f;
    const float step = 4.0f;
    const float gate_w = 3.0f;

    //walls
    glColor3f(0.35f, 0.25f, 0.12f);

    //+Y
    draw_box(0.0f, half_size, wall_height * 0.5f, half_size * 2.0f, t, wall_height);

    //-Y split with gate
    float gap = gate_w + post;
    float total = half_size * 2.0f;
    float side_len = (total - gap) * 0.5f;
    if (side_len < 0.5f) side_len = 0.5f;

    float left_cx = -(gap * 0.5f + side_len * 0.5f);
    float right_cx = +(gap * 0.5f + side_len * 0.5f);

    draw_box(left_cx, -half_size, wall_height * 0.5f, side_len, t, wall_height);
    draw_box(right_cx, -half_size, wall_height * 0.5f, side_len, t, wall_height);

    //+X
    draw_box(half_size, 0.0f, wall_height * 0.5f, t, half_size * 2.0f, wall_height);

    //-X
    draw_box(-half_size, 0.0f, wall_height * 0.5f, t, half_size * 2.0f, wall_height);

    //posts
    glColor3f(0.25f, 0.18f, 0.08f);
    float gate_clear = gate_w * 0.5f + post * 0.6f;

    for (float x = -half_size; x <= half_size; x += step) {
        draw_box(x, half_size, wall_height * 0.5f, post, post, wall_height);
        if (fabsf(x) > gate_clear) {
            draw_box(x, -half_size, wall_height * 0.5f, post, post, wall_height);
        }
    }

    for (float y = -half_size; y <= half_size; y += step) {
        draw_box(half_size, y, wall_height * 0.5f, post, post, wall_height);
        draw_box(-half_size, y, wall_height * 0.5f, post, post, wall_height);
    }

    //gate posts
    glColor3f(0.40f, 0.30f, 0.15f);
    float gate_y = -half_size - t * 0.6f;
    float gate_x0 = -gate_w * 0.5f;
    float gate_x1 = gate_w * 0.5f;

    draw_box(gate_x0, gate_y, wall_height * 0.5f, post, post, wall_height);
    draw_box(gate_x1, gate_y, wall_height * 0.5f, post, post, wall_height);
}

void scene_init(Scene* scene) {
    scene->ground_half_size = 200.0f;
    scene->box_count = 0;
    scene->fence_count = 0;
    scene->obstacle_count = 0;
    scene->gate.exists = false;
    scene->gate.speed_deg_per_s = 120.0f;
    scene->gate_request_to_open = false;
}

void scene_add_box(Scene* scene, float cx, float cy, float cz, float sx, float sy, float sz, Color3 color, bool collidable) {
    if (scene->box_count >= SCENE_MAX_BOXES) return;

    SceneBox* b = &scene->boxes[scene->box_count++];
    b->cx = cx; b->cy = cy; b->cz = cz;
    b->sx = sx; b->sy = sy; b->sz = sz;
    b->color = color;
    b->collidable = collidable;
}

void scene_add_fence(Scene* scene, float cx, float cy, float half_size, float wall_height, bool collidable) {
    if (scene->fence_count >= SCENE_MAX_FENCES) return;

    SceneFence* f = &scene->fences[scene->fence_count++];
    f->cx = cx; f->cy = cy;
    f->half_size = half_size;
    f->wall_height = wall_height;
    f->collidable = collidable;
}

void scene_add_gate(Scene* scene, float hinge_x, float hinge_y, float hinge_z, float width, float thickness, float height, Color3 color, float closed_deg, float open_deg) {
    scene->gate.exists = true;
    scene->gate.hx = hinge_x;
    scene->gate.hy = hinge_y;
    scene->gate.hz = hinge_z;
    scene->gate.w = width;
    scene->gate.t = thickness;
    scene->gate.h = height;
    scene->gate.color = color;

    scene->gate.closed_deg = closed_deg;
    scene->gate.open_deg = open_deg;

    scene->gate.angle_deg = closed_deg;
    scene->gate.target_deg = closed_deg;
}



bool scene_gate_can_interact(const Scene* scene, float cam_x, float cam_y, float cam_fx, float cam_fy) {
    const SceneGate* g = &scene->gate;
    if(!g->exists) return false;

    float a = deg2radf(g->angle_deg);
    float ax = g->hx;
    float ay = g->hy;
    float bx = g->hx + cosf(a) * g->w;
    float by = g->hy + sinf(a) * g->w;

    float abx = bx - ax;
    float aby = by - ay;
    float apx = cam_x - ax;
    float apy = cam_y - ay;

    float ab2 = abx * abx + aby * aby;
    float t = (ab2 > 1e-8f) ? (apx * abx + apy * aby) / ab2 : 0.0f;
    t = clampf(t, 0.0f, 1.0f);

    float px = ax + abx * t;
    float py = ay + aby * t;

    float dx = px - cam_x;
    float dy = py - cam_y;
    float dist2 = dx * dx + dy * dy;

    const float MAX_DIST = 3.0f;
    if (dist2 > MAX_DIST * MAX_DIST) return false;

    float len = sqrtf(dist2);
    if (len < 1e-4f) return true;

    dx /= len;
    dy /= len;

    float fl = sqrtf(cam_fx * cam_fx + cam_fy * cam_fy);
    if (fl > 1e-4f) {
        cam_fx /= fl;
        cam_fy /= fl;
    }

    float dot = cam_fx * dx + cam_fy * dy;

    return dot > 0.6f;
}

void scene_toggle_gate(Scene* scene) {
    if (!scene->gate.exists) return;

    float deg_to_close = fabsf(scene->gate.target_deg - scene->gate.closed_deg);
    if (deg_to_close < 0.01f) {
        scene->gate.target_deg = scene->gate.open_deg;
    } else {
        scene->gate.target_deg = scene->gate.closed_deg;
    }
}

static void add_gate_obstacles(Scene* scene) {
    SceneGate* g = &scene->gate;
    if (!g->exists) return;

    const int SEGMENTS = 6;
    const float half_h = g->h * 0.5f;
    const float cz = g->hz + half_h;

    float a = deg2radf(g->angle_deg);
    float ca = cosf(a);
    float sa = sinf(a);

    float seg_len = g->w / (float)SEGMENTS;

    for (int i = 0; i < SEGMENTS; i++) {
        float local_x = (i + 0.5f) * seg_len;

        float wx = g->hx + ca * local_x;
        float wy = g->hy + sa * local_x;

        float half_w = seg_len * 0.55f;

        obs_add(scene, (AABB){
            wx - half_w, wy - half_w, cz - half_h, wx + half_w, wy + half_w, cz + half_h
        });
    }
}

void scene_collect_obstacles(Scene* scene) {
    obs_clear(scene);

    //fences
    for (int i = 0; i < scene->fence_count; i++) {
        const SceneFence* f = &scene->fences[i];
        if(f->collidable) {
            add_fence_obstacles(scene, f->cx, f->cy, f->half_size);
        }
    }

    //boxes
    for (int i = 0; i < scene->box_count; i++) {
        const SceneBox* b = &scene->boxes[i];
        if(!b->collidable) continue;

        obs_add(scene, (AABB){
            b->cx - b->sx * 0.5f, b->cy - b->sy * 0.5f, b->cz - b->sz * 0.5f, b->cx + b->sx * 0.5f, b->cy + b->sy * 0.5f, b->cz + b->sz * 0.5f
        });
    }

    //gate collision
    add_gate_obstacles(scene);
}

void scene_update(Scene* scene, float delta_time) {
    SceneGate* g = &scene->gate;

    if (!g->exists) return;

    g->target_deg = scene->gate_request_to_open ? g->open_deg : g->closed_deg;

    float delta = g->speed_deg_per_s * delta_time;

    if (g->angle_deg < g->target_deg) {
        g->angle_deg += delta;
        if (g->angle_deg > g->target_deg) g->angle_deg = g->target_deg;
    } else if(g->angle_deg > g->target_deg) {
        g->angle_deg -= delta;
        if (g->angle_deg < g->target_deg) g->angle_deg = g->target_deg;
    }
}

void scene_render(const Scene* scene) {
    draw_ground(scene->ground_half_size, 0.0f);

    //fences
    for (int i = 0; i < scene->fence_count; i++) {
        const SceneFence* f = &scene->fences[i];
        glPushMatrix();
        glTranslatef(f->cx, f->cy, 0.0f);
        draw_fence_visual(f->half_size, f->wall_height);
        glPopMatrix();
    }

    //boxes
    glDisable(GL_LIGHTING);
    for (int i = 0; i < scene->box_count; i++) {
        const SceneBox* b = &scene->boxes[i];
        glColor3f(b->color.r, b->color.g, b->color.b);
        draw_box(b->cx, b->cy, b->cz, b->sx, b->sy, b->sz);
    }

    //gates
    if (scene->gate.exists) {
        glDisable(GL_LIGHTING);
        glColor3f(scene->gate.color.r, scene->gate.color.g, scene->gate.color.b);

        glPushMatrix();

        glTranslatef(scene->gate.hx, scene->gate.hy, scene->gate.hz);

        glRotatef(scene->gate.angle_deg, 0.0f, 0.0f, 1.0f);

        draw_box(scene->gate.w * 0.5f, 0.0f, scene->gate.h * 0.5f, scene->gate.w, scene->gate.t, scene->gate.h);

        glPopMatrix();
    }
}

void scene_debug_draw_obstacles(const Scene* scene) {
    glDisable(GL_LIGHTING);
    glColor3f(1, 0, 0);

    for (int i = 0; i < scene->obstacle_count; i++) {
        const AABB* b = &scene->obstacles[i];

        float x0 = b->minx, x1 = b->maxx;
        float y0 = b->miny, y1 = b->maxy;
        float z0 = b->minz, z1 = b->maxz;

        glBegin(GL_LINES);

        // bottom rectangle
        glVertex3f(x0, y0, z0);
        glVertex3f(x1, y0, z0);
        glVertex3f(x1, y0, z0);
        glVertex3f(x1, y1, z0);
        glVertex3f(x1, y1, z0);
        glVertex3f(x0, y1, z0);
        glVertex3f(x0, y1, z0);
        glVertex3f(x0, y0, z0);

        // top rectangle
        glVertex3f(x0, y0, z1);
        glVertex3f(x1, y0, z1);
        glVertex3f(x1, y0, z1);
        glVertex3f(x1, y1, z1);
        glVertex3f(x1, y1, z1);
        glVertex3f(x0, y1, z1);
        glVertex3f(x0, y1, z1);
        glVertex3f(x0, y0, z1);

        // vertical edges
        glVertex3f(x0, y0, z0);
        glVertex3f(x0, y0, z1);
        glVertex3f(x1, y0, z0);
        glVertex3f(x1, y0, z1);
        glVertex3f(x1, y1, z0);
        glVertex3f(x1, y1, z1);
        glVertex3f(x0, y1, z0);
        glVertex3f(x0, y1, z1);

        glEnd();
    }
}

bool scene_resolve_circle_2d(const Scene* scene, float* cx, float* cy, float r) {
    bool moved = false;

    for (int iter = 0; iter < 4; iter++) {
        bool any = false;

        for (int i = 0; i < scene->obstacle_count; i++) {
            const AABB* b = &scene->obstacles[i];

            float px = clampf(*cx, b->minx, b->maxx);
            float py = clampf(*cy, b->miny, b->maxy);

            float dx = *cx - px;
            float dy = *cy - py;
            float d2 = dx * dx + dy * dy;

            if (d2 >= r * r) continue;

            if (d2 < 1e-8f) {
                float left = (*cx - b->minx);
                float right = (b->maxx - *cx);
                float down = (*cy - b->miny);
                float up = (b->maxy - *cy);

                float m = left;
                int dir = 0;

                if (right < m) {
                    m = right;
                    dir = 1;
                }
                if (down < m) {
                    m = down;
                    dir = 2;
                }
                if (up < m) {
                    m = up;
                    dir = 3;
                }

                if (dir == 0) *cx = b->minx - r;
                if (dir == 1) *cx = b->maxx + r;
                if (dir == 2) *cx = b->miny - r;
                if (dir == 3) *cx = b->maxy + r;

                any = true;
                moved = true;
                continue;
            }

            float d = sqrtf(d2);
            float push = (r - d) + 0.001f;
            float nx = dx / d;
            float ny = dy / d;

            *cx += nx * push;
            *cy += ny * push;

            any = true;
            moved = false;
        }

        if (!any) break;
    }

    return moved;
}