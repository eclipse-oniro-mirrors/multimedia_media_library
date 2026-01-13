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

#include "media_assets_change_request_ani.h"
#include <memory>
#include <string>
#include <vector>
#include <unordered_set>
#include <utility>
#include "ani_class_name.h"
#include "medialibrary_ani_utils.h"
#include "userfile_client.h"
#include "user_define_ipc_client.h"
#include "medialibrary_business_code.h"
#include "modify_assets_vo.h"

namespace OHOS::Media {

constexpr int32_t YES = 1;
constexpr int32_t NO = 0;
constexpr int32_t USER_COMMENT_MAX_LEN = 420;

MediaAssetsChangeRequestAni::MediaAssetsChangeRequestAni(vector<shared_ptr<FileAsset>> fileAssets)
{
    fileAssets_ = fileAssets;
    isFavorite_ = false;
    isHidden_ = false;
    isRecentShow_ = false;
}

MediaAssetsChangeRequestAni::~MediaAssetsChangeRequestAni()
{
    assetsChangeOperations_.clear();
    fileAssets_.clear();
}

void MediaAssetsChangeRequestAni::SetFavorite([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object,
    ani_boolean isFavorite)
{
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return;
    }
    auto asyncContext = std::make_unique<MediaAssetsChangeRequestAniContext>();
    CHECK_NULL_PTR_RETURN_VOID(asyncContext, "Failed to create asyncContext");
    asyncContext->objectInfo = Unwrap(env, object);
    auto changeRequest = asyncContext->objectInfo;
    CHECK_NULL_PTR_RETURN_VOID(changeRequest, "changeRequest is null");
    changeRequest->isFavorite_ = isFavorite;
    for (const auto& fileAsset : changeRequest->fileAssets_) {
        CHECK_NULL_PTR_RETURN_VOID(fileAsset, "fileAsset is null");
        fileAsset->SetFavorite(isFavorite);
    }
    changeRequest->assetsChangeOperations_.push_back(AssetsChangeOperation::BATCH_SET_FAVORITE);
}

void MediaAssetsChangeRequestAni::SetHidden([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object,
    ani_boolean isHidden)
{
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return;
    }

    auto asyncContext = std::make_unique<MediaAssetsChangeRequestAniContext>();
    CHECK_NULL_PTR_RETURN_VOID(asyncContext, "Failed to create asyncContext");
    asyncContext->objectInfo = Unwrap(env, object);
    auto changeRequest = asyncContext->objectInfo;
    CHECK_NULL_PTR_RETURN_VOID(changeRequest, "changeRequest is null");
    changeRequest->isHidden_ = isHidden;
    for (const auto& fileAsset : changeRequest->fileAssets_) {
        CHECK_NULL_PTR_RETURN_VOID(fileAsset, "fileAsset is null");
        fileAsset->SetHidden(isHidden);
    }
    changeRequest->assetsChangeOperations_.push_back(AssetsChangeOperation::BATCH_SET_HIDDEN);
}

ani_object MediaAssetsChangeRequestAni::SetUserComment([[maybe_unused]] ani_env *env,
    [[maybe_unused]] ani_object object, ani_string comment)
{
    ANI_DEBUG_LOG("%{public}s is called", __func__);
    ani_object result {};
    CHECK_COND_RET(env != nullptr, nullptr, "env is null");
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return result;
    }

    auto asyncContext = std::make_unique<MediaAssetsChangeRequestAniContext>();
    CHECK_COND_WITH_MESSAGE(env, asyncContext != nullptr, "Failed to create asyncContext");
    asyncContext->objectInfo = Unwrap(env, object);
    auto changeRequest = asyncContext->objectInfo;
    CHECK_COND_WITH_MESSAGE(env, changeRequest != nullptr, "changeRequest is null");
    std::string userComment("");
    CHECK_COND_WITH_MESSAGE(env, MediaLibraryAniUtils::GetString(env, comment, userComment) == ANI_OK,
        "Failed to get comment");
    CHECK_COND_WITH_MESSAGE(env, userComment.length() <= USER_COMMENT_MAX_LEN, "user comment too long");
    for (const auto& fileAsset : changeRequest->fileAssets_) {
        CHECK_COND_WITH_MESSAGE(env, fileAsset != nullptr, "fileAsset is null");
        fileAsset->SetUserComment(userComment);
    }
    changeRequest->userComment_ = std::move(userComment);
    changeRequest->assetsChangeOperations_.push_back(AssetsChangeOperation::BATCH_SET_USER_COMMENT);
    return result;
}

