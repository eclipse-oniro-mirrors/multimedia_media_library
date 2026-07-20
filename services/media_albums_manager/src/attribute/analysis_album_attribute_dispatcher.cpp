/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "AlbumAttrDispatcher"

#include "analysis_album_attribute_dispatcher.h"

#include "analysis_album_attribute_validator.h"
#include "portrait_nickname_handler.h"
#include "portrait_is_removed_handler.h"
#include "media_log.h"
#include "portrait_extra_info_handler.h"
#include "portrait_friend_id_handler.h"

namespace OHOS::Media {
namespace {
using ValidateTargetFunc = int32_t (*)(const std::shared_ptr<PhotoAlbum> &photoAlbum);
using ExecuteFunc = int32_t (*)(const std::shared_ptr<PhotoAlbum> &photoAlbum,
    const AnalysisAlbumOperation &operation);
using GetAttributeExecuteFunc = int32_t (*)(const std::shared_ptr<PhotoAlbum> &photoAlbum,
        std::string &attributeValue);

struct AnalysisAlbumAttributeHandlerEntry {
    const AnalysisAlbumAttributeSpec *spec;
    ValidateTargetFunc validateTarget;
    ExecuteFunc execute;
    GetAttributeExecuteFunc getAttributeExecute = nullptr;
};

const AnalysisAlbumAttributeHandlerEntry PORTRAIT_NICK_NAME_HANDLER = {
    &ANALYSIS_ALBUM_NICK_NAME_SPEC,
    PortraitNickNameHandler::ValidateTarget,
    PortraitNickNameHandler::Execute,
};

const AnalysisAlbumAttributeHandlerEntry PORTRAIT_IS_REMOVED_HANDLER = {
    &ANALYSIS_ALBUM_IS_REMOVED_SPEC,
    PortraitIsRemovedHandler::ValidateTarget,
    PortraitIsRemovedHandler::Execute,
};

const AnalysisAlbumAttributeHandlerEntry PORTRAIT_EXTRA_INFO_HANDLER = {
    &ANALYSIS_ALBUM_EXTRA_INFO_SPEC,
    PortraitExtraInfoHandler::ValidateTarget,
    PortraitExtraInfoHandler::Execute,
    PortraitExtraInfoHandler::GetAttributeExecute,
};

const AnalysisAlbumAttributeHandlerEntry PORTRAIT_FRIEND_ID_HANDLER = {
    &ANALYSIS_ALBUM_FRIEND_ID_SPEC,
    PortraitFriendIdHandler::ValidateTarget,
    PortraitFriendIdHandler::Execute,
    PortraitFriendIdHandler::GetAttributeExecute,
};

const AnalysisAlbumAttributeHandlerEntry *ResolveHandler(const std::string &attr)
{
    if (attr == PORTRAIT_NICK_NAME_HANDLER.spec->attr) {
        return &PORTRAIT_NICK_NAME_HANDLER;
    } else if (attr == PORTRAIT_IS_REMOVED_HANDLER.spec->attr) {
        return &PORTRAIT_IS_REMOVED_HANDLER;
    } else if (attr == PORTRAIT_EXTRA_INFO_HANDLER.spec->attr) {
        return &PORTRAIT_EXTRA_INFO_HANDLER;
    } else if (attr == PORTRAIT_FRIEND_ID_HANDLER.spec->attr) {
        return &PORTRAIT_FRIEND_ID_HANDLER;
    }
    return nullptr;
}
} // namespace

int32_t AnalysisAlbumAttributeDispatcher::Execute(const std::shared_ptr<PhotoAlbum> &photoAlbum,
    const AnalysisAlbumOperation &operation)
{
    CHECK_AND_RETURN_RET_LOG(photoAlbum != nullptr, E_INVALID_VALUES, "photoAlbum is nullptr");
    int32_t checkResult = ValidateAnalysisAlbumOperationProtocol(operation.attr, operation.type, operation.values);
    CHECK_AND_RETURN_RET_LOG(checkResult == E_OK, checkResult,
        "invalid analysis album operation protocol, attr: %{public}s, type: %{public}s",
        operation.attr.c_str(), operation.type.c_str());
    checkResult = CheckAnalysisAlbumOperationSupport(operation.attr, operation.type, operation.values);
    CHECK_AND_RETURN_RET_LOG(checkResult == E_OK, checkResult,
        "unsupported analysis album operation, attr: %{public}s, type: %{public}s",
        operation.attr.c_str(), operation.type.c_str());

    const auto *handler = ResolveHandler(operation.attr);
    CHECK_AND_RETURN_RET_LOG(handler != nullptr, E_OPERATION_NOT_SUPPORT,
        "no handler for analysis album attribute: %{public}s", operation.attr.c_str());
    checkResult = handler->validateTarget(photoAlbum);
    CHECK_AND_RETURN_RET_LOG(checkResult == E_OK, checkResult,
        "analysis album attribute target validation failed, attr: %{public}s, albumId: %{public}d",
        operation.attr.c_str(), photoAlbum->GetAlbumId());
    return handler->execute(photoAlbum, operation);
}

int32_t AnalysisAlbumAttributeDispatcher::GetAttributeExecute(const std::shared_ptr<PhotoAlbum> &photoAlbum,
    std::vector<std::string> &attributeArray,
    std::vector<std::unordered_map<std::string, std::string>> &queryResults)
{
    CHECK_AND_RETURN_RET_LOG(photoAlbum != nullptr, E_INNER_FAIL, "photoAlbum is nullptr");
    for (const auto &attr : attributeArray) {
        const auto *handler = ResolveHandler(attr);
        CHECK_AND_RETURN_RET_LOG(handler != nullptr && handler->getAttributeExecute != nullptr,
            E_PARAM_CONVERT_FORMAT,
            "no handler for analysis album attribute: %{public}s", attr.c_str());
        int32_t checkResult = handler->validateTarget(photoAlbum);
        CHECK_AND_RETURN_RET_LOG(checkResult == E_OK, checkResult,
            "analysis album attribute target validation failed, attr: %{public}s, albumId: %{public}d",
            attr.c_str(), photoAlbum->GetAlbumId());
        std::string retVal;
        checkResult = handler->getAttributeExecute(photoAlbum, retVal);
        CHECK_AND_RETURN_RET_LOG(checkResult == E_OK, checkResult,
            "execute analysis album attribute failed, attr: %{public}s, albumId: %{public}d",
            attr.c_str(), photoAlbum->GetAlbumId());
        queryResults.push_back({{ attr, retVal }});
    }
    return E_OK;
}
} // namespace OHOS::Media
