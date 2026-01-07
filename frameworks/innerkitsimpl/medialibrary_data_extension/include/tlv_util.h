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

#ifndef TLV_UTIL_H
#define TLV_UTIL_H

#include <string>
#include <map>

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

using TlvFile = int;
using TlvLength = uint64_t;
enum TlvTag : uint32_t {
    TLV_TAG_HEADER = 0xff01,
    TLV_TAG_ORIGIN = 0xff10,
    TLV_TAG_CAMERA = 0xff11,
    TLV_TAG_EDITDATA = 0xff12,
    TLV_TAG_SOURCE_BACK = 0xff13,
    TLV_TAG_SOURCE = 0xff14,
    TLV_TAG_MOVING_PHOTO_VIDEO = 0xff20,
    TLV_TAG_MOVING_PHOTO_VIDEO_SOURCE = 0xff21,
    TLV_TAG_MOVING_PHOTO_VIDEO_SOURCE_BACK = 0xff22,
    TLV_TAG_EXTRA_DATA = 0xff23,
    TLV_TAG_JSON = 0xff50,
    TLV_TAG_PADDING = 0xff99,
};

#pragma pack(push, 1)
struct TlvNode {
    TlvTag type;
    TlvLength length;
};
#pragma pack(pop)

class EXPORT TlvUtil {
public:
    static TlvFile CreateTlvFile(const std::string& tlvPath);
    static int32_t WriteOriginFileToTlv(TlvFile tlvFile, const std::string &srcFilePath, int32_t srcFd);
    static int32_t WriteSourceFileToTlv(TlvFile tlvFile, int32_t srcFd);
    static int32_t WriteMovingPhotoVideoFileToTlv(TlvFile tlvFile, int32_t srcFd);
    static int32_t WriteMovingPhotoVideoSourceBackFileToTlv(TlvFile tlvFile, int32_t srcFd);
    static int32_t WriteMovingPhotoVideoSourceFileToTlv(TlvFile tlvFile, int32_t srcFd);
    static int32_t WriteSourceBackFileToTlv(TlvFile tlvFile, int32_t srcFd);
    static int32_t WriteCameraDataToTlv(TlvFile tlvFile, const std::string &srcFilePath);
    static int32_t WriteEditDataToTlv(TlvFile tlvFile, const std::string &srcFilePath);
    static int32_t WriteExtraDataFileToTlv(TlvFile tlvFile, const std::string &srcFilePath);
    static int32_t WriteJsonDataToTlv(TlvFile tlvFile, const std::string &jsonDataPath);
    static int32_t ValidateTlvFile(const std::string &tlvFilePath);
    static int32_t ValidateTlvFile(TlvFile tlvFile);
    static int32_t ExtractTlv(const std::string &tlvFilePath, const std::string &destDir,
        std::unordered_map<TlvTag, std::string> &extractedFiles, const std::string &originFileName = "");
    static int32_t UpdateTlvHeadSize(TlvFile tlvFile);
private:
    static int32_t WriteTagInfoToTlv(TlvFile tlvFile, TlvTag tag, TlvLength length);
    static int32_t WriteDataValueToTlv(TlvFile tlvFile, TlvFile srcFile);
    static int32_t WriteDataValueToTlv(TlvFile tlvFile, const std::string& data);
    static int32_t WriteTaggedFileToTlv(TlvFile tlvFile, TlvTag tag, const std::string &srcFilePath);
    static int32_t WriteTaggedFileToTlv(TlvFile tlvFile, TlvTag tag, int32_t srcFd);
    static int32_t ExtractFileData(TlvFile tlvFile, const std::string& outFilePath, TlvLength dataLength);
    static off_t GetCurrentPosition(TlvFile tlvFile);
    static int32_t UpdateTlvHeadSize(TlvFile tlvFile, TlvLength dataSize);
    static int32_t ProcessTlvNode(TlvFile tlvFile, TlvNode &node, const std::string &destDir,
        std::unordered_map<TlvTag, std::string> &extractedFiles, const std::string &originFileName = "");
    static int32_t ExtractOriginFile(TlvFile tlvFile, TlvLength dataLength, const std::string &destDir,
        std::unordered_map<TlvTag, std::string> &extractedFiles, const std::string &fileName = "");
    static int32_t ExtractMovingPhotoVideoFile(TlvFile tlvFile, TlvLength dataLength, const std::string &destDir,
        std::unordered_map<TlvTag, std::string> &extractedFiles, const std::string &fileName = "");
    static int32_t ExtractSourceFile(TlvFile tlvFile, TlvLength dataLength, const std::string &destDir,
        std::unordered_map<TlvTag, std::string> &extractedFiles);
    static int32_t ExtractMovingPhotoVideoSourceFile(TlvFile tlvFile, TlvLength dataLength, const std::string &destDir,
        std::unordered_map<TlvTag, std::string> &extractedFiles);
    static int32_t ExtractMovingPhotoVideoSourceBackFile(TlvFile tlvFile, TlvLength dataLength,
        const std::string &destDir, std::unordered_map<TlvTag, std::string> &extractedFiles);
    static int32_t ExtractCameraData(TlvFile tlvFile, TlvLength dataLength, const std::string &destDir,
        std::unordered_map<TlvTag, std::string> &extractedFiles);
    static int32_t ExtractEditData(TlvFile tlvFile, TlvLength dataLength, const std::string &destDir,
        std::unordered_map<TlvTag, std::string> &extractedFiles);
    static int32_t ExtractExtraDataFile(TlvFile tlvFile, TlvLength dataLength, const std::string &destDir,
        std::unordered_map<TlvTag, std::string> &extractedFiles);
    static int32_t ExtractSourceBackFile(TlvFile tlvFile, TlvLength dataLength, const std::string &destDir,
        std::unordered_map<TlvTag, std::string> &extractedFiles);
    static int32_t ExtractJsonData(TlvFile tlvFile, TlvLength dataLength, const std::string &destDir,
        std::unordered_map<TlvTag, std::string> &extractedFiles);
    static int32_t SkipUnknownField(TlvFile tlvFile, TlvLength dataLength);
    static bool ProcessWriteAll(int fd, const char* data, size_t size);
    static int32_t WritePaddingToTlv(TlvFile tlvFile);

    static thread_local size_t metaSize_;
};
} // namespace Media
} // namespace OHOS

#endif // TLV_UTIL_H