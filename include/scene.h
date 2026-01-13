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
} Scene;

void scene_init(Scene* scene);

void scene_add_box(Scene* scene, float cx, float cy, float cz, float sx, float sy, float sz, Color3 color, bool collidable);

void scene_add_fence(Scene* scene, float cx, float cy, float half_size, float wall_height, bool collidable);

void scene_collect_obstacles(Scene* scene);

bool scene_collides_circle_2d(const Scene* scene, float cx, float cy, float r);

void scene_begin_frame(Scene* scene);

#endif