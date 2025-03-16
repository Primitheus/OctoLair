#ifndef DOWNLOAD_MANAGER_H
#define DOWNLOAD_MANAGER_H

#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>
#include <vector>
#include <thread>

class DownloadManager {
public:
    DownloadManager();
    ~DownloadManager();

    void queueDownload(const std::string& console, const std::string& url, const std::string& gameTitle);
    void start();

    std::atomic<int> downloadProgress;
    std::atomic<bool> isDownloading;

    std::string getFirstQueuedGameTitle();
    std::vector<std::string> getQueuedGameTitles();

private:
    void downloadGameThread(const std::string& console, const std::string& url, const std::string& gameTitle);
    void processDownloadQueue();

    std::queue<std::pair<std::string, std::string>> downloadQueue;
    std::vector<std::string> queuedGameTitles;
    std::mutex queueMutex;
    std::condition_variable queueCV;
    std::thread queueThread;
    bool stopThread;
};

#endif // DOWNLOAD_MANAGER_H