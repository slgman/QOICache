#include <Geode/Geode.hpp>
#include <Geode/modify/CCImage.hpp>
#include <Geode/modify/LoadingLayer.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include "qoixx.hpp"

using namespace geode::prelude;

static void cacheImage(
    CCImage* self,
    std::filesystem::file_time_type qoiWriteTime,
    const std::filesystem::path& qoiPath
) {
    auto width    = self->m_nWidth;
    auto height   = self->m_nHeight;
    auto pData    = self->m_pData;
    auto hasAlpha = self->m_bHasAlpha;
    auto preMulti = self->m_bPreMulti;

    if (!pData || width == 0 || height == 0) return;

    qoixx::qoi::desc desc {
        .width      = width,
        .height     = height,
        .channels   = hasAlpha ? uint8_t(4) : uint8_t(3),
        .colorspace = preMulti
                    ? qoixx::qoi::colorspace::linear
                    : qoixx::qoi::colorspace::srgb
    };

    auto encoded = qoixx::qoi::encode<std::vector<char>>(
        pData,
        static_cast<size_t>(width) * height * desc.channels,
        desc
    );

    if (encoded.empty()) {
        log::warn("encode returned empty result for {}", qoiPath.string());
        return;
    }

    std::ofstream file(qoiPath, std::ios::out | std::ios::trunc | std::ios::binary);
    if (!file) {
        log::warn("failed to open {} for writing", qoiPath.string());
        return;
    }
    file.write(encoded.data(), static_cast<std::streamsize>(encoded.size()));
    file.close();

    // stamp the .qoi with the source files mtime so we can detect staleness
    std::filesystem::last_write_time(qoiPath, qoiWriteTime);

    log::debug("cached {}", qoiPath.string());
}

// read the .qoi file from disk and decode it directly into the CCImage
// this bypasses initWithImageFile entirely, avoiding any format-detection issues
static bool loadFromQoiCache(CCImage* self, const std::filesystem::path& qoiPath) {
    std::ifstream file(qoiPath, std::ios::binary | std::ios::ate);
    if (!file) return false;

    auto size = file.tellg();
    if (size <= 0) return false;
    file.seekg(0);

    std::vector<uint8_t> buf(static_cast<size_t>(size));
    file.read(reinterpret_cast<char*>(buf.data()), size);
    if (!file) return false;

    auto [pixels, desc] = qoixx::qoi::decode<std::vector<uint8_t>>(buf.data(), buf.size());

    if (pixels.empty()) {
        log::warn("decode returned empty result for {}", qoiPath.string());
        return false;
    }

    self->m_nWidth            = static_cast<uint16_t>(desc.width);
    self->m_nHeight           = static_cast<uint16_t>(desc.height);
    self->m_nBitsPerComponent = 8;
    self->m_bHasAlpha         = desc.channels >= 4u;
    self->m_bPreMulti         = desc.colorspace == qoixx::qoi::colorspace::linear;

    self->m_pData = new uint8_t[pixels.size()];
    std::copy(pixels.begin(), pixels.end(), self->m_pData);

    return true;
}

// returns true if the file is an image type we should QOI-cache
static bool isCacheable(const std::filesystem::path& p) {
    auto ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".png";
}

// returns true if a valid, up-to-date .qoi cache exists for the given path
static bool hasFreshCache(
    const std::filesystem::path&    srcPath,
    std::filesystem::path&          outQoiPath,
    std::filesystem::file_time_type& outSrcMtime
) {
    std::error_code ec;
    outSrcMtime = std::filesystem::last_write_time(srcPath, ec);
    if (ec) return false;

    outQoiPath = std::filesystem::path(srcPath).replace_extension("qoi");
    if (!std::filesystem::exists(outQoiPath, ec)) return false;

    auto qoiMtime = std::filesystem::last_write_time(outQoiPath, ec);
    if (ec) return false;

    return qoiMtime == outSrcMtime;
}

class $modify(CCImage) {
    bool initWithImageFile(const char* strPath, EImageFormat imageType) {
        if (!strPath) return CCImage::initWithImageFile(strPath, imageType);

        std::filesystem::path srcPath(strPath);
        if (!isCacheable(srcPath)) return CCImage::initWithImageFile(strPath, imageType);

        std::filesystem::path qoiPath;
        std::filesystem::file_time_type srcMtime;

        if (hasFreshCache(strPath, qoiPath, srcMtime)) {
            log::debug("hit: {}", qoiPath.string());
            return loadFromQoiCache(this, qoiPath);
        }

        bool ok = CCImage::initWithImageFile(strPath, imageType);
        if (ok) cacheImage(this, srcMtime, qoiPath);
        return ok;
    }

    bool initWithImageFileThreadSafe(const char* strPath, EImageFormat imageType) {
        if (!strPath) return CCImage::initWithImageFileThreadSafe(strPath, imageType);

        std::filesystem::path srcPath(strPath);
        if (!isCacheable(srcPath)) return CCImage::initWithImageFileThreadSafe(strPath, imageType);

        std::filesystem::path qoiPath;
        std::filesystem::file_time_type srcMtime;

        if (hasFreshCache(strPath, qoiPath, srcMtime)) {
            log::debug("hit (TS): {}", qoiPath.string());
            return loadFromQoiCache(this, qoiPath);
        }

        bool ok = CCImage::initWithImageFileThreadSafe(strPath, imageType);
        if (ok) cacheImage(this, srcMtime, qoiPath);
        return ok;
    }
};
