#ifndef INPUT_H
#define INPUT_H

#include <SDL2/SDL.h>
#include <stdbool.h>

/*
 * Stores the complete input state of the current frame.
 *
 * quit            - true if the application should close
 * resized         - true if the window was resized this frame
 * win_w, win_h    - new window size after resize
 *
 * mouse_captured  - true if relative mouse mode is active
 * mouse_dx, mouse_dy - mouse movement delta for the current frame
 *
 * mouse_down      - current pressed state of mouse buttons
 * mouse_pressed   - buttons pressed this frame
 * mouse_released  - buttons released this frame
 *
 * key_down        - current pressed state of keyboard keys
 * key_pressed     - keys pressed this frame
 * key_released    - keys released this frame
 */
typedef struct InputState
{
    bool quit;

    bool resized;
    int win_w;
    int win_h;

    bool mouse_captured;
    int mouse_dx;
    int mouse_dy;

    bool mouse_down[8];
    bool mouse_pressed[8];
    bool mouse_released[8];

    bool key_down[SDL_NUM_SCANCODES];
    bool key_pressed[SDL_NUM_SCANCODES];
    bool key_released[SDL_NUM_SCANCODES];
} InputState;

/*
 * Initialize the input state structure with default values.
 */
void input_init(InputState *in);

/*
 * Reset per-frame input values at the beginning of a new frame.
 * This clears pressed/released flags and mouse movement deltas.
 */
void input_begin_frame(InputState *in);

/*
 * Poll SDL events and update the input state accordingly.
 */
void input_poll_events(InputState *in);

/*
 * Enable or disable mouse capture (relative mouse mode).
 */
void input_set_mouse_capture(InputState *in, bool capture);

/*
 * Return true while the given key is being held down.
 */
static inline bool input_down(const InputState *in, SDL_Scancode sc) { return in->key_down[sc]; }

/*
 * Return true only on the frame when the given key was pressed.
 */
static inline bool input_pressed(const InputState *in, SDL_Scancode sc) { return in->key_pressed[sc]; }

/*
 * Return true only on the frame when the given key was released.
 */
static inline bool input_released(const InputState *in, SDL_Scancode sc) { return in->key_released[sc]; }

#endif