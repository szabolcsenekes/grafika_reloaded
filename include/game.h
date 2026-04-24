#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include <SDL2/SDL.h>

#include "camera.h"
#include "scene.h"
#include "input.h"
#include "model.h"

typedef struct Game
{
    SDL_Window *window;
    SDL_GLContext gl_context;

    Camera camera;
    Scene scene;
    InputState input;

    Model rock_model;
    Model monkey_model;
    Model banana_model;
    Model tree_model;

    bool rock_loaded;
    bool monkey_loaded;
    bool banana_loaded;
    bool tree_loaded;

    bool running;
    bool show_help;

    float light_intensity;
} Game;

bool game_init(Game *game);
void game_run(Game *game);
void game_shutdown(Game *game);

#endif