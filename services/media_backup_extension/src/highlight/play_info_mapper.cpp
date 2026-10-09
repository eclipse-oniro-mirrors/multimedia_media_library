/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "Media_PlayInfoMapper"
#include "play_info_mapper.h"

#include "media_log.h"
#include "medialibrary_data_manager_utils.h"
#include <optional>

namespace OHOS::Media {

namespace {
const std::vector<std::string> EFFECTLINE_ID = { "fileId", "prefileId" };
const std::vector<std::string> EFFECTLINE_URI = { "fileUri", "prefileUri" };
const std::string EFFECTLINE_TYPE_MASK1 = "TYPE_MASK1";
const std::string EFFECTLINE_TYPE_MASK2 = "TYPE_MASK2";

void ProcessIds(nlohmann::json &effectlineInfo, const PlayInfoMapperCallbacks &callbacks)
{
    CHECK_AND_RETURN(effectlineInfo.is_object());
    for (const auto &effectlineId : EFFECTLINE_ID) {
        CHECK_AND_CONTINUE(effectlineInfo.contains(effectlineId) && effectlineInfo[effectlineId].is_array());
        for (auto &fileId : effectlineInfo[effectlineId]) {
            CHECK_AND_CONTINUE(fileId.is_number());
            fileId = callbacks.idMapper(fileId.get<int32_t>());
        }
    }
}

void ProcessUris(nlohmann::json &effectlineInfo, const PlayInfoMapperCallbacks &callbacks)
{
    CHECK_AND_RETURN(effectlineInfo.is_object());
    for (const auto &effectlineUri : EFFECTLINE_URI) {
        CHECK_AND_CONTINUE(effectlineInfo.contains(effectlineUri) && effectlineInfo[effectlineUri].is_array());
        for (auto &fileUri : effectlineInfo[effectlineUri]) {
            CHECK_AND_CONTINUE(fileUri.is_string());
            fileUri = callbacks.photoUriMapper(fileUri.get<std::string>());
        }
    }
}

void ProcessEffectVideoUri(nlohmann::json &effectlineInfoArray, size_t effectlineIndex,
    const PlayInfoMapperCallbacks &callbacks)
{
    CHECK_AND_RETURN(effectlineInfoArray.is_array() && effectlineIndex < effectlineInfoArray.size());

    nlohmann::json &effectlineInfo = effectlineInfoArray[effectlineIndex];
    CHECK_AND_RETURN(effectlineInfo.is_object());
    CHECK_AND_RETURN(effectlineInfo.contains("effectVideoUri") && effectlineInfo["effectVideoUri"].is_string());
    std::string oldEffectVideoUri = effectlineInfo["effectVideoUri"];
    effectlineInfo["effectVideoUri"] = callbacks.effectVideoUriMapper(oldEffectVideoUri);
}

void ProcessTransitionVideoUri(nlohmann::json &effectlineInfoArray, size_t effectlineIndex,
    const PlayInfoMapperCallbacks &callbacks)
{
    CHECK_AND_RETURN(effectlineInfoArray.is_array() && effectlineIndex < effectlineInfoArray.size());

    nlohmann::json &effectlineInfo = effectlineInfoArray[effectlineIndex];
    CHECK_AND_RETURN(effectlineInfo.is_object());
    CHECK_AND_RETURN(effectlineInfo.contains("effect") && effectlineInfo["effect"].is_string() &&
        effectlineInfo["effect"] == EFFECTLINE_TYPE_MASK2 &&
        effectlineInfo.contains("transitionVideoUri") && effectlineInfo["transitionVideoUri"].is_string());
    std::string transVideoUri = callbacks.transitionVideoUriMapper(effectlineInfo["transitionVideoUri"]);
    effectlineInfo["transitionVideoUri"] = transVideoUri;

    CHECK_AND_RETURN(effectlineIndex > 0);
    nlohmann::json &prevEffectlineInfo = effectlineInfoArray[effectlineIndex - 1];
    CHECK_AND_RETURN(prevEffectlineInfo.is_object());
    CHECK_AND_RETURN(prevEffectlineInfo.contains("effect") && prevEffectlineInfo["effect"].is_string() &&
        prevEffectlineInfo["effect"] == EFFECTLINE_TYPE_MASK1 &&
        prevEffectlineInfo.contains("transitionVideoUri") && prevEffectlineInfo["transitionVideoUri"].is_string());
    prevEffectlineInfo["transitionVideoUri"] = transVideoUri;
}

void ParseEffectline(nlohmann::json &playInfo, const PlayInfoMapperCallbacks &callbacks)
{
    CHECK_AND_RETURN(playInfo.contains("effectline") && playInfo["effectline"].is_object() &&
        playInfo["effectline"].contains("effectline") && playInfo["effectline"]["effectline"].is_array());
    nlohmann::json &effectlineInfoArray = playInfo["effectline"]["effectline"];
    for (size_t i = 0; i < effectlineInfoArray.size(); i++) {
        ProcessEffectVideoUri(effectlineInfoArray, i, callbacks);
        ProcessTransitionVideoUri(effectlineInfoArray, i, callbacks);
        CHECK_AND_CONTINUE(effectlineInfoArray.is_array() && i < effectlineInfoArray.size());
        nlohmann::json &effectlineInfo = effectlineInfoArray[i];
        ProcessIds(effectlineInfo, callbacks);
        ProcessUris(effectlineInfo, callbacks);
    }
}

void ProcessTimelineInfo(nlohmann::json &timelineInfo, const PlayInfoMapperCallbacks &callbacks)
{
    CHECK_AND_RETURN(timelineInfo.is_object());
    if (timelineInfo.contains("effectVideoUri") && timelineInfo["effectVideoUri"].is_string()) {
        timelineInfo["effectVideoUri"] = callbacks.effectVideoUriMapper(timelineInfo["effectVideoUri"]);
    }
    if (timelineInfo.contains("transitionVideoUri") && timelineInfo["transitionVideoUri"].is_string()) {
        timelineInfo["transitionVideoUri"] = callbacks.transitionVideoUriMapper(timelineInfo["transitionVideoUri"]);
    }
    if (timelineInfo.contains("fileId") && timelineInfo["fileId"].is_array()) {
        for (auto &fileId : timelineInfo["fileId"]) {
            CHECK_AND_CONTINUE(fileId.is_number());
            fileId = callbacks.idMapper(fileId.get<int32_t>());
        }
    }
    if (timelineInfo.contains("fileUri") && timelineInfo["fileUri"].is_array()) {
        for (auto &fileUri : timelineInfo["fileUri"]) {
            CHECK_AND_CONTINUE(fileUri.is_string());
            fileUri = callbacks.photoUriMapper(fileUri.get<std::string>());
        }
    }
}

void ParseTimeline(nlohmann::json &playInfo, const PlayInfoMapperCallbacks &callbacks)
{
    CHECK_AND_RETURN(playInfo.contains("timeline") && playInfo["timeline"].is_array());
    for (nlohmann::json &timelineInfo : playInfo["timeline"]) {
        ProcessTimelineInfo(timelineInfo, callbacks);
    }
}
} // namespace

std::string MapPlayInfo(const std::string &playInfo, const PlayInfoMapperCallbacks &callbacks,
    const std::string &fallbackResult)
{
    nlohmann::json newPlayInfo = nlohmann::json::parse(playInfo, nullptr, false);
    CHECK_AND_RETURN_RET_LOG(!newPlayInfo.is_discarded(), fallbackResult, "parse json string failed.");
    ParseEffectline(newPlayInfo, callbacks);
    ParseTimeline(newPlayInfo, callbacks);
    return newPlayInfo.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
}

std::string RemapFilenameByFileIdMap(const std::string &filename,
    const std::unordered_map<int32_t, int32_t> &fileIdMap)
{
    std::string newFilename;
    size_t start = 0;
    bool first = true;
    while (start < filename.length()) {
        size_t underscorePos = filename.find("_", start);
        std::string segment = (underscorePos != std::string::npos)
            ? filename.substr(start, underscorePos - start)
            : filename.substr(start);
        if (!first) {
            newFilename += "_";
        }
        first = false;
        if (MediaLibraryDataManagerUtils::IsNumber(segment)) {
            int32_t id = std::stoi(segment);
            auto mapIt = fileIdMap.find(id);
            newFilename += (mapIt != fileIdMap.end()) ? std::to_string(mapIt->second) : segment;
        } else {
            newFilename += segment;
        }
        CHECK_AND_BREAK(underscorePos != std::string::npos);
        start = underscorePos + 1;
    }
    return newFilename;
}

struct UriFileIdSegment {
    size_t idStart;   // start index of the file_id segment in uri
    size_t idLen;     // length of the file_id segment
    size_t slashBeforeId; // position of the '/' just before the id segment
    size_t slashAfterId;  // position of the '/' just after the id segment
    int32_t fileId;
};

// Try to extract fileId from 3-level rfind pattern:
//   file://media/Photo|Video/{fileId}/{dirName}/{filename}
// Returns nullopt if the uri does not match this pattern or fileId is not in fileIdMap.
std::optional<UriFileIdSegment> ExtractFileIdSegment3Level(const std::string &uri,
    size_t lastSlash, const std::unordered_map<int32_t, int32_t> &fileIdMap)
{
    size_t midSlash = uri.rfind("/", lastSlash - 1);
    if (midSlash == std::string::npos || midSlash == 0) {
        return std::nullopt;
    }
    size_t firstSlash = uri.rfind("/", midSlash - 1);
    if (firstSlash == std::string::npos || midSlash <= firstSlash + 1) {
        return std::nullopt;
    }
    std::string idStr = uri.substr(firstSlash + 1, midSlash - firstSlash - 1);
    if (!MediaLibraryDataManagerUtils::IsNumber(idStr)) {
        return std::nullopt;
    }
    int32_t oldId = std::stoi(idStr);
    auto it = fileIdMap.find(oldId);
    if (it == fileIdMap.end()) {
        return std::nullopt;
    }
    return UriFileIdSegment{firstSlash + 1, midSlash - firstSlash - 1, firstSlash, midSlash, oldId};
}

// Try to extract fileId from 2-level rfind pattern:
//   file://media/highlight/video/{assetId}/{filename}
// Returns nullopt if the uri does not match this pattern or fileId is not in fileIdMap.
std::optional<UriFileIdSegment> ExtractFileIdSegment2Level(const std::string &uri,
    size_t lastSlash, const std::unordered_map<int32_t, int32_t> &fileIdMap)
{
    size_t leftIndex = uri.rfind("/", lastSlash - 1);
    if (leftIndex == std::string::npos || lastSlash <= leftIndex + 1) {
        return std::nullopt;
    }
    std::string idStr = uri.substr(leftIndex + 1, lastSlash - leftIndex - 1);
    if (!MediaLibraryDataManagerUtils::IsNumber(idStr)) {
        return std::nullopt;
    }
    int32_t oldId = std::stoi(idStr);
    auto it = fileIdMap.find(oldId);
    if (it == fileIdMap.end()) {
        return std::nullopt;
    }
    return UriFileIdSegment{leftIndex + 1, lastSlash - leftIndex - 1, leftIndex, lastSlash, oldId};
}

// Build remapped URI: replace fileId with newId, remap filename, preserve query string.
// For 3-level: prefix + newId + "/{dirName}/" + newFilename + suffix
// For 2-level: prefix + newId + "/" + newFilename + suffix
std::string BuildRemappedUri(const std::string &uri, const UriFileIdSegment &seg,
    size_t lastSlash, const std::unordered_map<int32_t, int32_t> &fileIdMap)
{
    auto it = fileIdMap.find(seg.fileId);
    std::string newIdStr = std::to_string(it->second);

    // seg.slashAfterId == lastSlash means 2-level pattern (id is directly before filename)
    if (seg.slashAfterId == lastSlash) {
        size_t queryPos = uri.find("?", lastSlash);
        std::string filename = (queryPos != std::string::npos)
            ? uri.substr(lastSlash + 1, queryPos - lastSlash - 1)
            : uri.substr(lastSlash + 1);
        std::string suffix = (queryPos != std::string::npos) ? uri.substr(queryPos) : "";
        std::string newFilename = RemapFilenameByFileIdMap(filename, fileIdMap);
        return uri.substr(0, seg.slashBeforeId + 1) + newIdStr + "/" + newFilename + suffix;
    }
    // 3-level pattern: only replace fileId in path, preserve filename as-is
    return uri.substr(0, seg.slashBeforeId + 1) + newIdStr + uri.substr(seg.slashAfterId);
}

// Extract file_id from URI path and remap it.
// Photo/Video URI: file://media/Photo/{fileId}/{dirName}/{filename}  — fileId is 3rd segment from end
// Highlight video URI: file://media/highlight/video/{assetId}/{filename} — assetId is 2nd segment from end
// The function tries 3-level rfind first (Photo/Video pattern), then falls back to 2-level (highlight).
std::string MapUriByFileIdMap(const std::string &uri,
    const std::unordered_map<int32_t, int32_t> &fileIdMap)
{
    CHECK_AND_RETURN_RET(!uri.empty(), uri);

    size_t lastSlash = uri.rfind("/");
    if (lastSlash == std::string::npos || lastSlash == 0) {
        return uri;
    }

    auto seg3 = ExtractFileIdSegment3Level(uri, lastSlash, fileIdMap);
    if (seg3.has_value()) {
        return BuildRemappedUri(uri, seg3.value(), lastSlash, fileIdMap);
    }

    auto seg2 = ExtractFileIdSegment2Level(uri, lastSlash, fileIdMap);
    if (seg2.has_value()) {
        return BuildRemappedUri(uri, seg2.value(), lastSlash, fileIdMap);
    }

    return uri;
}

PlayInfoMapperCallbacks CreateFileIdMapCallbacks(const std::unordered_map<int32_t, int32_t> &fileIdMap)
{
    PlayInfoMapperCallbacks callbacks;
    callbacks.idMapper = [&fileIdMap](int32_t oldId) -> int32_t {
        auto it = fileIdMap.find(oldId);
        return it != fileIdMap.end() ? it->second : oldId;
    };
    auto uriMapper = [&fileIdMap](const std::string &uri) -> std::string {
        return MapUriByFileIdMap(uri, fileIdMap);
    };
    callbacks.photoUriMapper = uriMapper;
    callbacks.effectVideoUriMapper = uriMapper;
    callbacks.transitionVideoUriMapper = uriMapper;
    return callbacks;
}

} // namespace OHOS::Media
