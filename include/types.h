#ifndef TYPES_H
#define TYPES_H

#include <string>
#include <unordered_map>

struct Game {
    std::string title;
    std::string region;
    std::string version;
    std::string languages;
    std::string rating;
    std::string url;
};

struct Console {
    std::string name;
    std::string url;
};

class Filter {
public:
    std::string value;
    Filter(const std::string& val) : value(val) {}
};



#endif // TYPES_H