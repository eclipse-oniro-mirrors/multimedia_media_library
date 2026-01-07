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

#include "mtp_dfx_reporter.h"

#include <set>
#include <thread>
#include <map>

#include "hisysevent.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "mtp_manager.h"
#include "mtp_constants.h"

namespace OHOS {
namespace Media {
using namespace std;
static constexpr char MEDIA_LIBRARY[] = "MEDIALIBRARY";
constexpr int32_t MTP_MODE = 1;
constexpr int32_t PTP_MODE = 2;
static std::mutex mutex;
static std::unordered_map<std::string, FileCountInfo> fileCountInfoMap;
static std::unordered_map<uint16_t, std::pair<int32_t, int32_t>> operationStats;
constexpr int32_t MAX_FILE_COUNT_INFO = 50;
const std::string GARBLE = "*";
constexpr uint32_t GARBLE_SMALL = 3;
constexpr uint32_t GARBLE_LARGE = 8;
constexpr uint32_t GARBLE_LAST_TWO = 2;
constexpr uint32_t GARBLE_LAST_ONE = 1;
static const char *UTF16_CERROR = "__CONVERSION_ERROR__";
static const char16_t *g_utf8Cerror = u"__CONVERSION_ERROR__";

static const std::vector<std::pair<uint16_t, std::string>> ObjMediaPropTypeMap = {
    {MTP_OPERATION_GET_OBJECT_HANDLES_CODE, "MTP_GET_OBJECT_HANDLES_INFO"},
    {MTP_OPERATION_GET_OBJECT_CODE, "MTP_GET_OBJECT_INFO"},
    {MTP_OPERATION_GET_PARTIAL_OBJECT_CODE, "MTP_GET_PARTIAL_OBJECT_INFO"},
    {MTP_OPERATION_DELETE_OBJECT_CODE, "MTP_DELETE_OBJECT_INFO"},
    {MTP_OPERATION_SEND_OBJECT_CODE, "MTP_SEND_OBJECT_INFO"},
    {MTP_OPERATION_SET_OBJECT_PROP_VALUE_CODE, "MTP_SET_OBJECT_PROP_VALUE_INFO"},
    {MTP_OPERATION_MOVE_OBJECT_CODE, "MTP_MOVE_OBJECT_INFO"},
    {MTP_OPERATION_COPY_OBJECT_CODE, "MTP_COPY_OBJECT_INFO"}
};

MtpDfxReporter &MtpDfxReporter::GetInstance()
{
    static MtpDfxReporter instance;
    return instance;
}

void MtpDfxReporter::Init()
{
    MEDIA_INFO_LOG("MtpDfxReporter::Init");
    std::lock_guard<std::mutex> lock(mutex);
    std::unordered_map<std::string, FileCountInfo>().swap(fileCountInfoMap);
    std::unordered_map<uint16_t, std::pair<int32_t, int32_t>>().swap(operationStats);
    for (const auto& key : ObjMediaPropTypeMap) {
        operationStats[key.first] = {0, 0};
    }
    lastReadResult_ = 0;
}

void MtpDfxReporter::DoFileCountInfoStatistics(const FileCountInfo &fileCountInfo)
{
    std::lock_guard<std::mutex> lock(mutex);
    auto it = fileCountInfoMap.find(fileCountInfo.albumName);
    if (it == fileCountInfoMap.end()) {
        fileCountInfoMap[fileCountInfo.albumName] = fileCountInfo;
        return;
    }
    it->second.pictureCount = fileCountInfo.pictureCount;
    it->second.videoCount = fileCountInfo.videoCount;
    it->second.normalCount = fileCountInfo.normalCount;
    it->second.livePhotoCount = fileCountInfo.livePhotoCount;
    it->second.onlyInCloudPhotoCount = fileCountInfo.onlyInCloudPhotoCount;
    it->second.burstCount = fileCountInfo.burstCount;
    it->second.burstTotalCount = fileCountInfo.burstTotalCount;
}

static bool IsKeyInObjMediaPropTypeMap(uint16_t key)
{
    auto it = std::find_if(ObjMediaPropTypeMap.begin(), ObjMediaPropTypeMap.end(),
        [key](const std::pair<uint16_t, std::string>& item) {
            return item.first == key;
        });

    return it != ObjMediaPropTypeMap.end();
}

void MtpDfxReporter::DoOperationResultStatistics(uint16_t operation, uint16_t responseCode)
{
    if (IsKeyInObjMediaPropTypeMap(operation)) {
        std::lock_guard<std::mutex> lock(mutex);
        if (responseCode == MTP_OK_CODE) {
            operationStats[operation].first++;
        } else {
            operationStats[operation].second++;
        }
    }
}

void MtpDfxReporter::DoFileCountInfoDfxReporter(int32_t mtpMode, std::vector<std::string> &result)
{
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_MTP_PTP_SYNC_RESULT",
        HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "MTP_MODE", mtpMode,
        "FILE_COUNT_INFO", result);
    if (ret != 0) {
        MEDIA_ERR_LOG("DoBatchFileCountInfoDfxReporter error:%{public}d", ret);
    }
}

void MtpDfxReporter::DoBatchFileCountInfoDfxReporter(int32_t mtpMode)
{
    vector<std::string> result;
    std::lock_guard<std::mutex> lock(mutex);
    uint32_t count = 0;
    for (const auto& [key, info] : fileCountInfoMap) {
        string albumName = GetSafeAlbumNameWhenChinese(key);
        string resultStr = std::move(albumName) + "," + to_string(info.pictureCount) + "," +
            to_string(info.videoCount) + "," + to_string(info.normalCount) + "," +
            to_string(info.livePhotoCount) + "," + to_string(info.onlyInCloudPhotoCount) + "," +
            to_string(info.burstCount) + "," + to_string(info.burstTotalCount);
        result.push_back(resultStr);
        count++;
        if (count >= MAX_FILE_COUNT_INFO) {
            count = 0;
            DoFileCountInfoDfxReporter(mtpMode, result);
            result.clear();
        }
    }
    MEDIA_INFO_LOG("MtpDfxReporter:DoBatchFileCountInfoDfxReporter result.size():%{public}zu", result.size());
    if (result.size() > 0) {
        DoFileCountInfoDfxReporter(mtpMode, result);
    }
    std::unordered_map<std::string, FileCountInfo>().swap(fileCountInfoMap);
}

void MtpDfxReporter::DoOperationResultDfxReporter(int32_t mtpMode)
{
    std::vector<std::pair<std::string, std::vector<int32_t>>> stats;
    {
        std::lock_guard<std::mutex> lock(mutex);
        for (const auto& mapping : ObjMediaPropTypeMap) {
            uint16_t operationCode = mapping.first;
            const std::string& keyName = mapping.second;
            std::vector<int32_t> operationInfoCount = {
                operationStats[operationCode].first,
                operationStats[operationCode].second
            };
            stats.emplace_back(keyName, operationInfoCount);
        }
    }

    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_MTP_OPERATION_RESULT",
        HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "MTP_MODE", mtpMode,
        stats[GET_OBJECT_HANDLES].first.c_str(), stats[GET_OBJECT_HANDLES].second,
        stats[GET_OBJECT].first.c_str(), stats[GET_OBJECT].second,
        stats[GET_PARTIAL_OBJECT].first.c_str(), stats[GET_PARTIAL_OBJECT].second,
        stats[DELETE_OBJECT].first.c_str(), stats[DELETE_OBJECT].second,
        stats[SEND_OBJECT].first.c_str(), stats[SEND_OBJECT].second,
        stats[SET_OBJECT_PROP_VALUE].first.c_str(), stats[SET_OBJECT_PROP_VALUE].second,
        stats[MOVE_OBJECT].first.c_str(), stats[MOVE_OBJECT].second,
        stats[COPY_OBJECT].first.c_str(), stats[COPY_OBJECT].second
    );
    if (ret != 0) {
        MEDIA_ERR_LOG("DoOperationResultDfxReporter error:%{public}d", ret);
    }
    {
        std::lock_guard<std::mutex> lock(mutex);
        std::unordered_map<uint16_t, std::pair<int32_t, int32_t>>().swap(operationStats);
    }
}

