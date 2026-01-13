#include "renderer.h"

#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL2/SDL.h>

static void apply_viewport_projection(int width, int height) {
    if (height <= 0) height = 1;

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

void renderer_init(int width, int height) {
    apply_viewport_projection(width, height);
}

void renderer_resize(int width, int height) {
    apply_viewport_projection(width, height);
}

void renderer_begin_frame(float r, float g, float b) {
    glClearColor(r, g, b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void renderer_end_frame(void* sdl_window) {
    SDL_GL_SwapWindow((SDL_Window*)sdl_window);
}