#ifndef SCENE_H
#define SCENE_H

#include <stdbool.h>
#include "model.h"
#include "geom.h"

struct Model;

/*
 * Maximum number of different scene elements.
 * These limits define the static storage size used by the scene.
 */
#define SCENE_MAX_OBSTACLES 2048
#define SCENE_MAX_BOXES 512
#define SCENE_MAX_FENCES 64
#define SCENE_MAX_ROCKS 256
#define SCENE_MAX_MONKEYS 64
#define SCENE_MAX_BANANAS 128
#define SCENE_MAX_TREES 256
#define SCENE_MAX_GATES 8
#define MAX_WATER_PARTICLES 128
#define WATER_SIZE 64
#define MAX_RAIN_DROPS 800

/*
 * Possible animation/behavior states of a monkey.
 */
typedef enum
{
    MONKEY_IDLE,
    MONKEY_EATING
} MonkeyState;

/*
 * Simple water simulation grid.
 * h = water height field
 * v = water velocity field
 */
typedef struct
{
    float h[WATER_SIZE][WATER_SIZE];
    float v[WATER_SIZE][WATER_SIZE];
} WaterSim;

/*
 * One rock instance placed in the scene.
 */
typedef struct
{
    float x, y, z;
    float scale;
    float yaw_deg;
    bool collidable;
} SceneRock;

/*
 * One monkey instance placed in the scene.
 *
 * eat_radius  - interaction radius for eating bananas
 * active      - whether the monkey is currently active
 * state       - current animation/behavior state
 * anim_time   - accumulated animation time
 * eat_timer   - timer used during eating animation/state
 */
typedef struct
{
    float x, y, z;
    float scale;
    float yaw_deg;
    float eat_radius;
    bool collidable;
    bool active;
    MonkeyState state;
    float anim_time;
    float eat_timer;
} SceneMonkey;

/*
 * Small particle used above the pond for splash/ambient water effects.
 */
typedef struct
{
    float x, y, z;
    float vx, vy, vz;
    float life;
    float max_life;
    bool active;
} WaterParticle;

/*
 * One rain drop in the rain effect system.
 */
typedef struct
{
    float x, y, z;
    float speed;
    float len;
    bool active;
} RainDrop;

/*
 * One banana object in the scene.
 *
 * vx, vy, vz       - linear velocity
 * yaw/pitch/roll   - current orientation
 * ang_vel_*        - angular velocities for spinning motion
 * active           - whether the banana currently exists in the scene
 * on_ground        - whether the banana has landed
 */
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

/*
 * One tree instance placed in the scene.
 */
typedef struct
{
    float x, y, z;
    float scale;
    float yaw_deg;
    bool collidable;
} SceneTree;

/*
 * Simple colored box primitive.
 */
typedef struct
{
    float cx, cy, cz;
    float sx, sy, sz;
    Color3 color;
    bool collidable;
} SceneBox;

/*
 * Fence enclosure definition.
 *
 * cx, cy      - center position
 * half_size   - half side length of the square fence
 * wall_height - fence height
 */
typedef struct
{
    float cx, cy;
    float half_size;
    float wall_height;
    bool collidable;
} SceneFence;

/*
 * One interactive gate attached to a fence.
 *
 * hx, hy, hz          - hinge position
 * w, t, h             - width, thickness and height
 * angle_deg           - current gate rotation
 * target_deg          - target gate rotation
 * closed_deg/open_deg - angles for closed/open states
 * speed_deg_per_s     - opening/closing speed
 */
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

/*
 * Main scene container.
 * Stores all renderable objects, simulation data and interaction state.
 */
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

    WaterParticle water_particles[MAX_WATER_PARTICLES];
    int water_particle_count;

    RainDrop rain_drops[MAX_RAIN_DROPS];
    bool rain_enabled;

    float pond_x;
    float pond_y;
    float pond_z;
    float pond_rx;
    float pond_ry;
    bool pond_enabled;
    float pond_emit_timer;

    WaterSim water;

    int eaten_banana_count;
    float global_time;

    float ground_half_size;

    SceneGate gates[SCENE_MAX_GATES];
    int gate_count;
} Scene;

