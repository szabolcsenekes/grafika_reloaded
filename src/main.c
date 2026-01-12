#include <SDL2/SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <stdio.h>
#include <stdbool.h>

#include "camera.h"

static void gl_setup(int width, int height) {
    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(70.0, (double)width / (double)height, 0.1, 500.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glDisable(GL_CULL_FACE);
}

static void draw_grid(float half_size, float step) {
    glDisable(GL_LIGHTING);
    glBegin(GL_LINES);

    for (float x = -half_size; x <= half_size; x += step) {
        glColor3f(0.25f, 0.25f, 0.25f);
        glVertex3f(x, -half_size, 0.0f);
        glVertex3f(x, half_size, 0.0f);
    }

    for (float y = -half_size; y <= half_size; y += step) {
        glColor3f(0.25f, 0.25f, 0.25f);
        glVertex3f(-half_size, y, 0.0f);
        glVertex3f(half_size, y, 0.0f);
    }

    glColor3f(1.0f, 0.2f, 0.2f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(2.0f, 0.0f, 0.0f);

    glColor3f(0.2f, 1.0f, 0.2f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 2.0f, 0.0f);

    glColor3f(0.2f, 0.2f, 1.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 2.0f);

    glEnd();
}

static void draw_box(float cx, float cy, float cz, float sx, float sy, float sz) {
    float x0 = cx - sx * 0.5f, x1 = cx + sx * 0.5f;
    float y0 = cy - sy * 0.5f, y1 = cy + sy * 0.5f;
    float z0 = cz - sz * 0.5f, z1 = cz + sz * 0.5f;

    glBegin(GL_QUADS);

    //front
    glVertex3f(x0, y0, z1);
    glVertex3f(x1, y0, z1);
    glVertex3f(x1, y1, z1);
    glVertex3f(x0, y1, z1);

    //bottom
    glVertex3f(x0, y0, z0);
    glVertex3f(x0, y1, z0);
    glVertex3f(x1, y1, z0);
    glVertex3f(x1, y0, z0);

    //front (y1)
    glVertex3f(x0, y1, z0);
    glVertex3f(x0, y1, z1);
    glVertex3f(x1, y1, z1);
    glVertex3f(x1, y1, z0);

    //back (y0)
    glVertex3f(x0, y0, z0);
    glVertex3f(x1, y0, z0);
    glVertex3f(x1, y0, z1);
    glVertex3f(x0, y0, z1);

    //left (x0)
    glVertex3f(x0, y0, z0);
    glVertex3f(x0, y0, z1);
    glVertex3f(x0, y1, z1);
    glVertex3f(x0, y1, z0);

    //right (x1)
    glVertex3f(x1, y0, z0);
    glVertex3f(x1, y1, z0);
    glVertex3f(x1, y1, z1);
    glVertex3f(x1, y0, z1);

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

static void draw_fence(float half_size, float wall_height) {
    glDisable(GL_LIGHTING);

    //wall width
    const float T = 0.25f;

    //wall color
    glColor3f(0.35f, 0.25f, 0.12f);

    //4 walls
    draw_box(0.0f, half_size, wall_height * 0.5f, half_size * 2.0f, T, wall_height);
    draw_box(0.0f, -half_size, wall_height * 0.5f, half_size * 2.0f, T, wall_height);
    draw_box(half_size, 0.0f, wall_height * 0.5f, T, half_size * 2.0f, wall_height);
    draw_box(-half_size, 0.0f, wall_height * 0.5f, T, half_size * 2.0f, wall_height);

    glColor3f(0.25f, 0.18f, 0.08f);
    const float post = 0.35f;
    const float step = 4.0f;

    for (float x = -half_size; x <= half_size; x += step) {
        draw_box(x, half_size, wall_height * 0.5f, post, post, wall_height);
        draw_box(x, -half_size, wall_height * 0.5f, post, post, wall_height);
    }

    for (float y = -half_size; y <= half_size; y += step) {
        draw_box(half_size, y, wall_height * 0.5f, post, post, wall_height);
        draw_box(-half_size, y, wall_height * 0.5f, post, post, wall_height);
    }

    glColor3f(0.40f, 0.30f, 0.15f);
    float gate_y = -half_size;
    float gate_w = 3.0f;
    float gate_x0 = -gate_w * 0.5f;
    float gate_x1 = gate_w * 0.5f;

    draw_box(gate_x0, gate_y, wall_height * 0.5f, post, post, wall_height);
    draw_box(gate_x1, gate_y, wall_height * 0.5f, post, post, wall_height);

    draw_box(0.0f, gate_y, wall_height - 0.2f, gate_w, post, post);
}

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
    gl_setup(width, height);

    Camera camera;
    camera_init(&camera);

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
                gl_setup(width, height);
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

        // --- update once per frame ---
        camera_update(&camera, delta_time);

        // --- render once per frame ---
        glClearColor(0.08f, 0.09f, 0.11f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        camera_apply_view(&camera);
        draw_grid(50.0f, 1.0f);
        draw_ground(60.0f, 0.0f);
        draw_fence(12.0f, 2.0f);

        glDisable(GL_LIGHTING);
        glColor3f(0.45f, 0.45f, 0.48f);
        draw_box(3.0f, 2.0f, 0.5f, 1.5f, 1.2f, 1.0f);

        glColor3f(0.30f, 0.22f, 0.10f);
        draw_box(-4.0f, -1.0f, 1.0f, 0.6f, 0.6f, 2.0f);

        SDL_GL_SwapWindow(window);
        }

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
    }
