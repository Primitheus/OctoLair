#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <iostream>
#include <vector>
#include <string>
#include "theme_manager.h"
#include "renderer.h"
#include "download_manager.h"
#include "game_controller.h"
#include "ui_manager.h"
#include "types.h"
#include "utils.h"
#include "config.h"


std::atomic<int> downloadProgress(0);
std::atomic<bool> isDownloading(false);

std::queue<std::pair<std::string, std::string>> downloadQueue;
std::vector<std::string> queuedGameTitles;

std::mutex queueMutex;
std::condition_variable queueCV;

bool isUnzipping = false;


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

int unzipGamesThread() {

    isUnzipping = true;

    int res = unzipGames("PlayStation");
    if (res == 0) {
        std::cout << "PlayStation Games Extracted Successfully" << std::endl;
    } else {
        std::cerr << "Failed to unzip one or more games" << std::endl;
    }

    res = unzipGames("PlayStation Portable");
    if (res == 0) {
        std::cout << "PSP Games Extracted Successfully" << std::endl;
    } else {
        std::cerr << "Failed to unzip one or more games" << std::endl;
    }

    isUnzipping = false;
    return 0;
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

    ThemeManager::applyTheme(ThemeManager::purpleTheme);

    std::string htmlConsoles = getHtml("https://vimm.net/vault");
    std::vector<Console> consoles = parseHTML(htmlConsoles);

    
    Renderer renderer;
    if (!renderer.initialize()) {
        std::cerr << "Renderer initialization failed" << std::endl;
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    GameController gameController;
    if (!gameController.initialize()) {
        std::cerr << "GameController initialization failed" << std::endl;
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    UIManager uiManager(renderer);
    std::vector<Filter> filters = {
        Filter("#"), Filter("A"), Filter("B"), Filter("C"), Filter("D"), Filter("E"), Filter("F"), Filter("G"), Filter("H"), Filter("I"),
        Filter("J"), Filter("K"), Filter("L"), Filter("M"), Filter("N"), Filter("O"), Filter("P"), Filter("Q"), Filter("R"), Filter("S"),
        Filter("T"), Filter("U"), Filter("V"), Filter("W"), Filter("X"), Filter("Y"), Filter("Z")
    };

    std::thread queueThread(processDownloadQueue);
    queueThread.detach();

    SDL_Event e;
    bool quit = false;
    int selectedConsole = 0;
    int selectedGame = 0;
    int selectedFilter = 0;
    std::vector<Game> games;
    bool showGames = false;
    bool showFilters = false;
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
                    } else if (e.cbutton.button == 4) {
                        // Unzipping PS games

                        std::thread unzipThread(unzipGamesThread, std::ref(renderer));
                        unzipThread.detach();

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
        renderer.clear();
        const int leftSectionWidth = SCREEN_WIDTH / 2;
        const int rightSectionWidth = SCREEN_WIDTH / 2;
        const int offset = 10;
        const int cornerRadius = 20;
        const int borderThickness = 5;
        SDL_Rect leftBox = {offset, offset, leftSectionWidth - 2 * offset, SCREEN_HEIGHT - 2 * offset};
        renderer.drawRoundedRect(leftBox, cornerRadius, borderThickness);
        if (!showGames && !showFilters) {
            uiManager.drawConsoleList(consoles, selectedConsole, scrollOffset);
        } else if (showFilters) {
            uiManager.drawFilterList(filters, selectedFilter, scrollOffset);
        } else if (showGames) {
            uiManager.drawGameList(games, selectedGame, scrollOffset);
        }
        SDL_Rect rightBox = {leftSectionWidth + offset, offset, rightSectionWidth - 2 * offset, SCREEN_HEIGHT - 2 * offset};
        renderer.drawRoundedRect(rightBox, cornerRadius, borderThickness);
        if (!showGames && !showFilters && selectedConsole < consoles.size()) {
            renderer.drawImage("res/placeholder.png", {leftSectionWidth + offset + 10, offset + 10, rightSectionWidth - 2 * offset - 20, SCREEN_HEIGHT - 2 * offset - 20});
        } else if (showGames && selectedGame < games.size()) {
            renderer.drawImage("res/placeholder.png", {leftSectionWidth + offset + 10, offset + 10, rightSectionWidth - 2 * offset - 20, SCREEN_HEIGHT - 2 * offset - 20});
        }
        if (isDownloading) {
            uiManager.drawProgressBar(downloadProgress, queuedGameTitles[0], queuedGameTitles);
        }

        if (isUnzipping) {
            renderer.drawMessageBox("Extracting Games from PSP and PS Folders");
        }

        renderer.present();
    
    } 

    std::cout << "Cleaning up..." << std::endl;

    std::cout << "Exiting..." << std::endl;


    return 0;
}