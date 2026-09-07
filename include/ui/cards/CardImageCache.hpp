#pragma once
#include "cards/Card.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <mutex>
#include <optional>
#include <queue>
#include <raylib.h>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace openjoey::ui {
using cards::Card;
using cards::CardDatabase;

// Downloads card images from the configured remote provider in a background
// thread. Call Get() each frame for cards on screen; call PollAndLoad() each
// frame to upload newly-downloaded images to GPU.
// Single instance lives in AppContext and is shared across all screens.
//
// Threading contract: Get(), PollAndLoad(), SetMaxTextures() and the
// destructor are main-thread only; the worker touches only state guarded by
// mtx_.
//
// Robustness:
//   * a failed download may be re-requested only after kRetryCooldown
//     (requestDownload honors the cooldown), instead of hammering the network
//   * a failed disk->texture load is remembered and not retried every frame;
//     PollAndLoad() clears that memory when a fresh download completes
//   * once maxTextures_ is exceeded, least-recently-used textures are evicted
//     (unloaded on the main thread; they re-load from disk on demand)
class CardImageCache {
public:
    // Upper bound on textures held in VRAM (LRU-evicted beyond this).
    static constexpr std::size_t kDefaultMaxTextures = 512;
    // Wait after a failed download before it may be re-requested.
    static constexpr std::chrono::seconds kRetryCooldown{30};

    // Base image URLs are supplied by the app's configuration layer — no
    // provider defaults live in the cards domain.
    CardImageCache(std::filesystem::path imgDir,
                   std::string remoteImageUrl,
                   std::string remoteImageUrlSmall)
        : imgDir_(std::move(imgDir)),
          imageUrl_(std::move(remoteImageUrl)),
          imageUrlSmall_(std::move(remoteImageUrlSmall)) {
        // settings.json is player-editable: the URLs are interpolated into a
        // shell command, so only a well-formed https base is accepted.
        downloadsEnabled_ = urlIsSafe(imageUrl_) && urlIsSafe(imageUrlSmall_);
        if (!downloadsEnabled_)
            std::fprintf(stderr,
                         "[CardImageCache] unsafe image URL in settings — "
                         "downloads disabled\n");
        worker_ = std::thread(&CardImageCache::workerLoop, this);
    }

    ~CardImageCache() {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            stop_ = true;
        }
        cv_.notify_all();
        if (worker_.joinable()) worker_.join();

        std::error_code ec;
        while (!jobQueue_.empty()) {
            std::filesystem::remove(jobQueue_.front().dest.string() + ".tmp", ec);
            jobQueue_.pop();
        }
        if (inFlight_) // a curl started before stop_ may still have left a .tmp
            std::filesystem::remove(inFlight_->dest.string() + ".tmp", ec);
        for (auto& [id, tex] : textures_)
            if (tex.id != 0) UnloadTexture(tex);
    }

    CardImageCache(const CardImageCache&) = delete;
    CardImageCache& operator=(const CardImageCache&) = delete;

    // Main-thread only. Returns texture if ready, nullptr otherwise.
    // Automatically queues a download if the image is not on disk yet.
    const Texture2D* Get(const openjoey::cards::Card& card) {
        uint32_t id = card.id;
        if (id == 0) return nullptr;

        auto it = textures_.find(id);
        if (it != textures_.end()) {
            touch(id);
            return &it->second;
        }

        // Don't retry a failed disk load every frame; PollAndLoad() clears
        // the memory when a fresh download completes for this id.
        if (loadFailed_.count(id)) return nullptr;

        std::filesystem::path dest = imgDir_ / (std::to_string(id) + ".jpg");
        if (std::filesystem::exists(dest)) {
            if (loadTextureFromDisk(id, dest)) {
                evictIfNeeded();
                return &textures_.at(id);
            }
            loadFailed_.insert(id);
            return nullptr;
        }
        requestDownload(id, dest);
        return nullptr;
    }

    // Call once per frame from the main thread to upload completed downloads.
    void PollAndLoad() {
        std::vector<uint32_t> done;
        {
            std::lock_guard<std::mutex> lk(mtx_);
            done = std::exchange(completed_, {});
        }
        for (uint32_t id : done) {
            loadFailed_.erase(id); // fresh download: allow one new load attempt
            if (textures_.count(id)) continue;
            std::filesystem::path dest = imgDir_ / (std::to_string(id) + ".jpg");
            if (std::filesystem::exists(dest) && !loadTextureFromDisk(id, dest))
                loadFailed_.insert(id);
        }
        evictIfNeeded();
    }

    // Main-thread only. Bounds VRAM use: once the cache holds more than
    // `maxTextures`, least-recently-used textures are evicted.
    void SetMaxTextures(std::size_t maxTextures) { maxTextures_ = maxTextures; }

