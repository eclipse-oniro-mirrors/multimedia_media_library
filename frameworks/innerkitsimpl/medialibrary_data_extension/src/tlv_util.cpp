/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#define MLOG_TAG "TLVUtil"
#include "tlv_util.h"

#include <chrono>
#include <cinttypes>
#include <filesystem>
#include <fcntl.h>
#include <string>
#include <sys/stat.h>

#include "dfx_utils.h"
#include "medialibrary_json_operation.h"
#include "medialibrary_errno.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "unique_fd.h"

namespace OHOS {
namespace Media {
namespace {
const uint32_t TLV_BUFFER_SIZE = 256 * 1024; // 256KB
const std::string TLV_FILE_EXTENSION = ".tlv";
const mode_t CHOWN_RW_USER_GRP = S_IRUSR | S_IWUSR | S_IRGRP;
const std::string MOVING_PHOTO_VIDEO_EDITDATA_DIR = "/moving_photo_video";
const size_t MAX_PADDING_SIZE = 1024;
} // namespace
thread_local size_t TlvUtil::metaSize_ = 0;

static off_t GetFileSize(TlvFile tlvFile)
{
    CHECK_AND_RETURN_RET_LOG(tlvFile >= 0, E_ERR, "fd is not valid");
    struct stat64 st;
    CHECK_AND_RETURN_RET_LOG(fstat(tlvFile, &st) == E_OK, E_ERR, "failed to get file size: %{public}d", errno);
    return st.st_size;
}

bool TlvUtil::ProcessWriteAll(int fd, const char* data, size_t size)
{
    MEDIA_DEBUG_LOG("ProcessWriteAll start, size: %{public}zu", size);
    const char* ptr = data;
    size_t left = size;
    
    while (left > 0) {
        ssize_t n = write(fd, ptr, left);
        if (n == -1) {
            CHECK_AND_CONTINUE(errno != EINTR);
            return false;
        }
        CHECK_AND_RETURN_RET_LOG(n != 0, false, "write returned 0 bytes");
        ptr += n;
        left -= n;
    }
    return true;
}

off_t TlvUtil::GetCurrentPosition(TlvFile tlvFile)
{
    CHECK_AND_RETURN_RET_LOG(tlvFile >= 0, E_ERR, "tlvFile is invalid");
    off_t currentPos = lseek(tlvFile, 0, SEEK_CUR);
    CHECK_AND_RETURN_RET_LOG(currentPos != E_ERR, E_ERR, "failed to get current position: %{public}d", errno);
    return currentPos;
}

int32_t TlvUtil::ValidateTlvFile(const std::string &tlvFilePath)
{
    MEDIA_INFO_LOG("ValidateTlvFile start");
    CHECK_AND_RETURN_RET_LOG(!tlvFilePath.empty(), E_ERR, "tlvFilePath is empty");
    
    TlvFile tlvFile = open(tlvFilePath.c_str(), O_RDONLY);
    CHECK_AND_RETURN_RET_LOG(tlvFile >= 0, E_ERR, "Failed to open TLV file");
    UniqueFd tlvFileFd(tlvFile);
    
    return ValidateTlvFile(tlvFileFd.Get());
}

int32_t TlvUtil::ValidateTlvFile(TlvFile tlvFile)
{
    CHECK_AND_RETURN_RET_LOG(tlvFile >= 0, E_ERR, "tlvFile is invalid");
    
    TlvNode header;
    ssize_t bytes = read(tlvFile, reinterpret_cast<char*>(&header), sizeof(TlvNode));
    CHECK_AND_RETURN_RET_LOG(bytes == sizeof(TlvNode), E_ERR,
        "Failed to read TLV header, got %zd bytes, expected %zu", bytes, sizeof(TlvNode));
    
    CHECK_AND_RETURN_RET_LOG(header.type == TlvTag::TLV_TAG_HEADER, E_ERR,
        "Invalid TLV header tag: 0x%{public}x, expected: 0x%{public}x",
        header.type, TlvTag::TLV_TAG_HEADER);
    
    off_t fileSize = GetFileSize(tlvFile);
    TlvLength expectedDataSize = fileSize - sizeof(TlvNode);
    
    CHECK_AND_RETURN_RET_LOG(header.length == expectedDataSize, E_ERR,
        "TLV data size mismatch: header says %{public}" PRId64 ", file says %{public}" PRId64,
        header.length, expectedDataSize);
    
    MEDIA_INFO_LOG("TLV file validation passed: tag=0x%{public}x, data_size=%{public}" PRId64,
        header.type, header.length);
    return E_OK;
}

TlvFile TlvUtil::CreateTlvFile(const std::string& tlvPath)
{
    MEDIA_INFO_LOG("Create tlv file start");
    metaSize_ = 0;
    CHECK_AND_RETURN_RET_LOG(!tlvPath.empty(), E_ERR, "tlvPath is empty");
    std::string parentPath = MediaFileUtils::GetParentPath(tlvPath);
    if (!MediaFileUtils::IsDirExists(parentPath)) {
        CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CreateDirectory(parentPath), E_ERR,
            "Failed to create directory");
    }
    TlvFile tlvFile = open(tlvPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, CHOWN_RW_USER_GRP);
    CHECK_AND_RETURN_RET_LOG(tlvFile >= 0, E_ERR, "fd is not valid");

