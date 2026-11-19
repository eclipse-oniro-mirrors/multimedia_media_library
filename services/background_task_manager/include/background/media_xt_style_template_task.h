/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_BACKGROUND_MEDIA_XT_STYLE_TEMPLATE_TASK_H
#define OHOS_MEDIA_BACKGROUND_MEDIA_XT_STYLE_TEMPLATE_TASK_H

#include <cstdint>
#include <string>
#include <vector>

#include "i_media_background_task.h"

namespace OHOS::Media::Background {

struct XtStyleTemplateAssetInfo {
    int32_t fileId = 0;
    int32_t number = 0;
    std::string path;
    std::string name;
};

/**
 * @brief 补齐全量升级后 xt_style_template_number 仍为 -2 的历史数据。
 *
 * 按数据库已有色卡名：直接换算编号，不读取文件；
 * 按文件 EXIF：对色卡名缺失的本地图片解析 EXIF，跳过云端、临时与回收站文件。
 * 写入仅更新色卡字段，不修改 dirty 标记，因此不会触发上云。
 */
class MediaXtStyleTemplateTask : public IMediaBackGroundTask {
public:
    MediaXtStyleTemplateTask() = default;
    virtual ~MediaXtStyleTemplateTask() = default;

public:
    bool Accept() override;
    void Execute() override;

private:
    void HandleXtStyleTemplateNumber();
    void FillUnprocessedNumbersByDbName();
    void FillUnprocessedNumbersByFileExif();
    bool HasUnprocessedAssets();
    int32_t QueryMaxFileId();
    std::vector<XtStyleTemplateAssetInfo> QueryUnprocessedAssetsByDbName(int32_t startFileId);
    std::vector<XtStyleTemplateAssetInfo> QueryUnprocessedAssetsByFileExif(int32_t startFileId);
    bool ExtractNumberFromFile(XtStyleTemplateAssetInfo &assetInfo);
    int32_t BatchUpdateNumber(const std::vector<XtStyleTemplateAssetInfo> &assetInfos);
    int32_t BatchUpdateNameAndNumber(const std::vector<XtStyleTemplateAssetInfo> &assetInfos);
    int32_t GetProgress(const std::string &progressKey);
    void SetProgress(const std::string &progressKey, int32_t startFileId);
};

}  // namespace OHOS::Media::Background
#endif  // OHOS_MEDIA_BACKGROUND_MEDIA_XT_STYLE_TEMPLATE_TASK_H
