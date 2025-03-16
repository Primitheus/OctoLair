#ifndef THEME_MANAGER_H
#define THEME_MANAGER_H

#include "theme.h"

class ThemeManager {
public:
    static void applyTheme(const Theme& theme);
    static Theme darkTheme;
    static Theme lightTheme;
    static Theme purpleTheme;
};

#endif // THEME_MANAGER_H