    if (WriteTagInfoToTlv(tlvFile, TlvTag::TLV_TAG_HEADER, 0) != E_OK) {
        close(tlvFile);
        MediaFileUtils::DeleteFile(tlvPath);
        MEDIA_ERR_LOG("WriteTagInfoToTlv failed");
        return E_ERR;
    }
    return tlvFile;
}

int32_t TlvUtil::WriteOriginFileToTlv(TlvFile tlvFile, const std::string &srcFilePath, int32_t srcFd)
{
    MEDIA_DEBUG_LOG("WriteOriginFileToTlv start");
    CHECK_AND_RETURN_RET_LOG(srcFd >= 0, E_ERR, "src file open failed");

    return WriteTaggedFileToTlv(tlvFile, TlvTag::TLV_TAG_ORIGIN, srcFd);
}

int32_t TlvUtil::WriteMovingPhotoVideoFileToTlv(TlvFile tlvFile, int32_t srcFd)
{
    MEDIA_DEBUG_LOG("WriteMovingPhotoVideoFileToTlv start");
    return WriteTaggedFileToTlv(tlvFile, TlvTag::TLV_TAG_MOVING_PHOTO_VIDEO, srcFd);
}

int32_t TlvUtil::WriteSourceFileToTlv(TlvFile tlvFile, int32_t srcFd)
{
    MEDIA_DEBUG_LOG("WriteSourceFileToTlv start");
    return WriteTaggedFileToTlv(tlvFile, TlvTag::TLV_TAG_SOURCE, srcFd);
}

int32_t TlvUtil::WriteMovingPhotoVideoSourceBackFileToTlv(TlvFile tlvFile, int32_t srcFd)
{
    MEDIA_DEBUG_LOG("WriteMovingPhotoVideoSourceBackFileToTlv start");
    return WriteTaggedFileToTlv(tlvFile, TlvTag::TLV_TAG_MOVING_PHOTO_VIDEO_SOURCE_BACK, srcFd);
}

int32_t TlvUtil::WriteMovingPhotoVideoSourceFileToTlv(TlvFile tlvFile, int32_t srcFd)
{
    MEDIA_DEBUG_LOG("WriteMovingPhotoVideoSourceFileToTlv start");
    return WriteTaggedFileToTlv(tlvFile, TlvTag::TLV_TAG_MOVING_PHOTO_VIDEO_SOURCE, srcFd);
}

int32_t TlvUtil::WriteTaggedFileToTlv(TlvFile tlvFile, TlvTag tag, const std::string &srcFilePath)
{
    MEDIA_INFO_LOG("WriteTaggedFileToTlv start, tag: 0x%{public}x, srcFilePath: %{public}s",
        tag, DfxUtils::GetSafePath(srcFilePath).c_str());
    CHECK_AND_RETURN_RET_LOG(tlvFile >= 0, E_ERR, "tlvFile is invalid");
    std::error_code ec;
    CHECK_AND_RETURN_RET_LOG(std::filesystem::exists(srcFilePath, ec), E_ERR, "src file does not exist");
    
    auto srcFd = open(srcFilePath.c_str(), O_RDONLY);
    UniqueFd srcUniqueFd(srcFd);
    CHECK_AND_RETURN_RET_LOG(srcUniqueFd.Get() >= 0, E_ERR, "src file open failed");
    return TlvUtil::WriteTaggedFileToTlv(tlvFile, tag, srcUniqueFd.Get());
}

int32_t TlvUtil::WriteTaggedFileToTlv(TlvFile tlvFile, TlvTag tag, int32_t srcFd)
{
    MEDIA_INFO_LOG("WriteTaggedFileToTlv start, tag: 0x%{public}x", tag);
    CHECK_AND_RETURN_RET_LOG(srcFd >= 0, E_ERR, "src file open failed");

    TlvLength outSize = static_cast<TlvLength>(GetFileSize(srcFd));
    CHECK_AND_RETURN_RET_LOG(WriteTagInfoToTlv(tlvFile, tag, outSize) == E_OK, E_ERR,
        "fail to write tag info for tag 0x%{public}x", tag);
    CHECK_AND_RETURN_RET_LOG(WriteDataValueToTlv(tlvFile, srcFd) == E_OK, E_ERR,
        "fail to write file data for tag 0x%{public}x", tag);
    
    MEDIA_INFO_LOG("WriteTaggedFileToTlv end, tag: 0x%{public}x, size: %{public}" PRId64, tag, outSize);
    return E_OK;
}

int32_t TlvUtil::WriteCameraDataToTlv(TlvFile tlvFile, const std::string &srcFilePath)
{
    MEDIA_DEBUG_LOG("WriteCameraDataToTlv start");
    return WriteTaggedFileToTlv(tlvFile, TlvTag::TLV_TAG_CAMERA, srcFilePath);
}

int32_t TlvUtil::WriteEditDataToTlv(TlvFile tlvFile, const std::string &srcFilePath)
{
    MEDIA_DEBUG_LOG("WriteEditDataToTlv start");
    return WriteTaggedFileToTlv(tlvFile, TlvTag::TLV_TAG_EDITDATA, srcFilePath);
}

int32_t TlvUtil::WriteExtraDataFileToTlv(TlvFile tlvFile, const std::string &srcFilePath)
{
    MEDIA_DEBUG_LOG("WriteExtraDataFileToTlv start");
    return WriteTaggedFileToTlv(tlvFile, TlvTag::TLV_TAG_EXTRA_DATA, srcFilePath);
}

int32_t TlvUtil::WriteSourceBackFileToTlv(TlvFile tlvFile, int32_t srcFd)
{
    MEDIA_DEBUG_LOG("WriteSourceBackFileToTlv start");
    return WriteTaggedFileToTlv(tlvFile, TlvTag::TLV_TAG_SOURCE_BACK, srcFd);
}

int32_t TlvUtil::WriteJsonDataToTlv(TlvFile tlvFile, const std::string &jsonDataPath)
{
    MEDIA_INFO_LOG("WriteJsonDataToTlv start");
    nlohmann::json jsonData;
    CHECK_AND_RETURN_RET_LOG(MediaJsonOperation::CheckPathAndLoadJson(jsonDataPath, jsonData), E_ERR,
        "fail to load json data");
    CHECK_AND_RETURN_RET_LOG(!jsonData.empty(), E_ERR, "json data is empty");
    CHECK_AND_RETURN_RET_LOG(tlvFile >= 0, E_ERR, "tlvFile is invalid");
    
    std::string jsonStr = jsonData.dump();
    TlvLength outSize = static_cast<TlvLength>(jsonStr.size());
    metaSize_ += outSize;
    CHECK_AND_RETURN_RET_LOG(WriteTagInfoToTlv(tlvFile, TlvTag::TLV_TAG_JSON, outSize) == E_OK, E_ERR,
        "fail to write json tag info");
    
    const char* dataPtr = jsonStr.data();
    size_t dataSize = jsonStr.size();
    auto ret = ProcessWriteAll(tlvFile, dataPtr, dataSize);
    CHECK_AND_RETURN_RET_LOG(ret, E_ERR, "tlv file write json data error: %{public}d", errno);
    
    MEDIA_INFO_LOG("WriteJsonDataToTlv end, size: %{public}" PRId64, outSize);
    return E_OK;
}

int32_t TlvUtil::WriteTagInfoToTlv(TlvFile tlvFile, TlvTag tag, TlvLength length)
{
    CHECK_AND_RETURN_RET_LOG(tlvFile >= 0, E_ERR, "tlvFile is invalid");
    TlvNode node = { tag, length };
    const char* data = reinterpret_cast<const char*>(&node);
    size_t remaining = sizeof(TlvNode);
    auto ret = ProcessWriteAll(tlvFile, data, remaining);
    CHECK_AND_RETURN_RET_LOG(ret, E_ERR, "tlv file write error [%{public}d]", errno);
    metaSize_ += sizeof(TlvNode);
    return E_OK;
}

int32_t TlvUtil::WriteDataValueToTlv(TlvFile tlvFile, TlvFile srcFile)
{
    CHECK_AND_RETURN_RET_LOG(tlvFile >= 0, E_ERR, "tlvFile is invalid");
    CHECK_AND_RETURN_RET_LOG(srcFile >= 0, E_ERR, "srcFile is invalid");

    char buffer[TLV_BUFFER_SIZE] = {0};
    size_t bytesRead = 0;
    size_t bytesWritten = 0;
    bool ret = false;
    while ((bytesRead = read(srcFile, buffer, sizeof(buffer))) > 0) {
        ret = ProcessWriteAll(tlvFile, buffer, bytesRead);
        CHECK_AND_RETURN_RET_LOG(ret, E_ERR, "failed to write tlv file, errno: %{public}d", errno);
    }
    CHECK_AND_RETURN_RET_LOG(bytesRead >= 0, E_ERR, "failed to write tlv file, errno: %{public}d", errno);
    return E_OK;
}

int32_t TlvUtil::WriteDataValueToTlv(TlvFile tlvFile, const std::string& data)
{
    CHECK_AND_RETURN_RET_LOG(!data.empty(), E_ERR, "data is empty");
    CHECK_AND_RETURN_RET_LOG(tlvFile >= 0, E_ERR, "tlvFile is invalid");
    const char* dataPtr = data.data();
    size_t dataSize = data.size();
    auto ret = ProcessWriteAll(tlvFile, dataPtr, dataSize);
    CHECK_AND_RETURN_RET_LOG(ret, E_ERR, "tlv file write string value error: %{public}d", errno);
    return E_OK;
}

int32_t TlvUtil::UpdateTlvHeadSize(TlvFile tlvFile)
{
    CHECK_AND_RETURN_RET_LOG(tlvFile >= 0, E_ERR, "tlvFile is invalid");
    int32_t ret = WritePaddingToTlv(tlvFile);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_ERR, "WritePaddingToTlv failed");
    off_t currentPos = GetCurrentPosition(tlvFile);
    CHECK_AND_RETURN_RET_LOG(currentPos > sizeof(TlvNode), E_ERR,
        "Invalid current position: %{public}lld", (long long)currentPos);
    
