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
#ifndef FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_MTP_PACKET_TOOLS_H_
#define FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_MTP_PACKET_TOOLS_H_
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <time.h>
#include "mtp_constants.h"

// these numbers are defined by protocol, have no exact meaning
constexpr int BIT_4 = 4;
constexpr int BIT_8 = 8;
constexpr int BIT_16 = 16;
constexpr int BIT_24 = 24;
constexpr int BIT_32 = 32;
constexpr int BIT_40 = 40;
constexpr int BIT_48 = 48;
constexpr int BIT_56 = 56;
constexpr int BIT_64 = 64;
constexpr int BIT_128 = 128;
constexpr int OFFSET_0 = 0;
constexpr int OFFSET_1 = 1;
constexpr int OFFSET_2 = 2;
constexpr int OFFSET_3 = 3;
constexpr int OFFSET_4 = 4;
constexpr int OFFSET_5 = 5;
constexpr int OFFSET_6 = 6;
constexpr int OFFSET_7 = 7;
constexpr int OFFSET_8 = 8;
constexpr int OFFSET_9 = 9;
constexpr int OFFSET_10 = 10;
constexpr int OFFSET_11 = 11;
constexpr int CONTAINER_TYPE_2 = 2;

namespace OHOS {
namespace Media {
class MtpPacketTool {
public:
    MtpPacketTool();
    virtual ~MtpPacketTool();

    static uint16_t GetUInt16(uint8_t numFirst, uint8_t numSecond);
    static uint32_t GetUInt32(uint8_t numFirst, uint8_t numSecond, uint8_t numThird, uint8_t numFourth);
    
    static void PutUInt8(std::vector<uint8_t> &outBuffer, uint16_t value);
    static void PutUInt16(std::vector<uint8_t> &outBuffer, uint16_t value);
    static void PutUInt32(std::vector<uint8_t> &outBuffer, uint32_t value);
    static void PutUInt64(std::vector<uint8_t> &outBuffer, uint64_t value);
    static void PutUInt128(std::vector<uint8_t> &outBuffer, uint64_t value);
    static void PutUInt128(std::vector<uint8_t> &outBuffer, const uint128_t value);
    static void PutAUInt16(std::vector<uint8_t> &outBuffer, const uint16_t *values, int count);
    static void PutAUInt32(std::vector<uint8_t> &outBuffer, const uint32_t *values, int count);
    static void PutInt8(std::vector<uint8_t> &outBuffer, int8_t value);
    static void PutInt16(std::vector<uint8_t> &outBuffer, int16_t value);
    static void PutInt32(std::vector<uint8_t> &outBuffer, int32_t value);
    static void PutInt64(std::vector<uint8_t> &outBuffer, int64_t value);
    static void PutInt128(std::vector<uint8_t> &outBuffer, int64_t value);
    static void PutInt128(std::vector<uint8_t> &outBuffer, const int128_t value);
    static void PutString(std::vector<uint8_t> &outBuffer, const std::string &string);
    
    static uint8_t GetUInt8(const std::vector<uint8_t> &buffer, size_t &offset);
    static uint16_t GetUInt16(const std::vector<uint8_t> &buffer, size_t &offset);
    static uint32_t GetUInt32(const std::vector<uint8_t> &buffer, size_t &offset);
    static std::shared_ptr<UInt16List> GetAUInt16(const std::vector<uint8_t> &buffer, size_t &offset);
    static std::shared_ptr<UInt32List> GetAUInt32(const std::vector<uint8_t> &buffer, size_t &offset);
    static bool GetUInt8(const std::vector<uint8_t> &buffer, size_t &offset, uint8_t &value);
    static bool GetUInt16(const std::vector<uint8_t> &buffer, size_t &offset, uint16_t &value);
    static bool GetUInt32(const std::vector<uint8_t> &buffer, size_t &offset, uint32_t &value);
    static bool GetUInt64(const std::vector<uint8_t> &buffer, size_t &offset, uint64_t &value);
    static bool GetUInt128(const std::vector<uint8_t> &buffer, size_t &offset, uint128_t &value);
    static bool GetInt8(const std::vector<uint8_t> &buffer, size_t &offset, int8_t &value);
    static bool GetInt16(const std::vector<uint8_t> &buffer, size_t &offset, int16_t &value);
    static bool GetInt32(const std::vector<uint8_t> &buffer, size_t &offset, int32_t &value);
    static bool GetInt64(const std::vector<uint8_t> &buffer, size_t &offset, int64_t &value);
    static bool GetInt128(const std::vector<uint8_t> &buffer, size_t &offset, int128_t &value);
    static std::string GetString(const std::vector<uint8_t> &buffer, size_t &offset);
    static std::u16string Utf8ToUtf16(const std::string &inputStr);
    static std::string Utf16ToUtf8(const std::u16string &inputStr);
    static bool GetString(const std::vector<uint8_t> &buffer, size_t &offset, std::string &str);
    static std::string FormatDateTime(time_t sec);
    static const std::string &GetOperationName(uint16_t code);
    static const std::string &GetEventName(uint16_t code);
    static const std::string &GetFormatName(uint16_t code);
    static const std::string &GetObjectPropName(uint16_t code);
    static const std::string &GetDataTypeName(int type);
    static const std::string &GetAssociationName(int type);
    static const std::string &CodeToStrByMap(int type, const std::map<int, std::string> &theMap);
    static const std::string &CodeToStrByMap(uint32_t code, const std::map<uint32_t, std::string> &theMap);
    static int GetObjectPropTypeByPropCode(uint16_t propCode);

    static bool Int8ToString(const int8_t &value, std::string &outStr);
    static bool UInt8ToString(const uint8_t &value, std::string &outStr);
    static bool Int16ToString(const int16_t &value, std::string &outStr);
    static bool UInt16ToString(const uint16_t &value, std::string &outStr);
    static bool Int32ToString(const int32_t &value, std::string &outStr);
    static bool UInt32ToString(const uint32_t &value, std::string &outStr);
    static bool Int64ToString(const int64_t &value, std::string &outStr);
    static bool UInt64ToString(const uint64_t &value, std::string &outStr);
    static bool Int128ToString(const int128_t &value, std::string &outStr);
    static bool UInt128ToString(const uint128_t &value, std::string &outStr);
    static std::string StrToString(const std::string &value);

    static const std::string &GetIndentBlank();
    static std::string GetIndentBlank(size_t indent);
    static bool CanDump();
    static void DumpPacket(const std::vector<uint8_t> &outBuffer);
    static void Dump(const std::vector<uint8_t> &data, uint32_t offset = 0, uint32_t sum = UINT32_MAX);

private:
    static bool DumpClear(size_t loc, std::unique_ptr<char[]> &hexBuf, int hexBufSize,
        std::unique_ptr<char[]> &txtBuf, int txtBufSize);
    static bool DumpChar(uint8_t u8, std::unique_ptr<char[]> &hexBuf, int hexBufSize,
        std::unique_ptr<char[]> &txtBuf, int txtBufSize);
    static void DumpShow(const std::unique_ptr<char[]> &hexBuf, int hexBufSize,
        const std::unique_ptr<char[]> &txtBuf, int txtBufSize);
};
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_PAYLOAD_DATA_H_
