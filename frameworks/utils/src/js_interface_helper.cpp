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

#include "js_interface_helper.h"
#include "media_file_utils.h"
#include "media_column.h"
#include "photo_album_column.h"

using namespace std;
namespace OHOS {
namespace Media {

string JsInterfaceHelper::MaskString(const string& str)
{
    constexpr char garble = '*';
    constexpr size_t garbleSmall = 3;
    constexpr size_t garbleLastTwo = 2;
    constexpr size_t garbleLastOne = 1;
    size_t length = str.size();
    string maskedString;
    if (length == 0) {
        return maskedString;
    }
    if (length == 1) { // only one character, needs to mask all
        maskedString = garble;
    } else if (length <= garbleSmall) {
        maskedString = string(length - garbleLastOne, garble) + str.substr(length - garbleLastOne);
    } else {
        maskedString = string(length - garbleLastTwo, garble) + str.substr(length - garbleLastTwo);
    }
    return maskedString;
}

string JsInterfaceHelper::GetSafeDisplayName(const string& displayName)
{
    if (displayName == "") {
        return displayName;
    }
    string extension;
    size_t splitIndex = displayName.find_last_of(".");
    if (splitIndex == string::npos) {
        extension = "";
    } else {
        extension = displayName.substr(splitIndex);
    }
    string title = MediaFileUtils::GetTitleFromDisplayName(displayName);
    if (title.empty()) {
        return displayName;
    }

    string safeTitle = MaskString(title);
    string safeDisplayName = safeTitle + extension;

    return safeDisplayName;
}

string JsInterfaceHelper::GetSafeUri(const string& uri)
{
    string safeUri = uri;
    if (uri == "") {
        return safeUri;
    }
    size_t splitIndex = safeUri.find_last_of("/");
    string displayName;
    if (splitIndex == string::npos) {
        return safeUri;
    } else {
        displayName = safeUri.substr(splitIndex + 1);
    }
    string safeDisplayName = GetSafeDisplayName(displayName);
    safeUri = safeUri.substr(0, splitIndex) + "/" + safeDisplayName;
    return safeUri;
}

template <typename T>
static string VectorToString(const vector<T>& vector, bool needsRedact, bool isUri, bool isDisplayName)
{
    std::string out = "[";
    for (size_t i = 0; i < vector.size(); ++i) {
        if (i > 0) {
            out += ", ";
        }

        if constexpr (std::is_same_v<T, std::string>) {
            if (!needsRedact) {
                out += '"' + vector[i] + '"';
            } else if (isUri) {
                out += '"' + JsInterfaceHelper::GetSafeUri(vector[i]) + '"';
            } else if (isDisplayName) {
                out += '"' + JsInterfaceHelper::GetSafeDisplayName(vector[i]) + '"';
            } else {
                out += '"' + JsInterfaceHelper::MaskString(vector[i]) + '"';
            }
        } else {
            out += std::to_string(vector[i]);
        }
    }
    out += "]";
    return out;
}

static string MultiValueTypeToString(const DataShare::MutliValue::Type& multiValue, bool needsRedact, bool isUri,
    bool isDisplayName)
{
    // DataShare::SingleValue::Type = std::variant<std::monostate, std::vector<int>, std::vector<int64_t>,
    //     std::vector<double>, std::vector<std::string>>;
    return std::visit([needsRedact, isUri, isDisplayName](const auto& value) -> std::string {
        using T = std::decay_t<decltype(value)>;

        if constexpr (std::is_same_v<T, std::monostate>) {
            return "T::monostate";
        } else if constexpr (std::is_same_v<T, std::vector<int>>) {
            return "int" + VectorToString(value, needsRedact, isUri, isDisplayName);
        } else if constexpr (std::is_same_v<T, std::vector<int64_t>>) {
            return "int64_t" + VectorToString(value, needsRedact, isUri, isDisplayName);
        } else if constexpr (std::is_same_v<T, std::vector<double>>) {
            return "double" + VectorToString(value, needsRedact, isUri, isDisplayName);
        } else {
            return "string" + VectorToString(value, needsRedact, isUri, isDisplayName);
        }
        }, multiValue);
}

static string SingleValueTypeToString(const DataShare::SingleValue::Type& singleValue, bool needsRedact, bool isUri,
    bool isDisplayName)
{
    // DataShare::SingleValue::Type = std::variant<std::monostate, int, double, std::string, bool, int64_t>;
    return std::visit([needsRedact, isUri, isDisplayName](const auto& value) -> std::string {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            return "T::monostate";
        } else if constexpr (std::is_same_v<T, bool>) {
            return value ? "bool::true" : "bool::false";
        } else if constexpr (std::is_same_v<T, std::string>) {
            if (!needsRedact) {
                return '"' + value + '"';
            } else if (isUri) {
                return '"' + JsInterfaceHelper::GetSafeUri(value) + '"';
            } else if (isDisplayName) {
                return '"' + JsInterfaceHelper::GetSafeDisplayName(value) + '"';
            } else {
                return '"' + JsInterfaceHelper::MaskString(value) + '"';
            }
        } else {
            return std::to_string(value);
        }
        }, singleValue);
}

static void LogParams(const DataShare::OperationItem& item, string& toPrint)
{
    const std::string mediaUri = "uri";
    const vector<string> sensitiveInfo = {
        mediaUri, MediaColumn::MEDIA_FILE_PATH, MediaColumn::MEDIA_TITLE, MediaColumn::MEDIA_NAME,
        MediaColumn::MEDIA_RELATIVE_PATH, MediaColumn::MEDIA_VIRTURL_PATH, PhotoColumn::PHOTO_LATITUDE,
        PhotoColumn::PHOTO_LONGITUDE, PhotoColumn::PHOTO_SOURCE_PATH, PhotoColumn::PHOTO_STORAGE_PATH,
        PhotoAlbumColumns::ALBUM_NAME, PhotoAlbumColumns::ALBUM_COVER_URI, PhotoAlbumColumns::ALBUM_LPATH
    };
    bool needsRedact = false;
    bool isUri = false;
    bool isDisplayName = false;
    toPrint += "SingleParams: [";
    for (size_t i = 0; i < item.singleParams.size(); i++) {
        if (i > 0) {
            toPrint += ", ";
        }
        toPrint += SingleValueTypeToString(item.singleParams[i], needsRedact, isUri, isDisplayName);
        if (i == 0 && std::find(sensitiveInfo.begin(), sensitiveInfo.end(),
            static_cast<string>(item.GetSingle(i))) != sensitiveInfo.end()) {
            needsRedact = true;
            isUri = (static_cast<string>(item.GetSingle(i)) == mediaUri ||
                static_cast<string>(item.GetSingle(i)) == PhotoAlbumColumns::ALBUM_COVER_URI);
            isDisplayName = (static_cast<string>(item.GetSingle(i)) == MediaColumn::MEDIA_NAME);
        }
    }
    toPrint += "], MultiParams: [";
    for (size_t i = 0; i < item.multiParams.size(); i++) {
        toPrint += MultiValueTypeToString(item.multiParams[i], needsRedact, isUri, isDisplayName);
    }
    toPrint += "]";
}

string JsInterfaceHelper::PredicateToStringSafe(const shared_ptr<DataShare::DataShareAbsPredicates>& predicate)
{
    if (predicate == nullptr) {
        return "[null]";
    }
    auto &items = predicate->GetOperationList();
    string toPrint = "";
    for (size_t i = 0; i < items.size(); i++) {
        if (i > 0) {
            toPrint += " || ";
        }
        LogParams(items[i], toPrint);
        toPrint += ", Operation: " + to_string(static_cast<int>(items[i].operation));
    }
    return toPrint;
}

} // namespace Media
} // namespace OHOS