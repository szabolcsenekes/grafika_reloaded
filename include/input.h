#ifndef INPUT_H
#define INPUT_H

#include <SDL2/SDL.h>
#include <stdbool.h>

typedef struct InputState {
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

void input_init(InputState* in);

void input_begin_frame(InputState* in);

void input_poll_events(InputState* in);

void input_set_mouse_capture(InputState* in, bool capture);

static inline bool input_down(const InputState* in, SDL_Scancode sc) { return in->key_down[sc]; }

static inline bool input_pressed(const InputState* in, SDL_Scancode sc) { return in->key_pressed[sc]; }

static inline bool input_released(const InputState* in, SDL_Scancode sc) { return in->key_released[sc]; }

#endif