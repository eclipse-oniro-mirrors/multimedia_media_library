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

#ifndef MEDIALIBRARY_UNITTEST_UTILS_H
#define MEDIALIBRARY_UNITTEST_UTILS_H

#include <condition_variable>

#include "imedia_scanner_callback.h"
#include "file_asset.h"
#include "rdb_open_callback.h"
#include "rdb_store_config.h"
#include "medialibrary_rdbstore.h"

namespace OHOS {
namespace Media {
static const inline std::string TEST_CAMERA = "Camera";
static const inline std::string TEST_VIDEOS = "Videos";
static const inline std::string TEST_PICTURES = "Pictures";
static const inline std::string TEST_AUDIOS = "Audios";
static const inline std::string TEST_DOCUMENTS = "Docs/Documents";
static const inline std::string TEST_DOWNLOAD = "Docs/Download";
static const inline std::vector<std::string> TEST_ROOT_DIRS = { TEST_CAMERA, TEST_VIDEOS, TEST_PICTURES, TEST_AUDIOS,
    TEST_DOCUMENTS, TEST_DOWNLOAD };

class TestScannerCallback : public IMediaScannerCallback {
public:
    TestScannerCallback();
    ~TestScannerCallback() = default;

    int32_t status_;
    std::condition_variable condVar_;
    int32_t OnScanFinished(const int32_t status, const std::string &uri, const std::string &path) override;
};

class MediaLibraryUnitTestUtils {
public:
    MediaLibraryUnitTestUtils() {}
    virtual ~MediaLibraryUnitTestUtils() {}
    static bool IsValid();
    static void Init();
    static int32_t InitUnistore();
    static int32_t InitUnistore(const NativeRdb::RdbStoreConfig &config, int version,
        NativeRdb::RdbOpenCallback &openCallback);
    static void StopUnistore();
    static void InitRootDirs();
    static void CleanTestFiles();
    static void CleanBundlePermission();
    static std::shared_ptr<FileAsset> GetRootAsset(const std::string &dir);
    static bool IsFileExists(const std::string filePath);
    static bool GetFileAsset(const int fileId, std::shared_ptr<FileAsset> &fileAsset);
    static bool CreateAlbum(std::string displayName, std::shared_ptr<FileAsset> parentAlbumAsset,
        std::shared_ptr<FileAsset> &albumAsset);
    static bool CreateFile(std::string displayName, std::shared_ptr<FileAsset> parentAlbumAsset,
        std::shared_ptr<FileAsset> &fileAsset);
    static bool CreateFileFS(const std::string& filePath);
    static bool DeleteDir(const std::string &path, const std::string &dirId);
    static void TrashFile(std::shared_ptr<FileAsset> &fileAsset);
    static void RecoveryFile(std::shared_ptr<FileAsset> &fileAsset);
    static void WaitForCallback(std::shared_ptr<TestScannerCallback> callback);
    static int32_t GrantUriPermission(const int32_t fileId, const std::string &bundleName,
        const std::string &mode, const int32_t tableType);
    static bool writeBytesToFile(size_t numBytes, const char* path, size_t& resultFileSize);
    static void ClearTable(const std::string &table);
    static bool CreateTestTables(const std::shared_ptr<MediaLibraryRdbStore>& rdbStore,
        const std::vector<std::string>& tables);
    static bool CleanTestTables(const std::shared_ptr<MediaLibraryRdbStore>& rdbStore,
        const std::vector<std::string>& tables, bool dropTable = false);
    static bool CreateBasicTables(const std::shared_ptr<MediaLibraryRdbStore>& rdbStore);
    static std::mutex Mutex_;
private:
    static inline bool isValid_ = false;
    static inline std::unordered_map<std::string, std::shared_ptr<FileAsset>> rootDirAssetMap_;
};
}
}

#endif // MEDIALIBRARY_UNITTEST_UTILS_H