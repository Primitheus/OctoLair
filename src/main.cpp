#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>

#include "config.h"
#include "types.h"
#include "utils.h"
#include "theme.h"


#include <cmath>

Theme darkTheme = {
    {0, 0, 0, 255},       // backgroundColor
    {255, 255, 255, 255}, // textColor
    {255, 0, 0, 255},     // highlightColor
    {255, 0, 0, 255},     // borderColor
    {255, 255, 255, 255}  // progressBarColor
};

Theme lightTheme = {
    {255, 255, 255, 255}, // backgroundColor
    {0, 0, 0, 255},       // textColor
    {0, 0, 255, 255},     // highlightColor
    {0, 0, 255, 255},     // borderColor
    {0, 0, 0, 255}        // progressBarColor
};

Theme purpleTheme = {
    {0, 0, 0, 255}, // backgroundColor
    {180, 157, 250, 255}, // textColor
    {113, 66, 255, 255},     // highlightColor
    {113, 66, 255, 255},     // borderColor
    {113, 66, 255, 255}        // progressBarColor
};


extern Theme currentTheme;


std::atomic<int> downloadProgress(0);
std::atomic<bool> isDownloading(false);

std::queue<std::pair<std::string, std::string>> downloadQueue;
std::vector<std::string> queuedGameTitles;

std::mutex queueMutex;
std::condition_variable queueCV;

std::string shortenText(const std::string& text, int maxLength) {
    if (text.length() > maxLength) {
        return text.substr(0, maxLength - 3) + "...";
    }
    return text;
}

std::string scrollText(const std::string& text, int offset) {
    std::string spacedText;
    if (text.length() >= 2) {
        spacedText = text + "      ";
    } else {
        spacedText = text + " ";
    }
    int length = spacedText.length();
    std::string scrolledText = spacedText.substr(offset % length) + spacedText.substr(0, offset % length);
    return scrolledText.substr(0, 32);
}

void DrawCircleOutline(SDL_Renderer* renderer, int centerX, int centerY, int radius) {
    SDL_SetRenderDrawColor(renderer, currentTheme.highlightColor.r, currentTheme.highlightColor.g, currentTheme.highlightColor.b, currentTheme.highlightColor.a);

    for (int w = 0; w < radius * 2; w++) {
        for (int h = 0; h < radius * 2; h++) {
            int dx = radius - w;
            int dy = radius - h;
            if ((dx * dx + dy * dy) <= (radius * radius) && (dx * dx + dy * dy) >= ((radius - 1) * (radius - 1))) {
                SDL_RenderDrawPoint(renderer, centerX + dx, centerY + dy);
            }
        }
    }
}

void DrawFilledCircle(SDL_Renderer* renderer, int centerX, int centerY, int radius) {
    SDL_SetRenderDrawColor(renderer, currentTheme.highlightColor.r, currentTheme.highlightColor.g, currentTheme.highlightColor.b, currentTheme.highlightColor.a);

    for (int w = 0; w < radius * 2; w++) {
        for (int h = 0; h < radius * 2; h++) {
            int dx = radius - w;
            int dy = radius - h;
            if ((dx * dx + dy * dy) <= (radius * radius)) {
                SDL_RenderDrawPoint(renderer, centerX + dx, centerY + dy);
            }
        }
    }
}


