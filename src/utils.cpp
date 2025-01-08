#include "utils.h"
#include "types.h"
#include <atomic>

std::unordered_map<std::string, std::string> systemToRomFolder = {
    {"Atari 2600", "ATARI2600"},
    {"Atari 5200", "ATARI5200"},
    {"Atari 7800", "ATARI7800"},
    {"Nintendo", "FC"}, // Assuming "Nintendo" refers to NES
    {"Master System", "MS"},
    {"TurboGrafx-16", "PCE"}, // TurboGrafx-16 is equivalent to PC Engine
    {"Genesis", "MD"}, // Genesis is equivalent to Mega Drive
    {"TurboGrafx-CD", "PCECD"}, // TurboGrafx-CD is equivalent to PC Engine CD-ROM
    {"Super Nintendo", "SFC"},
    {"Sega CD", "SEGACD"},
    {"Sega 32X", "SEGA32X"},
    {"Saturn", "SATURN"},
    {"PlayStation", "PS"},
    {"Nintendo 64", "N64"},
    {"Dreamcast", "DC"},
    {"Game Boy", "GB"}, // Assuming Game Boy is handled separately (add folder if known)
    {"Lynx", "LYNX"}, // Add specific ROM folder if known
    {"Game Gear", "GG"}, // Add specific ROM folder if known
    {"Virtual Boy", "VB"}, // Add specific ROM folder if known
    {"Game Boy Advance", "GBA"}, // Add specific ROM folder if known
    {"Nintendo DS", "NDS"}, // Add specific ROM folder if known
    {"PlayStation Portable", "PSP"} // Add specific ROM folder if known
};


typedef void* CURL;
typedef CURL* (*curl_easy_init_t)();
typedef void (*curl_easy_cleanup_t)(CURL*);
typedef int (*curl_easy_setopt_t)(CURL*, int, ...);
typedef int (*curl_easy_perform_t)(CURL*);
typedef const char* (*curl_easy_strerror_t)(int);
typedef struct curl_slist* (*curl_slist_append_t)(struct curl_slist*, const char*);

// curl_off_t
typedef long long curl_off_t;


extern std::atomic<int> downloadProgress;

int progressCallback(void* ptr, curl_off_t total, curl_off_t now, curl_off_t, curl_off_t) {
    if (total > 0) {
        downloadProgress = static_cast<int>((now * 100) / total);
    }
    return 0;
}


size_t header_callback(void* ptr, size_t size, size_t nmemb, std::string* filename) {
    std::string header((char*)ptr, size * nmemb);
    if (header.find("Content-Disposition:") != std::string::npos) {
        size_t pos = header.find("filename=\"");
        if (pos != std::string::npos) {
            size_t endPos = header.find("\"", pos + 10);
            if (endPos != std::string::npos) {
                *filename = header.substr(pos + 10, endPos - pos - 10);
            }
        }
    }
    return size * nmemb;
}

std::string getHtml(const std::string& url) {
    void* handle = dlopen("/usr/lib/libcurl.so.4", RTLD_LAZY);
    if (!handle) {
        std::cerr << "Failed to load libcurl: " << dlerror() << std::endl;
        return "1";
    }

    auto curl_easy_init = (curl_easy_init_t)dlsym(handle, "curl_easy_init");
    auto curl_easy_cleanup = (curl_easy_cleanup_t)dlsym(handle, "curl_easy_cleanup");
    auto curl_easy_setopt = (curl_easy_setopt_t)dlsym(handle, "curl_easy_setopt");
    auto curl_easy_perform = (curl_easy_perform_t)dlsym(handle, "curl_easy_perform");

    if (!curl_easy_init || !curl_easy_cleanup || !curl_easy_setopt || !curl_easy_perform) {
        std::cerr << "Failed to resolve libcurl functions." << std::endl;
        dlclose(handle);
        return "1";
    }

    CURL* curl = curl_easy_init();
    std::string html;
    if (curl) {
        curl_easy_setopt(curl, 10002 /* CURLOPT_URL */, url.c_str());
        curl_easy_setopt(curl, 64 /* CURLOPT_SSL_VERIFYPEER */, 0L); // Disable SSL verification
        curl_easy_setopt(curl, 81 /* CURLOPT_SSL_VERIFYHOST */, 0L); // Disable host verification
        curl_easy_setopt(curl, 10001 /* CURLOPT_WRITEDATA */, &html);
        curl_easy_setopt(curl, 20011 /* CURLOPT_WRITEFUNCTION */, +[](char* ptr, size_t size, size_t nmemb, std::string* data) {
            data->append(ptr, size * nmemb);
            return size * nmemb;
        });
        int res = curl_easy_perform(curl);
        if (res != 0) {
            std::cerr << "curl_easy_perform failed with error code: " << res << std::endl;
        }
        curl_easy_cleanup(curl);
        dlclose(handle);
    }

    return html;
}

