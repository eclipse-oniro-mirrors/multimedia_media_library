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

#include "medialibrary_command.h"

#include "media_file_uri.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_data_manager_utils.h"
#include "medialibrary_db_const.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_errno.h"
#include "userfilemgr_uri.h"

using namespace std;
using namespace OHOS::NativeRdb;
using namespace OHOS::DataShare;

namespace OHOS {
namespace Media {

MediaLibraryCommand::MediaLibraryCommand(const Uri &uri) : uri_(uri)
{
    ParseOprnObjectFromUri();
    ParseOprnTypeFromUri();
    ParseQuerySetMapFromUri();
    SetApiFromQuerySetMap();
    ParseOprnObjectFromFileUri();
    ParseTableName();
}

MediaLibraryCommand::MediaLibraryCommand(const Uri &uri, const ValuesBucket &value) : uri_(uri), insertValue_(value)
{
    ParseOprnObjectFromUri();
    ParseOprnTypeFromUri();
    ParseQuerySetMapFromUri();
    SetApiFromQuerySetMap();
    ParseOprnObjectFromFileUri();
    ParseTableName();
}

MediaLibraryCommand::MediaLibraryCommand(const Uri &uri, const OperationType &oprnType) : uri_(uri), oprnType_(oprnType)
{
    ParseOprnObjectFromUri();
    ParseQuerySetMapFromUri();
    SetApiFromQuerySetMap();
    ParseOprnObjectFromFileUri();
    ParseTableName();
}

MediaLibraryCommand::MediaLibraryCommand(const OperationObject &oprnObject, const OperationType &oprnType,
    MediaLibraryApi api) : oprnObject_(oprnObject), oprnType_(oprnType), api_(api)
{
    ParseTableName();
}

MediaLibraryCommand::MediaLibraryCommand(const OperationObject &oprnObject, const OperationType &oprnType,
    const ValuesBucket &value, MediaLibraryApi api) :insertValue_(value), oprnObject_(oprnObject),
    oprnType_(oprnType), api_(api)
{
    ParseTableName();
}

MediaLibraryCommand::MediaLibraryCommand(const OperationObject &oprnObject, const OperationType &oprnType,
    const string &networkId, MediaLibraryApi api) : oprnObject_(oprnObject), oprnType_(oprnType),
    oprnDevice_(networkId), api_(api)
{
    ParseTableName();
}

MediaLibraryCommand::MediaLibraryCommand(const string &tableName, MediaLibraryApi api)
    : tableName_(tableName), api_(api)
{}

MediaLibraryCommand::MediaLibraryCommand(const ValuesBucket &value, MediaLibraryApi api)
    : insertValue_(value), api_(api)
{}

MediaLibraryCommand::MediaLibraryCommand(const DataSharePredicates &pred, MediaLibraryApi api)
    : datasharePred_(make_unique<const DataSharePredicates>(pred)), api_(api)
{}

MediaLibraryCommand::~MediaLibraryCommand() {}

// set functions
void MediaLibraryCommand::SetApiParam(const std::string &key, const std::string &param)
{
    querySetMap_[key] = param;
}

void MediaLibraryCommand::SetOprnObject(OperationObject object)
{
    oprnObject_ = object;
}

void MediaLibraryCommand::SetOprnAssetId(const std::string &oprnId)
{
    oprnFileId_ = oprnId;
}

void MediaLibraryCommand::SetValueBucket(const NativeRdb::ValuesBucket &value)
{
    insertValue_ = value;
}

void MediaLibraryCommand::SetTableName(const std::string &tableName)
{
    tableName_ = tableName;
}

void MediaLibraryCommand::SetBundleName(const std::string &bundleName)
{
    bundleName_ = bundleName;
}

void MediaLibraryCommand::SetDeviceName(const std::string &deviceName)
{
    deviceName_ = deviceName;
}

void MediaLibraryCommand::SetResult(const std::string &result)
{
    result_ = result;
}

bool MediaLibraryCommand::IsDataSharePredNull() const
{
    return datasharePred_ == nullptr;
}

// get functions
OperationObject MediaLibraryCommand::GetOprnObject() const
{
    return oprnObject_;
}

OperationType MediaLibraryCommand::GetOprnType() const
{
    return oprnType_;
}

ValuesBucket &MediaLibraryCommand::GetValueBucket()
{
    return insertValue_;
}

AbsRdbPredicates *MediaLibraryCommand::GetAbsRdbPredicates()
{
    if (absRdbPredicates_ == nullptr) {
        InitAbsRdbPredicates();
    }
    return absRdbPredicates_.get();
}

const string &MediaLibraryCommand::GetTableName()
{
    if (tableName_.empty()) {
        ParseTableName();
    }

    return tableName_;
}

const string &MediaLibraryCommand::GetOprnFileId()
{
    if (oprnFileId_.empty()) {
        ParseFileId();
    }

    return oprnFileId_;
}

const string &MediaLibraryCommand::GetOprnDevice()
{
    if (oprnDevice_.empty()) {
        oprnDevice_ = MediaFileUtils::GetNetworkIdFromUri(uri_.ToString());
    }
    return oprnDevice_;
}

const Uri &MediaLibraryCommand::GetUri() const
{
    return uri_;
}

const string &MediaLibraryCommand::GetBundleName()
{
    return bundleName_;
}

const string &MediaLibraryCommand::GetDeviceName()
{
    return deviceName_;
}

const string &MediaLibraryCommand::GetResult()
{
    return result_;
}

string MediaLibraryCommand::GetUriStringWithoutSegment() const
{
    string uriString = uri_.ToString();
    size_t questionMaskPoint = uriString.rfind('?');
    size_t hashKeyPoint = uriString.rfind('#');
    if (questionMaskPoint != string::npos) {
        return uriString.substr(0, questionMaskPoint);
    }
    if (hashKeyPoint != string::npos) {
        return uriString.substr(0, hashKeyPoint);
    }
    return uriString;
}

MediaLibraryApi MediaLibraryCommand::GetApi()
{
    return api_;
}

string MediaLibraryCommand::GetQuerySetParam(const std::string &key)
{
    if (key.empty() || querySetMap_.find(key) == querySetMap_.end()) {
        return "";
    }
    return querySetMap_[key];
}

void MediaLibraryCommand::ParseOprnObjectFromUri()
{
    const string opObject = MediaFileUri::GetPathFirstDentry(uri_);
    if (OPRN_OBJ_MAP.find(opObject) != OPRN_OBJ_MAP.end()) {
        oprnObject_ = OPRN_OBJ_MAP.at(opObject);
    }
    MEDIA_DEBUG_LOG("Command operation object is %{public}d", oprnObject_);
}

void MediaLibraryCommand::ParseOprnTypeFromUri()
{
    const string opType = MediaFileUri::GetPathSecondDentry(uri_);
    if (OPRN_TYPE_MAP.find(opType) != OPRN_TYPE_MAP.end()) {
        oprnType_ = OPRN_TYPE_MAP.at(opType);
    } else {
        oprnType_ = OperationType::QUERY;
    }
    MEDIA_DEBUG_LOG("Command operation type is %{public}d", oprnType_);
}

static string GetDistTable(const string &table, const string &networkId)
{
    string ret = MEDIALIBRARY_TABLE;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        return ret;
    }

