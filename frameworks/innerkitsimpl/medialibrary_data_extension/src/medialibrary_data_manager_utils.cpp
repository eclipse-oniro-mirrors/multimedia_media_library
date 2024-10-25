/*
 * Copyright (C) 2021-2023 Huawei Device Co., Ltd.
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

#include "medialibrary_data_manager_utils.h"

#include "media_file_uri.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_common_utils.h"
#include "medialibrary_db_const.h"
#include "photo_album_column.h"

using namespace std;

namespace OHOS {
namespace Media {
bool MediaLibraryDataManagerUtils::IsNumber(const string &str)
{
    if (str.empty()) {
        MEDIA_ERR_LOG("IsNumber input is empty ");
        return false;
    }

    for (char const &c : str) {
        if (isdigit(c) == 0) {
            return false;
        }
    }
    return true;
}

string MediaLibraryDataManagerUtils::GetOperationType(const string &uri)
{
    string oprn;
    size_t found = uri.rfind('/');
    if (found != string::npos) {
        oprn = uri.substr(found + 1);
    }

    return oprn;
}

string MediaLibraryDataManagerUtils::GetDisPlayNameFromPath(const std::string &path)
{
    string displayName;
    size_t lastSlashPosition = path.rfind("/");
    if (lastSlashPosition != string::npos) {
        displayName = path.substr(lastSlashPosition + 1);
    }
    return displayName;
}

string MediaLibraryDataManagerUtils::ObtionCondition(string &strQueryCondition, const vector<string> &whereArgs)
{
    for (string args : whereArgs) {
        size_t pos = strQueryCondition.find('?');
        if (pos != string::npos) {
            strQueryCondition.replace(pos, 1, "'" + args + "'");
        }
    }
    return strQueryCondition;
}

std::string MediaLibraryDataManagerUtils::GetTypeUriByUri(std::string &uri)
{
    string typeUri;
    if (uri.find(PhotoColumn::PHOTO_URI_PREFIX) != string::npos) {
        typeUri = PhotoColumn::PHOTO_URI_PREFIX;
    } else if (uri.find(PhotoAlbumColumns::ALBUM_URI_PREFIX) != string::npos) {
        typeUri = PhotoAlbumColumns::ALBUM_URI_PREFIX;
    } else if (uri.find(AudioColumn::AUDIO_URI_PREFIX) != string::npos) {
        typeUri = AudioColumn::AUDIO_URI_PREFIX;
    }
    return typeUri;
}
} // namespace Media
} // namespace OHOS