std::vector<Console> parseHTML(const std::string& html) {
    std::vector<Console> consoles;

    htmlDocPtr doc = htmlReadMemory(html.c_str(), html.size(), NULL, NULL, HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
    if (doc == NULL) {
        std::cerr << "Failed to parse HTML" << std::endl;
        return consoles;
    }

    xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);
    if (xpathCtx == NULL) {
        std::cerr << "Failed to create XPath context" << std::endl;
        xmlFreeDoc(doc);
        return consoles;
    }

    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression((xmlChar*)"//div[@style='display:flex; justify-content:center; align-items:flex-start; flex-wrap:wrap; gap:15px; margin:auto']//a", xpathCtx);
    if (xpathObj == NULL) {
        std::cerr << "Failed to evaluate XPath expression" << std::endl;
        xmlXPathFreeContext(xpathCtx);
        xmlFreeDoc(doc);
        return consoles;
    }

    xmlNodeSetPtr nodes = xpathObj->nodesetval;
    for (int i = 0; i < nodes->nodeNr; ++i) {
        xmlNodePtr node = nodes->nodeTab[i];
        xmlChar* href = xmlGetProp(node, (xmlChar*)"href");
        xmlChar* content = xmlNodeGetContent(node);
        if (href && content) {
            consoles.push_back({(char*)content, (char*)href});
        }
        xmlFree(href);
        xmlFree(content);
    }

    xmlXPathFreeObject(xpathObj);
    xmlXPathFreeContext(xpathCtx);
    xmlFreeDoc(doc);

    int i = 0;
    for (const auto& console : consoles) {
        i++;
        std::cout << console.name << " - " << i << std::endl;
    }

    return consoles;
}

