#include "game.h"

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    Game game;

    if (!game_init(&game))
    {
        return 1;
    }

    game_run(&game);
    game_shutdown(&game);

    return 0;
}