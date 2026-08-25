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

#include "dfx_utils.h"

#include <chrono>
#include <iomanip>
#include <sstream>

#include "dfx_const.h"
#include "media_file_utils.h"
#include "media_string_utils.h"
#include "media_log.h"
using namespace std;
namespace OHOS {
namespace Media {
namespace {
    constexpr int ONE_MORE         = 1;
    constexpr int ONE_SECOND       = 1;
    constexpr int32_t BASEYEAR     = 1900;
    constexpr int32_t LENGTH_TWO   = 2;
    constexpr int32_t LENGTH_THREE = 3;
}

static const char *UTF16_CERROR = "__CONVERSION_ERROR__";
static const char16_t *g_utf8Cerror = u"__CONVERSION_ERROR__";

vector<string> DfxUtils::Split(string &input, const string &pattern)
{
    vector<string> result;
    if (input == "") {
        return result;
    }
    string strs = input + pattern;
    size_t pos = strs.find(pattern);
    while (pos != strs.npos) {
        string temp = strs.substr(0, pos);
        result.push_back(temp);
        strs = strs.substr(pos + 1, strs.size());
        pos = strs.find(pattern);
    }
    return result;
}

string DfxUtils::GetSafePath(const string &path)
{
    string safePath = path;
    if (path == "") {
        return safePath;
    }
    if (MediaStringUtils::StartsWith(path, CLOUD_PHOTO_PATH)) {
        return safePath.replace(0, CLOUD_PHOTO_PATH.length(), GARBLE);
    }
    safePath = safePath.replace(0, CLOUD_FILE_PATH.length(), GARBLE);
    size_t splitIndex = safePath.find_last_of(SPLIT_PATH);
    string displayName;
    if (splitIndex == string::npos) {
        displayName = "";
    } else {
        displayName = safePath.substr(splitIndex + 1);
    }
    string safeDisplayName = GetSafeDiaplayName(displayName);
    safePath = safePath.substr(0, splitIndex) + safeDisplayName;
    return safePath;
}

string DfxUtils::GetSafeUri(const string &uri)
{
    string safeUri = uri;
    if (uri == "") {
        return safeUri;
    }
    size_t splitIndex = safeUri.find_last_of(SPLIT_PATH);
    string displayName;
    if (splitIndex == string::npos) {
        return safeUri;
    } else {
        displayName = safeUri.substr(splitIndex + 1);
    }
    string safeDisplayName = GetSafeDiaplayName(displayName);
    safeUri = safeUri.substr(0, splitIndex) + "/" + safeDisplayName;
    return safeUri;
}

string DfxUtils::GetSafeDiaplayName(string &displayName)
{
    if (displayName == "") {
        return displayName;
    }
    string extension;
    size_t splitIndex = displayName.find_last_of(DOT);
    if (splitIndex == string::npos) {
        extension = "";
    } else {
        extension = displayName.substr(splitIndex);
    }
    string title = MediaFileUtils::GetTitleFromDisplayName(displayName);
    if (title == "") {
        return title;
    }
    uint32_t length = title.size();
    string safeDisplayName;
    if (length <= GARBLE_SMALL) {
        safeDisplayName = GARBLE + title.substr(length - GARBLE_LAST_ONE) + extension;
    } else if (length > GARBLE_LARGE) {
        safeDisplayName = GARBLE + title.substr(GARBLE_LARGE) + extension;
    } else {
        safeDisplayName = GARBLE + title.substr(length - GARBLE_LAST_TWO) + extension;
    }
    return safeDisplayName;
}

string DfxUtils::GetCurrentDate()
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm* tmPtr = std::localtime(&time);
    if (tmPtr == nullptr) {
        MEDIA_ERR_LOG("GetCurrentDate failed: tmPtr is nullptr");
        return "";
    }
    std::stringstream ss;
    ss << std::put_time(tmPtr, "%Y-%m-%d");
    return ss.str();
}

string DfxUtils::GetCurrentDateMillisecond()
{
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm* now_tm = std::localtime(&now_time);
    if (now_tm == nullptr) {
        MEDIA_ERR_LOG("GetCurrentDateMillisecond failed: now_tm is nullptr");
        return "";
    }
    auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
    std::chrono::duration<int, std::milli> ms_part = now_ms.time_since_epoch() % std::chrono::seconds(ONE_SECOND);

    std::stringstream ss;
    ss << (now_tm->tm_year + BASEYEAR) << '-'
        << std::setw(LENGTH_TWO) << std::setfill('0') << (now_tm->tm_mon + ONE_MORE) << '-'
        << std::setw(LENGTH_TWO) << std::setfill('0') << now_tm->tm_mday << ' '
        << std::setw(LENGTH_TWO) << std::setfill('0') << now_tm->tm_hour << ':'
        << std::setw(LENGTH_TWO) << std::setfill('0') << now_tm->tm_min << ':'
        << std::setw(LENGTH_TWO) << std::setfill('0') << now_tm->tm_sec << '.'
        << std::setw(LENGTH_THREE) << ms_part.count();
    return ss.str();
}

string DfxUtils::JoinStrings(const unordered_set<string>& strSet, char delimiter)
{
    string result;
    for (auto it = strSet.begin(); it != strSet.end(); ++it) {
        if (it != strSet.begin()) {
            result += delimiter;
        }
        result += *it;
    }
    return result;
}

unordered_set<string> DfxUtils::SplitString(const string& input, char delimiter)
{
    if (input.empty()) {
        return {};
    }

    unordered_set<std::string> result;
    std::istringstream iss(input);
    string token;
    while (std::getline(iss, token, delimiter)) {
        result.emplace(token);
    }
    return result;
}

string DfxUtils::GetSafeAlbumName(const string& albumName)
{
    if (albumName == "") {
        return albumName;
    }
    uint32_t length = albumName.size();
    string safeAlbumName;
    if (length <= GARBLE_SMALL) {
        safeAlbumName = GARBLE + albumName.substr(length - GARBLE_LAST_ONE);
    } else if (length > GARBLE_LARGE) {
        safeAlbumName = GARBLE + albumName.substr(GARBLE_LARGE);
    } else {
        safeAlbumName = GARBLE + albumName.substr(length - GARBLE_LAST_TWO);
    }
    return safeAlbumName;
}

std::u16string DfxUtils::Str8ToStr16(const std::string &inputStr)
{
    if (inputStr.empty()) {
        return u"";
    }
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert(UTF16_CERROR, g_utf8Cerror);
    std::u16string result = convert.from_bytes(inputStr);
    return result == g_utf8Cerror ? u"" : result;
}

std::string DfxUtils::Str16ToStr8(const std::u16string &inputStr)
{
    if (inputStr.empty()) {
        return "";
    }
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert(UTF16_CERROR, g_utf8Cerror);
    std::string result = convert.to_bytes(inputStr);
    return result == UTF16_CERROR ? "" : result;
}

string DfxUtils::GetSafeAlbumNameWhenChinese(const string &albumName)
{
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

string DfxUtils::GetSafeDiaplayNameWhenChinese(const string &displayName)
{
    CHECK_AND_RETURN_RET_LOG(!displayName.empty(), "", "input displayName is empty");
    string extension;
    size_t splitIndex = displayName.find_last_of(DOT);
    if (splitIndex == string::npos) {
        extension = "";
    } else {
        extension = displayName.substr(splitIndex);
    }
    string title = MediaFileUtils::GetTitleFromDisplayName(displayName);
    CHECK_AND_RETURN_RET_LOG(!title.empty(), "", "input title is empty");
    std::u16string wideStr = Str8ToStr16(title);
    uint32_t length = wideStr.size();
    if (length <= 0) {
        return GARBLE + extension;
    }
    std::u16string safeTitle;
    if (length <= GARBLE_SMALL) {
        safeTitle = wideStr.substr(length - GARBLE_LAST_ONE);
    } else if (length > GARBLE_LARGE) {
        safeTitle = wideStr.substr(GARBLE_LARGE);
    } else {
        safeTitle = wideStr.substr(length - GARBLE_LAST_TWO);
    }
 
    return GARBLE + Str16ToStr8(safeTitle) + extension;
}
} // namespace Media
} // namespace OHOS