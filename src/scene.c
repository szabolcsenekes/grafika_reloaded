#include "scene.h"
#include "model.h"

#include <GL/gl.h>
#include <stdlib.h>

#define _USE_MATH_DEFINES
#include <math.h>

static float randf_range(float minv, float maxv) {
    return minv + (maxv - minv) * ((float)rand() / (float)RAND_MAX);
}

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

static bool sphere_aabb_hit(float cx, float cy, float cz, float r, const AABB *b)
{
    float px = clampf(cx, b->minx, b->maxx);
    float py = clampf(cy, b->miny, b->maxy);
    float pz = clampf(cz, b->minz, b->maxz);

    float dx = cx - px;
    float dy = cy - py;
    float dz = cz - pz;

    return (dx * dx + dy * dy + dz * dz) <= (r * r);
}

static bool banana_hits_any_obstacle(const Scene *scene, float x, float y, float z, float r)
{
    for (int i = 0; i < scene->obstacle_count; i++)
    {
        if (sphere_aabb_hit(x, y, z, r, &scene->obstacles[i]))
            return true;
    }
    return false;
}

static float deg2radf(float d) {
    return d * M_PI / 180.0f;
}

static bool segment_sphere_hit(
    float x0, float y0, float z0,
    float x1, float y1, float z1,
    float cx, float cy, float cz,
    float r)
{
    float dx = x1 - x0;
    float dy = y1 - y0;
    float dz = z1 - z0;

    float fx = x0 - cx;
    float fy = y0 - cy;
    float fz = z0 - cz;

    float a = dx * dx + dy * dy + dz * dz;
    if (a < 1e-8f)
    {
        float ddx = x0 - cx;
        float ddy = y0 - cy;
        float ddz = z0 - cz;
        return (ddx * ddx + ddy * ddy + ddz * ddz) <= r * r;
    }

    float t = -(fx * dx + fy * dy + fz * dz) / a;
    t = clampf(t, 0.0f, 1.0f);

    float px = x0 + dx * t;
    float py = y0 + dy * t;
    float pz = z0 + dz * t;

    float qx = px - cx;
    float qy = py - cy;
    float qz = pz - cz;

    return (qx * qx + qy * qy + qz * qz) <= r * r;
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

static void draw_box(float cx, float cy, float cz, float sx, float sy, float sz)
{
    float x0 = cx - sx * 0.5f, x1 = cx + sx * 0.5f;
    float y0 = cy - sy * 0.5f, y1 = cy + sy * 0.5f;
    float z0 = cz - sz * 0.5f, z1 = cz + sz * 0.5f;

    glBegin(GL_QUADS);

    // top
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(x0, y0, z1);
    glVertex3f(x1, y0, z1);
    glVertex3f(x1, y1, z1);
    glVertex3f(x0, y1, z1);

    // bottom
    glNormal3f(0.0f, 0.0f, -1.0f);
    glVertex3f(x0, y1, z0);
    glVertex3f(x1, y1, z0);
    glVertex3f(x1, y0, z0);
    glVertex3f(x0, y0, z0);

    // front (+Y)
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(x0, y1, z0);
    glVertex3f(x0, y1, z1);
    glVertex3f(x1, y1, z1);
    glVertex3f(x1, y1, z0);

    // back (-Y)
    glNormal3f(0.0f, -1.0f, 0.0f);
    glVertex3f(x0, y0, z0);
    glVertex3f(x1, y0, z0);
    glVertex3f(x1, y0, z1);
    glVertex3f(x0, y0, z1);

    // left (-X)
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glVertex3f(x0, y0, z0);
    glVertex3f(x0, y0, z1);
    glVertex3f(x0, y1, z1);
    glVertex3f(x0, y1, z0);

    // right (+X)
    glNormal3f(1.0f, 0.0f, 0.0f);
    glVertex3f(x1, y0, z0);
    glVertex3f(x1, y1, z0);
    glVertex3f(x1, y1, z1);
    glVertex3f(x1, y0, z1);

    glEnd();
}

static void draw_ground_patch(float x0, float y0, float x1, float y1, float z, float r, float g, float b)
{
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(x0, y0, z);
    glVertex3f(x1, y0, z);
    glVertex3f(x1, y1, z);
    glVertex3f(x0, y1, z);
    glEnd();
}

static void draw_road_segment(float x0, float y0, float x1, float y1, float z)
{
    draw_ground_patch(
        x0 - 1.5f, y0 - 1.5f,
        x1 + 1.5f, y1 + 1.5f,
        z,
        0.48f, 0.50f, 0.30f);

    draw_ground_patch(
        x0, y0,
        x1, y1,
        z + 0.001f,
        0.58f, 0.48f, 0.30f);
}

static void draw_ground(float half_size, float z)
{
    glEnable(GL_LIGHTING);

    // base ground
    draw_ground_patch(-half_size, -half_size, half_size, half_size, z, 0.34f, 0.50f, 0.24f);

    // bigger patches
    draw_ground_patch(-half_size, -half_size, 0.0f, 0.0f, z + 0.001f, 0.30f, 0.46f, 0.22f);
    draw_ground_patch(0.0f, -half_size, half_size, 0.0f, z + 0.001f, 0.32f, 0.48f, 0.23f);
    draw_ground_patch(-half_size, 0.0f, 0.0f, half_size, z + 0.001f, 0.36f, 0.53f, 0.25f);
    draw_ground_patch(0.0f, 0.0f, half_size, half_size, z + 0.001f, 0.33f, 0.49f, 0.24f);

    // lighter parts around the enclosures
    draw_ground_patch(-30.0f, -30.0f, 30.0f, 30.0f, z + 0.002f, 0.38f, 0.47f, 0.24f);
    draw_ground_patch(25.0f, -5.0f, 55.0f, 25.0f, z + 0.002f, 0.39f, 0.48f, 0.25f);
    draw_ground_patch(-63.0f, -38.0f, -27.0f, -2.0f, z + 0.002f, 0.39f, 0.48f, 0.25f);

    // main road
    draw_road_segment(-3.0f, -95.0f, 3.0f, -25.0f, z + 0.004f);

    // road to right enclosure
    draw_road_segment(0.0f, -31.0f, 44.0f, -25.0f, z + 0.004f);
    draw_road_segment(38.0f, -25.0f, 44.0f, -6.0f, z + 0.0045f);
    draw_road_segment(36.0f, -6.0f, 44.0f, 0.0f, z + 0.005f);

    // road to left enclosure
    draw_road_segment(-46.0f, -36.0f, 0.0f, -30.0f, z + 0.004f);
    draw_road_segment(-49.0f, -38.0f, -43.0f, -32.0f, z + 0.005f);

    // dry patches
    draw_ground_patch(10.0f, 20.0f, 28.0f, 32.0f, z + 0.003f, 0.46f, 0.50f, 0.26f);
    draw_ground_patch(-80.0f, 18.0f, -55.0f, 35.0f, z + 0.003f, 0.44f, 0.49f, 0.25f);
    draw_ground_patch(60.0f, -50.0f, 88.0f, -30.0f, z + 0.003f, 0.45f, 0.50f, 0.26f);
}

static float dist2_xy(float ax, float ay, float bx, float by)
{
    float dx = ax - bx;
    float dy = ay - by;
    return dx * dx + dy * dy;
}

static void draw_banana_mesh(void)
{
    glPushMatrix();
    glScalef(0.45f, 0.12f, 0.12f);
    glColor3f(0.95f, 0.85f, 0.15f);
    draw_box(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.26f, 0.0f, 0.03f);
    glScalef(0.14f, 0.10f, 0.10f);
    glColor3f(0.88f, 0.78f, 0.10f);
    draw_box(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    glPopMatrix();
}

static void draw_monkey_mesh(float t, float eat_amount)
{
    float arm_anim = sinf(t * 4.0f) * 18.0f * (1.0f - eat_amount);
    float body_bob = sinf(t * 3.0f) * 0.03f;
    float head_pitch = eat_amount * 25.0f;

    glPushMatrix();
    glTranslatef(0.0f, 0.0f, body_bob);

    glEnable(GL_LIGHTING);

    glColor3f(0.38f, 0.25f, 0.14f);
    draw_box(0.0f, 0.0f, 0.75f, 0.55f, 0.35f, 0.75f);

    glColor3f(0.50f, 0.35f, 0.22f);
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 1.25f);
    glRotatef(-head_pitch, 1.0f, 0.0f, 0.0f);
    draw_box(0.0f, 0.0f, 0.0f, 0.42f, 0.36f, 0.42f);
    glPopMatrix();

    glColor3f(0.33f, 0.20f, 0.12f);

    glPushMatrix();
    glTranslatef(-0.35f, 0.0f, 0.95f);
    glRotatef(arm_anim, 0.0f, 1.0f, 0.0f);
    draw_box(0.0f, 0.0f, -0.18f, 0.14f, 0.14f, 0.58f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.35f, 0.0f, 0.95f);
    glRotatef(-arm_anim, 0.0f, 1.0f, 0.0f);
    draw_box(0.0f, 0.0f, -0.18f, 0.14f, 0.14f, 0.58f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-0.13f, 0.0f, 0.28f);
    draw_box(0.0f, 0.0f, -0.15f, 0.16f, 0.16f, 0.55f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.13f, 0.0f, 0.28f);
    draw_box(0.0f, 0.0f, -0.15f, 0.16f, 0.16f, 0.55f);
    glPopMatrix();

    glPopMatrix();
}

static void draw_fence_visual(float half_size, float wall_height) {
    glEnable(GL_LIGHTING);

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

void scene_init(Scene *scene)
{
    scene->ground_half_size = 200.0f;
    scene->box_count = 0;
    scene->fence_count = 0;
    scene->obstacle_count = 0;
    scene->gate_count = 0;

    scene->rock_model = NULL;
    scene->rock_count = 0;

    scene->monkey_model = NULL;
    scene->monkey_count = 0;

    scene->banana_model = NULL;
    scene->banana_count = 0;

    scene->tree_model = NULL;
    scene->tree_count = 0;

    scene->global_time = 0.0f;
    scene->eaten_banana_count = 0;
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

void scene_add_gate(Scene *scene, float hinge_x, float hinge_y, float hinge_z,
                    float width, float thickness, float height,
                    Color3 color, float closed_deg, float open_deg)
{
    if (scene->gate_count >= SCENE_MAX_GATES)
        return;

    SceneGate *g = &scene->gates[scene->gate_count++];

    g->exists = true;
    g->hx = hinge_x;
    g->hy = hinge_y;
    g->hz = hinge_z;
    g->w = width;
    g->t = thickness;
    g->h = height;
    g->color = color;

    g->closed_deg = closed_deg;
    g->open_deg = open_deg;

    g->angle_deg = closed_deg;
    g->target_deg = closed_deg;
    g->speed_deg_per_s = 120.0f;
}

int scene_find_interactable_gate(const Scene *scene, float cam_x, float cam_y, float cam_fx, float cam_fy)
{
    int best_index = -1;
    float best_dist2 = 1e30f;

    float fl = sqrtf(cam_fx * cam_fx + cam_fy * cam_fy);
    if (fl > 1e-4f)
    {
        cam_fx /= fl;
        cam_fy /= fl;
    }

    for (int gi = 0; gi < scene->gate_count; gi++)
    {
        const SceneGate *g = &scene->gates[gi];
        if (!g->exists)
            continue;

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
        if (dist2 > MAX_DIST * MAX_DIST)
            continue;

        float len = sqrtf(dist2);
        if (len > 1e-4f)
        {
            dx /= len;
            dy /= len;

            float dot = cam_fx * dx + cam_fy * dy;
            if (dot <= 0.6f)
                continue;
        }

        if (dist2 < best_dist2)
        {
            best_dist2 = dist2;
            best_index = gi;
        }
    }

    return best_index;
}

void scene_set_rock_model(Scene* scene, const Model* rock_model) {
    scene->rock_model = rock_model;
}

void scene_add_rock(Scene* scene, float x, float y, float z, float scale, float yaw_deg, bool collidable) {
    if (scene->rock_count >= SCENE_MAX_ROCKS) return;
    SceneRock* r = &scene->rocks[scene->rock_count++];
    r->x = x; r->y = y; r->z = z;
    r->scale = scale;
    r->yaw_deg = yaw_deg;
    r->collidable = collidable;
}

void scene_toggle_gate(Scene *scene, int gate_index)
{
    if (gate_index < 0 || gate_index >= scene->gate_count)
        return;

    SceneGate *g = &scene->gates[gate_index];

    float deg_to_close = fabsf(g->target_deg - g->closed_deg);
    if (deg_to_close < 0.01f)
        g->target_deg = g->open_deg;
    else
        g->target_deg = g->closed_deg;
}

static void add_gate_obstacles(Scene *scene)
{
    const int SEGMENTS = 6;

    for (int gi = 0; gi < scene->gate_count; gi++)
    {
        SceneGate *g = &scene->gates[gi];
        if (!g->exists)
            continue;

        float half_h = g->h * 0.5f;
        float cz = g->hz + half_h;

        float a = deg2radf(g->angle_deg);
        float ca = cosf(a);
        float sa = sinf(a);

        float seg_len = g->w / (float)SEGMENTS;

        for (int i = 0; i < SEGMENTS; i++)
        {
            float local_x = (i + 0.5f) * seg_len;

            float wx = g->hx + ca * local_x;
            float wy = g->hy + sa * local_x;

            float half_w = seg_len * 0.55f;

            obs_add(scene, (AABB){
                               wx - half_w, wy - half_w, cz - half_h,
                               wx + half_w, wy + half_w, cz + half_h});
        }
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

    if (scene->tree_model)
    {
        for (int i = 0; i < scene->tree_count; i++)
        {
            const SceneTree *t = &scene->trees[i];
            if (!t->collidable)
                continue;

            float trunk_half = 0.22f * t->scale;
            float trunk_h = 2.2f * t->scale;

            obs_add(scene, (AABB){
                               t->x - trunk_half,
                               t->y - trunk_half,
                               t->z,
                               t->x + trunk_half,
                               t->y + trunk_half,
                               t->z + trunk_h});
        }
    }

    if (scene->rock_model)
    {
        const Model *m = scene->rock_model;
        for (int i = 0; i < scene->rock_count; i++)
        {
            const SceneRock *r = &scene->rocks[i];
            if (!r->collidable)
                continue;

            float inset_xy = -0.09f * r->scale;
            float lift_z = 0.10f * r->scale;

            float minx = r->x + m->local_bounds.minx * r->scale + inset_xy;
            float maxx = r->x + m->local_bounds.maxx * r->scale - inset_xy;
            float miny = r->y + m->local_bounds.miny * r->scale + inset_xy;
            float maxy = r->y + m->local_bounds.maxy * r->scale - inset_xy;
            float minz = r->z + m->local_bounds.minz * r->scale + lift_z;
            float maxz = r->z + m->local_bounds.maxz * r->scale;

            obs_add(scene, (AABB){minx, miny, minz, maxx, maxy, maxz});
        }
    }

    if (scene->monkey_model)
    {
        const Model *m = scene->monkey_model;
        for (int i = 0; i < scene->monkey_count; i++)
        {
            const SceneMonkey *mk = &scene->monkeys[i];
            if (!mk->active || !mk->collidable)
                continue;

            float z_lift = -m->local_bounds.minz * mk->scale;
            float base_z = mk->z + z_lift;

            float rad = m->radius_xy * mk->scale;
            float minz = base_z + m->local_bounds.minz * mk->scale;
            float maxz = base_z + m->local_bounds.maxz * mk->scale;

            obs_add(scene, (AABB){mk->x - rad, mk->y - rad, minz, mk->x + rad, mk->y + rad, maxz});
        }
    }

    if (scene->tree_model)
    {
        for (int i = 0; i < scene->tree_count; i++)
        {
            const SceneTree *t = &scene->trees[i];
            if (!t->collidable)
                continue;

            float trunk_half = 0.22f * t->scale;
            float trunk_h = 2.2f * t->scale;

            obs_add(scene, (AABB){
                               t->x - trunk_half,
                               t->y - trunk_half,
                               t->z,
                               t->x + trunk_half,
                               t->y + trunk_half,
                               t->z + trunk_h});
        }
    }

    if (scene->banana_model)
    {
        for (int i = 0; i < scene->banana_count; i++)
        {
            const SceneBanana *b = &scene->bananas[i];
            if (!b->active || !b->collidable)
                continue;

            float rad = 0.16f * b->scale;

            float bottom_offset = 0.60f * b->scale;
            float height = 1.35f * b->scale;

            float minz = b->z - bottom_offset;
            float maxz = minz + height;

            obs_add(scene, (AABB){b->x - rad, b->y - rad, minz, b->x + rad, b->y + rad, maxz});
        }
    }

    add_gate_obstacles(scene);
}

void scene_update(Scene *scene, float delta_time)
{

    scene->global_time += delta_time;
    scene_collect_obstacles(scene);

    for (int gi = 0; gi < scene->gate_count; gi++)
    {
        SceneGate *g = &scene->gates[gi];
        if (!g->exists)
            continue;

        float delta = g->speed_deg_per_s * delta_time;

        if (g->angle_deg < g->target_deg)
        {
            g->angle_deg += delta;
            if (g->angle_deg > g->target_deg)
                g->angle_deg = g->target_deg;
        }
        else if (g->angle_deg > g->target_deg)
        {
            g->angle_deg -= delta;
            if (g->angle_deg < g->target_deg)
                g->angle_deg = g->target_deg;
        }
    }

    for (int i = 0; i < scene->banana_count; i++)
    {
        SceneBanana *b = &scene->bananas[i];
        if (!b->active)
            continue;

        if (!b->on_ground)
        {
            b->vz -= 9.81f * delta_time;

            b->pitch_deg += b->ang_vel_pitch * delta_time;
            b->roll_deg += b->ang_vel_roll * delta_time;

            b->vx *= powf(0.995f, delta_time * 60.0f);
            b->vy *= powf(0.995f, delta_time * 60.0f);

            const float banana_r = 0.14f * b->scale;

            float next_x = b->x + b->vx * delta_time;
            float next_y = b->y + b->vy * delta_time;
            float next_z = b->z + b->vz * delta_time;

            bool eaten = false;

            for (int j = 0; j < scene->monkey_count; j++)
            {
                SceneMonkey *m = &scene->monkeys[j];
                if (!m->active)
                    continue;

                float a = deg2radf(m->yaw_deg);
                float fx = cosf(a);
                float fy = sinf(a);

                float mouth_x = m->x + fx * (0.42f * m->scale);
                float mouth_y = m->y + fy * (0.42f * m->scale);
                float mouth_z = m->z + 1.05f * m->scale;

                float eat_r = m->eat_radius + 0.58f * m->scale;

                if (segment_sphere_hit(
                        b->x, b->y, b->z,
                        next_x, next_y, next_z,
                        mouth_x, mouth_y, mouth_z,
                        eat_r))
                {
                    b->active = false;
                    scene->eaten_banana_count++;
                    eaten = true;
                    break;
                }
            }

            if (eaten)
                continue;

            bool hit_any = banana_hits_any_obstacle(scene, next_x, next_y, next_z, banana_r);

            if (!hit_any)
            {
                b->x = next_x;
                b->y = next_y;
                b->z = next_z;
            }
            else
            {
                float try_x = b->x + b->vx * delta_time;
                if (!banana_hits_any_obstacle(scene, try_x, b->y, b->z, banana_r))
                {
                    b->x = try_x;
                }
                else
                {
                    b->vx *= -0.28f;
                }

                float try_y = b->y + b->vy * delta_time;
                if (!banana_hits_any_obstacle(scene, b->x, try_y, b->z, banana_r))
                {
                    b->y = try_y;
                }
                else
                {
                    b->vy *= -0.28f;
                }

                float try_z = b->z + b->vz * delta_time;
                if (!banana_hits_any_obstacle(scene, b->x, b->y, try_z, banana_r))
                {
                    b->z = try_z;
                }
                else
                {
                    b->vz *= -0.18f;
                }

                b->vx *= 0.72f;
                b->vy *= 0.72f;
                b->vz *= 0.72f;

                b->ang_vel_pitch *= 0.75f;
                b->ang_vel_roll *= 0.75f;
            }

            if (b->z < 0.0f)
            {
                b->z = 0.0f;

                if (fabsf(b->vz) > 1.0f)
                {
                    b->vz = -b->vz * 0.18f;
                    b->vx *= 0.82f;
                    b->vy *= 0.82f;

                    b->ang_vel_pitch *= 0.72f;
                    b->ang_vel_roll *= 0.72f;
                }
                else
                {
                    b->z = 0.0f;
                    b->vz = 0.0f;
                    b->on_ground = true;
                }
            }
        }
        else
        {
            b->z = 0.0f;

            b->vx *= powf(0.08f, delta_time * 60.0f);
            b->vy *= powf(0.08f, delta_time * 60.0f);

            if (fabsf(b->vx) < 0.03f)
                b->vx = 0.0f;
            if (fabsf(b->vy) < 0.03f)
                b->vy = 0.0f;

            b->x += b->vx * delta_time;
            b->y += b->vy * delta_time;

            b->ang_vel_pitch *= 0.85f;
            b->ang_vel_roll *= 0.85f;

            if (fabsf(b->ang_vel_pitch) < 8.0f)
                b->ang_vel_pitch = 0.0f;
            if (fabsf(b->ang_vel_roll) < 8.0f)
                b->ang_vel_roll = 0.0f;

            b->pitch_deg += b->ang_vel_pitch * delta_time;
            b->roll_deg += b->ang_vel_roll * delta_time;
        }

    }
}

void scene_render(const Scene *scene)
{
    draw_ground(scene->ground_half_size, 0.0f);

    for (int i = 0; i < scene->fence_count; i++)
    {
        const SceneFence *f = &scene->fences[i];
        glPushMatrix();
        glTranslatef(f->cx, f->cy, 0.0f);
        draw_fence_visual(f->half_size, f->wall_height);
        glPopMatrix();
    }

    glEnable(GL_LIGHTING);
    for (int i = 0; i < scene->box_count; i++)
    {
        const SceneBox *b = &scene->boxes[i];
        glColor3f(b->color.r, b->color.g, b->color.b);
        draw_box(b->cx, b->cy, b->cz, b->sx, b->sy, b->sz);
    }

    if (scene->rock_model)
    {
        for (int i = 0; i < scene->rock_count; i++)
        {
            const SceneRock *r = &scene->rocks[i];

            glPushMatrix();
            glTranslatef(r->x, r->y, r->z);
            glRotatef(r->yaw_deg, 0.0f, 0.0f, 1.0f);
            glScalef(r->scale, r->scale, r->scale);
            model_draw(scene->rock_model);
            glPopMatrix();
        }
    }

    for (int gi = 0; gi < scene->gate_count; gi++)
    {
        const SceneGate *g = &scene->gates[gi];
        if (!g->exists)
            continue;

        glEnable(GL_LIGHTING);
        glColor3f(g->color.r, g->color.g, g->color.b);

        glPushMatrix();
        glTranslatef(g->hx, g->hy, g->hz);
        glRotatef(g->angle_deg, 0.0f, 0.0f, 1.0f);
        draw_box(g->w * 0.5f, 0.0f, g->h * 0.5f, g->w, g->t, g->h);
        glPopMatrix();
    }

    if (scene->tree_model)
    {
        for (int i = 0; i < scene->tree_count; i++)
        {
            const SceneTree *t = &scene->trees[i];

            float z_lift = -scene->tree_model->local_bounds.minz * t->scale;
            float extra_lift = 0.15f * t->scale;

            glPushMatrix();
            glTranslatef(t->x, t->y, t->z + z_lift + extra_lift);
            glRotatef(t->yaw_deg, 0.0f, 0.0f, 1.0f);
            glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
            glScalef(t->scale, t->scale, t->scale);
            model_draw(scene->tree_model);
            glPopMatrix();
        }
    }

    if (scene->monkey_model)
    {
        for (int i = 0; i < scene->monkey_count; i++)
        {
            const SceneMonkey *m = &scene->monkeys[i];
            if (!m->active)
                continue;

            float z_lift = -scene->monkey_model->local_bounds.minz * m->scale;

            glPushMatrix();
            glTranslatef(m->x, m->y, m->z + z_lift);
            glRotatef(m->yaw_deg, 0.0f, 0.0f, 1.0f);
            glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
            glScalef(m->scale, m->scale, m->scale);
            model_draw(scene->monkey_model);
            glPopMatrix();
        }
    }

    if (scene->banana_model)
    {
        for (int i = 0; i < scene->banana_count; i++)
        {
            const SceneBanana *b = &scene->bananas[i];
            if (!b->active)
                continue;

            float z_lift = -scene->banana_model->local_bounds.minz * b->scale;

            glPushMatrix();
            glTranslatef(b->x, b->y, b->z + z_lift);

            glRotatef(b->yaw_deg, 0.0f, 0.0f, 1.0f);
            glRotatef(b->pitch_deg, 1.0f, 0.0f, 0.0f);
            glRotatef(b->roll_deg, 0.0f, 1.0f, 0.0f);

            glScalef(b->scale, b->scale, b->scale);
            model_draw(scene->banana_model);
            glPopMatrix();
        }
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
            moved = true;
        }

        if (!any) break;
    }

    return moved;
}

void scene_throw_banana(Scene *scene, float x, float y, float z, float vx, float vy, float vz)
{
    for (int i = 0; i < SCENE_MAX_BANANAS; i++)
    {
        SceneBanana *b = &scene->bananas[i];

        if (!b->active)
        {
            b->active = true;
            b->on_ground = false;

            b->x = x;
            b->y = y;
            b->z = z;

            b->vx = vx;
            b->vy = vy;
            b->vz = vz;

            b->scale = 1.0f;

            b->yaw_deg = atan2f(vy, vx) * 180.0f / (float)M_PI + randf_range(-12.0f, 12.0f);
            b->pitch_deg = randf_range(-20.0f, 20.0f);
            b->roll_deg = randf_range(-25.0f, 25.0f);

            b->ang_vel_pitch = randf_range(320.0f, 620.0f);
            b->ang_vel_roll = randf_range(-260.0f, 260.0f);

            b->collidable = false;

            if (i >= scene->banana_count)
                scene->banana_count = i + 1;

            return;
        }
    }
}

int scene_get_active_banana_count(const Scene *scene)
{
    int count = 0;
    for (int i = 0; i < scene->banana_count; i++)
    {
        if (scene->bananas[i].active)
            count++;
    }
    return count;
}

int scene_get_eaten_banana_count(const Scene *scene)
{
    return scene->eaten_banana_count;
}

void scene_set_monkey_model(Scene *scene, const Model *monkey_model)
{
    scene->monkey_model = monkey_model;
}

void scene_add_monkey(Scene *scene, float x, float y, float z, float scale, float yaw_deg, float eat_radius, bool collidable)
{
    if (scene->monkey_count >= SCENE_MAX_MONKEYS)
        return;
    SceneMonkey *m = &scene->monkeys[scene->monkey_count++];
    m->x = x;
    m->y = y;
    m->z = z;
    m->scale = scale;
    m->yaw_deg = yaw_deg;
    m->eat_radius = eat_radius;
    m->collidable = collidable;
    m->active = true;
}

void scene_set_banana_model(Scene *scene, const Model *banana_model)
{
    scene->banana_model = banana_model;
}

void scene_add_banana(Scene *scene, float x, float y, float z, float scale, float yaw_deg, bool collidable)
{
    if (scene->banana_count >= SCENE_MAX_BANANAS)
        return;
    SceneBanana *b = &scene->bananas[scene->banana_count++];
    b->x = x;
    b->y = y;
    b->z = z;
    b->scale = scale;
    b->yaw_deg = yaw_deg;
    b->collidable = collidable;
    b->active = true;
}

void scene_set_tree_model(Scene *scene, const Model *tree_model)
{
    scene->tree_model = tree_model;
}

void scene_add_tree(Scene *scene, float x, float y, float z, float scale, float yaw_deg, bool collidable)
{
    if (scene->tree_count >= SCENE_MAX_TREES)
        return;

    SceneTree *t = &scene->trees[scene->tree_count++];

    t->x = x;
    t->y = y;
    t->z = z;
    t->scale = scale;
    t->yaw_deg = yaw_deg;
    t->collidable = collidable;
}

void scene_begin_frame(Scene *scene)
{
    (void)scene;
}