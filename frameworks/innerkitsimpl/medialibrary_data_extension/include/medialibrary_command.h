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

#ifndef OHOS_MEDIALIBRARY_COMMAND_PARSE_H
#define OHOS_MEDIALIBRARY_COMMAND_PARSE_H

#include <string>
#include <unordered_map>
#include <vector>

#include "abs_rdb_predicates.h"
#include "datashare_predicates.h"
#include "dir_asset.h"
#include "medialibrary_db_const.h"
#include "medialibrary_operation.h"
#include "uri.h"
#include "values_bucket.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class MediaLibraryCommand {
public:
    EXPORT explicit MediaLibraryCommand(const Uri &uri);
    EXPORT MediaLibraryCommand(const Uri &uri, const NativeRdb::ValuesBucket &value);
    EXPORT MediaLibraryCommand(const Uri &uri, const OperationType &oprnType);
    EXPORT MediaLibraryCommand(const OperationObject &oprnObject, const OperationType &oprnType,
        MediaLibraryApi api = MediaLibraryApi::API_OLD);
    EXPORT MediaLibraryCommand(const OperationObject &oprnObject, const OperationType &oprnType,
        const NativeRdb::ValuesBucket &value, MediaLibraryApi api = MediaLibraryApi::API_OLD);
    EXPORT MediaLibraryCommand(const OperationObject &oprnObject, const OperationType &oprnType,
        const std::string &networkId, MediaLibraryApi api = MediaLibraryApi::API_OLD);
    MediaLibraryCommand(const std::string &tableName, MediaLibraryApi api = MediaLibraryApi::API_OLD);
    EXPORT MediaLibraryCommand(const NativeRdb::ValuesBucket &value, MediaLibraryApi api = MediaLibraryApi::API_OLD);
    MediaLibraryCommand(const DataShare::DataSharePredicates &pred, MediaLibraryApi api = MediaLibraryApi::API_OLD);
    EXPORT MediaLibraryCommand() = delete;
    EXPORT ~MediaLibraryCommand();
    EXPORT MediaLibraryCommand(const MediaLibraryCommand &) = delete;
    EXPORT MediaLibraryCommand &operator=(const MediaLibraryCommand &) = delete;
    EXPORT MediaLibraryCommand(MediaLibraryCommand &&) = delete;
    EXPORT MediaLibraryCommand &operator=(MediaLibraryCommand &&) = delete;

    EXPORT OperationObject GetOprnObject() const;
    EXPORT OperationType GetOprnType() const;
    EXPORT const std::string &GetTableName();
    EXPORT NativeRdb::ValuesBucket &GetValueBucket();
    EXPORT NativeRdb::AbsRdbPredicates *GetAbsRdbPredicates();
    EXPORT const std::string &GetOprnFileId();
    EXPORT const std::string &GetOprnDevice();
    EXPORT const Uri &GetUri() const;
    EXPORT const std::string &GetBundleName();
    EXPORT const std::string &GetDeviceName();
    EXPORT std::string GetUriStringWithoutSegment() const;
    EXPORT MediaLibraryApi GetApi();
    EXPORT std::string GetQuerySetParam(const std::string &key);
    EXPORT void SetDataSharePred(const DataShare::DataSharePredicates &pred);
    EXPORT const DataShare::DataSharePredicates &GetDataSharePred() const;
    EXPORT const std::string &GetResult();
    EXPORT int32_t GetVideoType();

    EXPORT void SetApiParam(const std::string &key, const std::string &param);
    EXPORT void SetOprnObject(OperationObject object);
    EXPORT void SetOprnAssetId(const std::string &oprnId);
    EXPORT void SetValueBucket(const NativeRdb::ValuesBucket &value);
    EXPORT void SetTableName(const std::string &tableName);
    EXPORT void SetBundleName(const std::string &bundleName);
    EXPORT void SetDeviceName(const std::string &deviceName);
    EXPORT void SetResult(const std::string &result);
    EXPORT bool IsDataSharePredNull() const;

private:
    void ParseOprnObjectFromUri();
    void ParseOprnTypeFromUri();
    void ParseTableName();
    void InitAbsRdbPredicates();
    void ParseFileId();
    void ParseQuerySetMapFromUri();
    void SetApiFromQuerySetMap();
    void ParseOprnObjectFromFileUri();

    Uri uri_ {""};
    NativeRdb::ValuesBucket insertValue_;
    std::unique_ptr<NativeRdb::AbsRdbPredicates> absRdbPredicates_;
    std::unique_ptr<const DataShare::DataSharePredicates> datasharePred_;
    OperationObject oprnObject_ {OperationObject::UNKNOWN_OBJECT};
    OperationType oprnType_ {OperationType::UNKNOWN_TYPE};
    std::string oprnFileId_;
    std::string oprnDevice_;
    std::string tableName_;
    std::string bundleName_;
    std::string deviceName_;
    std::unordered_map<std::string, std::string> querySetMap_;
    std::string result_ = "";
    MediaLibraryApi api_;
};

} // namespace Media
} // namespace OHOS

#endif // OHOS_MEDIALIBRARY_COMMAND_PARSE_H
