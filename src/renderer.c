#include "renderer.h"

#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL2/SDL.h>

static void apply_viewport_projection(int width, int height)
{
    if (height <= 0)
        height = 1;

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

static void draw_skybox_cube(float s)
{
    glBegin(GL_QUADS);

    //top
    glColor3f(0.42f, 0.68f, 0.95f);
    glVertex3f(-s, -s, s);
    glVertex3f(s, -s, s);
    glVertex3f(s, s, s);
    glVertex3f(-s, s, s);

    //front
    glColor3f(0.55f, 0.78f, 0.98f);
    glVertex3f(-s, s, -s);
    glVertex3f(s, s, -s);
    glVertex3f(s, s, s);
    glVertex3f(-s, s, s);

    //back
    glColor3f(0.55f, 0.78f, 0.98f);
    glVertex3f(s, -s, -s);
    glVertex3f(-s, -s, -s);
    glVertex3f(-s, -s, s);
    glVertex3f(s, -s, s);

    //left
    glColor3f(0.60f, 0.82f, 0.99f);
    glVertex3f(-s, -s, -s);
    glVertex3f(-s, s, -s);
    glVertex3f(-s, s, s);
    glVertex3f(-s, -s, s);

    //right
    glColor3f(0.60f, 0.82f, 0.99f);
    glVertex3f(s, s, -s);
    glVertex3f(s, -s, -s);
    glVertex3f(s, -s, s);
    glVertex3f(s, s, s);

    //bottom
    glColor3f(0.30f, 0.35f, 0.40f);
    glVertex3f(-s, s, -s);
    glVertex3f(s, s, -s);
    glVertex3f(s, -s, -s);
    glVertex3f(-s, -s, -s);

    glEnd();
}

void renderer_draw_sky_gradient(float intensity)
{
    float bottom_r, bottom_g, bottom_b;
    float top_r, top_g, top_b;
    float t;

    if (intensity < 0.2f)
        intensity = 0.2f;
    if (intensity > 2.0f)
        intensity = 2.0f;

    t = intensity;
    if (t > 1.0f)
        t = 1.0f;

    bottom_r = 0.78f * t;
    bottom_g = 0.88f * t;
    bottom_b = 0.98f * t;

    top_r = 0.44f * t;
    top_g = 0.66f * t;
    top_b = 0.90f * t;

    glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT);

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(-1, 1, -1, 1, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glBegin(GL_QUADS);
    glColor3f(bottom_r, bottom_g, bottom_b);
    glVertex2f(-1.0f, -1.0f);
    glVertex2f(1.0f, -1.0f);

    glColor3f(top_r, top_g, top_b);
    glVertex2f(1.0f, 1.0f);
    glVertex2f(-1.0f, 1.0f);
    glEnd();

    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);

    glPopAttrib();
}

void renderer_init(int width, int height)
{
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

void renderer_resize(int width, int height)
{
    apply_viewport_projection(width, height);
}

void renderer_begin_frame(float r, float g, float b)
{
    glClearColor(r, g, b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void renderer_end_frame(void *sdl_window)
{
    SDL_GL_SwapWindow((SDL_Window *)sdl_window);
}

void renderer_apply_light(float intensity)
{
    if (intensity < 0.1f)
        intensity = 0.1f;
    if (intensity > 2.5f)
        intensity = 2.5f;

    const GLfloat ambient[4] = {
        0.20f * intensity,
        0.20f * intensity,
        0.20f * intensity,
        1.0f};

    const GLfloat diffuse[4] = {
        0.85f * intensity,
        0.85f * intensity,
        0.85f * intensity,
        1.0f};

    const GLfloat specular[4] = {
        0.10f * intensity,
        0.10f * intensity,
        0.10f * intensity,
        1.0f};

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);

    const GLfloat pos[4] = {0.2f, -0.6f, 1.0f, 0.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, pos);
}