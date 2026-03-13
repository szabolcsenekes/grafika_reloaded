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

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    const GLfloat ambient[4] = {0.20f, 0.20f, 0.20f, 1.0f};
    const GLfloat diffuse[4] = {0.85f, 0.85f, 0.85f, 1.0f};
    const GLfloat specular[4] = {0.10f, 0.10f, 0.10f, 1.0f};
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
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

void renderer_apply_light(float intensity)
{
    if (intensity < 0.1f) intensity = 0.1f;
    if (intensity > 2.5f) intensity = 2.5f;

    const GLfloat ambient[4] = {
        0.20f * intensity,
        0.20f * intensity,
        0.20f * intensity,
        1.0f
    };

    const GLfloat diffuse[4] = {
        0.85f * intensity,
        0.85f * intensity,
        0.85f * intensity,
        1.0f
    };

    const GLfloat specular[4] = {
        0.10f * intensity,
        0.10f * intensity,
        0.10f * intensity,
        1.0f
    };

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);

    const GLfloat pos[4] = {0.2f, -0.6f, 1.0f, 0.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, pos);
}