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
#define MLOG_TAG "Media_ORM"

#include "photo_album_po_writer.h"

#include "media_log.h"

namespace OHOS::Media::ORM {
int32_t PhotoAlbumPoWriter::SetMemberVariable(
    const std::string &name, std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool isIdentify = false;
    auto it = this->HANDLERS.find(name);
    if (it != this->HANDLERS.end()) {
        (this->*(it->second.funSetPtr))(val);
        isIdentify = true;
    }
    CHECK_AND_RETURN_RET(!isIdentify, E_OK);
    std::string columnValue = "";
    if (std::holds_alternative<int32_t>(val)) {
        columnValue = std::to_string(std::get<int32_t>(val));
    } else if (std::holds_alternative<int64_t>(val)) {
        columnValue = std::to_string(std::get<int64_t>(val));
    } else if (std::holds_alternative<double>(val)) {
        columnValue = std::to_string(std::get<double>(val));
    } else if (std::holds_alternative<std::string>(val)) {
        columnValue = std::get<std::string>(val);
    } else {
        MEDIA_ERR_LOG("PhotoAlbumPoWriter: SetMemberVariable: variant type is not supported");
    }
    CHECK_AND_RETURN_RET(!columnValue.empty(), E_OK);
    this->objPo_.attributes[name] = columnValue;
    return E_OK;
}

std::unordered_map<std::string, std::string> PhotoAlbumPoWriter::ToMap(bool isIdentifyOnly)
{
    std::string val;
    std::unordered_map<std::string, std::string> res;
    for (const auto &pair : HANDLERS) {
        CHECK_AND_CONTINUE((this->*(pair.second.funGetPtr))(val));
        res[pair.first] = val;
    }
    CHECK_AND_RETURN_RET(!isIdentifyOnly, res);
    bool isValid = true;
    for (const auto &pair : this->objPo_.attributes) {
        isValid = res.find(pair.first) == res.end();
        CHECK_AND_CONTINUE(isValid);
        res[pair.first] = pair.second;
    }
    return res;
}

void PhotoAlbumPoWriter::SetAlbumId(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<int32_t>(val);
    CHECK_AND_RETURN(!errConn);
    this->objPo_.albumId = std::get<int32_t>(val);
}

bool PhotoAlbumPoWriter::GetAlbumId(std::string &val)
{
    CHECK_AND_RETURN_RET(objPo_.albumId.has_value(), false);
    val = std::to_string(this->objPo_.albumId.value());
    return true;
}

void PhotoAlbumPoWriter::SetAlbumType(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<int32_t>(val);
    CHECK_AND_RETURN(!errConn);
    this->objPo_.albumType = std::get<int32_t>(val);
}

bool PhotoAlbumPoWriter::GetAlbumType(std::string &val)
{
    CHECK_AND_RETURN_RET(objPo_.albumType.has_value(), false);
    val = std::to_string(this->objPo_.albumType.value());
    return true;
}

void PhotoAlbumPoWriter::SetAlbumName(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<std::string>(val);
    CHECK_AND_RETURN(!errConn);
    this->objPo_.albumName = std::get<std::string>(val);
}

bool PhotoAlbumPoWriter::GetAlbumName(std::string &val)
{
    CHECK_AND_RETURN_RET(objPo_.albumName.has_value(), false);
    val = this->objPo_.albumName.value();
    return true;
}

void PhotoAlbumPoWriter::SetAlbumLpath(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<std::string>(val);
    CHECK_AND_RETURN(!errConn);
    this->objPo_.lpath = std::get<std::string>(val);
}

bool PhotoAlbumPoWriter::GetAlbumLpath(std::string &val)
{
    CHECK_AND_RETURN_RET(objPo_.lpath.has_value(), false);
    val = this->objPo_.lpath.value();
    return true;
}

void PhotoAlbumPoWriter::SetAlbumCloudId(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<std::string>(val);
    CHECK_AND_RETURN(!errConn);
    this->objPo_.cloudId = std::get<std::string>(val);
}

bool PhotoAlbumPoWriter::GetAlbumCloudId(std::string &val)
{
    CHECK_AND_RETURN_RET(objPo_.cloudId.has_value(), false);
    val = this->objPo_.cloudId.value();
    return true;
}

void PhotoAlbumPoWriter::SetAlbumSubType(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<int32_t>(val);
    CHECK_AND_RETURN(!errConn);
    this->objPo_.albumSubtype = std::get<int32_t>(val);
}

bool PhotoAlbumPoWriter::GetAlbumSubType(std::string &val)
{
    CHECK_AND_RETURN_RET(objPo_.albumSubtype.has_value(), false);
    val = std::to_string(this->objPo_.albumSubtype.value());
    return true;
}

void PhotoAlbumPoWriter::SetAlbumDateAdded(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<int64_t>(val);
    CHECK_AND_RETURN(!errConn);
    this->objPo_.dateAdded = std::get<int64_t>(val);
}

bool PhotoAlbumPoWriter::GetAlbumDateAdded(std::string &val)
{
    CHECK_AND_RETURN_RET(objPo_.dateAdded.has_value(), false);
    val = std::to_string(this->objPo_.dateAdded.value());
    return true;
}

void PhotoAlbumPoWriter::SetAlbumDateModified(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<int64_t>(val);
    CHECK_AND_RETURN(!errConn);
    this->objPo_.dateModified = std::get<int64_t>(val);
}

bool PhotoAlbumPoWriter::GetAlbumDateModified(std::string &val)
{
    CHECK_AND_RETURN_RET(objPo_.dateModified.has_value(), false);
    val = std::to_string(this->objPo_.dateModified.value());
    return true;
}

void PhotoAlbumPoWriter::SetAlbumBundleName(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<std::string>(val);
    CHECK_AND_RETURN(!errConn);
    this->objPo_.bundleName = std::get<std::string>(val);
}

bool PhotoAlbumPoWriter::GetAlbumBundleName(std::string &val)
{
    CHECK_AND_RETURN_RET(objPo_.bundleName.has_value(), false);
    val = this->objPo_.bundleName.value();
    return true;
}

void PhotoAlbumPoWriter::SetAlbumLocalLanguage(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<std::string>(val);
    CHECK_AND_RETURN(!errConn);
    this->objPo_.localLanguage = std::get<std::string>(val);
}

bool PhotoAlbumPoWriter::GetAlbumLocalLanguage(std::string &val)
{
    CHECK_AND_RETURN_RET(objPo_.localLanguage.has_value(), false);
    val = this->objPo_.localLanguage.value();
    return true;
}

void PhotoAlbumPoWriter::SetAlbumOrder(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<int32_t>(val);
    CHECK_AND_RETURN(!errConn);
    this->objPo_.albumOrder = std::get<int32_t>(val);
}

bool PhotoAlbumPoWriter::GetAlbumOrder(std::string &val)
{
    CHECK_AND_RETURN_RET(objPo_.albumOrder.has_value(), false);
    val = std::to_string(this->objPo_.albumOrder.value());
    return true;
}

void PhotoAlbumPoWriter::SetPriority(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<int32_t>(val);
    CHECK_AND_RETURN(!errConn);
    this->objPo_.priority = std::get<int32_t>(val);
}

bool PhotoAlbumPoWriter::GetPriority(std::string &val)
{
    CHECK_AND_RETURN_RET(objPo_.priority.has_value(), false);
    val = std::to_string(this->objPo_.priority.value());
    return true;
}

void PhotoAlbumPoWriter::SetDirty(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<int32_t>(val);
    CHECK_AND_RETURN(!errConn);
    this->objPo_.dirty = std::get<int32_t>(val);
}

bool PhotoAlbumPoWriter::GetDirty(std::string &val)
{
    CHECK_AND_RETURN_RET(objPo_.dirty.has_value(), false);
    val = std::to_string(this->objPo_.dirty.value());
    return true;
}

void PhotoAlbumPoWriter::SetCoverUriSource(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool conn = std::holds_alternative<int32_t>(val);
    CHECK_AND_RETURN(conn);
    this->objPo_.coverUriSource = std::get<int32_t>(val);
}

bool PhotoAlbumPoWriter::GetCoverUriSource(std::string &val)
{
    CHECK_AND_RETURN_RET(objPo_.coverUriSource.has_value(), false);
    val = std::to_string(this->objPo_.coverUriSource.value());
    return true;
}

void PhotoAlbumPoWriter::SetCoverCloudId(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool conn = std::holds_alternative<std::string>(val);
    CHECK_AND_RETURN(conn);
    this->objPo_.coverCloudId = std::get<std::string>(val);
}

bool PhotoAlbumPoWriter::GetCoverCloudId(std::string &val)
{
    CHECK_AND_RETURN_RET(objPo_.coverCloudId.has_value(), false);
    val = this->objPo_.coverCloudId.value();
    return true;
}

void PhotoAlbumPoWriter::SetUploadStatus(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool conn = std::holds_alternative<int32_t>(val);
    CHECK_AND_RETURN(conn);
    this->objPo_.uploadStatus = std::get<int32_t>(val);
}

bool PhotoAlbumPoWriter::GetUploadStatus(std::string &val)
{
    CHECK_AND_RETURN_RET(objPo_.uploadStatus.has_value(), false);
    val = std::to_string(this->objPo_.uploadStatus.value());
    return true;
}

void PhotoAlbumPoWriter::SetUniqueId(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool conn = std::holds_alternative<std::string>(val);
    CHECK_AND_RETURN(conn);
    this->objPo_.uniqueId = std::get<std::string>(val);
}

bool PhotoAlbumPoWriter::GetUniqueId(std::string &val)
{
    CHECK_AND_RETURN_RET(objPo_.uniqueId.has_value(), false);
    val = this->objPo_.uniqueId.value();
    return true;
}

void PhotoAlbumPoWriter::SetShareAlbumOwner(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool conn = std::holds_alternative<std::string>(val);
    CHECK_AND_RETURN(conn);
    this->objPo_.shareAlbumOwner = std::get<std::string>(val);
}

bool PhotoAlbumPoWriter::GetShareAlbumOwner(std::string &val)
{
    CHECK_AND_RETURN_RET(objPo_.shareAlbumOwner.has_value(), false);
    val = this->objPo_.shareAlbumOwner.value();
    return true;
}
}  // namespace OHOS::Media::ORM