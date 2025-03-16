#ifndef GAME_CONTROLLER_H
#define GAME_CONTROLLER_H

#include <SDL.h>

class GameController {
public:
    GameController();
    ~GameController();
    bool initialize();
    SDL_GameController* getController() const;

private:
    SDL_GameController* controller;
};

#endif // GAME_CONTROLLER_H