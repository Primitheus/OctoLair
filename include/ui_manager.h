#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <string>
#include <vector>
#include "renderer.h"
#include "types.h"

class UIManager {
public:
    UIManager(Renderer& renderer);
    ~UIManager();
    void drawConsoleList(const std::vector<Console>& consoles, int selectedConsole, int scrollOffset);
    void drawFilterList(const std::vector<Filter>& filters, int selectedFilter, int scrollOffset);
    void drawGameList(const std::vector<Game>& games, int selectedGame, int scrollOffset);
    void drawProgressBar(int progress, const std::string& title, const std::vector<std::string>& queuedTitles);

private:
    Renderer& renderer;
    std::string shortenText(const std::string& text, int maxLength);
    std::string scrollText(const std::string& text, int offset);
};

#endif // UI_MANAGER_H