    TlvLength totalDataSize = currentPos - sizeof(TlvNode);
    MEDIA_INFO_LOG("UpdateTlvHeadSize tlv file size: currentPos=%{public}" PRId64 ", dataSize=%{public}" PRId64,
        static_cast<int64_t>(currentPos), totalDataSize);
    
    return UpdateTlvHeadSize(tlvFile, totalDataSize);
}

int32_t TlvUtil::UpdateTlvHeadSize(TlvFile tlvFile, TlvLength dataSize)
{
    CHECK_AND_RETURN_RET_LOG(tlvFile >= 0, E_ERR, "tlvFile is invalid");
    off_t originalPos = GetCurrentPosition(tlvFile);
    CHECK_AND_RETURN_RET_LOG(lseek(tlvFile, sizeof(TlvNode::type), SEEK_SET) != E_ERR, E_ERR, "failed to lseek file");
    const char* dataPtr = reinterpret_cast<const char*>(&dataSize);
    size_t bytesToWrite = sizeof(TlvNode::length);
    auto ret = ProcessWriteAll(tlvFile, dataPtr, bytesToWrite);
    CHECK_AND_RETURN_RET_LOG(ret, E_ERR, "failed to update head size [%{public}d]", errno);
    CHECK_AND_RETURN_RET_LOG(lseek(tlvFile, originalPos, SEEK_SET) != E_ERR,
        E_ERR, "failed to restore original position");
    MEDIA_INFO_LOG("TLV head size updated: %{public}" PRId64, dataSize);
    return E_OK;
}

