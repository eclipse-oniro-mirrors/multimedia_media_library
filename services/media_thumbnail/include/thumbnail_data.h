/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_THUMBNAIL_DATA_H
#define FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_THUMBNAIL_DATA_H

#include <picture.h>
#include <pixel_map.h>

#include "medialibrary_rdb_utils.h"
#include "thumbnail_const.h"

namespace OHOS {
namespace Media {

struct ThumbRdbOpt {
    EXPORT std::shared_ptr<MediaLibraryRdbStore> store;
    EXPORT std::shared_ptr<AbilityRuntime::Context> context;
    EXPORT std::string networkId;
    EXPORT std::string path;
    EXPORT std::string table;
    EXPORT std::string udid;
    EXPORT std::string row;
    EXPORT std::string uri;
    EXPORT std::string dateAdded;
    EXPORT std::string dateTaken;
    EXPORT std::string dateModified;
    EXPORT std::string fileUri;
    EXPORT std::string fileId;
    EXPORT Size screenSize;
};

#define EXPORT __attribute__ ((visibility ("default")))
enum class SourceState : int32_t {
    BEGIN = -3,
    LOCAL_THUMB,
    LOCAL_LCD,
    LOCAL_ORIGIN,
    CLOUD_THUMB,
    CLOUD_LCD,
    CLOUD_ORIGIN,
    ERROR,
    FINISH,
};

class ThumbnailSource {
public:
    void SetPixelMap(std::shared_ptr<PixelMap> &pixelMap)
    {
        pixelMapSource_  = pixelMap;
    }

    std::shared_ptr<PixelMap> GetPixelMap()
    {
        return pixelMapSource_;
    }

    void SetPixelMapEx(std::shared_ptr<PixelMap> &pixelMapEx)
    {
        pixelMapSourceEx_  = pixelMapEx;
    }

    std::shared_ptr<PixelMap> GetPixelMapEx()
    {
        return pixelMapSourceEx_;
    }

    void SetPicture(std::shared_ptr<Picture> &picture)
    {
        pictureSource_  = picture;
        if (picture != nullptr) {
            hasPictureSource_ = true;
        }
    }

    std::shared_ptr<Picture> GetPicture()
    {
        return pictureSource_;
    }

    void SetPictureEx(std::shared_ptr<Picture> &pictureEx)
    {
        pictureSourceEx_  = pictureEx;
    }

    std::shared_ptr<Picture> GetPictureEx()
    {
        return pictureSourceEx_;
    }

    void ClearAllSource()
    {
        pixelMapSource_ = nullptr;
        pixelMapSourceEx_ = nullptr;
        pictureSource_ = nullptr;
        pictureSourceEx_ = nullptr;
        hasPictureSource_ = false;
    }
    
    bool IsEmptySource()
    {
        return pixelMapSource_ == nullptr && pictureSource_ == nullptr;
    }

    bool HasPictureSource()
    {
        return hasPictureSource_;
    }

private:
    bool hasPictureSource_ {false};
    std::shared_ptr<PixelMap> pixelMapSource_;
    std::shared_ptr<PixelMap> pixelMapSourceEx_;
    std::shared_ptr<Picture> pictureSource_;
    std::shared_ptr<Picture> pictureSourceEx_;
};

class ThumbnailData {
public:
    struct GenerateStats {
        std::string uri;
        GenerateScene scene {GenerateScene::LOCAL};
        int32_t openThumbCost {0};
        int32_t openLcdCost {0};
        int32_t openOriginCost {0};
        LoadSourceType sourceType {LoadSourceType::LOCAL_PHOTO};
        int32_t sourceWidth {0};
        int32_t sourceHeight {0};
        int32_t totalCost {0};
        int32_t errorCode {0};
        int64_t startTime {0};
        int64_t finishTime {0};
    };

    struct SourceLoaderOptions {
        // if false, target decode size is LCD size
        bool decodeInThumbSize {false};
        bool needUpload {false};

        // if true, create HDR pixelmap
        EXPORT bool isHdr {false};

        // largest thumbnail type for the source to generate
        ThumbnailType desiredType {ThumbnailType::NOT_DEFINED};
        std::unordered_map<SourceState, SourceState> loadingStates;
    };

    EXPORT ThumbnailData() = default;

    EXPORT virtual ~ThumbnailData()
    {
        source.ClearAllSource();
        thumbnail.clear();
        lcd.clear();
        monthAstc.clear();
        yearAstc.clear();
    }

    EXPORT int32_t mediaType {-1};
    EXPORT int32_t orientation {0};
    EXPORT int32_t exifRotate {-1};
    EXPORT int32_t photoHeight {0};
    EXPORT int32_t photoWidth {0};
    EXPORT int32_t dirty {-1};
    EXPORT ThumbnailQuality thumbnailQuality {ThumbnailQuality::DEFAULT};

    // Loaded lcd source can be resized to generate thumbnail in order
    EXPORT bool needResizeLcd {false};
    EXPORT bool isLocalFile {true};
    EXPORT bool isOpeningCloudFile {false};
    EXPORT bool isNeedStoreSize {true};
    EXPORT bool isRegenerateStage {false};
    EXPORT bool isUpgradeStage {false};
    EXPORT bool needCheckWaitStatus {false};
    EXPORT bool needUpdateDb {true};
    EXPORT bool createLowQulityLcd {false};
    EXPORT bool needGenerateExThumbnail {true};
    EXPORT ThumbnailSource source;
    EXPORT std::vector<uint8_t> thumbnail;
    EXPORT std::vector<uint8_t> thumbAstc;
    EXPORT std::vector<uint8_t> monthAstc;
    EXPORT std::vector<uint8_t> yearAstc;
    EXPORT std::vector<uint8_t> lcd;
    EXPORT std::string dateAdded;
    EXPORT std::string dateTaken;
    EXPORT std::string dateModified;
    EXPORT std::string displayName;
    EXPORT std::string fileUri;
    EXPORT std::string id;
    EXPORT std::string cloudId;
    EXPORT std::string udid;
    EXPORT std::string path;
    EXPORT std::string thumbnailKey;
    EXPORT std::string lcdKey;
    EXPORT std::string tracks;
    EXPORT std::string trigger;
    EXPORT std::string frame;
    EXPORT std::string timeStamp;
    EXPORT int32_t position;
    EXPORT Size lcdDesiredSize;
    EXPORT Size thumbDesiredSize;
    EXPORT GenerateStats stats;
    EXPORT SourceLoaderOptions loaderOpts;
    EXPORT int64_t thumbnailReady { -1 };
    EXPORT int64_t lcdVisitTime { -1 };
    EXPORT std::shared_ptr<Picture> originalPhotoPicture = nullptr;
    EXPORT NativeRdb::ValuesBucket rdbUpdateCache;
    EXPORT SourceState lastLoadSource {SourceState::BEGIN};
    EXPORT std::string taskCreatedInfo = "sync task";
    EXPORT GenThumbScene genThumbScene {GenThumbScene::UNDEFINED_SCENE};
    EXPORT int32_t thumbnailStatus {0};
};

struct ThumbnailDataBatch {
    EXPORT std::vector<std::string> ids;
    EXPORT std::vector<std::string> paths;
    EXPORT std::vector<std::string> dateTakens;
};
} // namespace Media
} // namespace OHOS

#endif  // FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_THUMBNAIL_DATA_H
