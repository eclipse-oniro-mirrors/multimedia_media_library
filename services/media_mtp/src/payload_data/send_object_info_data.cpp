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
#include "payload_data/send_object_info_data.h"
#include <string>
#include "media_log.h"
#include "media_mtp_utils.h"
#include "mtp_packet_tools.h"
using namespace std;
namespace OHOS {
namespace Media {
static constexpr int32_t PARSER_PARAM_SUM = 2;

SendObjectInfoData::SendObjectInfoData(std::shared_ptr<MtpOperationContext> &context)
    : PayloadData(context)
{
}

SendObjectInfoData::SendObjectInfoData()
{
}

SendObjectInfoData::~SendObjectInfoData()
{
}

int SendObjectInfoData::Parser(const std::vector<uint8_t> &buffer, int32_t readSize)
{
    if (context_ == nullptr) {
        MEDIA_ERR_LOG("SendObjectInfoData::parser null");
        return MTP_ERROR_INVALID_OBJECTHANDLE;
    }

    if (!context_->indata) {
        int32_t parameterCount = (readSize - MTP_CONTAINER_HEADER_SIZE) / MTP_PARAMETER_SIZE;
        if (parameterCount < PARSER_PARAM_SUM) {
            MEDIA_ERR_LOG("SendObjectInfoData::parser paramCount=%{public}u, needCount=%{public}d",
                parameterCount, PARSER_PARAM_SUM);
            return MTP_ERROR_PACKET_INCORRECT;
        }

        size_t offset = MTP_CONTAINER_HEADER_SIZE;
        context_->storageID = MtpPacketTool::GetUInt32(buffer, offset);
        context_->parent = MtpPacketTool::GetUInt32(buffer, offset);
    } else {
        size_t offset = MTP_CONTAINER_HEADER_SIZE;
        int res = ParserData(buffer, offset);
        if (res != MTP_SUCCESS) {
            return res;
        }
    }
    return MTP_SUCCESS;
}

int SendObjectInfoData::Maker(std::vector<uint8_t> &outBuffer)
{
    if (!hasSetParam_) {
        MEDIA_ERR_LOG("SendObjectInfoData::maker set");
        return MTP_ERROR_INVALID_OBJECTHANDLE;
    }

    MtpPacketTool::PutUInt32(outBuffer, storageID_);
    MtpPacketTool::PutUInt32(outBuffer, parent_);
    MtpPacketTool::PutUInt32(outBuffer, handle_);
    return MTP_SUCCESS;
}

uint32_t SendObjectInfoData::CalculateSize()
{
    std::vector<uint8_t> tmpVar;
    int res = Maker(tmpVar);
    if (res != MTP_SUCCESS) {
        return res;
    }
    return tmpVar.size();
}

bool SendObjectInfoData::SetSetParam(uint32_t storageID, uint32_t parent, uint32_t handle)
{
    if (hasSetParam_) {
        return false;
    }
    hasSetParam_ = true;
    storageID_ = storageID;
    parent_ = parent;
    handle_ = handle;
    return true;
}

int SendObjectInfoData::ParserData(const std::vector<uint8_t> &buffer, size_t &offset)
{
    uint16_t tmpUse16 = 0;
    uint32_t tmpUse32 = 0;
    if (!MtpPacketTool::GetUInt32(buffer, offset, tmpUse32)) { // storage ID
        return MTP_ERROR_PACKET_INCORRECT;
    }
    if (!MtpPacketTool::GetUInt16(buffer, offset, context_->format)) {
        return MTP_ERROR_PACKET_INCORRECT;
    }
    if (!MtpPacketTool::GetUInt16(buffer, offset, tmpUse16)) { // protection status
        return MTP_ERROR_PACKET_INCORRECT;
    }
    if (!MtpPacketTool::GetUInt32(buffer, offset, context_->sendObjectFileSize)) {
        return MTP_ERROR_PACKET_INCORRECT;
    }
    int res = ParserDataForImageInfo(buffer, offset);
    if (res != MTP_SUCCESS) {
        return res;
    }
    if (!MtpPacketTool::GetUInt32(buffer, offset, tmpUse32)) { // parent
        return MTP_ERROR_PACKET_INCORRECT;
    }
    if (!MtpPacketTool::GetUInt16(buffer, offset, tmpUse16)) {
        return MTP_ERROR_PACKET_INCORRECT;
    }
    if (!MtpPacketTool::GetUInt32(buffer, offset, tmpUse32)) {
        return MTP_ERROR_PACKET_INCORRECT;
    }
    if (!MtpPacketTool::GetUInt32(buffer, offset, tmpUse32)) { // sequence number
        return MTP_ERROR_PACKET_INCORRECT;
    }

    return ParserDataForFileInfo(buffer, offset);
}

int SendObjectInfoData::ParserDataForImageInfo(const std::vector<uint8_t> &buffer, size_t &offset)
{
    uint16_t tmpUse16 = 0;
    uint32_t tmpUse32 = 0;

    if (!MtpPacketTool::GetUInt16(buffer, offset, tmpUse16)) { // thumb format
        return MTP_ERROR_PACKET_INCORRECT;
    }
    if (!MtpPacketTool::GetUInt32(buffer, offset, tmpUse32)) { // thumb compressed size
        return MTP_ERROR_PACKET_INCORRECT;
    }
    if (!MtpPacketTool::GetUInt32(buffer, offset, tmpUse32)) { // thumb pix width
        return MTP_ERROR_PACKET_INCORRECT;
    }
    if (!MtpPacketTool::GetUInt32(buffer, offset, tmpUse32)) { // thumb pix height
        return MTP_ERROR_PACKET_INCORRECT;
    }
    if (!MtpPacketTool::GetUInt32(buffer, offset, tmpUse32)) { // image pix width
        return MTP_ERROR_PACKET_INCORRECT;
    }
    if (!MtpPacketTool::GetUInt32(buffer, offset, tmpUse32)) { // image pix height
        return MTP_ERROR_PACKET_INCORRECT;
    }
    if (!MtpPacketTool::GetUInt32(buffer, offset, tmpUse32)) { // image bit depth
        return MTP_ERROR_PACKET_INCORRECT;
    }

    return MTP_SUCCESS;
}

int SendObjectInfoData::ParserDataForFileInfo(const std::vector<uint8_t> &buffer, size_t &offset)
{
    if (!MtpPacketTool::GetString(buffer, offset, context_->name)) { // file name
        return MTP_ERROR_PACKET_INCORRECT;
    }
    if (context_->name.empty()) {
        MEDIA_ERR_LOG("ParserDataForFileInfo: empty name");
        return MTP_ERROR_PACKET_INCORRECT;
    }
    if (!MtpPacketTool::GetString(buffer, offset, context_->created)) { // date created
        return MTP_ERROR_PACKET_INCORRECT;
    }
    if (!MtpPacketTool::GetString(buffer, offset, context_->modified)) { // date modified
        return MTP_ERROR_PACKET_INCORRECT;
    }

    return MTP_SUCCESS;
}
} // namespace Media
} // namespace OHOS