int32_t TlvUtil::WritePaddingToTlv(TlvFile tlvFile)
{
    MEDIA_DEBUG_LOG("WritePaddingToTlv start");
    CHECK_AND_RETURN_RET_LOG(tlvFile >= 0, E_ERR, "TlvFile is invalid");
    metaSize_ += sizeof(TlvNode);
    size_t paddingSize = (metaSize_ < MAX_PADDING_SIZE) ? (MAX_PADDING_SIZE - metaSize_) : 0;
    MEDIA_DEBUG_LOG("paddingSize: %{public}zu", paddingSize);
    if (paddingSize > 0) {
        CHECK_AND_RETURN_RET_LOG(WriteTagInfoToTlv(tlvFile, TLV_TAG_PADDING, paddingSize) == E_OK, E_ERR,
            "Failed to write tag info for tag 0x%{public}x", TLV_TAG_PADDING);
        std::string paddingData(paddingSize, 0);
        CHECK_AND_RETURN_RET_LOG(WriteDataValueToTlv(tlvFile, paddingData) == E_OK, E_ERR,
            "Failed to write padding data");
    }
    return E_OK;
}


int32_t TlvUtil::ExtractFileData(TlvFile tlvFile, const std::string& outFilePath, TlvLength dataLength)
{
    CHECK_AND_RETURN_RET_LOG(tlvFile >= 0, E_ERR, "tlvFile is invalid");
    TlvFile outTlv = open(outFilePath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, CHOWN_RW_USER_GRP);
    CHECK_AND_RETURN_RET_LOG(outTlv >= 0, E_ERR, "outTlv open error %{public}d", errno);
    UniqueFd outTlvFd(outTlv);

    char buffer[TLV_BUFFER_SIZE] = {0};
    TlvLength remaining = dataLength;
    ssize_t bytes = 0;
    bool ret = false;
    while (remaining > 0) {
        size_t toRead = (remaining < TLV_BUFFER_SIZE) ? remaining : TLV_BUFFER_SIZE;
        bytes = read(tlvFile, buffer, toRead);
        CHECK_AND_RETURN_RET_LOG(bytes > 0, E_ERR, "failed to read file data, errno: %{public}d", errno);
        ret = ProcessWriteAll(outTlvFd.Get(), buffer, bytes);
        CHECK_AND_RETURN_RET_LOG(ret, E_ERR, "failed to write to output file, errno: %{public}d", errno);

        remaining -= bytes;
    }
    return E_OK;
}

