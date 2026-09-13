#pragma once
#include <raylib.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <filesystem>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "cards/Card.hpp"

namespace openjoey::ui {

class CardImageCache {
   public:
    static constexpr std::size_t kDefaultMaxTextures = 512;
    static constexpr std::chrono::seconds kRetryCooldown{30};

    CardImageCache(std::filesystem::path imgDir, std::string remoteImageUrl, std::string remoteImageUrlSmall, bool allowDownloads = true);
    ~CardImageCache();

    CardImageCache(const CardImageCache&) = delete;
    CardImageCache& operator=(const CardImageCache&) = delete;

    const Texture2D* Get(const openjoey::cards::Card& card);
    void PollAndLoad();
    void SetMaxTextures(std::size_t maxTextures);

   private:
    struct Job {
        uint32_t id;
        std::filesystem::path dest;
    };

    void touch(uint32_t id);
    void evictIfNeeded();
    bool loadTextureFromDisk(uint32_t id, const std::filesystem::path& dest);
    void requestDownload(uint32_t id, const std::filesystem::path& dest);
    void workerLoop();
    bool stopping();
    static bool curlDownload(const std::string& url, const std::filesystem::path& dest);
    static bool shellUnsafeChar(char c);
    static bool urlIsSafe(const std::string& url);
    static bool pathIsSafe(const std::filesystem::path& p);

    std::filesystem::path imgDir_;
    std::string imageUrl_;
    std::string imageUrlSmall_;
    std::unordered_map<uint32_t, Texture2D> textures_;
    std::unordered_map<uint32_t, uint64_t> lastUse_;
    std::unordered_set<uint32_t> loadFailed_;
    std::size_t maxTextures_ = kDefaultMaxTextures;
    uint64_t useClock_ = 0;
    std::queue<Job> jobQueue_;
    std::optional<Job> inFlight_;
    std::vector<uint32_t> completed_;
    std::unordered_set<uint32_t> queued_;
    std::unordered_map<uint32_t, std::chrono::steady_clock::time_point> retryNotBefore_;
    std::mutex mtx_;
    std::condition_variable cv_;
    std::atomic<bool> stop_{false};
    bool downloadsEnabled_ = true;
    std::thread worker_;
};

}  // namespace openjoey::ui
