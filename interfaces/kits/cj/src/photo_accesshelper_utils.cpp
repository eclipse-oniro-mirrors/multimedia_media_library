/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "photo_accesshelper_utils.h"

#include "media_device_column.h"
#include "medialibrary_napi_enum_comm.h"

using namespace std;
using namespace OHOS::DataShare;
using namespace OHOS::FFI;

namespace OHOS {
namespace Media {

static const int32_t FIELD_IDX = 0;
static const int32_t VALUE_IDX = 1;

char *MallocCString(const std::string &origin)
{
    if (origin.empty()) {
        return nullptr;
    }
    auto len = origin.length() + 1;
    char *res = static_cast<char *>(malloc(sizeof(char) * len));
    if (res == nullptr) {
        return nullptr;
    }
    return std::char_traits<char>::copy(res, origin.c_str(), len);
}

static bool HandleSpecialDateTypePredicate(const OperationItem &item,
    vector<OperationItem> &operations, const FetchOptionType &fetchOptType)
{
    vector<string>dateTypes = { MEDIA_DATA_DB_DATE_ADDED, MEDIA_DATA_DB_DATE_TRASHED, MEDIA_DATA_DB_DATE_MODIFIED,
        MEDIA_DATA_DB_DATE_TAKEN};
    string dateType = item.GetSingle(FIELD_IDX);
    auto it = find(dateTypes.begin(), dateTypes.end(), dateType);
    if (it != dateTypes.end() && item.operation != DataShare::ORDER_BY_ASC &&
        item.operation != DataShare::ORDER_BY_DESC) {
        dateType += "_s";
        operations.push_back({ item.operation, { dateType, static_cast<double>(item.GetSingle(VALUE_IDX)) } });
        return true;
    }
    if (DATE_TRANSITION_MAP.count(dateType) != 0) {
        dateType = DATE_TRANSITION_MAP.at(dateType);
        operations.push_back({ item.operation, { dateType, static_cast<double>(item.GetSingle(VALUE_IDX)) } });
        return true;
    }
    return false;
}

static void MultiParamLpathToLowerCase(const vector<DataShare::MutliValue::Type>& originMultiParams,
    vector<DataShare::MutliValue::Type>& newMultiParams)
{
    for (const auto& multiParam : originMultiParams) {
        vector<string> stringVec = std::get_if<vector<string>>(&multiParam) ?
            std::get<vector<string>>(multiParam) : vector<string>();
        if (stringVec.empty()) {
            newMultiParams.push_back(multiParam);
            continue;
        }
        for (auto& str : stringVec) {
            std::transform(str.begin(), str.end(), str.begin(),
                [](unsigned char c) { return std::tolower(c); });
        }
        newMultiParams.push_back(stringVec);
    }
}

static void MakeLpathParamsCaseInsensitive(vector<OperationItem>& operations,
    const OperationItem& item)
{
    vector<DataShare::SingleValue::Type> newSingleParams {};
    vector<DataShare::MutliValue::Type> newMultiParams {};
    string lowerField = "lower(" + PhotoAlbumColumns::ALBUM_LPATH + ")";
    newSingleParams.push_back(lowerField);
    for (size_t i = 1; i < item.singleParams.size(); i++) { // start with 1 to skip field param
        string value = static_cast<string>(item.GetSingle(i));
        if (!value.empty()) {
            std::transform(value.begin(), value.end(), value.begin(),
                [](unsigned char c) { return std::tolower(c); });
            newSingleParams.push_back(value);
        } else {
            newSingleParams.push_back(item.singleParams[i]);
        }
    }
    if (!item.multiParams.empty()) {
        MultiParamLpathToLowerCase(item.multiParams, newMultiParams);
    }
    operations.push_back(
        { item.operation, newSingleParams, newMultiParams });
}

static void HandleUriParams(const OperationItem& item,
    vector<OperationItem>& operations, ExtraInfo& extraInfo)
{
    string uri = static_cast<string>(item.GetSingle(VALUE_IDX));
    MediaFileUri::RemoveAllFragment(uri);
    MediaFileUri fileUri(uri);
    extraInfo.uri = uri;
    if ((extraInfo.fetchOptType != ALBUM_FETCH_OPT) && (!fileUri.IsApi10())) {
        fileUri = MediaFileUri(MediaFileUtils::GetRealUriFromVirtualUri(uri));
    }
    extraInfo.networkId = fileUri.GetNetworkId();
    string field = (extraInfo.fetchOptType == ALBUM_FETCH_OPT) ? PhotoAlbumColumns::ALBUM_ID : MEDIA_DATA_DB_ID;
    operations.push_back({ item.operation, { field, fileUri.GetFileId() } });
}

static bool HandleSpecialPredicate(shared_ptr<DataSharePredicates> &predicatePtr,
    DataSharePredicates &predicates, ExtraInfo &extraInfo)
{
    vector<OperationItem> operations;
    auto &items = predicatePtr->GetOperationList();
    for (auto &item : items) {
        if (item.singleParams.empty()) {
            operations.push_back(item);
            continue;
        }
        if (HandleSpecialDateTypePredicate(item, operations, extraInfo.fetchOptType)) {
            continue;
        }
        if (static_cast<string>(item.GetSingle(FIELD_IDX)) == DEVICE_DB_NETWORK_ID) {
            if (item.operation != DataShare::EQUAL_TO || static_cast<string>(item.GetSingle(VALUE_IDX)).empty()) {
                LOGE("DEVICE_DB_NETWORK_ID predicates not support %{public}d", item.operation);
                return false;
            }
            extraInfo.networkId = static_cast<string>(item.GetSingle(VALUE_IDX));
            continue;
        } else if (static_cast<string>(item.GetSingle(FIELD_IDX)) == MEDIA_DATA_DB_URI) {
            if (item.operation != DataShare::EQUAL_TO) {
                LOGE("MEDIA_DATA_DB_URI predicates not support %{public}d", item.operation);
                return false;
            }
            HandleUriParams(item, operations, extraInfo);
            continue;
        }
        if (static_cast<string>(item.GetSingle(FIELD_IDX)) == PENDING_STATUS) {
            continue;
        }
        if (LOCATION_PARAM_MAP.find(static_cast<string>(item.GetSingle(FIELD_IDX))) != LOCATION_PARAM_MAP.end()) {
            continue;
        }
        if (item.operation != DataShare::ORDER_BY_ASC && item.operation != DataShare::ORDER_BY_DESC &&
            static_cast<string>(item.GetSingle(FIELD_IDX)) == PhotoAlbumColumns::ALBUM_LPATH) {
            MakeLpathParamsCaseInsensitive(operations, item);
            continue;
        }
        operations.push_back(item);
    }
    predicates = DataSharePredicates(move(operations));
    return true;
}

static bool GetLocationPredicate(shared_ptr<DataSharePredicates> &predicatePtr,
    DataSharePredicates &predicates, vector<string> &fetchColumn)
{
    map<string, string> locationMap;
    auto &items = predicatePtr->GetOperationList();
    for (auto &item : items) {
        if (item.singleParams.empty()) {
            continue;
        }
        if (LOCATION_PARAM_MAP.find(static_cast<string>(item.GetSingle(FIELD_IDX))) != LOCATION_PARAM_MAP.end()) {
            if (item.operation != DataShare::EQUAL_TO) {
                LOGE("location predicates not support %{public}d", item.operation);
                return false;
            }
            string param = static_cast<string>(item.GetSingle(FIELD_IDX));
            string value = static_cast<string>(item.GetSingle(VALUE_IDX));
            locationMap.insert(make_pair(param, value));
            if (param == DIAMETER) {
                continue;
            }
            if (LOCATION_PARAM_MAP.at(param).second == DataShare::GREATER_THAN_OR_EQUAL_TO) {
                predicates.GreaterThanOrEqualTo(LOCATION_PARAM_MAP.at(param).first, value);
                continue;
            }
            if (LOCATION_PARAM_MAP.at(param).second == DataShare::LESS_THAN) {
                predicates.LessThan(LOCATION_PARAM_MAP.at(param).first, value);
                continue;
            }
            if (LOCATION_PARAM_MAP.at(param).second == DataShare::EQUAL_TO) {
                predicates.EqualTo(LOCATION_PARAM_MAP.at(param).first, value);
                continue;
            }
        }
    }
    if (locationMap.count(DIAMETER) == 1 && locationMap.count(START_LATITUDE) == 1
        && locationMap.count(START_LONGITUDE) == 1) {
        // 0.5:Used for rounding down
        string latitudeIndex = "round((latitude - " + locationMap.at(START_LATITUDE) + ") / " +
            locationMap.at(DIAMETER) + " - 0.5)";
        string longitudeIndex = "round((longitude - " + locationMap.at(START_LONGITUDE) + ") / " +
            locationMap.at(DIAMETER) + " - 0.5)";
        string albumName = LATITUDE + "||'_'||" + LONGITUDE + "||'_'||" + latitudeIndex + "||'_'||" +
            longitudeIndex + " AS " + ALBUM_NAME;
        fetchColumn.push_back(albumName);
        string locationGroup = latitudeIndex + "," + longitudeIndex;
        predicates.GroupBy({ locationGroup });
    }
    return true;
}

bool GetPredicate(COptions options, DataSharePredicates &predicates, vector<string> &fetchColumn,
    ExtraInfo &extraInfo, int32_t &errCode)
{
    auto native = FFIData::GetData<DataSharePredicatesImpl>(options.predicates);
    if (native == nullptr) {
        LOGE("get DataSharePredicatesImpl failed.");
        errCode = JS_INNER_FAIL;
        return false;
    }
    auto predicatePtr = native->GetPredicates();
    if (predicatePtr == nullptr) {
        LOGE("DataSharePredicates is null");
        errCode = JS_INNER_FAIL;
        return false;
    }
    if (!HandleSpecialPredicate(predicatePtr, predicates, extraInfo) ||
        (!GetLocationPredicate(predicatePtr, predicates, fetchColumn))) {
        LOGE("invalid predicate");
        errCode = JS_ERR_PARAMETER_INVALID;
        return false;
    }
    return true;
}

static bool GetArrayProperty(COptions options,
    vector<string> &fetchColumn, int32_t &errCode)
{
    if (errCode != E_SUCCESS) {
        return false;
    }
    for (int64_t i = 0; i < options.fetchColumns.size; i++) {
        fetchColumn.emplace_back(string(options.fetchColumns.head[i]));
    }
    return true;
}

void GetFetchOption(COptions options, DataSharePredicates &predicates,
    vector<string> &fetchColumn, ExtraInfo &extraInfo, int32_t &errCode)
{
    errCode = E_SUCCESS;
    if (!GetPredicate(options, predicates, fetchColumn, extraInfo, errCode)) {
        LOGE("invalid predicate");
        return;
    }
    if (!GetArrayProperty(options, fetchColumn, errCode)) {
        LOGE("Failed to parse fetchColumn");
        return;
    }
}

void AddDefaultAssetColumns(vector<string> &fetchColumn,
    function<bool(const string &columnName)> isValidColumn, NapiAssetType assetType,
    int32_t &errCode, const PhotoAlbumSubType subType)
{
    auto validFetchColumns = MediaColumn::DEFAULT_FETCH_COLUMNS;
    if (assetType == TYPE_PHOTO) {
        validFetchColumns.insert(
            PhotoColumn::DEFAULT_FETCH_COLUMNS.begin(), PhotoColumn::DEFAULT_FETCH_COLUMNS.end());
    }
    switch (subType) {
        case PhotoAlbumSubType::FAVORITE:
            validFetchColumns.insert(MediaColumn::MEDIA_IS_FAV);
            break;
        case PhotoAlbumSubType::VIDEO:
            validFetchColumns.insert(MediaColumn::MEDIA_TYPE);
            break;
        case PhotoAlbumSubType::HIDDEN:
            validFetchColumns.insert(MediaColumn::MEDIA_HIDDEN);
            break;
        case PhotoAlbumSubType::TRASH:
            validFetchColumns.insert(MediaColumn::MEDIA_DATE_TRASHED);
            break;
        case PhotoAlbumSubType::SCREENSHOT:
        case PhotoAlbumSubType::CAMERA:
            validFetchColumns.insert(PhotoColumn::PHOTO_SUBTYPE);
            break;
        default:
            break;
    }
    for (const auto &column : fetchColumn) {
        if (column == PENDING_STATUS) {
            validFetchColumns.insert(MediaColumn::MEDIA_TIME_PENDING);
        } else if (isValidColumn(column) || (column == MEDIA_SUM_SIZE && MediaLibraryNapiUtils::IsSystemApp())) {
            validFetchColumns.insert(column);
        } else if (column == MEDIA_DATA_DB_URI) {
            continue;
        } else if (DATE_TRANSITION_MAP.count(column) != 0) {
            validFetchColumns.insert(DATE_TRANSITION_MAP.at(column));
        } else {
            errCode = JS_ERR_PARAMETER_INVALID;
            return;
        }
    }
    fetchColumn.assign(validFetchColumns.begin(), validFetchColumns.end());
}
}
}