int32_t TlvUtil::ProcessTlvNode(TlvFile tlvFile, TlvNode &node, const std::string &destDir,
    std::unordered_map<TlvTag, std::string> &extractedFiles, const std::string &originFilename)
{
    switch (node.type) {
        case TlvTag::TLV_TAG_ORIGIN:
            return ExtractOriginFile(tlvFile, node.length, destDir, extractedFiles, originFilename);
        case TlvTag::TLV_TAG_MOVING_PHOTO_VIDEO:
            return ExtractMovingPhotoVideoFile(tlvFile, node.length, destDir, extractedFiles, originFilename);
        case TlvTag::TLV_TAG_SOURCE:
            return ExtractSourceFile(tlvFile, node.length, destDir, extractedFiles);
        case TlvTag::TLV_TAG_CAMERA:
            return ExtractCameraData(tlvFile, node.length, destDir, extractedFiles);
        case TlvTag::TLV_TAG_EDITDATA:
            return ExtractEditData(tlvFile, node.length, destDir, extractedFiles);
        case TlvTag::TLV_TAG_EXTRA_DATA:
            return ExtractExtraDataFile(tlvFile, node.length, destDir, extractedFiles);
        case TlvTag::TLV_TAG_SOURCE_BACK:
            return ExtractSourceBackFile(tlvFile, node.length, destDir, extractedFiles);
        case TlvTag::TLV_TAG_JSON:
            return ExtractJsonData(tlvFile, node.length, destDir, extractedFiles);
        case TlvTag::TLV_TAG_MOVING_PHOTO_VIDEO_SOURCE:
            return ExtractMovingPhotoVideoSourceFile(tlvFile, node.length, destDir, extractedFiles);
        case TlvTag::TLV_TAG_MOVING_PHOTO_VIDEO_SOURCE_BACK:
            return ExtractMovingPhotoVideoSourceBackFile(tlvFile, node.length, destDir, extractedFiles);
        case TlvTag::TLV_TAG_PADDING:
            CHECK_AND_RETURN_RET_LOG(lseek(tlvFile, node.length, SEEK_CUR) != E_ERR, E_ERR, "failed to skip padding");
            return E_OK;
        default:
            return SkipUnknownField(tlvFile, node.length);
    }
}