    int errCode = E_ERR;
    if (table == PhotoColumn::PHOTOS_TABLE || table == AudioColumn::AUDIOS_TABLE) {
        ret = rdbStore->ObtainDistributedTableName(networkId, table, errCode);
    } else {
        ret = rdbStore->ObtainDistributedTableName(networkId, MEDIALIBRARY_TABLE, errCode);
    }
    return ret;
}

void MediaLibraryCommand::ParseTableName()
{
    if (TABLE_NAME_MAP.find(oprnObject_) != TABLE_NAME_MAP.end()) {
        auto cmdObj = TABLE_NAME_MAP.at(oprnObject_);
        if (cmdObj.find(oprnType_) != cmdObj.end()) {
            tableName_ = cmdObj.at(oprnType_);
        } else if (cmdObj.find(OperationType::UNKNOWN_TYPE) != cmdObj.end()) {
            tableName_ = cmdObj.at(OperationType::UNKNOWN_TYPE);
        } else {
            tableName_ = MEDIALIBRARY_TABLE;
        }
    } else {
        tableName_ = MEDIALIBRARY_TABLE;
    }
    // distributed tablename, smartalbum and smartalbumMap can not distributed
    if ((oprnObject_ == OperationObject::SMART_ALBUM) || (oprnObject_ == OperationObject::SMART_ALBUM_MAP)) {
        MEDIA_DEBUG_LOG("smart table name is %{public}s", tableName_.c_str());
        return;
    }
    // distributed tablename
    auto networkId = GetOprnDevice();
    if (networkId.empty()) {
        return;
    }
    tableName_ = GetDistTable(tableName_, networkId);
    MEDIA_INFO_LOG("Table name is %{public}s", tableName_.c_str());
}

void MediaLibraryCommand::InitAbsRdbPredicates()
{
    if (tableName_.empty()) {
        ParseTableName();
    }
    absRdbPredicates_ = make_unique<AbsRdbPredicates>(tableName_);
}

void MediaLibraryCommand::ParseFileId()
{
    int32_t fileIdInValue = -1;
    ValueObject valueObject;
    if (insertValue_.GetObject(MEDIA_DATA_DB_ID, valueObject)) {
        valueObject.GetInt(fileIdInValue);
    }
    if (fileIdInValue != -1) {
        oprnFileId_ = to_string(fileIdInValue);
        return;
    }
    string uriInValue;
    if (insertValue_.GetObject(MEDIA_DATA_DB_URI, valueObject)) {
        valueObject.GetString(uriInValue);
    }
    if (uriInValue.empty()) {
        uriInValue = GetUriStringWithoutSegment();
    }

    string idFromUri = MediaFileUtils::GetIdFromUri(uriInValue);
    if (!MediaLibraryDataManagerUtils::IsNumber(idFromUri)) {
        return;
    }
    oprnFileId_ = idFromUri;
}

void MediaLibraryCommand::ParseQuerySetMapFromUri()
{
    // uri format: datashare:///media/photo_operation/create_asset?op1=xxx&op2=yyy&api_version=10#abc
    // QuerySetMap: {"op1": "xxx", "op2": "yyy", "api_version": "10"}
    string uriStr = uri_.ToString();
    size_t cutPoint = uriStr.find('#');
    if (cutPoint != string::npos) {
        uriStr = uriStr.substr(0, cutPoint);
    }
    MediaFileUri mediaUri(uriStr);
    querySetMap_ = mediaUri.GetQueryKeys();
}

void MediaLibraryCommand::SetApiFromQuerySetMap()
{
    if (querySetMap_.find(URI_PARAM_API_VERSION) == querySetMap_.end()) {
        api_ = MediaLibraryApi::API_OLD;
    } else {
        string apiString = querySetMap_[URI_PARAM_API_VERSION];
        if (!MediaLibraryDataManagerUtils::IsNumber(apiString)) {
            api_ = MediaLibraryApi::API_OLD;
            return;
        }
        int32_t apiNum = stoi(apiString);
        if (apiNum <= static_cast<int32_t>(MediaLibraryApi::API_START) ||
            apiNum >= static_cast<int32_t>(MediaLibraryApi::API_END)) {
            MEDIA_ERR_LOG("this api num is wrong: %{public}d", apiNum);
            api_ = MediaLibraryApi::API_OLD;
        } else {
            api_ = static_cast<MediaLibraryApi>(apiNum);
        }
    }
}

int32_t MediaLibraryCommand::GetVideoType()
{
    if (querySetMap_.find(VIDEO_TYPE_KEYWORD) != querySetMap_.end()) {
        return std::stoul(querySetMap_[VIDEO_TYPE_KEYWORD]);
    }
    return 0;
}

void MediaLibraryCommand::ParseOprnObjectFromFileUri()
{
    if (oprnObject_ != OperationObject::UNKNOWN_OBJECT) {
        return;
    }

    string uri = uri_.ToString();
    // parse cache file uri
    if (MediaFileUtils::StartsWith(uri, PhotoColumn::PHOTO_CACHE_URI_PREFIX)) {
        oprnObject_ = OperationObject::PAH_PHOTO;
        return;
    }

    for (const auto &item : OPRN_MAP) {
        if (MediaFileUtils::StartsWith(uri, item.first)) {
            oprnObject_ = item.second;
            break;
        }
    }
}

void MediaLibraryCommand::SetDataSharePred(const DataSharePredicates &pred)
{
    datasharePred_ = make_unique<const DataSharePredicates>(pred);
}

// Caller is responsible for calling SetDataSharePred() firstly, before calling GetDataSharePred()
const DataSharePredicates &MediaLibraryCommand::GetDataSharePred() const
{
    return *datasharePred_;
}

} // namespace Media
} // namespace OHOS
