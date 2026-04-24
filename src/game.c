#include "game.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "camera.h"
#include "scene.h"
#include "renderer.h"
#include "input.h"
#include "model.h"
#include "ui.h"

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

#define WALK_SPEED 4.0f
#define RUN_SPEED 9.0f
#define CROUCH_MULTIPLIER 0.4f

#define MAX_DELTA_TIME 0.05f
#define CAMERA_RADIUS 0.35f
#define MAX_COLLISION_STEPS 12

static float randf_range(float minv, float maxv)
{
    return minv + (maxv - minv) * ((float)rand() / (float)RAND_MAX);
}

static void game_load_assets(Game *game);
static void game_build_scene(Game *game);

static void game_handle_light_input(Game *game);
static void game_handle_camera_input(Game *game);
static void game_handle_gameplay_input(Game *game);
static void game_update_camera(Game *game, float delta_time);

static void print_help(void)
{
    printf("\n=== Monkey Zoo - Hasznalati utmutato ===\n");
    printf("W / A / S / D : mozgas\n");
    printf("Eger          : kamera forgatas\n");
    printf("Shift         : futas\n");
    printf("Ctrl          : guggolas\n");
    printf("Space         : ugras\n");
    printf("E             : kapu nyitasa / interakcio\n");
    printf("Q             : banan eldobasa\n");
    printf("+ / -         : fenyero novelese / csokkentese\n");
    printf("F1            : utmutato ki/be\n");
    printf("ESC           : kilepes\n");
    printf("=======================================\n\n");
}

