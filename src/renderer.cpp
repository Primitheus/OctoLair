#include "renderer.h"
#include <iostream>
#include "config.h"

Renderer::Renderer() : window(nullptr), renderer(nullptr), font(nullptr) {}

Renderer::~Renderer() {
    if (font) {
        TTF_CloseFont(font);
    }
    if (renderer) {
        SDL_DestroyRenderer(renderer);
    }
    if (window) {
        SDL_DestroyWindow(window);
    }
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();

}

bool Renderer::initialize() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow("OctoLair", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        SDL_DestroyWindow(window);
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return false;
    }

    if (TTF_Init() == -1) {
        std::cerr << "TTF_Init Error: " << TTF_GetError() << std::endl;
        return false;
    }

    font = TTF_OpenFont("res/HyperlacRegular.ttf", 26);
    if (!font) {
        std::cerr << "TTF_OpenFont Error: " << TTF_GetError() << std::endl;
        return false;
    }

    if (IMG_Init(IMG_INIT_PNG) == 0) {
        std::cerr << "IMG_Init Error: " << IMG_GetError() << std::endl;
        return false;
    }

    return true;
}

void Renderer::clear() {
    SDL_SetRenderDrawColor(renderer, currentTheme.backgroundColor.r, currentTheme.backgroundColor.g, currentTheme.backgroundColor.b, currentTheme.backgroundColor.a);
    SDL_RenderClear(renderer);
}

void Renderer::present() {
    SDL_RenderPresent(renderer);
}

void Renderer::drawText(const std::string& text, int x, int y, SDL_Color color) {
    SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), color);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect rect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, nullptr, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void DrawFilledCircle(SDL_Renderer* renderer, int x, int y, int radius) {
    for (int w = 0; w < radius * 2; w++) {
        for (int h = 0; h < radius * 2; h++) {
            int dx = radius - w; // horizontal offset
            int dy = radius - h; // vertical offset
            if ((dx * dx + dy * dy) <= (radius * radius)) {
                SDL_RenderDrawPoint(renderer, x + dx, y + dy);
            }
        }
    }
}

void Renderer::drawRoundedRect(SDL_Rect rect, int radius, int thickness) {
    SDL_SetRenderDrawColor(renderer, currentTheme.highlightColor.r, currentTheme.highlightColor.g, currentTheme.highlightColor.b, currentTheme.highlightColor.a);

    if (radius <= 0 || thickness <= 0 || rect.w <= 0 || rect.h <= 0) return;

    for (int t = 0; t < thickness; t++) {
        SDL_Rect innerRect = { rect.x + t, rect.y + t, rect.w - t * 2, rect.h - t * 2 };

        SDL_RenderDrawLine(renderer, innerRect.x + radius, innerRect.y, innerRect.x + innerRect.w - radius, innerRect.y);
        SDL_RenderDrawLine(renderer, innerRect.x + radius, innerRect.y + innerRect.h - 1, innerRect.x + innerRect.w - radius, innerRect.y + innerRect.h - 1);

        SDL_RenderDrawLine(renderer, innerRect.x, innerRect.y + radius, innerRect.x, innerRect.y + innerRect.h - radius);
        SDL_RenderDrawLine(renderer, innerRect.x + innerRect.w - 1, innerRect.y + radius, innerRect.x + innerRect.w - 1, innerRect.y + innerRect.h - radius);

        DrawFilledCircle(renderer, innerRect.x + radius, innerRect.y + radius, radius - t);                   // Top-left corner
        DrawFilledCircle(renderer, innerRect.x + innerRect.w - radius, innerRect.y + radius, radius - t);    // Top-right corner
        DrawFilledCircle(renderer, innerRect.x + radius, innerRect.y + innerRect.h - radius, radius - t);    // Bottom-left corner
        DrawFilledCircle(renderer, innerRect.x + innerRect.w - radius, innerRect.y + innerRect.h - radius, radius - t); // Bottom-right corner
    }
}

void Renderer::drawProgressBar(int progress, const std::string& title, const std::vector<std::string>& queuedTitles) {
    // Outline For Progress Box
    SDL_Rect fullBox = {SCREEN_WIDTH / 2 + 10 + 70, SCREEN_HEIGHT - 10 - 60, (SCREEN_WIDTH / 2 - 2 * 10 - 140), 25};
    SDL_SetRenderDrawColor(renderer, currentTheme.highlightColor.r, currentTheme.highlightColor.g, currentTheme.highlightColor.b, currentTheme.highlightColor.a);
    SDL_RenderDrawRect(renderer, &fullBox);

    // Define Progress Bar
    SDL_Rect progressBar = {SCREEN_WIDTH / 2 + 10 + 70, SCREEN_HEIGHT - 10 - 60, (SCREEN_WIDTH / 2 - 2 * 10 - 140) * progress / 100, 25};
    SDL_SetRenderDrawColor(renderer, currentTheme.progressBarColor.r, currentTheme.progressBarColor.g, currentTheme.progressBarColor.b, currentTheme.progressBarColor.a);
    SDL_RenderFillRect(renderer, &progressBar);

    SDL_Color color = currentTheme.textColor;
    drawText(title, SCREEN_WIDTH / 2 + 10 + 70, SCREEN_HEIGHT - 10 - 90, color);

    if (queuedTitles.size() > 1) {
        drawText("Next: " + queuedTitles[1], SCREEN_WIDTH / 2 + 10 + 70, SCREEN_HEIGHT - 10 - 120, color);
    }
}

void Renderer::drawImage(const std::string& imagePath, SDL_Rect rect) {
    SDL_Texture* imageTexture = IMG_LoadTexture(renderer, imagePath.c_str());
    if (imageTexture) {
        SDL_RenderCopy(renderer, imageTexture, nullptr, &rect);
        SDL_DestroyTexture(imageTexture);
    }
}

void Renderer::drawMessageBox(const std::string& message) {
    // Define the dimensions of the message box
    int boxWidth = 400;
    int boxHeight = 200;
    SDL_Rect messageBox = { (SCREEN_WIDTH - boxWidth) / 2, (SCREEN_HEIGHT - boxHeight) / 2, boxWidth, boxHeight };

    // Draw the background of the message box
    SDL_SetRenderDrawColor(renderer, currentTheme.backgroundColor.r, currentTheme.backgroundColor.g, currentTheme.backgroundColor.b, currentTheme.backgroundColor.a);
    SDL_RenderFillRect(renderer, &messageBox);

    // Draw the border of the message box
    SDL_SetRenderDrawColor(renderer, currentTheme.highlightColor.r, currentTheme.highlightColor.g, currentTheme.highlightColor.b, currentTheme.highlightColor.a);
    SDL_RenderDrawRect(renderer, &messageBox);

    // Draw the message text
    SDL_Color textColor = currentTheme.textColor;
    int textX = messageBox.x + 20;
    int textY = messageBox.y + (boxHeight / 2) - 10; // Adjust the Y position to center the text vertically
    drawText(message, textX, textY, textColor);

    // Present the message box
    SDL_RenderPresent(renderer);

    // Wait for 1 second
    SDL_Delay(1000);

    // Clear the message box
    clear();
    present();
}