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

#include "medialibrary_unittest_utils.h"

#include <fstream>

#include "ability_context_impl.h"
#include "fetch_result.h"
#include "media_log.h"
#include "media_file_utils.h"
#include "media_smart_map_column.h"
#include "medialibrary_data_manager.h"
#include "mimetype_utils.h"
#include "result_set_utils.h"
#include "scanner_utils.h"

using namespace std;
using namespace OHOS::DataShare;
using namespace OHOS::AppExecFwk;
namespace OHOS {
namespace Media {
bool MediaLibraryUnitTestUtils::IsValid()
{
    return isValid_;
}

void MediaLibraryUnitTestUtils::Init()
{
    auto stageContext = std::make_shared<AbilityRuntime::ContextImpl>();
    auto abilityContextImpl = std::make_shared<OHOS::AbilityRuntime::AbilityContextImpl>();
    abilityContextImpl->SetStageContext(stageContext);
    MediaLibraryDataManager::GetInstance()->InitMediaLibraryMgr(abilityContextImpl, abilityContextImpl);
    auto ret = MediaLibraryDataManager::GetInstance()->InitMediaLibraryMgr(abilityContextImpl, abilityContextImpl);
    CHECK_AND_RETURN_LOG(ret == E_OK, "InitMediaLibraryMgr failed, ret: %{public}d", ret);
    isValid_ = true;
}

void MediaLibraryUnitTestUtils::InitRootDirs()
{
    for (const auto &dir : TEST_ROOT_DIRS) {
        shared_ptr<FileAsset> dirAsset = nullptr;
        if (!CreateAlbum(dir, nullptr, dirAsset)) {
            isValid_ = false;
            return;
        }
        rootDirAssetMap_[dir] = dirAsset;
    }
}

void MediaLibraryUnitTestUtils::CleanTestFiles()
{
    system("rm -rf /storage/cloud/files/Audio/*");
    system("rm -rf /storage/cloud/files/Audios/*");
    system("rm -rf /storage/cloud/files/Camera/*");
    system("rm -rf /storage/cloud/files/Docs/Documents/*");
    system("rm -rf /storage/cloud/files/Docs/Download/*");
    system("rm -rf /storage/cloud/files/Photo/*");
    system("rm -rf /storage/cloud/files/Pictures/*");
    system("rm -rf /storage/cloud/files/Pictures/*");
    system("rm -rf /storage/cloud/files/Videos/*");
    system("rm -rf /storage/cloud/files/.*");
    auto rdbStore = MediaLibraryDataManager::GetInstance()->rdbStore_;
    NativeRdb::AbsRdbPredicates predicates(MEDIALIBRARY_TABLE);
    predicates.GreaterThan(MEDIA_DATA_DB_ID, to_string(0));
    int32_t deletedRows = -1;
    auto ret = rdbStore->Delete(deletedRows, predicates);
    MEDIA_INFO_LOG("CleanTestFiles Delete retVal: %{public}d, deletedRows: %{public}d", ret, deletedRows);
}

void MediaLibraryUnitTestUtils::CleanBundlePermission()
{
    auto rdbStore = MediaLibraryDataManager::GetInstance()->rdbStore_;
    NativeRdb::AbsRdbPredicates predicates(BUNDLE_PERMISSION_TABLE);
    predicates.GreaterThan(MEDIA_DATA_DB_ID, to_string(0));
    int32_t deletedRows = -1;
    auto ret = rdbStore->Delete(deletedRows, predicates);
    MEDIA_INFO_LOG("CleanBundlePermission Delete retVal: %{public}d, deletedRows: %{public}d", ret, deletedRows);
}

shared_ptr<FileAsset> MediaLibraryUnitTestUtils::GetRootAsset(const string &dir)
{
    if (rootDirAssetMap_.find(dir) != rootDirAssetMap_.end()) {
        return rootDirAssetMap_[dir];
    }
    return nullptr;
}

bool MediaLibraryUnitTestUtils::IsFileExists(const string filePath)
{
    struct stat statInfo {};
    int errCode = stat(filePath.c_str(), &statInfo);
    return (errCode == 0);
}

bool MediaLibraryUnitTestUtils::GetFileAsset(const int fileId, shared_ptr<FileAsset> &fileAsset)
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MEDIA_ERR_LOG("MediaLibraryDataManager invalid");
        exit(1);
    }
    vector<string> columns;
    DataSharePredicates predicates;
    string selections = MEDIA_DATA_DB_ID + " = " + to_string(fileId);
    predicates.SetWhereClause(selections);
    Uri queryFileUri(MEDIALIBRARY_DATA_URI);
    int errCode = 0;
    MediaLibraryCommand cmd(queryFileUri, Media::OperationType::QUERY);
    auto resultSet = MediaLibraryDataManager::GetInstance()->Query(cmd, columns, predicates, errCode);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("GetFileAsset::resultSet == nullptr");
        return false;
    }
    auto result = make_shared<DataShareResultSet>(resultSet);
    shared_ptr<FetchResult<FileAsset>> fetchFileResult = make_unique<FetchResult<FileAsset>>(move(result));
    if (fetchFileResult->GetCount() <= 0) {
        MEDIA_ERR_LOG("GetFileAsset::GetCount <= 0");
        return false;
    }
    auto firstAsset = fetchFileResult->GetFirstObject();
    fileAsset = move(firstAsset);
    if (fileAsset == nullptr) {
        MEDIA_ERR_LOG("GetFileAsset::fileAsset = nullptr.");
        return false;
    }
    return true;
}

