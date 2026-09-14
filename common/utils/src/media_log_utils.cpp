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
#define MLOG_TAG "MediaLogUtils"

#include "media_log_utils.h"

#include <filesystem>

#include "media_log.h"

namespace OHOS::Media {
const char EXTENSION_DOT = '.';
const char GARBLE_MARKER = '*';
const size_t GARBLE_SIZE_DEFAULT = 3;
const size_t GARBLE_SIZE_RATIO = 2;

std::string MediaLogUtils::GarbleFilePath(const std::string &filePath)
{
    std::filesystem::path inputPath(filePath);
    std::filesystem::path outputPath(filePath);
    for (auto iter = inputPath.begin(); iter != inputPath.end(); iter++) {
        outputPath /= GarbleFile(iter->string());
    }
    return outputPath;
}

std::string MediaLogUtils::GarbleFile(const std::string &file)
{
    return HasExtension(file) ? GarbleFileWithExtension(file) : GarbleFileWithoutExtension(file);
}

bool MediaLogUtils::HasExtension(const std::string &file)
{
    return file.find(EXTENSION_DOT) != std::string::npos;
}

std::string MediaLogUtils::GarbleFileWithExtension(const std::string &file)
{
    size_t pos = file.find_last_of(EXTENSION_DOT);
    CHECK_AND_RETURN_RET_LOG(pos != std::string::npos, "", "file.path not cotain EXTENSION_DOT");
    std::string name = file.substr(0, pos);
    std::string extension = file.substr(pos);
    return GarbleFileWithoutExtension(name) + extension;
}

std::string MediaLogUtils::GarbleFileWithoutExtension(const std::string &file)
{
    size_t garbleSize = GetGarbleSize(file);
    std::string result(file);
    result.replace(0, garbleSize, garbleSize, GARBLE_MARKER);
    return result;
}

size_t MediaLogUtils::GetGarbleSize(const std::string &file)
{
    return file.size() >= GARBLE_SIZE_DEFAULT * GARBLE_SIZE_RATIO ? GARBLE_SIZE_DEFAULT :
        file.size() / GARBLE_SIZE_RATIO;
}
} // namespace OHOS::Media