int32_t TlvUtil::ExtractMovingPhotoVideoFile(TlvFile tlvFile, TlvLength dataLength, const std::string &destDir,
    std::unordered_map<TlvTag, std::string> &extractedFiles,  const std::string &fileName)
{
    std::string originDir = destDir + "/origin";
    if (!std::filesystem::exists(originDir)) {
        std::filesystem::create_directories(originDir);
    }
    std::string outPutFileName = "origin_file.mp4";
    if (!fileName.empty() && fileName.find('/') == std::string::npos) {
        outPutFileName = MediaFileUtils::GetTitleFromDisplayName(fileName) + ".mp4";
    }
    std::string outFilePath = originDir + "/" + outPutFileName;
    MEDIA_INFO_LOG("Extracting moving photo video file to: %{public}s, size: %{public}" PRId64,
        DfxUtils::GetSafePath(outFilePath).c_str(), dataLength);
    CHECK_AND_RETURN_RET_LOG(ExtractFileData(tlvFile, outFilePath, dataLength) == E_OK,
        E_ERR, "Failed to extract moving photo video file");
    extractedFiles[TLV_TAG_MOVING_PHOTO_VIDEO] = outFilePath;
    return E_OK;
}

int32_t TlvUtil::ExtractOriginFile(TlvFile tlvFile, TlvLength dataLength, const std::string &destDir,
    std::unordered_map<TlvTag, std::string> &extractedFiles, const std::string &fileName)
{
    std::string originDir = destDir + "/origin";
    if (!std::filesystem::exists(originDir)) {
        std::filesystem::create_directories(originDir);
    }
    std::string outPutFileName = "origin_file";
    if (!fileName.empty() && fileName.find('/') == std::string::npos) {
        outPutFileName = fileName;
    }
    std::string outFilePath = originDir + "/" + outPutFileName;
    MEDIA_INFO_LOG("Extracting origin file to: %{public}s, size: %{public}" PRId64,
        DfxUtils::GetSafePath(outFilePath).c_str(), dataLength);
    CHECK_AND_RETURN_RET_LOG(ExtractFileData(tlvFile, outFilePath, dataLength) == E_OK,
        E_ERR, "failed to extract origin file");
    extractedFiles[TLV_TAG_ORIGIN] = outFilePath;
    return E_OK;
}

int32_t TlvUtil::ExtractSourceFile(TlvFile tlvFile, TlvLength dataLength, const std::string &destDir,
    std::unordered_map<TlvTag, std::string> &extractedFiles)
{
    std::string outFilePath = destDir + "/source";
    MEDIA_INFO_LOG("Extracting source file to: %{public}s, size: %{public}" PRId64,
        DfxUtils::GetSafePath(outFilePath).c_str(), dataLength);
    CHECK_AND_RETURN_RET_LOG(ExtractFileData(tlvFile, outFilePath, dataLength) == E_OK,
        E_ERR, "failed to extract source file");
    extractedFiles[TLV_TAG_SOURCE] = outFilePath;
    return E_OK;
}