bool MediaLibraryUnitTestUtils::CreateAlbum(string displayName, shared_ptr<FileAsset> parentAlbumAsset,
    shared_ptr<FileAsset> &albumAsset)
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MEDIA_ERR_LOG("MediaLibraryDataManager invalid");
        exit(1);
    }
    Uri createAlbumUri(MEDIALIBRARY_DATA_URI + "/" + MEDIA_ALBUMOPRN + "/" + MEDIA_ALBUMOPRN_CREATEALBUM);
    string dirPath;
    if (parentAlbumAsset == nullptr) {
        dirPath = ROOT_MEDIA_DIR + displayName;
    } else {
        dirPath = parentAlbumAsset->GetPath() + "/" + displayName;
    }
    DataShareValuesBucket valuesBucket;
    valuesBucket.Put(MEDIA_DATA_DB_FILE_PATH, dirPath);
    valuesBucket.Put(MEDIA_DATA_DB_NAME, displayName);
    MediaLibraryCommand cmd(createAlbumUri);
    auto retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    MEDIA_INFO_LOG("CreateAlbum:: %{private}s, retVal: %{public}d", dirPath.c_str(), retVal);
    if (retVal <= 0) {
        MEDIA_ERR_LOG("CreateAlbum::create failed, %{private}s", dirPath.c_str());
        return false;
    }
    if (!GetFileAsset(retVal, albumAsset)) {
        MEDIA_ERR_LOG("CreateAlbum::GetFileAsset failed, %{private}s", dirPath.c_str());
        return false;
    }
    return true;
}

bool MediaLibraryUnitTestUtils::CreateFile(string displayName, shared_ptr<FileAsset> parentAlbumAsset,
    shared_ptr<FileAsset> &fileAsset)
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MEDIA_ERR_LOG("MediaLibraryDataManager invalid");
        exit(1);
    }
    Uri createAssetUri(MEDIALIBRARY_DATA_URI + "/" + Media::MEDIA_FILEOPRN + "/" + Media::MEDIA_FILEOPRN_CREATEASSET);
    DataShareValuesBucket valuesBucket;
    string relativePath = MediaFileUtils::AddDocsToRelativePath(parentAlbumAsset->GetRelativePath() +
        parentAlbumAsset->GetDisplayName() + "/");
    string mimeType = MimeTypeUtils::GetMimeTypeFromExtension(ScannerUtils::GetFileExtension(displayName));
    MediaType mediaType = MimeTypeUtils::GetMediaTypeFromMimeType(mimeType);
    valuesBucket.Put(MEDIA_DATA_DB_MEDIA_TYPE, mediaType);
    valuesBucket.Put(MEDIA_DATA_DB_NAME, displayName);
    valuesBucket.Put(MEDIA_DATA_DB_RELATIVE_PATH, relativePath);
    MediaLibraryCommand cmd(createAssetUri);
    int32_t retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    MEDIA_INFO_LOG("CreateFile:: %{private}s, retVal: %{public}d", (relativePath + displayName).c_str(), retVal);
    if (retVal <= 0) {
        MEDIA_ERR_LOG("CreateFile::create failed, %{private}s", (relativePath + displayName).c_str());
        return false;
    }
    if (!GetFileAsset(retVal, fileAsset)) {
        MEDIA_ERR_LOG("CreateFile::GetFileAsset failed, %{private}s", (relativePath + displayName).c_str());
        return false;
    }
    return true;
}

