#ifndef UI_H
#define UI_H

/*
 * Draw the on-screen help overlay.
 *
 * screen_w, screen_h  - current window size
 * light_intensity     - current scene light intensity
 * active_bananas      - number of bananas currently active in the scene
 * eaten_bananas       - number of bananas already eaten by monkeys
 */
void ui_draw_help_overlay(int screen_w, int screen_h, float light_intensity, int active_bananas, int eaten_bananas);

#endif // UI_H