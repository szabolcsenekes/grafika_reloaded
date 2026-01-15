#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include "camera.h"
#include "scene.h"
#include "renderer.h"

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    const float WALK_SPEED = 4.0f;
    const float RUN_SPEED = 9.0f;
    const float CROUCH_MULT = 0.4f;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_Window* window = SDL_CreateWindow("Monkey Zoo - Empty Scene", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    if (!window) {
        fprintf(stderr, "SDL_CreateWindow error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        fprintf(stderr, "SDL_GL_CreateContext error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_GL_SetSwapInterval(1);

    int width;
    int height;
    SDL_GetWindowSize(window, &width, &height);
    renderer_init(width, height);

    Camera camera;
    camera_init(&camera);

    Scene scene;
    scene_init(&scene);

    //enclosures
    scene_add_fence(&scene, 0.0f, 0.0f, 25.0f, 2.0f, true);
    scene_add_fence(&scene, 40.0f, 10.0f, 12.0f, 2.0f, true);
    scene_add_fence(&scene, -45.0f, -20.0f, 15.0f, 2.0f, true);

    //gate
    scene_add_gate(&scene, -1.5f, -25.0f, 0.0f, 3.0f, 0.12f, 2.0f, (Color3){0.40f, 0.30f, 0.15f}, 0.0f, 90.0f);

    //props
    scene_add_box(&scene, 5.0f, 6.0f, 0.5f, 1.8f, 1.2f, 1.0f, (Color3){0.45f, 0.45f, 0.48f}, true);
    scene_add_box(&scene, -8.0f, 4.0f, 0.4f, 1.0f, 1.0f, 0.8f, (Color3){0.45f, 0.45f, 0.48f}, true);
    scene_add_box(&scene, 12.0f, -3.0f, 0.6f, 2.2f, 1.4f, 1.2f, (Color3){0.45f, 0.45f, 0.48f}, true);

    scene_add_box(&scene, -10.0f, -6.0f, 1.2f, 0.6f, 0.6f, 2.4f, (Color3){0.30f, 0.22f, 0.10f}, true);
    scene_add_box(&scene, 18.0f, 8.0f, 1.5f, 0.7f, 0.7f, 3.0f, (Color3){0.30f, 0.22f, 0.10f}, true);

    scene_add_box(&scene, 35.0f, -5.0f, 1.2f, 6.0f, 4.0f, 2.4f, (Color3){0.55f, 0.40f, 0.25f}, true);

    bool running = true;
    bool mouse_captured = false;

    uint64_t prev = SDL_GetPerformanceCounter();
    const double freq = (double)SDL_GetPerformanceFrequency();

    while (running) {
        // --- dt ---
        uint64_t now = SDL_GetPerformanceCounter();
        float delta_time = (float)((double)(now - prev) / freq);
        prev = now;
        if (delta_time > 0.05f)
            delta_time = 0.05f;

        // --- keyboard state ---
        const Uint8 *keystate = SDL_GetKeyboardState(NULL);

        // base speed (walk/run)
        float base_speed = WALK_SPEED;
        if (keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT])
        {
            base_speed = RUN_SPEED;
        }

        // crouch affects speed
        if (keystate[SDL_SCANCODE_LCTRL])
        {
            base_speed = WALK_SPEED * CROUCH_MULT;
        }

        // apply movement intent
        camera.speed_forward = 0.0f;
        camera.speed_side = 0.0f;

        if (keystate[SDL_SCANCODE_W])
            camera.speed_forward += base_speed;
        if (keystate[SDL_SCANCODE_S])
            camera.speed_forward -= base_speed;
        if (keystate[SDL_SCANCODE_A])
            camera.speed_side += base_speed;
        if (keystate[SDL_SCANCODE_D])
            camera.speed_side -= base_speed;

        // crouch target height
        camera.target_eye_height = keystate[SDL_SCANCODE_LCTRL] ? 1.0f : 1.7f;

        scene.gate_request_to_open = false;

        if (keystate[SDL_SCANCODE_E])
        {
            float fx, fy;
            camera_get_forward(&camera, &fx, &fy);

            if (scene_gate_can_interact(&scene, camera.position.x, camera.position.y, fx, fy))
            {
                scene.gate_request_to_open = true;
            }
        }

        // --- events ---
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                running = false;

            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED)
            {
                width = event.window.data1;
                height = event.window.data2;
                renderer_resize(width, height);
            }

            if (event.type == SDL_KEYDOWN && event.key.repeat == 0)
            {
                if (event.key.keysym.sym == SDLK_ESCAPE)
                    running = false;

                if (event.key.keysym.sym == SDLK_SPACE)
                {
                    if (camera.on_ground)
                    {
                        camera.vz = 7.0f;
                        camera.on_ground = 0;
                    }
                }
            }

            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT)
            {
                mouse_captured = true;
                SDL_SetRelativeMouseMode(SDL_TRUE);
            }

            if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_RIGHT)
            {
                mouse_captured = false;
                SDL_SetRelativeMouseMode(SDL_FALSE);
            }

            if (event.type == SDL_MOUSEMOTION && mouse_captured)
            {
                float sensitivity = 0.12f;
                float yaw_delta = (float)-event.motion.xrel * sensitivity;
                float pitch_delta = (float)-event.motion.yrel * sensitivity;
                camera_rotate(&camera, yaw_delta, pitch_delta);
            }
        }

        scene_update(&scene, delta_time);
        scene_collect_obstacles(&scene);

        const float camera_r = 0.35f;

        float max_speed = fabsf(camera.speed_forward) + fabsf(camera.speed_side);

        const float MAX_STEP = camera_r * 0.5f;
        
        int steps = (int)ceilf((max_speed * delta_time) / MAX_STEP);
        if (steps < 1) steps = 1;
        if (steps > 12) steps = 12;

        float dt_step = delta_time / (float)steps;

            for (int i = 0; i < steps; i++) {
                camera_update(&camera, dt_step);
                scene_resolve_circle_2d(&scene, &camera.position.x, &camera.position.y, camera_r);
            }

        renderer_begin_frame(0.08f, 0.09f, 0.11f);

        camera_apply_view(&camera);
        scene_render(&scene);
        scene_debug_draw_obstacles(&scene);

        renderer_end_frame(window);
        }

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
    }
