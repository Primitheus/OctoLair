#include "ui_manager.h"

UIManager::UIManager(Renderer& renderer) : renderer(renderer) {}

UIManager::~UIManager() {

    
}

void UIManager::drawConsoleList(const std::vector<Console>& consoles, int selectedConsole, int scrollOffset) {
    const int maxItemsPerPage = 40;
    const int itemsPerRow = 20;
    const int columnWidth = 250;
    const int rowHeight = 30;
    const int offset = 10;

    int currentPage = selectedConsole / maxItemsPerPage;

    for (size_t i = currentPage * maxItemsPerPage; i < consoles.size() && i < (currentPage + 1) * maxItemsPerPage; i++) {
        SDL_Color color = currentTheme.textColor;
        std::string displayText = shortenText(consoles[i].name, 20);
        if (i == selectedConsole) {
            color = currentTheme.highlightColor;
            displayText = scrollText(consoles[i].name, scrollOffset);
        }
        int column = (i % maxItemsPerPage) / itemsPerRow;
        int row = (i % maxItemsPerPage) % itemsPerRow;
        renderer.drawText(displayText, offset + 40 + column * columnWidth, offset + 40 + row * rowHeight, color);
    }
}

void UIManager::drawFilterList(const std::vector<Filter>& filters, int selectedFilter, int scrollOffset) {
    const int maxItemsPerPage = 40;
    const int itemsPerRow = 20;
    const int columnWidth = 250;
    const int rowHeight = 30;
    const int offset = 10;

    int currentPage = selectedFilter / maxItemsPerPage;

    for (size_t i = currentPage * maxItemsPerPage; i < filters.size() && i < (currentPage + 1) * maxItemsPerPage; i++) {
        SDL_Color color = currentTheme.textColor;
        std::string displayText = shortenText(filters[i].value, 20);
        if (i == selectedFilter) {
            color = currentTheme.highlightColor;
            displayText = scrollText(filters[i].value, scrollOffset);
        }
        int column = (i % maxItemsPerPage) / itemsPerRow;
        int row = (i % maxItemsPerPage) % itemsPerRow;
        renderer.drawText(displayText, offset + 40 + column * columnWidth, offset + 40 + row * rowHeight, color);
    }
}

void UIManager::drawGameList(const std::vector<Game>& games, int selectedGame, int scrollOffset) {
    const int maxItemsPerPage = 40;
    const int itemsPerRow = 20;
    const int columnWidth = 250;
    const int rowHeight = 30;
    const int offset = 10;

    int currentPage = selectedGame / maxItemsPerPage;

    for (size_t i = currentPage * maxItemsPerPage; i < games.size() && i < (currentPage + 1) * maxItemsPerPage; i++) {
        SDL_Color color = currentTheme.textColor;
        std::string displayText = shortenText(games[i].title, 20);
        if (i == selectedGame) {
            color = currentTheme.highlightColor;
            displayText = scrollText(games[i].title, scrollOffset);
        }
        int column = (i % maxItemsPerPage) / itemsPerRow;
        int row = (i % maxItemsPerPage) % itemsPerRow;
        renderer.drawText(displayText, offset + 40 + column * columnWidth, offset + 40 + row * rowHeight, color);
    }
}

void UIManager::drawProgressBar(int progress, const std::string& title, const std::vector<std::string>& queuedTitles) {
    renderer.drawProgressBar(progress, title, queuedTitles);
}

std::string UIManager::shortenText(const std::string& text, int maxLength) {
    if (text.length() > maxLength) {
        return text.substr(0, maxLength - 3) + "...";
    }
    return text;
}

std::string UIManager::scrollText(const std::string& text, int offset) {
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