int32_t TlvUtil::ExtractMovingPhotoVideoSourceFile(TlvFile tlvFile, TlvLength dataLength, const std::string &destDir,
    std::unordered_map<TlvTag, std::string> &extractedFiles)
{
    std::string realDir = destDir + MOVING_PHOTO_VIDEO_EDITDATA_DIR;
    std::string outFilePath = realDir + "/source.mp4";
    MEDIA_INFO_LOG("Extracting moving photo video source file to: %{public}s, size: %{public}" PRId64,
        DfxUtils::GetSafePath(outFilePath).c_str(), dataLength);
    if (!std::filesystem::exists(realDir)) {
        std::filesystem::create_directories(realDir);
    }
    CHECK_AND_RETURN_RET_LOG(ExtractFileData(tlvFile, outFilePath, dataLength) == E_OK,
        E_ERR, "failed to extract moving photo video source file");
    extractedFiles[TLV_TAG_MOVING_PHOTO_VIDEO_SOURCE] = outFilePath;
    return E_OK;
}

int32_t TlvUtil::ExtractMovingPhotoVideoSourceBackFile(TlvFile tlvFile, TlvLength dataLength,
    const std::string &destDir, std::unordered_map<TlvTag, std::string> &extractedFiles)
{
    std::string realDir = destDir + MOVING_PHOTO_VIDEO_EDITDATA_DIR;
    std::string outFilePath = realDir + "/source_back.mp4";
    MEDIA_INFO_LOG("Extracting moving photo image source file to: %{public}s, size: %{public}" PRId64,
        DfxUtils::GetSafePath(outFilePath).c_str(), dataLength);
    if (!std::filesystem::exists(realDir)) {
        std::filesystem::create_directories(realDir);
    }
    CHECK_AND_RETURN_RET_LOG(ExtractFileData(tlvFile, outFilePath, dataLength) == E_OK,
        E_ERR, "failed to extract source file");
    extractedFiles[TLV_TAG_MOVING_PHOTO_VIDEO_SOURCE_BACK] = outFilePath;
    return E_OK;
}

int32_t TlvUtil::ExtractCameraData(TlvFile tlvFile, TlvLength dataLength, const std::string &destDir,
    std::unordered_map<TlvTag, std::string> &extractedFiles)
{
    std::string outFilePath = destDir + "/camera_edit_data";
    MEDIA_INFO_LOG("Extracting camera edit data to: %{public}s, size: %{public}" PRId64,
        DfxUtils::GetSafePath(outFilePath).c_str(), dataLength);
    CHECK_AND_RETURN_RET_LOG(ExtractFileData(tlvFile, outFilePath, dataLength) == E_OK,
        E_ERR, "failed to extract camera edit data");
    extractedFiles[TLV_TAG_CAMERA] = outFilePath;
    return E_OK;
}

int32_t TlvUtil::ExtractExtraDataFile(TlvFile tlvFile, TlvLength dataLength, const std::string &destDir,
    std::unordered_map<TlvTag, std::string> &extractedFiles)
{
    std::string outFilePath = destDir + "/extraData";
    MEDIA_INFO_LOG("Extracting moving photo extra data to: %{public}s, size: %{public}" PRId64,
        DfxUtils::GetSafePath(outFilePath).c_str(), dataLength);
    CHECK_AND_RETURN_RET_LOG(ExtractFileData(tlvFile, outFilePath, dataLength) == E_OK,
        E_ERR, "Failed to extract moving photo extra data");
    extractedFiles[TLV_TAG_EXTRA_DATA] = outFilePath;
    return E_OK;
}

int32_t TlvUtil::ExtractEditData(TlvFile tlvFile, TlvLength dataLength, const std::string &destDir,
    std::unordered_map<TlvTag, std::string> &extractedFiles)
{
    std::string outFilePath = destDir + "/edit_data";
    MEDIA_INFO_LOG("Extracting gallery edit data to: %{public}s, size: %{public}" PRId64,
        DfxUtils::GetSafePath(outFilePath).c_str(), dataLength);
    CHECK_AND_RETURN_RET_LOG(ExtractFileData(tlvFile, outFilePath, dataLength) == E_OK,
        E_ERR, "failed to extract gallery edit data");
    extractedFiles[TLV_TAG_EDITDATA] = outFilePath;
    return E_OK;
}

