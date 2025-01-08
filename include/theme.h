#ifndef THEME_H
#define THEME_H

#include <SDL.h>

struct Theme {
    SDL_Color backgroundColor;
    SDL_Color textColor;
    SDL_Color highlightColor;
    SDL_Color borderColor;
    SDL_Color progressBarColor;
};

extern Theme currentTheme;

void applyTheme(const Theme& theme);

#endif // THEME_H