bool game_init(Game *game)
{
    srand((unsigned int)time(NULL));

    game->window = NULL;
    game->gl_context = NULL;

    game->rock_loaded = false;
    game->monkey_loaded = false;
    game->banana_loaded = false;
    game->tree_loaded = false;

    game->running = true;
    game->show_help = false;
    game->light_intensity = 1.0f;

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return false;
    }

    int img_flags = IMG_INIT_PNG | IMG_INIT_JPG;
    if ((IMG_Init(img_flags) & img_flags) != img_flags)
    {
        fprintf(stderr, "IMG_Init warning: %s\n", IMG_GetError());
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    game->window = SDL_CreateWindow(
        "Monkey Zoo",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    if (!game->window)
    {
        fprintf(stderr, "SDL_CreateWindow error: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }

    game->gl_context = SDL_GL_CreateContext(game->window);
    if (!game->gl_context)
    {
        fprintf(stderr, "SDL_GL_CreateContext error: %s\n", SDL_GetError());
        SDL_DestroyWindow(game->window);
        SDL_Quit();
        return false;
    }

    SDL_GL_SetSwapInterval(1);

    int width;
    int height;
    SDL_GetWindowSize(game->window, &width, &height);

    renderer_init(width, height);
    camera_init(&game->camera);
    scene_init(&game->scene);
    input_init(&game->input);

    game_load_assets(game);
    game_build_scene(game);

    return true;
}

void game_shutdown(Game *game)
{
    if (game->banana_loaded)
        model_free(&game->banana_model);

    if (game->monkey_loaded)
        model_free(&game->monkey_model);

    if (game->rock_loaded)
        model_free(&game->rock_model);

    if (game->tree_loaded)
        model_free(&game->tree_model);

    IMG_Quit();

    if (game->gl_context)
        SDL_GL_DeleteContext(game->gl_context);

    if (game->window)
        SDL_DestroyWindow(game->window);

    SDL_Quit();
}

void game_run(Game *game)
{
    uint64_t prev = SDL_GetPerformanceCounter();
    const double freq = (double)SDL_GetPerformanceFrequency();

    while (game->running)
    {
        uint64_t now = SDL_GetPerformanceCounter();
        float delta_time = (float)((double)(now - prev) / freq);
        prev = now;

        if (delta_time > MAX_DELTA_TIME)
            delta_time = MAX_DELTA_TIME;

        input_begin_frame(&game->input);
        input_poll_events(&game->input);

        if (game->input.quit)
            game->running = false;

        if (game->input.resized)
            renderer_resize(game->input.win_w, game->input.win_h);

        if (input_pressed(&game->input, SDL_SCANCODE_ESCAPE))
            game->running = false;

        if (input_pressed(&game->input, SDL_SCANCODE_F1))
        {
            game->show_help = !game->show_help;

            if (game->show_help)
            {
                print_help();
                SDL_SetWindowTitle(game->window, "Monkey Zoo - F1: help aktiv");
            }
            else
            {
                SDL_SetWindowTitle(game->window, "Monkey Zoo");
            }
        }

        game_handle_light_input(game);
        game_handle_camera_input(game);
        game_handle_gameplay_input(game);

        scene_update(&game->scene, delta_time);
        scene_collect_obstacles(&game->scene);

        game_update_camera(game, delta_time);

        renderer_begin_frame(0.78f, 0.88f, 0.98f);

        renderer_draw_sky_gradient(game->light_intensity);

        camera_apply_view(&game->camera);
        renderer_apply_light(game->light_intensity);

        renderer_apply_dynamic_fog(
            game->scene.global_time,
            game->camera.position.x,
            game->camera.position.y,
            game->scene.pond_enabled ? 1 : 0,
            game->scene.pond_x,
            game->scene.pond_y);

        scene_render(&game->scene);

        if (game->show_help)
        {
            int w;
            int h;
            SDL_GetWindowSize(game->window, &w, &h);

            ui_draw_help_overlay(
                w,
                h,
                game->light_intensity,
                scene_get_active_banana_count(&game->scene),
                scene_get_eaten_banana_count(&game->scene));
        }

        renderer_end_frame(game->window);
    }
}

static void generate_trees(Scene *scene, int count)
{
    for (int i = 0; i < count; i++)
    {
        float x;
        float y;
        int tries = 0;

        do
        {
            x = randf_range(-90.0f, 90.0f);
            y = randf_range(-90.0f, 90.0f);
            tries++;
        } while (
            tries < 100 &&
            ((fabsf(x) < 32.0f && fabsf(y) < 32.0f) ||
             (fabsf(x - 40.0f) < 18.0f && fabsf(y - 10.0f) < 18.0f) ||
             (fabsf(x + 45.0f) < 20.0f && fabsf(y + 20.0f) < 20.0f)));

        scene_add_tree(
            scene,
            x,
            y,
            0.0f,
            randf_range(5.0f, 8.0f),
            randf_range(0.0f, 360.0f),
            true);
    }
}

static void generate_border_trees(Scene *scene)
{
    const float min_x = -95.0f;
    const float max_x = 95.0f;
    const float min_y = -95.0f;
    const float max_y = 95.0f;
    const float step = 7.0f;

    for (float x = min_x; x <= max_x; x += step)
    {
        scene_add_tree(scene, x + randf_range(-1.5f, 1.5f), max_y + randf_range(-1.0f, 1.0f), 0.0f, randf_range(5.0f, 7.5f), randf_range(0.0f, 360.0f), true);
        scene_add_tree(scene, x + randf_range(-1.5f, 1.5f), min_y + randf_range(-1.0f, 1.0f), 0.0f, randf_range(5.0f, 7.5f), randf_range(0.0f, 360.0f), true);
    }

    for (float y = min_y + step; y <= max_y - step; y += step)
    {
        scene_add_tree(scene, min_x + randf_range(-1.0f, 1.0f), y + randf_range(-1.5f, 1.5f), 0.0f, randf_range(5.0f, 7.5f), randf_range(0.0f, 360.0f), true);
        scene_add_tree(scene, max_x + randf_range(-1.0f, 1.0f), y + randf_range(-1.5f, 1.5f), 0.0f, randf_range(5.0f, 7.5f), randf_range(0.0f, 360.0f), true);
    }
}

static void game_load_assets(Game *game)
{
    game->rock_loaded = model_load_obj(
        &game->rock_model,
        "assets/rock.obj",
        "assets/rock.png");

    if (game->rock_loaded)
    {
        scene_set_rock_model(&game->scene, &game->rock_model);
    }
    else
    {
        fprintf(stderr, "Rock model not loaded. Falling back to colored boxes.\n");
    }

    game->monkey_loaded = model_load_obj(
        &game->monkey_model,
        "assets/monkey.obj",
        "assets/monkey.png");

    if (game->monkey_loaded)
    {
        scene_set_monkey_model(&game->scene, &game->monkey_model);
    }
    else
    {
        fprintf(stderr, "Monkey model not loaded.\n");
    }

    game->banana_loaded = model_load_obj(
        &game->banana_model,
        "assets/banana.obj",
        "assets/banana.png");

    if (game->banana_loaded)
    {
        scene_set_banana_model(&game->scene, &game->banana_model);
    }
    else
    {
        fprintf(stderr, "Banana model not loaded.\n");
    }

    game->tree_loaded = model_load_obj(
        &game->tree_model,
        "assets/tree.obj",
        "assets/tree.png");

    if (game->tree_loaded)
    {
        scene_set_tree_model(&game->scene, &game->tree_model);
    }
    else
    {
        fprintf(stderr, "Tree model not loaded.\n");
    }
}

static void game_build_scene(Game *game)
{
    Scene *scene = &game->scene;

    if (game->rock_loaded)
    {
        scene_add_rock(scene, 5.0f, 6.0f, 2.0f, 8.0f, 25.0f, true);
        scene_add_rock(scene, -8.0f, 4.0f, 0.5f, 0.7f, -10.0f, true);
        scene_add_rock(scene, 12.0f, -3.0f, 0.7f, 1.1f, 70.0f, true);
        scene_add_rock(scene, -10.0f, 10.0f, 0.6f, 1.0f, 15.0f, true);
        scene_add_rock(scene, 8.0f, 11.0f, 0.5f, 0.9f, 120.0f, true);
        scene_add_rock(scene, 42.0f, 6.0f, 0.4f, 0.8f, 45.0f, true);
        scene_add_rock(scene, 36.0f, 14.0f, 0.5f, 1.0f, -20.0f, true);
        scene_add_rock(scene, -48.0f, -16.0f, 0.4f, 0.8f, 80.0f, true);
        scene_add_rock(scene, -39.0f, -24.0f, 0.5f, 1.1f, 150.0f, true);
    }

    scene_add_fence(scene, 0.0f, 0.0f, 25.0f, 2.0f, true);
    scene_add_fence(scene, 40.0f, 10.0f, 12.0f, 2.0f, true);
    scene_add_fence(scene, -45.0f, -20.0f, 15.0f, 2.0f, true);

    if (game->monkey_loaded)
    {
        scene_add_monkey(scene, 6.0f, 2.0f, 0.0f, 3.5f, 180.0f, 1.8f, true);
        scene_add_monkey(scene, -5.0f, 5.0f, 0.0f, 3.2f, 45.0f, 1.8f, true);

        scene_add_monkey(scene, 10.0f, -8.0f, 0.0f, 3.1f, 230.0f, 1.8f, true);
        scene_add_monkey(scene, -12.0f, -6.0f, 0.0f, 3.4f, 300.0f, 1.8f, true);

        scene_add_monkey(scene, 39.0f, 8.0f, 0.0f, 2.8f, 90.0f, 1.6f, true);
        scene_add_monkey(scene, 44.0f, 13.0f, 0.0f, 2.9f, 210.0f, 1.6f, true);

        scene_add_monkey(scene, -46.0f, -18.0f, 0.0f, 3.0f, 120.0f, 1.7f, true);
        scene_add_monkey(scene, -40.0f, -24.0f, 0.0f, 2.7f, 20.0f, 1.6f, true);
    }

    if (game->tree_loaded)
    {
        generate_border_trees(scene);
        generate_trees(scene, 64);
    }

    scene_add_gate(scene, -1.5f, -25.0f, 0.0f, 3.0f, 0.12f, 2.0f, (Color3){0.40f, 0.30f, 0.15f}, 0.0f, 90.0f);
    scene_add_gate(scene, 38.5f, -2.0f, 0.0f, 3.0f, 0.12f, 2.0f, (Color3){0.40f, 0.30f, 0.15f}, 0.0f, 90.0f);
    scene_add_gate(scene, -46.5f, -35.0f, 0.0f, 3.0f, 0.12f, 2.0f, (Color3){0.40f, 0.30f, 0.15f}, 0.0f, 90.0f);
}

static void game_handle_light_input(Game *game)
{
    InputState *in = &game->input;

    if (input_pressed(in, SDL_SCANCODE_EQUALS) ||
        input_pressed(in, SDL_SCANCODE_KP_PLUS))
    {
        game->light_intensity += 0.1f;

        if (game->light_intensity > 2.5f)
            game->light_intensity = 2.5f;
    }

    if (input_pressed(in, SDL_SCANCODE_MINUS) ||
        input_pressed(in, SDL_SCANCODE_KP_MINUS))
    {
        game->light_intensity -= 0.1f;

        if (game->light_intensity < 0.1f)
            game->light_intensity = 0.1f;
    }
}

static void game_handle_camera_input(Game *game)
{
    InputState *in = &game->input;
    Camera *camera = &game->camera;

    float base_speed = WALK_SPEED;

    if (input_down(in, SDL_SCANCODE_LSHIFT) ||
        input_down(in, SDL_SCANCODE_RSHIFT))
    {
        base_speed = RUN_SPEED;
    }

    if (input_down(in, SDL_SCANCODE_LCTRL))
    {
        base_speed = WALK_SPEED * CROUCH_MULTIPLIER;
    }

    camera->speed_forward = 0.0f;
    camera->speed_side = 0.0f;

    if (input_down(in, SDL_SCANCODE_W))
        camera->speed_forward += base_speed;

    if (input_down(in, SDL_SCANCODE_S))
        camera->speed_forward -= base_speed;

    if (input_down(in, SDL_SCANCODE_A))
        camera->speed_side += base_speed;

    if (input_down(in, SDL_SCANCODE_D))
        camera->speed_side -= base_speed;

    camera->target_eye_height =
        input_down(in, SDL_SCANCODE_LCTRL) ? 1.0f : 1.7f;

    if (input_pressed(in, SDL_SCANCODE_SPACE) && camera->on_ground)
    {
        camera->vz = 7.0f;
        camera->on_ground = 0;
    }

    if (in->mouse_captured)
    {
        const float sensitivity = 0.12f;

        float yaw_delta = -(float)in->mouse_dx * sensitivity;
        float pitch_delta = -(float)in->mouse_dy * sensitivity;

        camera_rotate(camera, yaw_delta, pitch_delta);
    }
}

static void game_throw_banana(Game *game)
{
    Camera *camera = &game->camera;
    Scene *scene = &game->scene;

    float fx;
    float fy;
    camera_get_forward(camera, &fx, &fy);

    float side_x = -fy;
    float side_y = fx;

    float forward_speed = randf_range(8.5f, 11.5f);
    float side_offset = randf_range(-1.4f, 1.4f);
    float up_speed = randf_range(3.2f, 4.6f);

    float throw_vx = fx * forward_speed + side_x * side_offset;
    float throw_vy = fy * forward_speed + side_y * side_offset;
    float throw_vz = up_speed;

    scene_throw_banana(
        scene,
        camera->position.x + fx * 0.8f,
        camera->position.y + fy * 0.8f,
        camera->position.z - 0.15f,
        throw_vx,
        throw_vy,
        throw_vz);
}

static void game_toggle_gate(Game *game)
{
    Camera *camera = &game->camera;
    Scene *scene = &game->scene;

    float fx;
    float fy;
    camera_get_forward(camera, &fx, &fy);

    int gate_index = scene_find_interactable_gate(
        scene,
        camera->position.x,
        camera->position.y,
        fx,
        fy);

    if (gate_index >= 0)
    {
        scene_toggle_gate(scene, gate_index);
    }
}

static void game_handle_gameplay_input(Game *game)
{
    InputState *in = &game->input;

    if (input_pressed(in, SDL_SCANCODE_Q))
    {
        game_throw_banana(game);
    }

    if (input_pressed(in, SDL_SCANCODE_E))
    {
        game_toggle_gate(game);
    }
}

static void game_update_camera(Game *game, float delta_time)
{
    Camera *camera = &game->camera;
    Scene *scene = &game->scene;

    const float max_speed =
        fabsf(camera->speed_forward) + fabsf(camera->speed_side);

    const float max_step = CAMERA_RADIUS * 0.5f;

    int steps = (int)ceilf((max_speed * delta_time) / max_step);

    if (steps < 1)
        steps = 1;

    if (steps > MAX_COLLISION_STEPS)
        steps = MAX_COLLISION_STEPS;

    float dt_step = delta_time / (float)steps;

    for (int i = 0; i < steps; i++)
    {
        camera_update_xy(camera, dt_step);

        scene_resolve_circle_2d(
            scene,
            &camera->position.x,
            &camera->position.y,
            CAMERA_RADIUS);
    }

    camera_update_z(camera, delta_time);
}