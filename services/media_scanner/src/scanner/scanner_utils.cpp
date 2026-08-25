/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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
#define MLOG_TAG "Scanner"

#include "scanner_utils.h"

#include <cerrno>
#include <fstream>

#include "directory_ex.h"
#include "media_column.h"
#include "media_log.h"
#include "medialibrary_type_const.h"
namespace OHOS {
namespace Media {
using namespace std;

// Check if file exists or not
bool ScannerUtils::IsExists(const string &path)
{
    struct stat statInfo {};

    if (path.empty()) {
        MEDIA_ERR_LOG("Given path name is empty");
        return false;
    }

    return ((stat(path.c_str(), &statInfo)) == ERR_SUCCESS);
}

// Get the file name from file URI
string ScannerUtils::GetFileNameFromUri(const string &path)
{
    if (!path.empty()) {
        size_t lastSlashPosition = path.rfind("/");
        if (lastSlashPosition != string::npos) {
            if (path.size() > lastSlashPosition) {
                return path.substr(lastSlashPosition + 1);
            }
        }
    }

    MEDIA_ERR_LOG("Failed to obtain file name because given pathname is empty");
    return "";
}

// Get file extension from the given filepath or displayName
string ScannerUtils::GetFileExtension(const string &pathOrDisplayName)
{
    if (!pathOrDisplayName.empty()) {
        size_t dotIndex = pathOrDisplayName.rfind(".");
        string extension {};
        if (dotIndex != string::npos) {
            extension = pathOrDisplayName.substr(dotIndex + 1);
            CHECK_AND_WARN_LOG(!extension.empty(), "Extension is empty, path/displayName: %{public}s",
                pathOrDisplayName.c_str());
            return extension;
        }
    }

    MEDIA_ERR_LOG("Failed to obtain file extension because given path/displayName is empty");
    return "";
}

// Check if the given path is a directory path
bool ScannerUtils::IsDirectory(const string &path)
{
    struct stat s;

    if (!path.empty()) {
        if (stat(path.c_str(), &s) == 0) {
            if (s.st_mode & S_IFDIR) {
                return true;
            }
        }
    }

    MEDIA_ERR_LOG("Either path is empty or it is not a directory");
    return false;
}

bool ScannerUtils::IsRegularFile(const string &path)
{
    struct stat s;
    if (!path.empty()) {
        if (stat(path.c_str(), &s) == 0) {
            if (s.st_mode & S_IFREG) {
                return true;
            }
        }
    }

    return false;
}

// Check if the given file starts with '.' , i.e. if it is hidden
bool ScannerUtils::IsFileHidden(const string &path)
{
    if (!path.empty()) {
        string fileName = GetFileNameFromUri(path);
        if (!fileName.empty() && fileName.at(0) == '.') {
            return true;
        }
    }

    return false;
}

// Get the parent path
string ScannerUtils::GetParentPath(const string &path)
{
    if (!path.empty()) {
        size_t lastSlashPosition = path.rfind("/");
        if (lastSlashPosition != string::npos && path.size() > lastSlashPosition) {
            return path.substr(0, lastSlashPosition);
        }
    }

    MEDIA_ERR_LOG("Failed to obtain the parent path");
    return "";
}

void ScannerUtils::GetRootMediaDir(string &dir)
{
    dir = ROOT_MEDIA_DIR;
}

string ScannerUtils::GetFileTitle(const string &displayName)
{
    string::size_type pos = displayName.find_last_of('.');
    return (pos == string::npos) ? displayName : displayName.substr(0, pos);
}

bool ScannerUtils::IsDirHidden(const string &path, bool skipPhoto)
{
    bool dirHid = false;

    if (!path.empty()) {
        string dirName = ScannerUtils::GetFileNameFromUri(path);
        if (!dirName.empty() && dirName.at(0) == '.') {
            MEDIA_DEBUG_LOG("hidden Directory, name:%{private}s path:%{private}s", dirName.c_str(), path.c_str());
            return true;
        }

        string curPath = path;
        string excludePath = curPath.append("/.nomedia");
        // Check is the folder consist of .nomedia file
        if (ScannerUtils::IsExists(excludePath)) {
            return true;
        }

        // Check is the dir is part of skiplist
        if (skipPhoto && CheckSkipScanList(path)) {
            MEDIA_DEBUG_LOG("skip Directory, path:%{private}s", path.c_str());
            return true;
        }
    }

    return dirHid;
}

bool ScannerUtils::IsDirHiddenRecursive(const string &path, bool skipPhoto)
{
    bool dirHid = false;
    string curPath = path;

    do {
        dirHid = IsDirHidden(curPath, skipPhoto);
        if (dirHid) {
            break;
        }

        curPath = ScannerUtils::GetParentPath(curPath);
        if (curPath.empty()) {
            break;
        }
    } while (true);

    return dirHid;
}

// Check if path is part of Skip scan list
bool ScannerUtils::CheckSkipScanList(const string &path)
{
    if (path.length() <= ROOT_MEDIA_DIR.length()) {
        return false;
    }
    static const string AUDIO = "Audios";
    static const string CAMERA = "Camera";
    static const string Pictures = "Pictures";
    static const string Videos = "Videos";
    static const string Doc = "Documents";
    static const string Download = "Download";
    // white list
    static vector<string> list = {
        { ROOT_MEDIA_DIR + AUDIO },
        { ROOT_MEDIA_DIR + CAMERA },
        { ROOT_MEDIA_DIR + Pictures },
        { ROOT_MEDIA_DIR + Videos },
        { ROOT_MEDIA_DIR + Doc },
        { ROOT_MEDIA_DIR + Download },
    };
    for (const auto &pathPrefix : list) {
        if (path.find(pathPrefix) != string::npos) {
            return false;
        }
    }
    return true;
}
} // namespace Media
} // namespace OHOS
