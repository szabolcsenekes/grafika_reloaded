#ifndef SCENE_H
#define SCENE_H

#include <stdbool.h>
#include "model.h"
#include "geom.h"

struct Model;

#define SCENE_MAX_OBSTACLES 2048
#define SCENE_MAX_BOXES 512
#define SCENE_MAX_FENCES 64
#define SCENE_MAX_ROCKS 256
#define SCENE_MAX_MONKEYS 64
#define SCENE_MAX_BANANAS 128
#define SCENE_MAX_TREES 256
#define SCENE_MAX_GATES 8

typedef struct
{
    float x, y, z;
    float scale;
    float yaw_deg;
    bool collidable;
} SceneRock;

typedef struct
{
    float x, y, z;
    float scale;
    float yaw_deg;
    float eat_radius;
    bool collidable;
    bool active;
} SceneMonkey;

typedef struct
{
    float x, y, z;
    float vx, vy, vz;

    float scale;

    float yaw_deg;
    float pitch_deg;
    float roll_deg;

    float ang_vel_pitch;
    float ang_vel_roll;

    bool collidable;
    bool active;
    bool on_ground;
} SceneBanana;

typedef struct
{
    float x, y, z;
    float scale;
    float yaw_deg;
    bool collidable;
} SceneTree;

typedef struct
{
    float cx, cy, cz;
    float sx, sy, sz;
    Color3 color;
    bool collidable;
} SceneBox;

typedef struct
{
    float cx, cy;
    float half_size;
    float wall_height;
    bool collidable;
} SceneFence;

typedef struct
{
    bool exists;

    float hx, hy, hz;
    float w, t, h;

    float angle_deg;
    float target_deg;
    float closed_deg;
    float open_deg;
    float speed_deg_per_s;

    Color3 color;
} SceneGate;

typedef struct
{
    SceneBox boxes[SCENE_MAX_BOXES];
    int box_count;

    SceneFence fences[SCENE_MAX_FENCES];
    int fence_count;

    AABB obstacles[SCENE_MAX_OBSTACLES];
    int obstacle_count;

    const struct Model *rock_model;
    SceneRock rocks[SCENE_MAX_ROCKS];
    int rock_count;

    const struct Model *monkey_model;
    SceneMonkey monkeys[SCENE_MAX_MONKEYS];
    int monkey_count;

    const struct Model *banana_model;
    SceneBanana bananas[SCENE_MAX_BANANAS];
    int banana_count;

    const struct Model *tree_model;
    SceneTree trees[SCENE_MAX_TREES];
    int tree_count;

    int eaten_banana_count;
    float global_time;

    float ground_half_size;

    SceneGate gates[SCENE_MAX_GATES];
    int gate_count;
} Scene;

void scene_init(Scene *scene);

void scene_add_box(Scene *scene, float cx, float cy, float cz, float sx, float sy, float sz, Color3 color, bool collidable);

void scene_add_fence(Scene *scene, float cx, float cy, float half_size, float wall_height, bool collidable);

void scene_add_gate(Scene *scene, float hinge_x, float hinge_y, float hinge_z, float width, float thickness, float height, Color3 color, float closed_deg, float open_deg);

int scene_find_interactable_gate(const Scene *scene, float cam_x, float cam_y, float cam_fx, float cam_fy);

void scene_toggle_gate(Scene *scene, int gate_index);

void scene_set_rock_model(Scene *scene, const struct Model *rock_model);
void scene_add_rock(Scene *scene, float x, float y, float z, float scale, float yaw_deg, bool collidable);

void scene_set_monkey_model(Scene *scene, const struct Model *monkey_model);
void scene_add_monkey(Scene *scene, float x, float y, float z, float scale, float yaw_deg, float eat_radius, bool collidable);

void scene_set_banana_model(Scene *scene, const struct Model *banana_model);
void scene_add_banana(Scene *scene, float x, float y, float z, float scale, float yaw_deg, bool collidable);

void scene_throw_banana(Scene *scene, float x, float y, float z, float vx, float vy, float vz);
int scene_get_active_banana_count(const Scene *scene);
int scene_get_eaten_banana_count(const Scene *scene);

void scene_set_tree_model(Scene *scene, const Model *tree_model);
void scene_add_tree(Scene *scene, float x, float y, float z, float scale, float yaw_deg, bool collidable);

void scene_update(Scene *scene, float delta_time);
void scene_collect_obstacles(Scene *scene);
void scene_render(const Scene *scene);

bool scene_collides_circle_2d(const Scene *scene, float cx, float cy, float r);
bool scene_resolve_circle_2d(const Scene *scene, float *cx, float *cy, float r);

void scene_begin_frame(Scene *scene);
void scene_debug_draw_obstacles(const Scene *scene);

#endif