private:
    struct Job { uint32_t id; std::filesystem::path dest; };

    void touch(uint32_t id) { lastUse_[id] = ++useClock_; }

    // Main thread only: unloads the least-recently-used textures while the
    // cache is over budget. Evicted images re-load from disk on demand.
    void evictIfNeeded() {
        while (textures_.size() > maxTextures_ && !lastUse_.empty()) {
            auto victim = std::min_element(
                lastUse_.begin(), lastUse_.end(),
                [](const auto& a, const auto& b) { return a.second < b.second; });
            const uint32_t id = victim->first;
            auto texIt = textures_.find(id);
            if (texIt != textures_.end()) {
                if (texIt->second.id != 0) UnloadTexture(texIt->second);
                textures_.erase(texIt);
            }
            lastUse_.erase(victim);
            loadFailed_.erase(id);
        }
    }

    // Main thread only (needs the raylib GL context).
    bool loadTextureFromDisk(uint32_t id, const std::filesystem::path& dest) {
        Texture2D tex = LoadTexture(dest.string().c_str());
        if (tex.id == 0) return false;
        GenTextureMipmaps(&tex);
        SetTextureFilter(tex, TEXTURE_FILTER_TRILINEAR);
        textures_[id] = tex;
        touch(id);
        return true;
    }

    void requestDownload(uint32_t id, const std::filesystem::path& dest) {
        if (!downloadsEnabled_) return;
        std::lock_guard<std::mutex> lk(mtx_);
        if (queued_.count(id)) return;
        auto it = retryNotBefore_.find(id);
        if (it != retryNotBefore_.end() &&
            std::chrono::steady_clock::now() < it->second)
            return; // failed recently — wait out the cooldown
        queued_.insert(id);
        jobQueue_.push({id, dest});
        cv_.notify_one();
    }

    void workerLoop() {
        while (true) {
            Job job;
            {
                std::unique_lock<std::mutex> lk(mtx_);
                cv_.wait(lk, [this] { return stop_.load() || !jobQueue_.empty(); });
                if (stop_) return;
                job = std::move(jobQueue_.front());
                jobQueue_.pop();
                inFlight_ = job; // so the destructor can sweep its .tmp
            }
            bool ok = curlDownload(imageUrl_ + std::to_string(job.id) + ".jpg", job.dest);
            if (!ok && !stopping())
                ok = curlDownload(imageUrlSmall_ + std::to_string(job.id) + ".jpg", job.dest);
            {
                std::lock_guard<std::mutex> lk(mtx_);
                inFlight_.reset();
                queued_.erase(job.id); // failed ids may be re-requested later
                if (ok)
                    completed_.push_back(job.id);
                else
                    retryNotBefore_[job.id] =
                        std::chrono::steady_clock::now() + kRetryCooldown;
            }
        }
    }

    bool stopping() {
        std::lock_guard<std::mutex> lk(mtx_);
        return stop_.load();
    }

    static bool curlDownload(const std::string& url,
                             const std::filesystem::path& dest) {
        std::filesystem::create_directories(dest.parent_path());
        std::string tmp = dest.string() + ".tmp";
        // URL is constructed from a fixed base + integer ID — no injection risk.
        // POSIX-only: curl via the shell (documented; UI consumers run POSIX).
        std::string cmd = "curl -sSL --max-time 20 -o '" + tmp + "' '" + url + "' 2>/dev/null";
        int ret = std::system(cmd.c_str()); // NOLINT
        if (ret == 0 && std::filesystem::exists(tmp) &&
            std::filesystem::file_size(tmp) > 1024) {
            std::filesystem::rename(tmp, dest);
            return true;
        }
        std::error_code ec;
        std::filesystem::remove(tmp, ec);
        return false;
    }

    std::filesystem::path            imgDir_;
    std::string                      imageUrl_;
    std::string                      imageUrlSmall_;
    std::unordered_map<uint32_t, Texture2D> textures_;
    std::unordered_map<uint32_t, uint64_t>  lastUse_;   // main thread only
    std::unordered_set<uint32_t>     loadFailed_;        // main thread only
    std::size_t                      maxTextures_ = kDefaultMaxTextures;
    uint64_t                         useClock_    = 0;
    std::queue<Job>                  jobQueue_;
    std::optional<Job>               inFlight_; // guarded by mtx_
    std::vector<uint32_t>            completed_;
    std::unordered_set<uint32_t>     queued_;
    std::unordered_map<uint32_t, std::chrono::steady_clock::time_point> retryNotBefore_;
    std::mutex                       mtx_;
    std::condition_variable          cv_;
    std::atomic<bool>                stop_{false};
    bool                             downloadsEnabled_ = true;
    std::thread                      worker_;

    // Only a plain https base URL is safe to interpolate into a shell command.
    static bool urlIsSafe(const std::string& url) {
        if (url.rfind("https://", 0) != 0) return false;
        for (char c : url)
            if (c == '\'' || c == '"' || c == '\\' || c == ';' || c == '|' ||
                c == '&' || c == '$' || c == '`' || c == '(' || c == ')' ||
                c == '<' || c == '>' || c == '\n')
                return false;
        return true;
    }
};

} // namespace openjoey::ui
