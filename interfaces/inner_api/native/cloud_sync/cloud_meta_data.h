/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_CLOUD_SYNC_CLOUD_META_DATA_H
#define OHOS_MEDIA_CLOUD_SYNC_CLOUD_META_DATA_H

#include <string>
#include <map>
#include <sstream>

#include "cloud_file_data.h"

namespace OHOS::Media::CloudSync {
#define EXPORT __attribute__ ((visibility ("default")))
class EXPORT CloudMetaData {
public:
    std::string cloudId;
    int64_t size;                 // 原文件大小
    int32_t fileId;               // fileName, path, fileId 拼接 URI，下载完成后，使用 URI 通知应用层
    std::string path;             // 原文件路径
    std::string fileName;         // 原文件名称
    int32_t type;                 // 原文件类型（图片/视频）, 1:图片, 2:视频
    int64_t modifiedTime;         // date_modified
    std::string originalCloudId;  // 复制体还未上行的下载
    // key值为fieldKey("lcd","thumbnail","content","raw","editData"等)
    // 同步流程->所有待创建dentry file的文件(缩略图/LCD/纹理图/缩略视频);
    // 下载流程-> 所有待下载的文件
    std::map<std::string, CloudFileData> attachment;
    std::string storagePath;      // 湖内文件路径
    int32_t fileSourceType{0};  // 默认值为0表示湖外文件，3表示湖内文件
    int32_t hidden {0};         // 判断湖内外（0=湖外，非0=湖内）
    int64_t dateTrashed {0};    // 删除时间戳，用于判断湖内外
    int32_t isShared;           // 0 - 普通资产， 1 - 共享资产
    std::string shareAlbumOwner = "";  // 分享相册所有者(资产owner)

public:  // constructor & destructor
    virtual ~CloudMetaData() = default;

public:  // basic function
    virtual std::string ToString() const
    {
        std::stringstream ss;
        ss << "{\"cloudId\": \"" << cloudId << "\","
           << "\"size\": " << size << ","
           << "\"fileId\": " << fileId << ","
           << "\"path\": \"" << path << "\","
           << "\"fileName\": \"" << fileName << "\","
           << "\"type\": " << type << ","
           << "\"modifiedTime\": " << modifiedTime << ","
           << "\"originalCloudId\": \"" << originalCloudId << "\","
           << "\"storagePath\": \"" << storagePath << "\","
           << "\"fileSourceType\": \"" << fileSourceType << "\","
           << "\"hidden\": " << hidden << ","
           << "\"dateTrashed\": " << dateTrashed << ","
           << "\"isShared\": " << isShared << ","
           << "\"shareAlbumOwner\": \"" << shareAlbumOwner << "\","
           << "\"attachment\": [";
        bool first = true;
        for (const auto &item : attachment) {
            if (first) {
                first = false;
            } else {
                ss << ",";
            }
            ss << "{\"" << item.first << "\":" << item.second.ToString() << "}";
            first = false;
        }
        ss << "]}";
        return ss.str();
    }
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_CLOUD_META_DATA_H