std::vector<Game> parseGamesHTML(const std::string &htmlContent) {
    std::vector<Game> games;

    htmlDocPtr doc = htmlReadMemory(htmlContent.c_str(), htmlContent.size(), nullptr, nullptr, HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
    if (!doc) {
        std::cerr << "Error: unable to parse HTML document\n";
        return games;
    }

    xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);
    if (!xpathCtx) {
        std::cerr << "Error: unable to create XPath context\n";
        xmlFreeDoc(doc);
        return games;
    }

    xmlXPathObjectPtr result = xmlXPathEvalExpression(reinterpret_cast<const xmlChar*>("//tr[td/a]"), xpathCtx);
    if (!result) {
        std::cerr << "Error: unable to evaluate XPath expression\n";
        xmlXPathFreeContext(xpathCtx);
        xmlFreeDoc(doc);
        return games;
    }

    for (int i = 0; i < result->nodesetval->nodeNr; ++i) {
        xmlNodePtr row = result->nodesetval->nodeTab[i];
        Game game;

        xmlNodePtr titleNode = row->children;
        while (titleNode && titleNode->type != XML_ELEMENT_NODE) {
            titleNode = titleNode->next;
        }

        if (titleNode && xmlStrEqual(titleNode->name, reinterpret_cast<const xmlChar*>("td"))) {
            xmlNodePtr aNode = titleNode->children;
            while (aNode && aNode->type != XML_ELEMENT_NODE) {
                aNode = aNode->next;
            }
            if (aNode && xmlStrEqual(aNode->name, reinterpret_cast<const xmlChar*>("a"))) {
                game.title = reinterpret_cast<const char*>(xmlNodeGetContent(aNode));
                xmlChar* href = xmlGetProp(aNode, reinterpret_cast<const xmlChar*>("href"));
                if (href) {
                    game.url = reinterpret_cast<const char*>(href);
                    xmlFree(href);
                }
            }
        }

        xmlNodePtr regionNode = titleNode->next;
        if (regionNode && regionNode->type == XML_ELEMENT_NODE && xmlStrEqual(regionNode->name, reinterpret_cast<const xmlChar*>("td"))) {
            xmlNodePtr imgNode = regionNode->children;
            if (imgNode && imgNode->type == XML_ELEMENT_NODE && xmlStrEqual(imgNode->name, reinterpret_cast<const xmlChar*>("img"))) {
                game.region = reinterpret_cast<const char*>(xmlGetProp(imgNode, reinterpret_cast<const xmlChar*>("title")));
            }
        }

        xmlNodePtr versionNode = regionNode->next;
        if (versionNode && versionNode->type == XML_ELEMENT_NODE && xmlStrEqual(versionNode->name, reinterpret_cast<const xmlChar*>("td"))) {
            game.version = reinterpret_cast<const char*>(xmlNodeGetContent(versionNode));
        }

        xmlNodePtr languagesNode = versionNode->next;
        if (languagesNode && languagesNode->type == XML_ELEMENT_NODE && xmlStrEqual(languagesNode->name, reinterpret_cast<const xmlChar*>("td"))) {
            game.languages = reinterpret_cast<const char*>(xmlNodeGetContent(languagesNode));
        }

        xmlNodePtr ratingNode = languagesNode->next;
        if (ratingNode && ratingNode->type == XML_ELEMENT_NODE && xmlStrEqual(ratingNode->name, reinterpret_cast<const xmlChar*>("td"))) {
            xmlNodePtr aNode = ratingNode->children;
            if (aNode && aNode->type == XML_ELEMENT_NODE && xmlStrEqual(aNode->name, reinterpret_cast<const xmlChar*>("a"))) {
                game.rating = reinterpret_cast<const char*>(xmlNodeGetContent(aNode));
            }
        }

        games.push_back(game);
    }

    xmlXPathFreeObject(result);
    xmlXPathFreeContext(xpathCtx);
    xmlFreeDoc(doc);

    return games;
}