/*
 * Initialize the whole scene with default values.
 */
void scene_init(Scene *scene);

/*
 * Add a simple colored box object to the scene.
 */
void scene_add_box(Scene *scene, float cx, float cy, float cz, float sx, float sy, float sz, Color3 color, bool collidable);

/*
 * Add a square fence enclosure to the scene.
 */
void scene_add_fence(Scene *scene, float cx, float cy, float half_size, float wall_height, bool collidable);

/*
 * Add an interactive gate to the scene.
 */
void scene_add_gate(Scene *scene, float hinge_x, float hinge_y, float hinge_z, float width, float thickness, float height, Color3 color, float closed_deg, float open_deg);

/*
 * Find the closest gate the player can currently interact with.
 * Returns the gate index, or -1 if none is suitable.
 */
int scene_find_interactable_gate(const Scene *scene, float cam_x, float cam_y, float cam_fx, float cam_fy);

/*
 * Toggle a gate between open and closed target states.
 */
void scene_toggle_gate(Scene *scene, int gate_index);

/*
 * Set the shared rock model used by rock instances.
 */
void scene_set_rock_model(Scene *scene, const struct Model *rock_model);

/*
 * Add one rock instance to the scene.
 */
void scene_add_rock(Scene *scene, float x, float y, float z, float scale, float yaw_deg, bool collidable);

/*
 * Set the shared monkey model used by monkey instances.
 */
void scene_set_monkey_model(Scene *scene, const struct Model *monkey_model);

/*
 * Add one monkey instance to the scene.
 */
void scene_add_monkey(Scene *scene, float x, float y, float z, float scale, float yaw_deg, float eat_radius, bool collidable);

/*
 * Set the shared banana model used by banana instances.
 */
void scene_set_banana_model(Scene *scene, const struct Model *banana_model);

/*
 * Add one banana instance directly to the scene.
 */
void scene_add_banana(Scene *scene, float x, float y, float z, float scale, float yaw_deg, bool collidable);

/*
 * Spawn a thrown banana with the given initial position and velocity.
 */
void scene_throw_banana(Scene *scene, float x, float y, float z, float vx, float vy, float vz);

/*
 * Return the number of currently active bananas.
 */
int scene_get_active_banana_count(const Scene *scene);

/*
 * Return how many bananas have been eaten by monkeys so far.
 */
int scene_get_eaten_banana_count(const Scene *scene);

/*
 * Set the shared tree model used by tree instances.
 */
void scene_set_tree_model(Scene *scene, const Model *tree_model);

/*
 * Add one tree instance to the scene.
 */
void scene_add_tree(Scene *scene, float x, float y, float z, float scale, float yaw_deg, bool collidable);

/*
 * Update scene logic, animations and simulations.
 */
void scene_update(Scene *scene, float delta_time);

/*
 * Rebuild the obstacle list used for collision handling.
 */
void scene_collect_obstacles(Scene *scene);

/*
 * Render the full scene.
 */
void scene_render(const Scene *scene);

/*
 * Test whether a 2D circle collides with any current obstacle.
 */
bool scene_collides_circle_2d(const Scene *scene, float cx, float cy, float r);

/*
 * Resolve collision between a 2D circle and scene obstacles.
 * The circle position may be modified to push it out of obstacles.
 */
bool scene_resolve_circle_2d(const Scene *scene, float *cx, float *cy, float r);

/*
 * Per-frame scene hook.
 * Currently reserved for future use.
 */
void scene_begin_frame(Scene *scene);

/*
 * Draw obstacle bounding boxes for debugging.
 */
void scene_debug_draw_obstacles(const Scene *scene);

#endif