bool MediaLibraryUnitTestUtils::CreateFileFS(const string &filePath)
{
    bool errCode = false;

    if (filePath.empty()) {
        return errCode;
    }

    ofstream file(filePath);
    if (!file) {
        MEDIA_ERR_LOG("Output file path could not be created");
        return errCode;
    }

    const mode_t CHOWN_RW_UG = 0660;
    if (chmod(filePath.c_str(), CHOWN_RW_UG) == 0) {
        errCode = true;
    }

    file.close();

    return errCode;
}

bool MediaLibraryUnitTestUtils::DeleteDir(const string &path, const string &dirId)
{
    string cmd = "rm -rf " + path;
    system(cmd.c_str());

    auto rdbStore = MediaLibraryDataManager::GetInstance()->rdbStore_;
    NativeRdb::AbsRdbPredicates predicates(MEDIALIBRARY_TABLE);
    predicates.EqualTo(MEDIA_DATA_DB_ID, dirId)->Or()->EqualTo(MEDIA_DATA_DB_PARENT_ID, dirId);
    int32_t deletedRows = -1;
    auto ret = rdbStore->Delete(deletedRows, predicates);
    return ret == 0;
}

void MediaLibraryUnitTestUtils::TrashFile(shared_ptr<FileAsset> &fileAsset)
{
    DataShareValuesBucket valuesBucket;
    valuesBucket.Put(SMARTALBUMMAP_DB_ALBUM_ID, TRASH_ALBUM_ID_VALUES);
    valuesBucket.Put(SMARTALBUMMAP_DB_CHILD_ASSET_ID, fileAsset->GetId());
    string uriString = MEDIALIBRARY_DATA_URI + "/" + MEDIA_SMARTALBUMMAPOPRN + "/" +
        MEDIA_SMARTALBUMMAPOPRN_ADDSMARTALBUM;
    Uri uri(uriString);
    MediaLibraryCommand cmd(uri);
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
}

void MediaLibraryUnitTestUtils::RecoveryFile(shared_ptr<FileAsset> &fileAsset)
{
    DataShareValuesBucket valuesBucket;
    valuesBucket.Put(SMARTALBUMMAP_DB_ALBUM_ID, TRASH_ALBUM_ID_VALUES);
    valuesBucket.Put(SMARTALBUMMAP_DB_CHILD_ASSET_ID, fileAsset->GetId());
    string uriString = MEDIALIBRARY_DATA_URI + "/" + MEDIA_SMARTALBUMMAPOPRN + "/" +
        MEDIA_SMARTALBUMMAPOPRN_REMOVESMARTALBUM;
    Uri uri(uriString);
    MediaLibraryCommand cmd(uri);
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
}

void MediaLibraryUnitTestUtils::WaitForCallback(shared_ptr<TestScannerCallback> callback)
{
    std::mutex mutex;
    std::unique_lock<std::mutex> lock(mutex);
    const int waitSeconds = 10;
    callback->condVar_.wait_until(lock, std::chrono::system_clock::now() + std::chrono::seconds(waitSeconds));
}

int32_t MediaLibraryUnitTestUtils::GrantUriPermission(const int32_t fileId, const string &bundleName,
    const string &mode, const int32_t tableType)
{
    Uri addPermission(MEDIALIBRARY_BUNDLEPERM_URI + "/" + BUNDLE_PERMISSION_INSERT);
    DataShareValuesBucket values;
    values.Put(PERMISSION_FILE_ID, fileId);
    values.Put(PERMISSION_BUNDLE_NAME, bundleName);
    values.Put(PERMISSION_MODE, mode);
    values.Put(PERMISSION_TABLE_TYPE, tableType);
    MediaLibraryCommand cmd(addPermission);
    return MediaLibraryDataManager::GetInstance()->Insert(cmd, values);
}

TestScannerCallback::TestScannerCallback() : status_(-1) {}

int32_t TestScannerCallback::OnScanFinished(const int32_t status, const std::string &uri, const std::string &path)
{
    status_ = status;
    condVar_.notify_all();
    return E_OK;
}
}
}