ani_object MediaAssetsChangeRequestAni::SetIsRecentShow([[maybe_unused]] ani_env *env,
    [[maybe_unused]] ani_object object, ani_boolean isRecentShowAni)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is null");
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    auto asyncContext = std::make_unique<MediaAssetsChangeRequestAniContext>();
    CHECK_COND_WITH_MESSAGE(env, asyncContext != nullptr, "Failed to create asyncContext");
    asyncContext->objectInfo = Unwrap(env, object);
    auto changeRequest = asyncContext->objectInfo;
    CHECK_COND_WITH_MESSAGE(env, changeRequest != nullptr, "changeRequest is null");
    bool isRecentShow;
    CHECK_COND_WITH_MESSAGE(env, MediaLibraryAniUtils::GetBool(env, isRecentShowAni, isRecentShow) == ANI_OK,
        "Failed to get isRecentShowAni");
    for (const auto& fileAsset : changeRequest->fileAssets_) {
        CHECK_COND_WITH_MESSAGE(env, fileAsset != nullptr, "fileAsset is null");
        fileAsset->SetRecentShow(isRecentShow);
    }
    changeRequest->isRecentShow_ = isRecentShow;
    changeRequest->assetsChangeOperations_.push_back(AssetsChangeOperation::BATCH_SET_RECENT_SHOW);

    ani_object result {};
    MediaLibraryAniUtils::GetUndefinedObject(env, result);
    return result;
}

static bool CallSetAssetProperty(MediaAssetsChangeRequestAniContext& context,
    const AssetsChangeOperation& changeOperation)
{
    uint32_t businessCode = 0;
    ModifyAssetsReqBody reqBody;
    auto changeRequest = context.objectInfo;
    CHECK_COND_RET(changeRequest != nullptr, false, "changeRequest is nullptr");
    switch (changeOperation) {
        case AssetsChangeOperation::BATCH_SET_FAVORITE:
            reqBody.favorite = changeRequest->GetFavoriteStatus() ? YES : NO;
            businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_BATCH_SET_FAVORITE);
            break;
        case AssetsChangeOperation::BATCH_SET_HIDDEN:
            reqBody.hiddenStatus = changeRequest->GetHiddenStatus() ? YES : NO;
            businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_BATCH_SET_HIDDEN);
            break;
        case AssetsChangeOperation::BATCH_SET_USER_COMMENT:
            reqBody.userComment = changeRequest->GetUserComment();
            businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_BATCH_SET_USER_COMMENT);
            break;
        case AssetsChangeOperation::BATCH_SET_RECENT_SHOW:
            reqBody.recentShowStatus = changeRequest->GetRecentShowStatus() ? YES : NO;
            businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_BATCH_SET_RECENT_SHOW);
            break;
        default:
            context.SaveError(E_FAIL);
            ANI_ERR_LOG("Unsupported assets change operation: %{public}d", changeOperation);
            return false;
    }

    changeRequest->GetFileAssetIds(reqBody.fileIds);
    int32_t result = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (result < 0) {
        ANI_ERR_LOG("after IPC::UserDefineIPCClient().Call, errCode: %{public}d.", result);
        context.SaveError(result);
        return false;
    }
    return true;
}

