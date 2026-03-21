#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "camera.h"
#include "scene.h"
#include "renderer.h"
#include "input.h"
#include "model.h"
#include "ui.h"

/*
 * Return a random floating-point value in the [minv, maxv] interval.
 */
static float randf_range(float minv, float maxv)
{
    return minv + (maxv - minv) * ((float)rand() / (float)RAND_MAX);
}

/*
 * Generate randomly placed trees inside the scene,
 * while avoiding the main enclosure areas.
 */
static void generate_trees(Scene *scene, int count)
{
    for (int i = 0; i < count; i++)
    {
        float x, y;
        int tries = 0;

        do
        {
            x = randf_range(-90.0f, 90.0f);
            y = randf_range(-90.0f, 90.0f);
            tries++;
        } while (
            tries < 100 &&
            ((fabsf(x - 0.0f) < 32.0f && fabsf(y - 0.0f) < 32.0f) ||
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

/*
 * Generate a dense border of trees around the outer edges of the map.
 * This visually closes the scene and helps limit the playable area.
 */
static void generate_border_trees(Scene *scene)
{
    const float min_x = -95.0f;
    const float max_x = 95.0f;
    const float min_y = -95.0f;
    const float max_y = 95.0f;

    const float step = 7.0f;

    for (float x = min_x; x <= max_x; x += step)
    {
        scene_add_tree(
            scene,
            x + randf_range(-1.5f, 1.5f),
            max_y + randf_range(-1.0f, 1.0f),
            0.0f,
            randf_range(5.0f, 7.5f),
            randf_range(0.0f, 360.0f),
            true);

        scene_add_tree(
            scene,
            x + randf_range(-1.5f, 1.5f),
            min_y + randf_range(-1.0f, 1.0f),
            0.0f,
            randf_range(5.0f, 7.5f),
            randf_range(0.0f, 360.0f),
            true);
    }

    for (float y = min_y + step; y <= max_y - step; y += step)
    {
        scene_add_tree(
            scene,
            min_x + randf_range(-1.0f, 1.0f),
            y + randf_range(-1.5f, 1.5f),
            0.0f,
            randf_range(5.0f, 7.5f),
            randf_range(0.0f, 360.0f),
            true);

        scene_add_tree(
            scene,
            max_x + randf_range(-1.0f, 1.0f),
            y + randf_range(-1.5f, 1.5f),
            0.0f,
            randf_range(5.0f, 7.5f),
            randf_range(0.0f, 360.0f),
            true);
    }
}

/*
 * Print the control guide to the console.
 */
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

/*
 * Program entry point.
 * Initializes SDL, OpenGL, scene data and runs the main loop.
 */
int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    /*
     * Movement-related gameplay constants.
     */
    const float WALK_SPEED = 4.0f;
    const float RUN_SPEED = 9.0f;
    const float CROUCH_MULT = 0.4f;

    /*
     * Initialize SDL video subsystem.
     */
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    /*
     * Initialize SDL_image with PNG and JPG support.
     */
    int img_flags = IMG_INIT_PNG | IMG_INIT_JPG;
    if ((IMG_Init(img_flags) & img_flags) != img_flags)
    {
        fprintf(stderr, "IMG_Init warning: %s\n", IMG_GetError());
    }

    /*
     * Request an OpenGL 2.1 context with double buffering and depth buffer.
     */
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    /*
     * Create the main application window.
     */
    SDL_Window *window = SDL_CreateWindow(
        "Monkey Zoo - Empty Scene",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1280,
        720,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    if (!window)
    {
        fprintf(stderr, "SDL_CreateWindow error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    /*
     * Create the OpenGL context.
     */
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context)
    {
        fprintf(stderr, "SDL_GL_CreateContext error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    /* Enable vertical sync */
    SDL_GL_SetSwapInterval(1);

    /*
     * Initialize renderer with the current window size.
     */
    int width;
    int height;
    SDL_GetWindowSize(window, &width, &height);
    renderer_init(width, height);

    /*
     * Create core game objects.
     */
    Camera camera;
    camera_init(&camera);

    Scene scene;
    scene_init(&scene);

    InputState in;
    input_init(&in);

    float light_intensity = 1.0f;
    bool show_help = false;

    srand((unsigned int)time(NULL));

    /*
     * Load rock model and place rock instances.
     */
    Model rock_model;
    bool rock_loaded = model_load_obj(&rock_model, "assets/rock.obj", "assets/rock.png");
    if (rock_loaded)
    {
        scene_set_rock_model(&scene, &rock_model);

        scene_add_rock(&scene, 5.0f, 6.0f, 2.0f, 8.0f, 25.0f, true);
        scene_add_rock(&scene, -8.0f, 4.0f, 0.5f, 0.7f, -10.0f, true);
        scene_add_rock(&scene, 12.0f, -3.0f, 0.7f, 1.1f, 70.0f, true);
        scene_add_rock(&scene, -10.0f, 10.0f, 0.6f, 1.0f, 15.0f, true);
        scene_add_rock(&scene, 8.0f, 11.0f, 0.5f, 0.9f, 120.0f, true);
        scene_add_rock(&scene, 42.0f, 6.0f, 0.4f, 0.8f, 45.0f, true);
        scene_add_rock(&scene, 36.0f, 14.0f, 0.5f, 1.0f, -20.0f, true);
        scene_add_rock(&scene, -48.0f, -16.0f, 0.4f, 0.8f, 80.0f, true);
        scene_add_rock(&scene, -39.0f, -24.0f, 0.5f, 1.1f, 150.0f, true);
    }
    else
    {
        fprintf(stderr, "Rock model not loaded. Falling back to colored boxes.\n");
    }

    /*
     * Create the main monkey enclosures.
     */
    scene_add_fence(&scene, 0.0f, 0.0f, 25.0f, 2.0f, true);
    scene_add_fence(&scene, 40.0f, 10.0f, 12.0f, 2.0f, true);
    scene_add_fence(&scene, -45.0f, -20.0f, 15.0f, 2.0f, true);

    /*
     * Load monkey model and place monkeys into the enclosures.
     */
    Model monkey_model;
    bool monkey_loaded = model_load_obj(&monkey_model, "assets/monkey.obj", "assets/monkey.png");
    if (monkey_loaded)
    {
        scene_set_monkey_model(&scene, &monkey_model);

        scene_add_monkey(&scene, 6.0f, 2.0f, 0.0f, 3.5f, 180.0f, 1.8f, true);
        scene_add_monkey(&scene, -5.0f, 5.0f, 0.0f, 3.2f, 45.0f, 1.8f, true);

        scene_add_monkey(&scene, 10.0f, -8.0f, 0.0f, 3.1f, 230.0f, 1.8f, true);
        scene_add_monkey(&scene, -12.0f, -6.0f, 0.0f, 3.4f, 300.0f, 1.8f, true);

        scene_add_monkey(&scene, 39.0f, 8.0f, 0.0f, 2.8f, 90.0f, 1.6f, true);
        scene_add_monkey(&scene, 44.0f, 13.0f, 0.0f, 2.9f, 210.0f, 1.6f, true);

        scene_add_monkey(&scene, -46.0f, -18.0f, 0.0f, 3.0f, 120.0f, 1.7f, true);
        scene_add_monkey(&scene, -40.0f, -24.0f, 0.0f, 2.7f, 20.0f, 1.6f, true);
    }
    else
    {
        fprintf(stderr, "Monkey model not loaded.\n");
    }

    /*
     * Load banana model used for thrown bananas.
     */
    Model banana_model;
    bool banana_loaded = model_load_obj(&banana_model, "assets/banana.obj", "assets/banana.png");
    if (banana_loaded)
    {
        scene_set_banana_model(&scene, &banana_model);
    }
    else
    {
        fprintf(stderr, "Banana model not loaded.\n");
    }

    /*
     * Load tree model and generate environmental trees.
     */
    Model tree_model;
    bool tree_loaded = model_load_obj(&tree_model, "assets/tree.obj", "assets/tree.png");
    if (tree_loaded)
    {
        scene_set_tree_model(&scene, &tree_model);

        generate_border_trees(&scene);
    }
    else
    {
        fprintf(stderr, "Tree model not loaded.\n");
    }

    if (tree_loaded)
    {
        generate_trees(&scene, 64);
    }

    /*
     * Add gates to each enclosure.
     */
    scene_add_gate(&scene, -1.5f, -25.0f, 0.0f, 3.0f, 0.12f, 2.0f, (Color3){0.40f, 0.30f, 0.15f}, 0.0f, 90.0f);
    scene_add_gate(&scene, 38.5f, -2.0f, 0.0f, 3.0f, 0.12f, 2.0f, (Color3){0.40f, 0.30f, 0.15f}, 0.0f, 90.0f);
    scene_add_gate(&scene, -46.5f, -35.0f, 0.0f, 3.0f, 0.12f, 2.0f, (Color3){0.40f, 0.30f, 0.15f}, 0.0f, 90.0f);

    bool running = true;

    /*
     * High-resolution timer setup for delta time computation.
     */
    uint64_t prev = SDL_GetPerformanceCounter();
    const double freq = (double)SDL_GetPerformanceFrequency();

    /*
     * Main application loop.
     */
    while (running)
    {
        /*
         * Compute delta time and clamp it to avoid large simulation jumps.
         */
        uint64_t now = SDL_GetPerformanceCounter();
        float delta_time = (float)((double)(now - prev) / freq);
        prev = now;
        if (delta_time > 0.05f)
            delta_time = 0.05f;

        /*
         * Update input state.
         */
        input_begin_frame(&in);
        input_poll_events(&in);

        if (in.quit)
            running = false;
        if (in.resized)
            renderer_resize(in.win_w, in.win_h);
        if (input_pressed(&in, SDL_SCANCODE_ESCAPE))
            running = false;

        /*
         * Toggle help overlay with F1.
         */
        if (input_pressed(&in, SDL_SCANCODE_F1))
        {
            show_help = !show_help;

            if (show_help)
            {
                print_help();
                SDL_SetWindowTitle(window, "Monkey Zoo - F1: help aktiv | WASD mozgas | E kapu | +/- feny");
            }
            else
            {
                SDL_SetWindowTitle(window, "Monkey Zoo - Empty Scene");
            }
        }

        /*
         * Adjust light intensity with +/- keys.
         */
        if (input_pressed(&in, SDL_SCANCODE_EQUALS) || input_pressed(&in, SDL_SCANCODE_KP_PLUS))
        {
            light_intensity += 0.1f;
            if (light_intensity > 2.5f)
                light_intensity = 2.5f;
        }

        if (input_pressed(&in, SDL_SCANCODE_MINUS) || input_pressed(&in, SDL_SCANCODE_KP_MINUS))
        {
            light_intensity -= 0.1f;
            if (light_intensity < 0.1f)
                light_intensity = 0.1f;
        }

        /*
         * Determine current movement speed based on walk/run/crouch.
         */
        float base_speed = WALK_SPEED;

        if (input_down(&in, SDL_SCANCODE_LSHIFT) || input_down(&in, SDL_SCANCODE_RSHIFT))
            base_speed = RUN_SPEED;
        if (input_down(&in, SDL_SCANCODE_LCTRL))
            base_speed = WALK_SPEED * CROUCH_MULT;

        camera.speed_forward = 0.0f;
        camera.speed_side = 0.0f;

        /*
         * Update camera movement input.
         */
        if (input_down(&in, SDL_SCANCODE_W))
            camera.speed_forward += base_speed;
        if (input_down(&in, SDL_SCANCODE_S))
            camera.speed_forward -= base_speed;
        if (input_down(&in, SDL_SCANCODE_A))
            camera.speed_side += base_speed;
        if (input_down(&in, SDL_SCANCODE_D))
            camera.speed_side -= base_speed;

        /*
         * Smooth crouch height.
         */
        camera.target_eye_height = input_down(&in, SDL_SCANCODE_LCTRL) ? 1.0f : 1.7f;

        /*
         * Jump if the camera is on the ground.
         */
        if (input_pressed(&in, SDL_SCANCODE_SPACE) && camera.on_ground)
        {
            camera.vz = 7.0f;
            camera.on_ground = 0;
        }

        /*
         * Throw a banana in the forward direction with some random variation.
         */
        if (input_pressed(&in, SDL_SCANCODE_Q))
        {
            float fx, fy;
            camera_get_forward(&camera, &fx, &fy);

            float side_x = -fy;
            float side_y = fx;

            float forward_speed = randf_range(8.5f, 11.5f);
            float side_offset = randf_range(-1.4f, 1.4f);
            float up_speed = randf_range(3.2f, 4.6f);

            float throw_vx = fx * forward_speed + side_x * side_offset;
            float throw_vy = fy * forward_speed + side_y * side_offset;
            float throw_vz = up_speed;

            scene_throw_banana(
                &scene,
                camera.position.x + fx * 0.8f,
                camera.position.y + fy * 0.8f,
                camera.position.z - 0.15f,
                throw_vx,
                throw_vy,
                throw_vz);
        }

        /*
         * Rotate camera while mouse capture is active.
         */
        if (in.mouse_captured)
        {
            float sensitivity = 0.12f;
            float yaw_delta = -(float)in.mouse_dx * sensitivity;
            float pitch_delta = -(float)in.mouse_dy * sensitivity;
            camera_rotate(&camera, yaw_delta, pitch_delta);
        }

        /*
         * Interact with the nearest gate in front of the player.
         */
        if (input_pressed(&in, SDL_SCANCODE_E))
        {
            float fx, fy;
            camera_get_forward(&camera, &fx, &fy);

            int gate_index = scene_find_interactable_gate(
                &scene,
                camera.position.x,
                camera.position.y,
                fx,
                fy);

            if (gate_index >= 0)
                scene_toggle_gate(&scene, gate_index);
        }

        /*
         * Update scene logic and rebuild obstacles.
         */
        scene_update(&scene, delta_time);
        scene_collect_obstacles(&scene);

        /*
         * Resolve camera movement in multiple smaller substeps
         * to improve collision stability.
         */
        const float camera_r = 0.35f;

        float max_speed = fabsf(camera.speed_forward) + fabsf(camera.speed_side);

        const float MAX_STEP = camera_r * 0.5f;

        int steps = (int)ceilf((max_speed * delta_time) / MAX_STEP);
        if (steps < 1)
            steps = 1;
        if (steps > 12)
            steps = 12;

        float dt_step = delta_time / (float)steps;

        for (int i = 0; i < steps; i++)
        {
            camera_update_xy(&camera, dt_step);
            scene_resolve_circle_2d(&scene, &camera.position.x, &camera.position.y, camera_r);
        }

        /*
         * Update vertical camera motion (gravity / jump / crouch).
         */
        camera_update_z(&camera, delta_time);

        /*
         * Render the frame.
         */
        renderer_begin_frame(0.78f, 0.88f, 0.98f);

        renderer_draw_sky_gradient(light_intensity);

        camera_apply_view(&camera);
        renderer_apply_light(light_intensity);
        renderer_apply_dynamic_fog(
            scene.global_time,
            camera.position.x,
            camera.position.y,
            scene.pond_enabled ? 1 : 0,
            scene.pond_x,
            scene.pond_y);

        scene_render(&scene);
        /* scene_debug_draw_obstacles(&scene); */

        /*
         * Draw on-screen help overlay if enabled.
         */
        if (show_help)
        {
            int w, h;
            SDL_GetWindowSize(window, &w, &h);
            ui_draw_help_overlay(
                w,
                h,
                light_intensity,
                scene_get_active_banana_count(&scene),
                scene_get_eaten_banana_count(&scene));
        }

        renderer_end_frame(window);
    }

    /*
     * Release loaded model resources.
     */
    if (banana_loaded)
        model_free(&banana_model);
    if (monkey_loaded)
        model_free(&monkey_model);
    if (rock_loaded)
        model_free(&rock_model);
    if (tree_loaded)
        model_free(&tree_model);

    IMG_Quit();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}