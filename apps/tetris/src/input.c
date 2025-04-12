#include "input.h"
#include "ewoksys/keydef.h"

Tetris_Action TETROMINO_ACTION;

void getInput() {
    SDL_Event event;

    /* Loop through waiting messages and process them */

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            /* Closing the Window or pressing Escape will exit the program */
            case SDL_QUIT:
                exit(0);
            break;

            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_DOWN:
                        TETROMINO_ACTION = DOWN;
                    break;

                    case SDLK_RIGHT:
                        TETROMINO_ACTION = RIGHT;
                    break;

                    case SDLK_LEFT:
                        TETROMINO_ACTION = LEFT;
                    break;

                    case SDLK_UP:
                    case KEY_BUTTON_A:
                        TETROMINO_ACTION = ROTATE;
                    break;

                    case SDLK_ESCAPE:
                    case KEY_BUTTON_START:
                        TETROMINO_ACTION = RESTART;
                    break;

                    case SDLK_SPACE:
                    case KEY_BUTTON_B:
                        TETROMINO_ACTION = DROP;
                    break;

                    default:
                    break;
                }
            break;

            case SDL_KEYUP:
                TETROMINO_ACTION = NONE;
            break;

            case SDL_USEREVENT:
                TETROMINO_ACTION = AUTO_DROP;
            break;

            default:
            break;
        }
    }
}
