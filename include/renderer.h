#ifndef RENDERER_H
#define RENDERER_H

#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <string>
#include <vector>
#include "theme.h"
#include "types.h"

class Renderer {
public:
    Renderer();
    ~Renderer();
    bool initialize();
    void clear();
    void present();
    void drawText(const std::string& text, int x, int y, SDL_Color color);
    void drawRoundedRect(SDL_Rect rect, int radius, int thickness);
    void drawProgressBar(int progress, const std::string& title, const std::vector<std::string>& queuedTitles);
    void drawImage(const std::string& imagePath, SDL_Rect rect);
    void drawMessageBox(const std::string& message);

private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font;
};

#endif // RENDERER_H