void MtpDfxReporter::NotifyDoDfXReporter(int32_t mtpMode)
{
    MEDIA_INFO_LOG("MtpDfxReporter:NotifyDoDfXReporter mtpMode:%{public}d", mtpMode);
    std::thread([&, mtpMode]() {
        DoBatchFileCountInfoDfxReporter(mtpMode);
        DoOperationResultDfxReporter(mtpMode);
    }).detach();
}

void MtpDfxReporter::DoSendResponseResultDfxReporter(uint16_t operationCode, int32_t operationResult,
    uint64_t duration, int32_t operationMode)
{
    if (operationResult == 0) {
        return;
    }
    if (operationMode == readmode && operationResult == E_USB_DISCONNECT) {
        if (lastReadResult_ == E_USB_DISCONNECT) {
            MEDIA_DEBUG_LOG("DoSendResponseResultDfxReporter is called, operationResult is E_USB_DISCONNECT");
            return;
        }
        lastReadResult_ = operationResult;
    }
    MEDIA_INFO_LOG("MtpDfxReporter operationCode:0x%{public}x, operationResult:%{public}d", operationCode,
        operationResult);
    int32_t mtpMode;
    if (MtpManager::GetInstance().IsMtpMode()) {
        mtpMode = MTP_MODE;
    } else {
        mtpMode = PTP_MODE;
    }
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_MTP_PTP_SEND_RESPONSE",
        HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "MTP_MODE", mtpMode,
        "OPERATION_CODE", operationCode,
        "OPERATION_RESULT", operationResult,
        "OPERATION_COST_TIME", duration,
        "OPERATION_MODE", operationMode);
    if (ret != 0) {
        MEDIA_ERR_LOG("DoSendResponseResultDfxReporter error:%{public}d", ret);
    }
}

