#include "ui/cards/CardImageCache.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <utility>

namespace openjoey::ui {

CardImageCache::CardImageCache(std::filesystem::path imgDir, std::string remoteImageUrl, std::string remoteImageUrlSmall, bool allowDownloads)
    : imgDir_(std::move(imgDir)), imageUrl_(std::move(remoteImageUrl)), imageUrlSmall_(std::move(remoteImageUrlSmall)) {
#ifdef __EMSCRIPTEN__
    (void)allowDownloads;
    downloadsEnabled_ = false;
#else
    downloadsEnabled_ = allowDownloads && urlIsSafe(imageUrl_) && urlIsSafe(imageUrlSmall_) && pathIsSafe(imgDir_);
    if (!downloadsEnabled_)
        std::fprintf(stderr, "[CardImageCache] unsafe download settings — downloads disabled\n");
    worker_ = std::thread(&CardImageCache::workerLoop, this);
#endif
}

CardImageCache::~CardImageCache() {
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
    if (inFlight_) std::filesystem::remove(inFlight_->dest.string() + ".tmp", ec);
    for (auto& [id, tex] : textures_)
        if (tex.id != 0) UnloadTexture(tex);
}

const Texture2D* CardImageCache::Get(const openjoey::cards::Card& card) {
    uint32_t id = card.id;
    if (id == 0) return nullptr;

    auto it = textures_.find(id);
    if (it != textures_.end()) {
        touch(id);
        return &it->second;
    }

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

void CardImageCache::PollAndLoad() {
    std::vector<uint32_t> done;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        done = std::exchange(completed_, {});
    }
    for (uint32_t id : done) {
        loadFailed_.erase(id);
        if (textures_.count(id)) continue;
        std::filesystem::path dest = imgDir_ / (std::to_string(id) + ".jpg");
        if (std::filesystem::exists(dest) && !loadTextureFromDisk(id, dest)) loadFailed_.insert(id);
    }
    evictIfNeeded();
}

void CardImageCache::SetMaxTextures(std::size_t maxTextures) { maxTextures_ = maxTextures; }

void CardImageCache::touch(uint32_t id) { lastUse_[id] = ++useClock_; }

void CardImageCache::evictIfNeeded() {
    while (textures_.size() > maxTextures_ && !lastUse_.empty()) {
        auto victim = std::min_element(lastUse_.begin(), lastUse_.end(), [](const auto& a, const auto& b) { return a.second < b.second; });
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

bool CardImageCache::loadTextureFromDisk(uint32_t id, const std::filesystem::path& dest) {
    Texture2D tex = LoadTexture(dest.string().c_str());
    if (tex.id == 0) return false;
    GenTextureMipmaps(&tex);
    SetTextureFilter(tex, TEXTURE_FILTER_TRILINEAR);
    textures_[id] = tex;
    touch(id);
    return true;
}

void CardImageCache::requestDownload(uint32_t id, const std::filesystem::path& dest) {
    if (!downloadsEnabled_) return;
    std::lock_guard<std::mutex> lk(mtx_);
    if (queued_.count(id)) return;
    auto it = retryNotBefore_.find(id);
    if (it != retryNotBefore_.end() && std::chrono::steady_clock::now() < it->second) return;
    queued_.insert(id);
    jobQueue_.push({id, dest});
    cv_.notify_one();
}

void CardImageCache::workerLoop() {
    while (true) {
        Job job;
        {
            std::unique_lock<std::mutex> lk(mtx_);
            cv_.wait(lk, [this] { return stop_.load() || !jobQueue_.empty(); });
            if (stop_) return;
            job = std::move(jobQueue_.front());
            jobQueue_.pop();
            inFlight_ = job;
        }
        bool ok = curlDownload(imageUrl_ + std::to_string(job.id) + ".jpg", job.dest);
        if (!ok && !stopping()) ok = curlDownload(imageUrlSmall_ + std::to_string(job.id) + ".jpg", job.dest);
        {
            std::lock_guard<std::mutex> lk(mtx_);
            inFlight_.reset();
            queued_.erase(job.id);
            if (ok) completed_.push_back(job.id);
            else retryNotBefore_[job.id] = std::chrono::steady_clock::now() + kRetryCooldown;
        }
    }
}

bool CardImageCache::stopping() {
    std::lock_guard<std::mutex> lk(mtx_);
    return stop_.load();
}

bool CardImageCache::curlDownload(const std::string& url, const std::filesystem::path& dest) {
    std::filesystem::create_directories(dest.parent_path());
    std::string tmp = dest.string() + ".tmp";
    std::string cmd = "curl -sSL --max-time 20 -o '" + tmp + "' '" + url + "' 2>/dev/null";
    int ret = std::system(cmd.c_str());
    if (ret == 0 && std::filesystem::exists(tmp) && std::filesystem::file_size(tmp) > 1024) {
        std::filesystem::rename(tmp, dest);
        return true;
    }
    std::error_code ec;
    std::filesystem::remove(tmp, ec);
    return false;
}

bool CardImageCache::shellUnsafeChar(char c) { return c == '\'' || c == '"' || c == '\\' || c == ';' || c == '|' || c == '&' || c == '$' || c == '`' || c == '(' || c == ')' || c == '<' || c == '>' || c == '\n'; }

bool CardImageCache::urlIsSafe(const std::string& url) {
    if (url.rfind("https://", 0) != 0) return false;
    return !std::any_of(url.begin(), url.end(), shellUnsafeChar);
}

bool CardImageCache::pathIsSafe(const std::filesystem::path& p) {
    const std::string s = p.string();
    return !std::any_of(s.begin(), s.end(), shellUnsafeChar);
}

}  // namespace openjoey::ui
