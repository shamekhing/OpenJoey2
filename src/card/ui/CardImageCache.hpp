#pragma once
#include "card/Card.hpp"
#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <filesystem>
#include <mutex>
#include <queue>
#include <raylib.h>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace openjoey::ui {

// Downloads card images from YGOProdeck in a background thread.
// Call Get() each frame for cards on screen; call PollAndLoad() each frame
// to upload newly-downloaded images to GPU.
// Single instance lives in AppContext and is shared across all screens.
class CardImageCache {
public:
        CardImageCache(std::filesystem::path imgDir,
                   std::string ygoprodeckUrl = "https://images.ygoprodeck.com/images/cards/",
                   std::string ygoprodeckUrlSmall = "https://images.ygoprodeck.com/images/cards_small/")
        : imgDir_(std::move(imgDir)),
          baseUrl_(std::move(ygoprodeckUrl)),
          baseUrlSmall_(std::move(ygoprodeckUrlSmall)) {
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
        for (auto& [id, tex] : textures_)
            if (tex.id != 0) UnloadTexture(tex);
    }

    CardImageCache(const CardImageCache&) = delete;
    CardImageCache& operator=(const CardImageCache&) = delete;

    // Main-thread only. Returns texture if ready, nullptr otherwise.
    // Automatically queues a download if the image is not on disk yet.
    const Texture2D* Get(const openjoey::Card& card) {
        uint32_t id = card.imageId ? card.imageId : card.cardId;
        if (id == 0) return nullptr;

        auto it = textures_.find(id);
        if (it != textures_.end()) return &it->second;

        std::filesystem::path dest = imgDir_ / (std::to_string(id) + ".jpg");
        if (std::filesystem::exists(dest)) {
            Texture2D tex = LoadTexture(dest.string().c_str());
            if (tex.id != 0) {
                GenTextureMipmaps(&tex);
                SetTextureFilter(tex, TEXTURE_FILTER_TRILINEAR);
                textures_[id] = tex;
                return &textures_[id];
            }
        } else {
            requestDownload(id, dest);
        }
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
            if (textures_.count(id)) continue;
            std::filesystem::path dest = imgDir_ / (std::to_string(id) + ".jpg");
            if (std::filesystem::exists(dest)) {
                Texture2D tex = LoadTexture(dest.string().c_str());
                if (tex.id != 0) {
                    GenTextureMipmaps(&tex);
                    SetTextureFilter(tex, TEXTURE_FILTER_TRILINEAR);
                    textures_[id] = tex;
                }
            }
        }
    }

private:
    struct Job { uint32_t id; std::filesystem::path dest; };

    void requestDownload(uint32_t id, const std::filesystem::path& dest) {
        std::lock_guard<std::mutex> lk(mtx_);
        if (queued_.count(id)) return;
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
            }
                        bool ok = curlDownload(baseUrl_ + std::to_string(job.id) + ".jpg", job.dest);
            if (!ok)
                ok = curlDownload(baseUrlSmall_ + std::to_string(job.id) + ".jpg", job.dest);
            if (ok) {
                std::lock_guard<std::mutex> lk(mtx_);
                completed_.push_back(job.id);
            }
        }
    }

    static bool curlDownload(const std::string& url,
                             const std::filesystem::path& dest) {
        std::filesystem::create_directories(dest.parent_path());
        std::string tmp = dest.string() + ".tmp";
        // URL is constructed from a fixed base + integer ID — no injection risk.
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
    std::string                      baseUrl_;
    std::string                      baseUrlSmall_;
    std::unordered_map<uint32_t, Texture2D> textures_;
    std::queue<Job>                  jobQueue_;
    std::vector<uint32_t>            completed_;
    std::unordered_set<uint32_t>     queued_;
    std::mutex                       mtx_;
    std::condition_variable          cv_;
    std::atomic<bool>                stop_{false};
    std::thread                      worker_;
};

} // namespace openjoey::ui