static bool SetAssetsPropertyExecute(MediaAssetsChangeRequestAniContext& context,
    const AssetsChangeOperation& changeOperation, bool useCall)
{
    if (useCall) {
        return CallSetAssetProperty(context, changeOperation);
    }

    string uri;
    DataShare::DataSharePredicates predicates;
    DataShare::DataShareValuesBucket valuesBucket;
    auto changeRequest = context.objectInfo;
    CHECK_COND_RET(changeRequest != nullptr, false, "changeRequest is null");
    predicates.In(PhotoColumn::MEDIA_ID, changeRequest->GetFileAssetUriArray());
    switch (changeOperation) {
        case AssetsChangeOperation::BATCH_SET_FAVORITE:
            uri = PAH_BATCH_UPDATE_FAVORITE;
            valuesBucket.Put(PhotoColumn::MEDIA_IS_FAV, changeRequest->GetFavoriteStatus() ? YES : NO);
            ANI_INFO_LOG("Batch set favorite: %{public}d", changeRequest->GetFavoriteStatus() ? YES : NO);
            break;
        case AssetsChangeOperation::BATCH_SET_HIDDEN:
            uri = PAH_HIDE_PHOTOS;
            valuesBucket.Put(PhotoColumn::MEDIA_HIDDEN, changeRequest->GetHiddenStatus() ? YES : NO);
            break;
        case AssetsChangeOperation::BATCH_SET_USER_COMMENT:
            uri = PAH_BATCH_UPDATE_USER_COMMENT;
            valuesBucket.Put(PhotoColumn::PHOTO_USER_COMMENT, changeRequest->GetUserComment());
            break;
        case AssetsChangeOperation::BATCH_SET_RECENT_SHOW:
            uri = PAH_BATCH_UPDATE_RECENT_SHOW;
            valuesBucket.Put(PhotoColumn::PHOTO_IS_RECENT_SHOW, changeRequest->GetRecentShowStatus() ? YES : NO);
            break;
        default:
            ANI_ERR_LOG("Unsupported assets change operation: %{public}d", changeOperation);
            return false;
    }
    ANI_INFO_LOG("changeOperation:%{public}d, size:%{public}zu",
        changeOperation, changeRequest->GetFileAssetUriArray().size());
    MediaLibraryAniUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri updateAssetsUri(uri);
    int32_t changedRows = UserFileClient::Update(updateAssetsUri, predicates, valuesBucket);
    if (changedRows < 0) {
        ANI_ERR_LOG("Failed to set property, operation: %{public}d, err: %{public}d", changeOperation, changedRows);
        return false;
    }
    return true;
}

ani_status MediaAssetsChangeRequestAni::ApplyChanges(ani_env *env)
{
    auto asyncContext = std::make_unique<MediaAssetsChangeRequestAniContext>();
    ANI_CHECK_RETURN_RET_LOG(asyncContext != nullptr, ANI_ERROR, "Failed to create asyncContext");
    asyncContext->objectInfo = this;
    ANI_CHECK_RETURN_RET_LOG(!assetsChangeOperations_.empty(), ANI_ERROR,
        "ApplyChanges assetsChangeOperations_ is empty");
    ANI_CHECK_RETURN_RET_LOG(!fileAssets_.empty(), ANI_ERROR, "ApplyChanges fileAssets_ is empty");
    for (const auto& fileAsset : fileAssets_) {
        ANI_CHECK_RETURN_RET_LOG(fileAsset != nullptr && fileAsset->GetId() > 0 && !fileAsset->GetUri().empty(),
            ANI_ERROR, "MediaAssetsChangeRequestAni::ApplyChanges check fileAssets_ failed");
    }
    asyncContext->assetsChangeOperations.swap(assetsChangeOperations_);

    unordered_set<AssetsChangeOperation> appliedOperations;
    for (const auto& changeOperation : asyncContext->assetsChangeOperations) {
        // Keep the final result(s) of each operation, and commit only once.
        if (appliedOperations.find(changeOperation) != appliedOperations.end()) {
            continue;
        }

        bool valid = SetAssetsPropertyExecute(*asyncContext, changeOperation, true);
        if (!valid) {
            ANI_ERR_LOG("Failed to apply assets change request, operation: %{public}d", changeOperation);
            return ANI_ERROR;
        }
        appliedOperations.insert(changeOperation);
    }
    return ANI_OK;
}