void DrawRoundedRect(SDL_Renderer* renderer, SDL_Rect rect, int radius, int thickness) {
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


void downloadGameThread(const std::string& console, const std::string& url, const std::string& gameTitle) {

    isDownloading = true;
    downloadProgress = 0;

    int res = downloadGame(console, getHtml(url));

    if (res == 0) {
        std::cout << "Game downloaded successfully: " << gameTitle << std::endl;
    } else {
        std::cerr << "Failed to download game: " << gameTitle << std::endl;
    }

    isDownloading = false;
    queueCV.notify_one();
}


void processDownloadQueue() {
    while (true) {
        std::unique_lock<std::mutex> lock(queueMutex);
        queueCV.wait(lock, [] { return !downloadQueue.empty() || !isDownloading; });

        if (!downloadQueue.empty()) {
            auto [console, url] = downloadQueue.front();
            downloadQueue.pop();
            std::string gameTitle = queuedGameTitles.front();
            lock.unlock();

            downloadGameThread(console, url, gameTitle);
            std::unique_lock<std::mutex> lock(queueMutex);
            queuedGameTitles.erase(queuedGameTitles.begin());
        }
    }
}

int main(int argc, char* argv[]) {

    applyTheme(purpleTheme);


    std::string htmlConsoles = getHtml("https://vimm.net/vault");
    std::vector<Console> consoles = parseHTML(htmlConsoles);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("OctoLair", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == nullptr) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr) {
        SDL_DestroyWindow(window);
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_GameController* controller = nullptr;
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            controller = SDL_GameControllerOpen(i);
            if (controller) {
                break;
            }
        }
    }

    TTF_Init();
    TTF_Font* font = TTF_OpenFont("res/HyperlacRegular.ttf", 26);
    if (!font) {
        std::cerr << "TTF_OpenFont Error: " << TTF_GetError() << std::endl;
        return 1;
    }

    if(IMG_Init(IMG_INIT_PNG) == 0) {
        std::cerr << "IMG_Init Error: " << IMG_GetError() << std::endl;
        return 1;
    }

    if (!controller) {
        std::cerr << "No controller found" << std::endl;
    }

    std::vector<Filter> filters = {
        Filter("#"), Filter("A"), Filter("B"), Filter("C"), Filter("D"), Filter("E"), Filter("F"), Filter("G"), Filter("H"), Filter("I"),
        Filter("J"), Filter("K"), Filter("L"), Filter("M"), Filter("N"), Filter("O"), Filter("P"), Filter("Q"), Filter("R"), Filter("S"),
        Filter("T"), Filter("U"), Filter("V"), Filter("W"), Filter("X"), Filter("Y"), Filter("Z")
    };

    SDL_Event e;
    bool quit = false;
    int selectedConsole = 0;
    int selectedGame = 0;
    int selectedFilter = 0;

    std::vector<Game> games;
    bool showGames = false;
    bool showFilters = false;

    std::thread queueThread(processDownloadQueue);
    queueThread.detach();

    Uint32 lastButtonPressTime = 0;
    Uint32 buttonPressDelay = 200; // Delay in milliseconds
    Uint32 buttonHoldDelay = 500;  // Delay before repeating action when holding button
    bool buttonHeld = false;

    bool dpadUpPressed = false;
    bool dpadDownPressed = false;

    while (!quit) {
        static int scrollOffset = 0;
        static int frameCount = 0;
        frameCount++;

        if (frameCount % 8 == 0) {
            scrollOffset++;
        }

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_CONTROLLERBUTTONDOWN || e.type == SDL_CONTROLLERBUTTONUP) {
                Uint32 currentTime = SDL_GetTicks();

                if (e.type == SDL_CONTROLLERBUTTONDOWN) {
                    lastButtonPressTime = currentTime;
                    std::cout << "Controller button pressed: " << (int)e.cbutton.button << std::endl;
                    if (e.cbutton.button == 3) {
                        quit = true;
                    } else if (e.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_UP) {
                        dpadUpPressed = true;
                        if (!showGames && !showFilters) {
                            selectedConsole = (selectedConsole - 1 + consoles.size()) % consoles.size();
                            scrollOffset = 0;
                        } else if (showFilters) {
                            selectedFilter = (selectedFilter - 1 + filters.size()) % filters.size();
                            scrollOffset = 0;
                        } else if (showGames) {
                            selectedGame = (selectedGame - 1 + games.size()) % games.size();
                            scrollOffset = 0;
                        }
                    } else if (e.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN) {
                        dpadDownPressed = true;
                        if (!showGames && !showFilters) {
                            selectedConsole = (selectedConsole + 1) % consoles.size();
                            scrollOffset = 0;
                        } else if (showFilters) {
                            selectedFilter = (selectedFilter + 1) % filters.size();
                            scrollOffset = 0;
                        } else if (showGames) {
                            selectedGame = (selectedGame + 1) % games.size();
                            scrollOffset = 0;
                        }
                    } else if (e.cbutton.button == 0) {
                        if (!showFilters && !showGames) {
                            showFilters = true;
                        } else if (showFilters) {
                            std::cout << "Selected console: " << consoles[selectedConsole].name << std::endl;
                            std::cout << "https://vimm.net" + consoles[selectedConsole].url + "/" + filters[selectedFilter].value << std::endl;

                            std::string htmlGames = getHtml("https://vimm.net" + consoles[selectedConsole].url + "/" + filters[selectedFilter].value);
                            games = parseGamesHTML(htmlGames);

                            std::cout << "Number of games parsed: " << games.size() << std::endl;

                            showGames = true;
                            selectedGame = 0;
                            showFilters = false;
                        } else if (showGames && !games.empty()) {
                            std::cout << "Selected game: " << games[selectedGame].title << std::endl;
                            std::cout << "Queueing game for download..." << std::endl;

                            {
                                std::lock_guard<std::mutex> lock(queueMutex);
                                downloadQueue.push({consoles[selectedConsole].name, "https://vimm.net" + games[selectedGame].url});
                                queuedGameTitles.push_back(games[selectedGame].title);
                            }
                            queueCV.notify_one();
                        }
                    } else if (e.cbutton.button == 1) {
                        if (showGames) {
                            showGames = false;
                            showFilters = true;
                        } else if (showFilters) {
                            showFilters = false;
                        }
                    }
                } else if (e.type == SDL_CONTROLLERBUTTONUP) {
                    if (e.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_UP) {
                        dpadUpPressed = false;
                    } else if (e.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN) {
                        dpadDownPressed = false;
                    }
                }
            }
        }

        // Handle button hold for D-Pad
        Uint32 currentTime = SDL_GetTicks();
        if (dpadUpPressed && (currentTime - lastButtonPressTime) >= buttonPressDelay) {
            lastButtonPressTime = currentTime;
            if (!showGames && !showFilters) {
                selectedConsole = (selectedConsole - 1 + consoles.size()) % consoles.size();
                scrollOffset = 0;
            } else if (showFilters) {
                selectedFilter = (selectedFilter - 1 + filters.size()) % filters.size();
                scrollOffset = 0;
            } else if (showGames) {
                selectedGame = (selectedGame - 1 + games.size()) % games.size();
                scrollOffset = 0;
            }
        } else if (dpadDownPressed && (currentTime - lastButtonPressTime) >= buttonPressDelay) {
            lastButtonPressTime = currentTime;
            if (!showGames && !showFilters) {
                selectedConsole = (selectedConsole + 1) % consoles.size();
                scrollOffset = 0;
            } else if (showFilters) {
                selectedFilter = (selectedFilter + 1) % filters.size();
                scrollOffset = 0;
            } else if (showGames) {
                selectedGame = (selectedGame + 1) % games.size();
                scrollOffset = 0;
            }
        }

        SDL_SetRenderDrawColor(renderer, currentTheme.backgroundColor.r, currentTheme.backgroundColor.g, currentTheme.backgroundColor.b, currentTheme.backgroundColor.a);
        SDL_RenderClear(renderer);

        const int maxItemsPerPage = 40;
        const int itemsPerRow = 20;
        const int columnWidth = 250;
        const int rowHeight = 30;
        const int leftSectionWidth = SCREEN_WIDTH / 2;
        const int rightSectionWidth = SCREEN_WIDTH / 2;
        const int borderThickness = 5;
        const int offset = 10;
        const int cornerRadius = 20;


        int currentPage = 0;
        if (!showGames && !showFilters) {
            currentPage = selectedConsole / maxItemsPerPage;
        } else if (showFilters) {
            currentPage = selectedFilter / maxItemsPerPage;
        } else if (showGames) {
            currentPage = selectedGame / maxItemsPerPage;
        }

        SDL_Rect leftBox = {offset, offset, leftSectionWidth - 2 * offset, SCREEN_HEIGHT - 2 * offset};
        DrawRoundedRect(renderer, leftBox, cornerRadius, borderThickness);

        if (!showGames && !showFilters) {
            for (size_t i = currentPage * maxItemsPerPage; i < consoles.size() && i < (currentPage + 1) * maxItemsPerPage; i++) {
                
                SDL_Color color = currentTheme.textColor;
                std::string displayText = shortenText(consoles[i].name, 20);
                if (i == selectedConsole) {
                    color = currentTheme.highlightColor;
                    displayText = scrollText(consoles[i].name, scrollOffset);
                }
                SDL_Surface* surface = TTF_RenderText_Solid(font, displayText.c_str(), color);
                SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

                int column = (i % maxItemsPerPage) / itemsPerRow;
                int row = (i % maxItemsPerPage) % itemsPerRow;

                SDL_Rect rect = {offset + 40 + column * columnWidth, offset + 40 + row * rowHeight, surface->w, surface->h};

                SDL_RenderCopy(renderer, texture, nullptr, &rect);
                SDL_FreeSurface(surface);
                SDL_DestroyTexture(texture);
            }
        } else if (showFilters) {
            for (size_t i = currentPage * maxItemsPerPage; i < filters.size() && i < (currentPage + 1) * maxItemsPerPage; i++) {
                SDL_Color color = currentTheme.textColor;
                std::string displayText = shortenText(filters[i].value, 20);
                if (i == selectedFilter) {
                    color = currentTheme.highlightColor;
                    displayText = scrollText(filters[i].value, scrollOffset);
                }
                SDL_Surface* surface = TTF_RenderText_Solid(font, displayText.c_str(), color);
                SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

                int column = (i % maxItemsPerPage) / itemsPerRow;
                int row = (i % maxItemsPerPage) % itemsPerRow;

                SDL_Rect rect = {offset + 40 + column * columnWidth, offset + 40 + row * rowHeight, surface->w, surface->h};

                SDL_RenderCopy(renderer, texture, nullptr, &rect);
                SDL_FreeSurface(surface);
                SDL_DestroyTexture(texture);
            }
        } else if (showGames) {
            for (size_t i = currentPage * maxItemsPerPage; i < games.size() && i < (currentPage + 1) * maxItemsPerPage; i++) {
                SDL_Color color = currentTheme.textColor;
                std::string displayText = shortenText(games[i].title, 20);
                if (i == selectedGame) {
                    color = currentTheme.highlightColor;
                    displayText = scrollText(games[i].title, scrollOffset);
                }
                SDL_Surface* surface = TTF_RenderText_Solid(font, displayText.c_str(), color);
                SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

                int column = (i % maxItemsPerPage) / itemsPerRow;
                int row = (i % maxItemsPerPage) % itemsPerRow;

                SDL_Rect rect = {offset + 40 + column * columnWidth, offset + 40 + row * rowHeight, surface->w, surface->h};


                SDL_RenderCopy(renderer, texture, nullptr, &rect);
                SDL_FreeSurface(surface);
                SDL_DestroyTexture(texture);
            }
        }

        SDL_Rect rightBox = {leftSectionWidth + offset, offset, rightSectionWidth - 2 * offset, SCREEN_HEIGHT - 2 * offset};
        DrawRoundedRect(renderer, rightBox, cornerRadius, borderThickness);

        SDL_Texture* imageTexture = nullptr;
        if (!showGames && !showFilters && selectedConsole < consoles.size()) {
            //Load console image
            imageTexture = IMG_LoadTexture(renderer, ("res/placeholder.png"));
        } else if (showGames && selectedGame < games.size()) {
            //Load game image
            imageTexture = IMG_LoadTexture(renderer, ("res/placeholder.png"));
        }

        if (imageTexture) {
            SDL_Rect imageRect = {leftSectionWidth + offset + 10, offset + 10, rightSectionWidth - 2 * offset - 20, SCREEN_HEIGHT - 2 * offset - 20};
            SDL_RenderCopy(renderer, imageTexture, nullptr, &imageRect);
            SDL_DestroyTexture(imageTexture);
        }

        if (isDownloading) {

            // Outline For Progress Box
            SDL_Rect fullBox = {leftSectionWidth + offset + 70, SCREEN_HEIGHT - offset - 60, (rightSectionWidth - 2 * offset - 140), 25};
            SDL_SetRenderDrawColor(renderer, currentTheme.highlightColor.r, currentTheme.highlightColor.g, currentTheme.highlightColor.b, currentTheme.highlightColor.a);
            SDL_RenderDrawRect(renderer, &fullBox);

            // Define Progress Bar
            SDL_Rect progressBar = {leftSectionWidth + offset + 70, SCREEN_HEIGHT - offset - 60, (rightSectionWidth - 2 * offset - 140) * downloadProgress / 100, 25};
            SDL_SetRenderDrawColor(renderer, currentTheme.progressBarColor.r, currentTheme.progressBarColor.g, currentTheme.progressBarColor.b, currentTheme.progressBarColor.a);
            SDL_RenderFillRect(renderer, &progressBar);

            SDL_Color color = currentTheme.textColor;
            SDL_Surface* textSurface = TTF_RenderText_Solid(font, queuedGameTitles[0].c_str(), color);
            SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
            int textWidth = textSurface->w;
            int textHeight = textSurface->h;
            SDL_FreeSurface(textSurface);
            SDL_Rect textRect = {leftSectionWidth + offset + 70, SCREEN_HEIGHT - offset - 90, textWidth, textHeight};
            SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
            SDL_DestroyTexture(textTexture);

            if (queuedGameTitles.size() > 1) {
                SDL_Surface* nextTextSurface = TTF_RenderText_Solid(font, ("Next: " + queuedGameTitles[1]).c_str(), color);
                SDL_Texture* nextTextTexture = SDL_CreateTextureFromSurface(renderer, nextTextSurface);
                int nextTextWidth = nextTextSurface->w;
                int nextTextHeight = nextTextSurface->h;
                SDL_FreeSurface(nextTextSurface);
                SDL_Rect nextTextRect = {leftSectionWidth + offset + 70, SCREEN_HEIGHT - offset - 120, nextTextWidth, nextTextHeight};
                SDL_RenderCopy(renderer, nextTextTexture, NULL, &nextTextRect);
                SDL_DestroyTexture(nextTextTexture);
            }
        }

        SDL_RenderPresent(renderer);
    }

    if (controller) {
        SDL_GameControllerClose(controller);
    }

    TTF_CloseFont(font);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}