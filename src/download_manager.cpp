#include "download_manager.h"
#include "utils.h"
#include <iostream>

DownloadManager::DownloadManager() : downloadProgress(0), isDownloading(false), stopThread(false) {
    std::cout << "DownloadManager initialized" << std::endl;
    queueThread = std::thread(&DownloadManager::processDownloadQueue, this);
}

DownloadManager::~DownloadManager() {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        stopThread = true;
    }
    queueCV.notify_one();
    if (queueThread.joinable()) {
        queueThread.join();
    }
    std::cout << "DownloadManager destroyed" << std::endl;
}

void DownloadManager::queueDownload(const std::string& console, const std::string& url, const std::string& gameTitle) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        std::cout << "Queueing download: " << gameTitle << " from " << url << std::endl;
        downloadQueue.push({console, url});
        queuedGameTitles.push_back(gameTitle);
    }
    queueCV.notify_one();
}

void DownloadManager::start() {
    queueThread.detach();
}

void DownloadManager::downloadGameThread(const std::string& console, const std::string& url, const std::string& gameTitle) {
    std::cout << "Starting download: " << gameTitle << std::endl;
    std::cout << "Console: " << console << ", URL: " << url << std::endl;
    isDownloading = true;
    downloadProgress = 0;

    std::string htmlContent = getHtml(url);
    if (htmlContent.empty()) {
        std::cerr << "Failed to fetch HTML content from URL: " << url << std::endl;
        isDownloading = false;
        return;
    }

    int res = downloadGame(console, htmlContent);

    if (res == 0) {
        std::cout << "Game downloaded successfully: " << gameTitle << std::endl;
    } else {
        std::cerr << "Failed to download game: " << gameTitle << std::endl;
    }

    isDownloading = false;
    queueCV.notify_one();
}

void DownloadManager::processDownloadQueue() {
    std::cout << "Processing download queue" << std::endl;
    while (true) {
        std::unique_lock<std::mutex> lock(queueMutex);
        queueCV.wait(lock, [this] { return !downloadQueue.empty() || stopThread; });

        if (stopThread && downloadQueue.empty()) {
            break;
        }

        if (!downloadQueue.empty()) {
            auto [console, url] = downloadQueue.front();
            downloadQueue.pop();
            std::string gameTitle = queuedGameTitles.front();
            queuedGameTitles.erase(queuedGameTitles.begin());
            lock.unlock();

            std::cout << "Dequeued download: " << gameTitle << " from " << url << std::endl;
            downloadGameThread(console, url, gameTitle);
        }
    }
    std::cout << "Download queue processing stopped" << std::endl;
}

std::string DownloadManager::getFirstQueuedGameTitle() {
    std::lock_guard<std::mutex> lock(queueMutex);
    return !queuedGameTitles.empty() ? queuedGameTitles[0] : "";
}

std::vector<std::string> DownloadManager::getQueuedGameTitles() {
    std::lock_guard<std::mutex> lock(queueMutex);
    return queuedGameTitles;
}