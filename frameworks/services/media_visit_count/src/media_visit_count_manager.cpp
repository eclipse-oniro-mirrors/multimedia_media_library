/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#define MLOG_TAG "MediaVisitCountManager"

#include "media_visit_count_manager.h"
#include <thread>
#include "accesstoken_kit.h"
#include "ipc_skeleton.h"
#include "media_log.h"
#include "medialibrary_unistore_manager.h"
#include "media_file_utils.h"

namespace OHOS {
namespace Media {
namespace {
constexpr int64_t WAIT_TIMEOUT = 10000; // 10s
constexpr size_t VISIT_COUNT_MAX_QUEUE_SIZE = 100;
} // namespace

using VisitType = MediaVisitCountManager::VisitCountType;
// LCOV_EXCL_START
void MediaVisitCountManager::AddVisitCount(VisitCountType type, const std::string &fileId)
{
    MEDIA_DEBUG_LOG("MediaVisitCountManager::%{public}s is called, type: %{public}d fileId: %{public}s",
        __func__, static_cast<int>(type), fileId.c_str());
    CHECK_AND_RETURN_LOG(IsValidType(type), "type is not valid");
    CHECK_AND_RETURN_LOG(!fileId.empty(), "fileId is empty");

    auto tokenCaller = IPCSkeleton::GetCallingTokenID();
    auto tokenType = Security::AccessToken::AccessTokenKit::GetTokenType(tokenCaller);
    if (tokenType != Security::AccessToken::ATokenTypeEnum::TOKEN_HAP) {
        MEDIA_DEBUG_LOG("AddVisitCount tokenType is not hap, do not add to queue_");
        return;
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(std::make_pair(type, fileId));
        isTimerRefresh_.store(true);
        if (isThreadRunning_.load()) {
            cv_.notify_all();
        } else {
            isThreadRunning_.store(true);
            std::thread([&] { VisitCountThread(); }).detach();
        }
    }
}

static void GenerateSql(const std::unordered_map<std::string, uint32_t> &infoMap,
    const std::string &visitCount, const std::string &visitTime, std::string &sql)
{
    sql = "UPDATE Photos SET " + visitCount + " = " + visitCount + " + CASE file_id ";
    std::string fileIdList("");
    for (const auto &[fileId, count] : infoMap) {
        sql += "WHEN " + fileId + " THEN " + std::to_string(count) + " ";
        fileIdList += fileId + ",";
    }
    sql += "ELSE 0 END, " + visitTime + " = " + std::to_string(MediaFileUtils::UTCTimeMilliSeconds()) +
        " WHERE file_id IN (";
    fileIdList.pop_back();
    sql += fileIdList + ") AND " + PhotoColumn::PHOTO_IS_SHARED + " != 1";
}

static void ExcuteSqls(std::queue<std::pair<VisitType, std::string>> &queue)
{
    std::unordered_map<VisitType, std::unordered_map<std::string, uint32_t>> sqlsMap;
    while (!queue.empty()) {
        auto visit = queue.front();
        queue.pop();

        auto it = sqlsMap.find(visit.first);
        if (it == sqlsMap.end()) {
            std::unordered_map<std::string, uint32_t> map;
            map.emplace(visit.second, 1);
            sqlsMap.emplace(visit.first, map);
        } else {
            auto iter = it->second.find(visit.second);
            if (iter == it->second.end()) {
                it->second.emplace(visit.second, 1);
            } else {
                iter->second++;
            }
        }
    }

    for (const auto &[type, info] : sqlsMap) {
        std::string sql("");
        switch (type) {
            case VisitType::PHOTO_FS:
                GenerateSql(info, PhotoColumn::PHOTO_VISIT_COUNT, PhotoColumn::PHOTO_LAST_VISIT_TIME, sql);
                break;
            case VisitType::PHOTO_LCD:
                GenerateSql(info, PhotoColumn::PHOTO_LCD_VISIT_COUNT, PhotoColumn::PHOTO_REAL_LCD_VISIT_TIME, sql);
                break;
            default:
                MEDIA_ERR_LOG("type is not valid");
                break;
        }
        if (!sql.empty()) {
            auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
            CHECK_AND_RETURN_LOG(rdbStore != nullptr, "VisitCountThread ExcuteSqls rdbStore is nullptr");
            auto ret = rdbStore->ExecuteSql(sql);
            MEDIA_DEBUG_LOG("Update visit count: sql[%{public}s] result[%{public}d]", sql.c_str(), ret);
        }
    }
}

void MediaVisitCountManager::VisitCountThread()
{
    MEDIA_DEBUG_LOG("MediaVisitCountManager::VisitCountThread start");
    while (isThreadRunning_.load()) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            isTimerRefresh_.store(false);
            cv_.wait_for(lock, std::chrono::milliseconds(WAIT_TIMEOUT), [] { return isTimerRefresh_.load(); });
            if (queue_.empty()) {
                isThreadRunning_.store(false);
                MEDIA_DEBUG_LOG("VisitCountThread Exit.");
                return;
            }
            CHECK_AND_CONTINUE(!(isTimerRefresh_.load() && queue_.size() < VISIT_COUNT_MAX_QUEUE_SIZE));
        }

        std::queue<std::pair<VisitType, std::string>> tmpQueue;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            tmpQueue.swap(queue_);
        }
        MEDIA_DEBUG_LOG("VisitCountThread ExcuteSqls start");
        ExcuteSqls(tmpQueue);
        MEDIA_DEBUG_LOG("VisitCountThread ExcuteSqls end");
    }
}
// LCOV_EXCL_STOP
} // namespace Media
} // namespace OHOS