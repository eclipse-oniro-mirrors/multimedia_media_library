/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIALIBRARY_COMMAND_PARSE_H
#define OHOS_MEDIALIBRARY_COMMAND_PARSE_H

#include <string>
#include <vector>

#include "abs_rdb_predicates.h"
#include "data_ability_predicates.h"
#include "hilog/log.h"
#include "media_data_ability_const.h"
#include "uri.h"
#include "values_bucket.h"

namespace OHOS {
namespace Media {

enum OperationObject {
    UNKNOWN_OBJECT = 0,
    FILESYSTEM_ASSET = 1, // real file
    FILESYSTEM_DIR,       // real dictionary
    FILESYSTEM_ALBUM,     // real album
    SMART_ALBUM,          // virtual album = smart album
    SMART_ALBUM_MAP,
    THUMBNAIL,
    KVSTORE,
    SMART_ABLUM_ASSETS,
    ASSETMAP,
    ALL_DEVICE,
    ACTIVE_DEVICE,
    MEDIA_VOLUME,
};

enum OperationType {
    UNKNOWN_TYPE = 0,
    OPEN = 1,
    CLOSE,
    CREATE,
    DELETE,
    UPDATE,
    QUERY,
    ISDICTIONARY,
    GETCAPACITY,
    SCAN
};

class MediaLibraryCommand {
public:
    ~MediaLibraryCommand();
    MediaLibraryCommand(const MediaLibraryCommand &) = delete;
    MediaLibraryCommand &operator=(const MediaLibraryCommand &) = delete;
    MediaLibraryCommand(MediaLibraryCommand &&) = delete;
    MediaLibraryCommand &operator=(MediaLibraryCommand &&) = delete;

    explicit MediaLibraryCommand(const Uri &uri);
    MediaLibraryCommand(const Uri &uri, const NativeRdb::ValuesBucket &value);
    MediaLibraryCommand(const Uri &uri, const OperationType oprnType);
    MediaLibraryCommand(const OperationObject oprnObject, const OperationType oprnType);
    MediaLibraryCommand(const OperationObject oprnObject, const OperationType oprnType,
                        const NativeRdb::ValuesBucket &value);

    OperationObject GetOprnObject() const;
    OperationType GetOprnType() const;
    const std::string &GetTableName();
    const NativeRdb::ValuesBucket &GetValueBucket() const;
    NativeRdb::AbsRdbPredicates *GetAbsRdbPredicates();
    const std::string &GetOprnFileId();
    const std::string &GetOprnDevice();
    const Uri &GetUri() const;

    void SetOprnObject(const OperationObject oprnObject);
    void SetOprnType(const OperationType oprnType);
    void SetOprnAssetId(const std::string &oprnId);
    void SetOprnDevice(const std::string &deviceId);
    void SetValueBucket(const NativeRdb::ValuesBucket &value);
    void SetTableName(const std::string &tableName);

private:
    MediaLibraryCommand() = delete;

    void ParseOprnObjectFromUri();
    void ParseOprnTypeFromUri();
    void ParseTableName();
    void InitAbsRdbPredicates();
    void ParseFileId();

    Uri uri_{""};
    NativeRdb::ValuesBucket insertValue_{};
    std::unique_ptr<NativeRdb::AbsRdbPredicates> absRdbPredicates_{nullptr};
    OperationObject oprnObject_{UNKNOWN_OBJECT};
    OperationType oprnType_{UNKNOWN_TYPE};
    std::string oprnFileId_{""};
    std::string oprnDevice_;
    std::string tableName_;
};

} // namespace Media
} // namespace OHOS

#endif // OHOS_MEDIALIBRARY_COMMAND_PARSE_H