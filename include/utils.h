#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <iostream>
#include <dlfcn.h>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#include <regex>
#include <fstream>

#include "types.h"

size_t header_callback(void* ptr, size_t size, size_t nmemb, std::string* filename);
std::string getHtml(const std::string& url);
std::vector<Console> parseHTML(const std::string& html);
std::vector<Game> parseGamesHTML(const std::string &htmlContent);
int downloadGame(std::string console, const std::string &htmlContent);

#endif