vector<string> MediaAssetsChangeRequestAni::GetFileAssetUriArray()
{
    vector<string> uriArray;
    uriArray.reserve(fileAssets_.size());
    for (const auto& fileAsset : fileAssets_) {
        if (fileAsset == nullptr || fileAsset->GetUri().empty()) {
            ANI_ERR_LOG("fileAsset is null or uri is empty");
            continue;
        }
        uriArray.push_back(fileAsset->GetUri());
    }
    return uriArray;
}

void MediaAssetsChangeRequestAni::GetFileAssetIds(std::vector<int32_t> &fileIds) const
{
    for (const auto& fileAsset : fileAssets_) {
        fileIds.push_back(fileAsset->GetId());
    }
}

bool MediaAssetsChangeRequestAni::GetFavoriteStatus() const
{
    return isFavorite_;
}

bool MediaAssetsChangeRequestAni::GetHiddenStatus() const
{
    return isHidden_;
}

std::string MediaAssetsChangeRequestAni::GetUserComment() const
{
    return userComment_;
}

bool MediaAssetsChangeRequestAni::GetRecentShowStatus() const
{
    return isRecentShow_;
}

MediaAssetsChangeRequestAni* MediaAssetsChangeRequestAni::Unwrap(ani_env *env, ani_object object)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is null");
    ani_long context;
    if (ANI_OK != env->Object_GetFieldByName_Long(object, "nativeHandle", &context)) {
        return nullptr;
    }
    return reinterpret_cast<MediaAssetsChangeRequestAni *>(context);
}

ani_status MediaAssetsChangeRequestAni::Constructor(ani_env *env, ani_object object, ani_object arrayPhotoAssets)
{
    vector<shared_ptr<FileAsset>> newAssetArray;
    CHECK_COND_WITH_RET_MESSAGE(env,
        MediaLibraryAniUtils::GetArrayFromAssets(env, arrayPhotoAssets, newAssetArray) == ANI_OK, ANI_INVALID_ARGS,
        "Failed to get arrayPhotoAssets");
    auto nativeHandle = std::make_unique<MediaAssetsChangeRequestAni>(newAssetArray);
    if (nativeHandle == nullptr) {
        return ANI_ERROR;
    }
    if (ANI_OK != env->Object_CallMethodByName_Void(object, "create", nullptr,
        reinterpret_cast<ani_long>(nativeHandle.get()))) {
        ANI_ERR_LOG("New MediaAssetsChangeRequest Fail");
        return ANI_ERROR;
    }
    (void)nativeHandle.release();
    return ANI_OK;
}

ani_status MediaAssetsChangeRequestAni::Init(ani_env *env)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is null");
    ani_class cls;
    if (ANI_OK != env->FindClass(PAH_ANI_CLASS_MEDIA_ASSETS_CHANGE_REQUEST.c_str(), &cls)) {
        return ANI_ERROR;
    }

    std::array methods = {
        ani_native_function {"setFavorite", nullptr, reinterpret_cast<void *>(SetFavorite)},
        ani_native_function {"setHidden", nullptr, reinterpret_cast<void *>(SetHidden)},
        ani_native_function {"setUserComment", "C{std.core.String}:", reinterpret_cast<void *>(SetUserComment)},
        ani_native_function {"setIsRecentShow", nullptr, reinterpret_cast<void *>(SetIsRecentShow)},
        ani_native_function {"nativeConstructor", nullptr, reinterpret_cast<void *>(Constructor)},
    };

    if (ANI_OK != env->Class_BindNativeMethods(cls, methods.data(), methods.size())) {
        return ANI_ERROR;
    }
    return ANI_OK;
}
} // namespace OHOS::Media
