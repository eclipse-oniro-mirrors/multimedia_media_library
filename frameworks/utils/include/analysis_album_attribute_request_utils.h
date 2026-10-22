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

#ifndef OHOS_MEDIA_ANALYSIS_ALBUM_ATTRIBUTE_REQUEST_UTILS_H
#define OHOS_MEDIA_ANALYSIS_ALBUM_ATTRIBUTE_REQUEST_UTILS_H

#include <memory>
#include <string>
#include <vector>

#include "analysis_album_attribute_const.h"
#include "analysis_album_attribute_validator.h"
#include "photo_album.h"

namespace OHOS::Media {
inline const std::string ANALYSIS_ALBUM_OPERATION_ATTR = "attr";
inline const std::string ANALYSIS_ALBUM_OPERATION_TYPE = "type";
inline const std::string ANALYSIS_ALBUM_OPERATION_VALUES = "values";

struct AnalysisAlbumOperation {
    std::string attr;
    std::string type;
    std::vector<std::string> values;

    bool HasValues() const
    {
        return !values.empty();
    }

    bool IsNoOp() const
    {
        return values.empty();
    }
};

inline bool IsPortraitAlbumAttributeTarget(const std::shared_ptr<PhotoAlbum> &photoAlbum)
{
    return photoAlbum != nullptr &&
        PhotoAlbum::IsSmartPortraitPhotoAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType());
}

inline bool IsUserPhotoAlbumAttributeTarget(const std::shared_ptr<PhotoAlbum> &photoAlbum)
{
    return photoAlbum != nullptr &&
        PhotoAlbum::IsUserPhotoAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType());
}

inline bool IsSourcePhotoAlbumAttributeTarget(const std::shared_ptr<PhotoAlbum> &photoAlbum)
{
    return photoAlbum != nullptr &&
        PhotoAlbum::IsSourceAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType());
}

template<typename RequestBody>
inline bool PrepareAnalysisAlbumOperationReqBody(const std::shared_ptr<PhotoAlbum> &photoAlbum,
    const AnalysisAlbumOperation &operation, RequestBody &reqBody)
{
    CHECK_AND_RETURN_RET(photoAlbum != nullptr && !operation.values.empty(), false);
    reqBody.albumId = std::to_string(photoAlbum->GetAlbumId());
    reqBody.attr = operation.attr;
    reqBody.type = operation.type;
    reqBody.values = operation.values;
    reqBody.albumType = photoAlbum->GetPhotoAlbumType();
    reqBody.albumSubType = photoAlbum->GetPhotoAlbumSubType();
    return true;
}

inline int32_t CheckAnalysisAlbumOperation(const AnalysisAlbumOperation &operation)
{
    return CheckAnalysisAlbumOperation(operation.attr, operation.type, operation.values);
}

inline int32_t CheckAnalysisAlbumOperationWithClientNoOpGuard(const AnalysisAlbumOperation &operation)
{
    return CheckAnalysisAlbumOperationWithClientNoOpGuard(operation.attr, operation.type, operation.values);
}

template<typename ParseStringProperty, typename ParseArrayProperty>
inline bool ParseAnalysisAlbumOperation(ParseStringProperty parseStringProperty,
    ParseArrayProperty parseArrayProperty, AnalysisAlbumOperation &operation)
{
    return parseStringProperty(ANALYSIS_ALBUM_OPERATION_ATTR, operation.attr) &&
        parseStringProperty(ANALYSIS_ALBUM_OPERATION_TYPE, operation.type) &&
        parseArrayProperty(ANALYSIS_ALBUM_OPERATION_VALUES, operation.values);
}

template<typename ThrowInvalid, typename ThrowUnsupported>
inline void ThrowAnalysisAlbumOperationCheckError(int32_t checkResult, ThrowInvalid throwInvalid,
    ThrowUnsupported throwUnsupported)
{
    checkResult == E_INVALID_VALUES ? throwInvalid() : throwUnsupported();
}

template<typename ThrowInvalid, typename ThrowUnsupported>
inline bool CheckAnalysisAlbumOperationOrThrow(const AnalysisAlbumOperation &operation, ThrowInvalid throwInvalid,
    ThrowUnsupported throwUnsupported)
{
    int32_t checkResult = CheckAnalysisAlbumOperationWithClientNoOpGuard(operation);
    CHECK_AND_RETURN_RET(checkResult == E_OK, (ThrowAnalysisAlbumOperationCheckError(checkResult,
        throwInvalid, throwUnsupported), false));
    return true;
}

} // namespace OHOS::Media

#endif // OHOS_MEDIA_ANALYSIS_ALBUM_ATTRIBUTE_REQUEST_UTILS_H
