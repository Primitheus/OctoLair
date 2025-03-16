#include "game_controller.h"
#include <iostream>

GameController::GameController() : controller(nullptr) {}

GameController::~GameController() {
    if (controller) {
        SDL_GameControllerClose(controller);
    }
}

bool GameController::initialize() {
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            controller = SDL_GameControllerOpen(i);
            if (controller) {
                return true;
            }
        }
    }
    std::cerr << "No controller found" << std::endl;
    return false;
}

SDL_GameController* GameController::getController() const {
    return controller;
}