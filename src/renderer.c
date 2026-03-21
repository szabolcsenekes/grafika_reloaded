#include "renderer.h"

#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL2/SDL.h>
#include <math.h>

/*
 * Configure the OpenGL viewport and perspective projection
 * based on the current window size.
 */
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

/*
 * Draw a fullscreen background quad with a vertical sky gradient.
 * The gradient brightness depends on the current light intensity.
 */
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

    /*
     * Temporarily disable lighting and depth testing,
     * then draw the sky in normalized screen space.
     */
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

/*
 * Initialize the renderer and set up default OpenGL states,
 * including fog, lighting and color material.
 */
void renderer_init(int width, int height)
{
    apply_viewport_projection(width, height);

    glEnable(GL_FOG);

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

/*
 * Recalculate viewport and projection after the window size changes.
 */
void renderer_resize(int width, int height)
{
    apply_viewport_projection(width, height);
}

/*
 * Begin a new frame by clearing the screen and depth buffer.
 */
void renderer_begin_frame(float r, float g, float b)
{
    glClearColor(r, g, b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

/*
 * Present the rendered frame on the SDL window.
 */
void renderer_end_frame(void *sdl_window)
{
    SDL_GL_SwapWindow((SDL_Window *)sdl_window);
}

/*
 * Update the main light source based on the given intensity value.
 * This affects ambient, diffuse and specular light components.
 */
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

    /*
     * Directional light source above the scene.
     * The last value is 0.0, so this is treated as a direction, not a position.
     */
    const GLfloat pos[4] = {0.2f, -0.6f, 1.0f, 0.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, pos);
}

/*
 * Configure animated fog for the scene.
 * Fog density changes over time and becomes stronger near the pond.
 */
void renderer_apply_dynamic_fog(float global_time, float cam_x, float cam_y,
                                int pond_enabled, float pond_x, float pond_y)
{
    /*
     * Base fog animation using a smooth sine wave pulse.
     */
    float pulse = (sinf(global_time * 0.35f) + 1.0f) * 0.5f;
    float start = 26.0f - pulse * 8.0f;
    float end = 92.0f - pulse * 24.0f;

    float fog_r = 0.72f;
    float fog_g = 0.82f;
    float fog_b = 0.90f;

    /*
     * Increase fog strength and slightly shift its color
     * when the camera is close to the pond.
     */
    if (pond_enabled)
    {
        float dx = cam_x - pond_x;
        float dy = cam_y - pond_y;
        float dist = sqrtf(dx * dx + dy * dy);

        if (dist < 18.0f)
        {
            float t = 1.0f - dist / 18.0f;
            start -= t * 7.0f;
            end -= t * 18.0f;

            fog_r -= t * 0.08f;
            fog_g -= t * 0.05f;
            fog_b -= t * 0.02f;
        }
    }

    /*
     * Keep fog values within reasonable limits
     * to avoid invalid or overly aggressive fog settings.
     */
    if (start < 6.0f)
        start = 6.0f;
    if (end < start + 8.0f)
        end = start + 8.0f;

    {
        GLfloat fog_color[4] = {fog_r, fog_g, fog_b, 1.0f};
        glEnable(GL_FOG);
        glFogi(GL_FOG_MODE, GL_LINEAR);
        glFogfv(GL_FOG_COLOR, fog_color);
        glFogf(GL_FOG_START, start);
        glFogf(GL_FOG_END, end);
        glHint(GL_FOG_HINT, GL_NICEST);
    }
}