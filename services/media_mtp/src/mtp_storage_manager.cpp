/*
 * Copyright (C) 2021-2022 Huawei Device Co., Ltd.
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
#define MLOG_TAG "MtpStorageManager"
#include "mtp_storage_manager.h"
#include <filesystem>
#include <mutex>
#include "media_log.h"
#include "medialibrary_errno.h"
#include "parameter.h"

using namespace std;

namespace OHOS {
namespace Media {
namespace {
enum SizeType {
    TOTAL,
    FREE,
    USED
};
const std::string PUBLIC_PATH_DATA      = "/storage/media/local/files/Docs";
const std::string CHINESE_ABBREVIATION  = "zh-Hans";
const std::string ENGLISH_ABBREVIATION  = "en-Latn-US";
const std::string INNER_STORAGE_DESC_ZH = "内部存储";
const std::string INNER_STORAGE_DESC_EN = "Internal storage";
const std::string EXTER_STORAGE_DESC_ZH = "存储卡";
const std::string EXTER_STORAGE_DESC_EN = "Memory Card";
const std::string UNSPECIFIED           = "Unspecified";
const std::string LANGUAGE_KEY          = "persist.global.language";
const std::string DEFAULT_LANGUAGE_KEY  = "const.global.language";
constexpr int32_t SYSPARA_SIZE          = 64;
} // namespace

std::shared_ptr<MtpStorageManager> MtpStorageManager::instance_ = nullptr;
std::mutex MtpStorageManager::mutex_;

std::shared_ptr<MtpStorageManager> MtpStorageManager::GetInstance()
{
    if (instance_ == nullptr) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (instance_ == nullptr) {
            instance_ = std::make_shared<MtpStorageManager>();
        }
    }
    return instance_;
}

int64_t MtpStorageManager::GetTotalSize(const std::string &path)
{
    std::string p = path.empty() ? PUBLIC_PATH_DATA : path;
    std::error_code ec;
    auto info = std::filesystem::space(p, ec);
    CHECK_AND_RETURN_RET_LOG(ec.value() == E_OK, 0, "GetTotalSize failed, errno: %{public}d", errno);

    return info.capacity;
}

int64_t MtpStorageManager::GetFreeSize(const std::string &path)
{
    std::string p = path.empty() ? PUBLIC_PATH_DATA : path;
    std::error_code ec;
    auto info = std::filesystem::space(p, ec);
    CHECK_AND_RETURN_RET_LOG(ec.value() == E_OK, 0, "GetFreeSize failed, errno: %{public}d", errno);

    return info.available;
}

void MtpStorageManager::AddStorage(shared_ptr<Storage> &storage)
{
    if (!storages.empty()) {
        for (auto stor : storages) {
            if (stor->GetStorageID() == storage->GetStorageID()) {
                return;
            }
        }
    }
    storages.push_back(storage);
}

void MtpStorageManager::RemoveStorage(std::shared_ptr<Storage> &storage)
{
    auto iter = std::find(storages.begin(), storages.end(), storage);
    if (iter != storages.end()) {
        storages.erase(iter);
    }
}

shared_ptr<Storage> MtpStorageManager::GetStorage(uint32_t id)
{
    for (auto storage : storages) {
        if (storage->GetStorageID() == id) {
            return storage;
        }
    }
    return nullptr;
}

bool MtpStorageManager::HasStorage(uint32_t id)
{
    bool result = false;

    if (id == MTP_STORAGE_ID_ALL || id == MTP_STORAGE_ID_ALL2) {
        result = (storages.size() > 0);
    } else {
        result = (GetStorage(id) != nullptr);
    }

    return result;
}

std::vector<std::shared_ptr<Storage>> MtpStorageManager::GetStorages()
{
    return storages;
}

void MtpStorageManager::ClearStorages()
{
    std::vector<std::shared_ptr<Storage>>().swap(storages);
}

std::string MtpStorageManager::GetSystemLanguage()
{
    char param[SYSPARA_SIZE] = {0};
    int status = GetParameter(LANGUAGE_KEY.c_str(), "", param, SYSPARA_SIZE);
    CHECK_AND_RETURN_RET(status <= 0, param);
    status = GetParameter(DEFAULT_LANGUAGE_KEY.c_str(), "", param, SYSPARA_SIZE);
    CHECK_AND_RETURN_RET(status <= 0, param);
    MEDIA_ERR_LOG("Failed to get system language");
    return "";
}

std::string MtpStorageManager::GetStorageDescription(const uint16_t type)
{
    std::string language = GetSystemLanguage();
    if (language.empty()) {
        language = CHINESE_ABBREVIATION;
    }
    switch (type) {
        case MTP_STORAGE_FIXEDRAM:
            return language == CHINESE_ABBREVIATION ? INNER_STORAGE_DESC_ZH : INNER_STORAGE_DESC_EN;
        case MTP_STORAGE_REMOVABLERAM:
            return language == CHINESE_ABBREVIATION ? EXTER_STORAGE_DESC_ZH : EXTER_STORAGE_DESC_EN;
        default:
            break;
    }
    return UNSPECIFIED;
}

}  // namespace Media
}  // namespace OHOS
