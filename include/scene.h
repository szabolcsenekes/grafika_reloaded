#ifndef SCENE_H
#define SCENE_H

#include <stdbool.h>

typedef struct {
    float minx, miny, minz;
    float maxx, maxy, maxz;
} AABB;

typedef struct {
    float r, g, b;
} Color3;

typedef struct {
    float cx, cy, cz;
    float sx, sy, sz;
    Color3 color;
    bool collidable;
} SceneBox;

typedef struct {
    float cx, cy;
    float half_size;
    float wall_height;
    bool collidable;
} SceneFence;

typedef struct {
    bool exists;

    float hx, hy, hz; //hinge world position

    float w, t, h; //door size

    //animation
    float angle_deg; //current angle
    float target_deg; //target angle
    float closed_deg;
    float open_deg;
    float speed_deg_per_s;

    Color3 color; //visuals
} SceneGate;

#define SCENE_MAX_OBSTACLES 2048
#define SCENE_MAX_BOXES 512
#define SCENE_MAX_FENCES 64

typedef struct {
    SceneBox boxes[SCENE_MAX_BOXES];
    int box_count;

    SceneFence fences[SCENE_MAX_FENCES];
    int fence_count;

    AABB obstacles[SCENE_MAX_OBSTACLES];
    int obstacle_count;

    float ground_half_size;

    SceneGate gate;
    bool gate_request_to_open;
} Scene;

void scene_init(Scene* scene);

void scene_add_box(Scene* scene, float cx, float cy, float cz, float sx, float sy, float sz, Color3 color, bool collidable);

void scene_add_fence(Scene* scene, float cx, float cy, float half_size, float wall_height, bool collidable);

void scene_add_gate(Scene* scene, float hinge_x, float hinge_y, float hinge_z, float width, float thickness, float height, Color3 color, float closed_deg, float open_deg);

bool scene_gate_can_interact(const Scene* scene, float cam_x, float cam_y, float cam_fx, float cam_fy);

void scene_toggle_gate(Scene* scene);

void scene_update(Scene* scene, float delta_time);

void scene_collect_obstacles(Scene* scene);

void scene_render(const Scene* scene);

bool scene_collides_circle_2d(const Scene* scene, float cx, float cy, float r);

void scene_begin_frame(Scene* scene);

void scene_debug_draw_obstacles(const Scene* scene);

bool scene_resolve_circle_2d(const Scene* scene, float* cx, float* cy, float r);

#endif