static std::u16string Str8ToStr16(const std::string &inputStr)
{
    if (inputStr.empty()) {
        return u"";
    }
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert(UTF16_CERROR, g_utf8Cerror);
    std::u16string result = convert.from_bytes(inputStr);
    return result == g_utf8Cerror ? u"" : result;
}

static std::string Str16ToStr8(const std::u16string &inputStr)
{
    if (inputStr.empty()) {
        return "";
    }
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert(UTF16_CERROR, g_utf8Cerror);
    std::string result = convert.to_bytes(inputStr);
    return result == UTF16_CERROR ? "" : result;
}

string MtpDfxReporter::GetSafeAlbumNameWhenChinese(const string &albumName)
{
    MEDIA_INFO_LOG("MtpDfxReporter:GetSafeAlbumNameWhenChinese");
    CHECK_AND_RETURN_RET_LOG(!albumName.empty(), "", "input albumName is empty");
    std::u16string wideStr = Str8ToStr16(albumName);
    uint32_t length = wideStr.size();
    if (length <= 0) {
        return GARBLE;
    }
    std::u16string safeAlbumName;
    if (length <= GARBLE_SMALL) {
        safeAlbumName = wideStr.substr(length - GARBLE_LAST_ONE);
    } else if (length > GARBLE_LARGE) {
        safeAlbumName = wideStr.substr(GARBLE_LARGE);
    } else {
        safeAlbumName = wideStr.substr(length - GARBLE_LAST_TWO);
    }
    return GARBLE + Str16ToStr8(safeAlbumName);
}
} // namespace Media
} // namespace OHOS
