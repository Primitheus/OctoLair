#include "theme_manager.h"

Theme ThemeManager::darkTheme = {
    {0, 0, 0, 255},       // backgroundColor
    {255, 255, 255, 255}, // textColor
    {255, 0, 0, 255},     // highlightColor
    {255, 0, 0, 255},     // borderColor
    {255, 255, 255, 255}  // progressBarColor
};

Theme ThemeManager::lightTheme = {
    {255, 255, 255, 255}, // backgroundColor
    {0, 0, 0, 255},       // textColor
    {0, 0, 255, 255},     // highlightColor
    {0, 0, 255, 255},     // borderColor
    {0, 0, 0, 255}        // progressBarColor
};

Theme ThemeManager::purpleTheme = {
    {0, 0, 0, 255},       // backgroundColor
    {180, 157, 250, 255}, // textColor
    {113, 66, 255, 255},  // highlightColor
    {113, 66, 255, 255},  // borderColor
    {113, 66, 255, 255}   // progressBarColor
};



void ThemeManager::applyTheme(const Theme& theme) {
    currentTheme = theme;
}