int downloadGame(std::string console, const std::string &htmlContent) {
    xmlInitParser();
    LIBXML_TEST_VERSION

    htmlDocPtr doc = htmlReadMemory(htmlContent.c_str(), htmlContent.size(), nullptr, nullptr, HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
    if (doc == nullptr) {
        std::cerr << "Failed to parse HTML" << std::endl;
        return -1;
    }

    xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);
    if (xpathCtx == nullptr) {
        std::cerr << "Failed to create XPath context" << std::endl;
        xmlFreeDoc(doc);
        return -1;
    }

    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression((const xmlChar*)"//form[@id='dl_form']/input[@name='mediaId']", xpathCtx);
    if (xpathObj == nullptr) {
        std::cerr << "Failed to evaluate XPath expression" << std::endl;
        xmlXPathFreeContext(xpathCtx);
        xmlFreeDoc(doc);
        return -1;
    }

    std::string mediaId;
    if (xpathObj->nodesetval && xpathObj->nodesetval->nodeNr > 0) {
        xmlNodePtr node = xpathObj->nodesetval->nodeTab[0];
        xmlChar* value = xmlGetProp(node, (const xmlChar*)"value");
        if (value) {
            mediaId = (const char*)value;
            xmlFree(value);
        }
    }

    xmlXPathFreeObject(xpathObj);
    xmlXPathFreeContext(xpathCtx);
    xmlFreeDoc(doc);
    xmlCleanupParser();

    std::cout << "Extracted mediaId: " << mediaId << std::endl;

    auto it = systemToRomFolder.find(console);
    if (it == systemToRomFolder.end()) {
        std::cerr << "Unsupported console: " << console << std::endl;
        return -1;
    }
    std::string romFolder = it->second;

    std::string downloadUrl = "https://download2.vimm.net/?mediaId=" + mediaId;
    std::string outputPath = "/mnt/SDCARD/Roms/" + romFolder + "/" + mediaId + ".zip";

    void* handle = dlopen("/usr/lib/libcurl.so.4", RTLD_LAZY);
    if (!handle) {
        std::cerr << "Failed to load libcurl: " << dlerror() << std::endl;
        return -1;
    }

    auto curl_easy_init = (curl_easy_init_t)dlsym(handle, "curl_easy_init");
    auto curl_easy_cleanup = (curl_easy_cleanup_t)dlsym(handle, "curl_easy_cleanup");
    auto curl_easy_setopt = (curl_easy_setopt_t)dlsym(handle, "curl_easy_setopt");
    auto curl_easy_perform = (curl_easy_perform_t)dlsym(handle, "curl_easy_perform");
    auto curl_easy_strerror = (curl_easy_strerror_t)dlsym(handle, "curl_easy_strerror");
    auto curl_slist_append = (curl_slist_append_t)dlsym(handle, "curl_slist_append");

    if (!curl_easy_init || !curl_easy_cleanup || !curl_easy_setopt || !curl_easy_perform) {
        std::cerr << "Failed to resolve libcurl functions." << std::endl;
        dlclose(handle);
        return -1;
    }

    CURL* curl = curl_easy_init();
    if (curl) {
        FILE* fp = fopen(outputPath.c_str(), "wb");
        if (!fp) {
            std::cerr << "Failed to open file for writing: " << outputPath << std::endl;
            curl_easy_cleanup(curl);
            dlclose(handle);
            return -1;
        }

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "GET /?mediaId=33028 HTTP/1.1");
        headers = curl_slist_append(headers, "Host: download2.vimm.net");
        headers = curl_slist_append(headers, "Sec-Ch-Ua: \"Not;A=Brand\";v=\"24\", \"Chromium\";v=\"128\"");
        headers = curl_slist_append(headers, "Sec-Ch-Ua-Mobile: ?0");
        headers = curl_slist_append(headers, "Sec-Ch-Ua-Platform: \"Windows\"");
        headers = curl_slist_append(headers, "Accept-Language: en-GB,en;q=0.9");
        headers = curl_slist_append(headers, "Upgrade-Insecure-Requests: 1");
        headers = curl_slist_append(headers, "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/128.0.6613.120 Safari/537.36");
        headers = curl_slist_append(headers, "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7");
        headers = curl_slist_append(headers, "Sec-Fetch-Site: same-site");
        headers = curl_slist_append(headers, "Sec-Fetch-Mode: navigate");
        headers = curl_slist_append(headers, "Sec-Fetch-User: ?1");
        headers = curl_slist_append(headers, "Sec-Fetch-Dest: document");
        headers = curl_slist_append(headers, "Referer: https://vimm.net/vault/40297");
        headers = curl_slist_append(headers, "Accept-Encoding: gzip, deflate, br");
        headers = curl_slist_append(headers, "Priority: u=0, i");
        headers = curl_slist_append(headers, "Connection: keep-alive");

        curl_easy_setopt(curl, 10023, headers);
        curl_easy_setopt(curl, 10002, downloadUrl.c_str());
        curl_easy_setopt(curl, 64, 0L); // Disable SSL verification
        curl_easy_setopt(curl, 81, 0L); // Disable host verification
        curl_easy_setopt(curl, 10001, fp);
        curl_easy_setopt(curl, 20011, NULL);
        curl_easy_setopt(curl, 10029, &outputPath); // WRITEHHEADER
        
        curl_easy_setopt(curl, 20219, progressCallback);
        curl_easy_setopt(curl, 43, 0L);


        std::string filename;
        curl_easy_setopt(curl, 20079, header_callback);
        curl_easy_setopt(curl, 10029, &filename);

        int res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        fclose(fp);
        dlclose(handle);

        if (!filename.empty()) {
            std::string newOutputPath = "/mnt/SDCARD/Roms/" + romFolder + "/" + filename;
            if (rename(outputPath.c_str(), newOutputPath.c_str()) == 0) {
                std::cout << "File renamed to: " << filename << std::endl;
            } else {
                std::cerr << "Failed to rename file: " << strerror(errno) << std::endl;
            }
        }

        if (res != 0) {
            std::cerr << "Failed to download game: " << curl_easy_strerror(res) << std::endl;
            return -1;
        } else {
            std::cout << "Game downloaded successfully to " << outputPath << std::endl;
        }
        
    } else {
        dlclose(handle);
        std::cerr << "Failed to initialize curl" << std::endl;
        return -1;
    }

    return 0;
}