int32_t TlvUtil::ExtractSourceBackFile(TlvFile tlvFile, TlvLength dataLength, const std::string &destDir,
    std::unordered_map<TlvTag, std::string> &extractedFiles)
{
    std::string outFilePath = destDir + "/source_back";
    MEDIA_INFO_LOG("Extracting source back file to: %{public}s, size: %{public}" PRId64,
        DfxUtils::GetSafePath(outFilePath).c_str(), dataLength);
    CHECK_AND_RETURN_RET_LOG(ExtractFileData(tlvFile, outFilePath, dataLength) == E_OK,
        E_ERR, "failed to extract source back file");
    extractedFiles[TLV_TAG_SOURCE_BACK] = outFilePath;
    return E_OK;
}

int32_t TlvUtil::ExtractJsonData(TlvFile tlvFile, TlvLength dataLength, const std::string &destDir,
    std::unordered_map<TlvTag, std::string> &extractedFiles)
{
    std::string outFilePath = destDir + "/media_info.json";
    MEDIA_INFO_LOG("Extracting JSON data to: %{public}s, size: %{public}" PRId64,
        DfxUtils::GetSafePath(outFilePath).c_str(), dataLength);
    CHECK_AND_RETURN_RET_LOG(ExtractFileData(tlvFile, outFilePath, dataLength) == E_OK,
        E_ERR, "failed to extract JSON data");
    extractedFiles[TLV_TAG_JSON] = outFilePath;
    return E_OK;
}

int32_t TlvUtil::SkipUnknownField(TlvFile tlvFile, TlvLength dataLength)
{
    MEDIA_ERR_LOG("unsupported field type, skipping %{public}" PRId64 " bytes", dataLength);
    if (lseek(tlvFile, dataLength, SEEK_CUR) == E_ERR) {
        MEDIA_ERR_LOG("failed to skip unknown field data");
        return E_ERR;
    }
    return E_OK;
}

int32_t TlvUtil::ExtractTlv(const std::string &tlvFilePath, const std::string &destDir,
    std::unordered_map<TlvTag, std::string> &extractedFiles, const std::string &originFileName)
{
    MEDIA_INFO_LOG("Extract tlv file start");
    extractedFiles.clear();
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsFileExists(tlvFilePath), E_ERR, "tlvFilePath is not exist");
    MEDIA_INFO_LOG("ExtractTlv tlv file path: %{public}s", DfxUtils::GetSafePath(tlvFilePath).c_str());
    CHECK_AND_RETURN_RET_LOG(ValidateTlvFile(tlvFilePath) == E_OK, E_ERR, "TLV file validation failed");
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    if (!std::filesystem::exists(destDir)) {
        std::filesystem::create_directories(destDir);
    }
    TlvFile tlvFile = open(tlvFilePath.c_str(), O_RDONLY);
    CHECK_AND_RETURN_RET_LOG(tlvFile >= 0, E_ERR, "tlvFile is invalid");
    UniqueFd tlvFileFd(tlvFile);
    if (lseek(tlvFileFd.Get(), sizeof(TlvNode), SEEK_SET) == E_ERR) {
        MEDIA_ERR_LOG("Failed to seek past TLV header");
        return E_ERR;
    }
    TlvNode node;
    ssize_t bytes = 0;
    int32_t fileCount = 0;
    size_t fileTotalSize = 0;
    while ((bytes = read(tlvFileFd.Get(), reinterpret_cast<char*>(&node), sizeof(TlvNode))) > 0) {
        int32_t result = ProcessTlvNode(tlvFileFd.Get(), node, destDir, extractedFiles, originFileName);
        if (result == E_OK) {
            fileCount++;
            fileTotalSize += node.length;
        } else {
            MEDIA_ERR_LOG("Failed to process TLV node of type 0x%{public}x", node.type);
        }
    }
    CHECK_AND_RETURN_RET_LOG(bytes >= 0, E_ERR, "failed to read type, errno: %{public}d", errno);
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("Extract tlv file completed successfully, extracted %{public}d files to: %{public}s,"
        " cost %{public}ld ms, total file size: %{public}zu bytes", fileCount, DfxUtils::GetSafePath(destDir).c_str(),
        static_cast<long>(endTime - startTime), fileTotalSize);
    return E_OK;
}
} // namespace Media
} // namespace OHOS