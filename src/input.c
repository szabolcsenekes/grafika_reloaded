#include "input.h"

#include <string.h>

/*
 * Convert an SDL mouse button value to a safe array index in the range [0, 7].
 * This prevents invalid indexing if SDL returns an unexpected button number.
 */
static int clamp_mouse_button(int sdl_button)
{
    int idx = sdl_button - 1;
    if (idx < 0)
        idx = 0;
    if (idx > 7)
        idx = 7;
    return idx;
}

/*
 * Initialize the input state structure to zero and disable mouse capture.
 */
void input_init(InputState *in)
{
    memset(in, 0, sizeof(*in));
    in->mouse_captured = false;
}

/*
 * Prepare input state for a new frame.
 * Per-frame values such as mouse delta and pressed/released flags are cleared here.
 */
void input_begin_frame(InputState *in)
{
    in->resized = false;

    in->mouse_dx = 0;
    in->mouse_dy = 0;

    memset(in->key_pressed, 0, sizeof(in->key_pressed));
    memset(in->key_released, 0, sizeof(in->key_released));

    memset(in->mouse_pressed, 0, sizeof(in->mouse_pressed));
    memset(in->mouse_released, 0, sizeof(in->mouse_released));
}

/*
 * Enable or disable relative mouse mode.
 * When enabled, the mouse cursor is captured and only movement deltas are reported.
 */
void input_set_mouse_capture(InputState *in, bool capture)
{
    in->mouse_captured = capture;
    SDL_SetRelativeMouseMode(capture ? SDL_TRUE : SDL_FALSE);
}

/*
 * Poll all pending SDL events and update the input state.
 * This handles quitting, window resize, keyboard events, mouse buttons and mouse motion.
 */
void input_poll_events(InputState *in)
{
    SDL_Event e;

    while (SDL_PollEvent(&e))
    {
        switch (e.type)
        {
        case SDL_QUIT:
            in->quit = true;
            break;

        case SDL_WINDOWEVENT:
            if (e.window.event == SDL_WINDOWEVENT_RESIZED)
            {
                in->resized = true;
                in->win_w = e.window.data1;
                in->win_h = e.window.data2;
            }
            break;

        case SDL_KEYDOWN:
            /*
             * Only register a new key press when this is not an auto-repeat event.
             */
            if (e.key.repeat == 0)
            {
                SDL_Scancode sc = e.key.keysym.scancode;
                in->key_down[sc] = true;
                in->key_pressed[sc] = true;
            }
            break;

        case SDL_KEYUP:
        {
            SDL_Scancode sc = e.key.keysym.scancode;
            in->key_down[sc] = false;
            in->key_released[sc] = true;
        }
        break;

        case SDL_MOUSEBUTTONDOWN:
        {
            int b = clamp_mouse_button(e.button.button);
            in->mouse_down[b] = true;
            in->mouse_pressed[b] = true;

            /*
             * Right mouse button enables mouse capture,
             * which is used for free camera look.
             */
            if (e.button.button == SDL_BUTTON_RIGHT)
            {
                input_set_mouse_capture(in, true);
            }
        }
        break;

        case SDL_MOUSEBUTTONUP:
        {
            int b = clamp_mouse_button(e.button.button);
            in->mouse_down[b] = false;
            in->mouse_released[b] = true;

            /*
             * Releasing the right mouse button disables mouse capture.
             */
            if (e.button.button == SDL_BUTTON_RIGHT)
            {
                input_set_mouse_capture(in, false);
            }
        }
        break;

        case SDL_MOUSEMOTION:
            /*
             * Mouse movement is only accumulated while the mouse is captured.
             */
            if (in->mouse_captured)
            {
                in->mouse_dx += e.motion.xrel;
                in->mouse_dy += e.motion.yrel;
            }
            break;

        default:
            break;
        }
    }
}