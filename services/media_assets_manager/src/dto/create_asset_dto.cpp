/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "create_asset_dto.h"

namespace OHOS::Media {

CreateAssetDto::CreateAssetDto(const CreateAssetReqBody &reqBody)
{
    this->mediaType = reqBody.mediaType;
    this->photoSubtype = reqBody.photoSubtype;
    this->title = reqBody.title;
    this->extension = reqBody.extension;
    this->displayName = reqBody.displayName;
    this->cameraShotKey = reqBody.cameraShotKey;
}

CreateAssetDto::CreateAssetDto(const CreateAssetForAppReqBody &reqBody)
{
    this->tokenId = reqBody.tokenId;
    this->mediaType = reqBody.mediaType;
    this->photoSubtype = reqBody.photoSubtype;
    this->title = reqBody.title;
    this->extension = reqBody.extension;
    this->bundleName = reqBody.bundleName;
    this->packageName = reqBody.packageName;
    this->appId = reqBody.appId;
    this->ownerAlbumId = reqBody.ownerAlbumId;
}

CreateAssetDto::CreateAssetDto(const CreateFileMgrAssetReqBody &reqBody)
{
    this->displayName = reqBody.displayName;
    this->ownerAlbumId = reqBody.ownerAlbumId;
}

CreateAssetDto::CreateAssetDto(const CreateAssetsWithAlbumReqBody &reqBody)
{
    this->mediaType = reqBody.mediaType;
    this->title = reqBody.title;
    this->extension = reqBody.extension;
    this->ownerAlbumId = reqBody.ownerAlbumId;
    this->isRealTimeThumb = reqBody.isRealTimeThumb;
}

CreateAssetRespBody CreateAssetDto::GetRespBody()
{
    CreateAssetRespBody respBody;
    respBody.fileId = this->fileId;
    respBody.outUri = this->outUri;
    return respBody;
}
}  // namespace OHOS::Media
