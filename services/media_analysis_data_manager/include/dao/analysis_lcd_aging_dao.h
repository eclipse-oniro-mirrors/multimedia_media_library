/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_ANALYSIS_LCD_AGING_DAO_H
#define OHOS_ANALYSIS_LCD_AGING_DAO_H

#include <string>
#include <vector>
#include <map>
#include <set>
#include "photos_po.h"

// LCOV_EXCL_START
namespace OHOS::Media::AnalysisData {

// 下载文件信息结构体
struct DownloadLcdFileInfo {
    int32_t fileId;
    std::string cloudId;
    std::string filePath;
    std::string fileName;
    bool hasLocalFile;        // 是否存在本地原图
    std::string localLcdPath;  // 本地LCD路径（空字符串表示不存在）
};

class AnalysisLcdAgingDao {
public:
    AnalysisLcdAgingDao() = default;
    ~AnalysisLcdAgingDao() = default;

    // 网络条件枚举
    enum class NetworkCondition {
        AVAILABLE,        // 网络可用且满足要求
        NO_NETWORK,       // 无网络
        PROHIBITED        // 网络不满足要求
    };

    // 查询需要下载LCD的文件信息（cloudId、filePath、fileName 和本地文件状态）
    int32_t QueryDownloadLcdInfo(const std::vector<int64_t> &fileIds,
        std::vector<DownloadLcdFileInfo> &downloadInfos);

    // 标记数据库中不存在的文件
    void MarkNotFoundFiles(const std::vector<int64_t> &fileIds, const std::set<int64_t> &foundFileIds,
        std::unordered_map<uint64_t, int32_t> &results);

    // 分类文件并处理本地LCD和本地生成的情况
    int32_t ClassifyLcdFiles(const std::vector<DownloadLcdFileInfo> &downloadInfos,
        std::vector<int64_t> &needDownloadFileIds, std::set<int64_t> &foundFileIds,
        std::unordered_map<uint64_t, int32_t> &results);

    // 检查网络条件
    NetworkCondition CheckNetworkCondition(uint32_t netBearerBitmap);

    // 处理需要下载的文件
    int32_t ProcessNeedDownloadFiles(const std::vector<int64_t> &needDownloadFileIds, uint32_t netBearerBitmap,
        std::unordered_map<uint64_t, int32_t> &results);

    // 从results中收集失败的fileIds并插入tab_photos_ext
    int32_t InsertFailedPhotosExt(const std::unordered_map<uint64_t, int32_t> &results);
private:
    // SQL查询语句：根据fileIds查询满足老化条件的数据
    static const std::string SQL_QUERY_AGING_LCD_BY_FILE_IDS;
}; // class AnalysisLcdAgingDao
} // namespace OHOS::Media::AnalysisData
// LCOV_EXCL_STOP
#endif // OHOS_ANALYSIS_LCD_AGING_DAO_H