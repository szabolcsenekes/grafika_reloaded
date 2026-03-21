#include "ui.h"

#include <GL/gl.h>
#include <stdio.h>
#include <string.h>

#define STB_EASY_FONT_IMPLEMENTATION
#include "stb_easy_font.h"

/*
 * Switch OpenGL to a simple 2D drawing mode for UI rendering.
 * This sets up an orthographic projection where coordinates match screen pixels.
 */
static void ui_begin_2d(int w, int h)
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, w, h, 0, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
}

/*
 * Restore the previous OpenGL state after 2D UI drawing.
 */
static void ui_end_2d(void)
{
    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
}

/*
 * Draw a filled semi-transparent rectangle.
 * This is used as the background panel of the help overlay.
 */
static void ui_draw_rect(float x, float y, float w, float h, float r, float g, float b, float a)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(r, g, b, a);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();

    glDisable(GL_BLEND);
}

/*
 * Draw text on the screen using stb_easy_font.
 */
static void ui_draw_text(float x, float y, const char *text, float r, float g, float b)
{
    char buffer[99999];
    int num_quads;

    num_quads = stb_easy_font_print(x, y, (char *)text, NULL, buffer, sizeof(buffer));

    glColor3f(r, g, b);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 16, buffer);
    glDrawArrays(GL_QUADS, 0, num_quads * 4);
    glDisableClientState(GL_VERTEX_ARRAY);
}

/*
 * Draw the help overlay with controls and current gameplay information.
 */
void ui_draw_help_overlay(
    int screen_w,
    int screen_h,
    float light_intensity,
    int active_bananas,
    int eaten_bananas)
{
    char line[128];

    ui_begin_2d(screen_w, screen_h);

    /* Background panel */
    ui_draw_rect(20.0f, 20.0f, 460.0f, 290.0f, 0.0f, 0.0f, 0.0f, 0.72f);

    /* Help text */
    ui_draw_text(35.0f, 40.0f, "MONKEY ZOO - HASZNALAT", 1.0f, 1.0f, 0.8f);
    ui_draw_text(35.0f, 70.0f, "W/A/S/D   - mozgas", 1.0f, 1.0f, 1.0f);
    ui_draw_text(35.0f, 90.0f, "Eger      - nezes", 1.0f, 1.0f, 1.0f);
    ui_draw_text(35.0f, 110.0f, "Shift     - futas", 1.0f, 1.0f, 1.0f);
    ui_draw_text(35.0f, 130.0f, "Ctrl      - guggolas", 1.0f, 1.0f, 1.0f);
    ui_draw_text(35.0f, 150.0f, "Space     - ugras", 1.0f, 1.0f, 1.0f);
    ui_draw_text(35.0f, 170.0f, "E         - kapu nyitas", 1.0f, 1.0f, 1.0f);
    ui_draw_text(35.0f, 190.0f, "Q         - banan dobas", 1.0f, 1.0f, 1.0f);
    ui_draw_text(35.0f, 210.0f, "+ / -     - fenyero", 1.0f, 1.0f, 1.0f);
    ui_draw_text(35.0f, 230.0f, "F1        - help ki/be", 1.0f, 1.0f, 1.0f);
    ui_draw_text(35.0f, 250.0f, "ESC       - kilepes", 1.0f, 1.0f, 1.0f);

    /* Dynamic status values */
    snprintf(line, sizeof(line), "Fenyerosseg: %.1f", light_intensity);
    ui_draw_text(35.0f, 275.0f, line, 0.8f, 1.0f, 0.8f);

    snprintf(line, sizeof(line), "Aktiv bananok: %d", active_bananas);
    ui_draw_text(250.0f, 275.0f, line, 1.0f, 1.0f, 0.7f);

    snprintf(line, sizeof(line), "Megevett bananok: %d", eaten_bananas);
    ui_draw_text(250.0f, 295.0f, line, 1.0f, 0.9f, 0.6f);

    ui_end_2d();
}