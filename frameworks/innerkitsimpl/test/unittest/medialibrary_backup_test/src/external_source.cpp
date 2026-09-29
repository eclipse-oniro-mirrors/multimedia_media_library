/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "external_source.h"

namespace OHOS {
namespace Media {
using namespace std;

// create external files simply
const string ExternalOpenCall::CREATE_EXTERNAL_FILES = string("CREATE TABLE IF NOT EXISTS files ") +
    "(_id INTEGER PRIMARY KEY AUTOINCREMENT, _data TEXT COLLATE NOCASE, _size INTEGER, date_added INTEGER," +
    "date_modified INTEGER, title TEXT, _display_name TEXT, orientation INTEGER, duration INTEGER," +
    "bucket_id TEXT, media_type INTEGER, storage_id INTEGER, width INTEGER, height INTEGER, " +
    "is_pending INTEGER, is_favorite INTEGER, datetaken INTEGER, special_file_type INTEGER DEFAULT 0);";

int ExternalOpenCall::OnCreate(NativeRdb::RdbStore &store)
{
    return store.ExecuteSql(CREATE_EXTERNAL_FILES);
}

int ExternalOpenCall::OnUpgrade(NativeRdb::RdbStore &store, int oldVersion, int newVersion)
{
    return 0;
}

void ExternalSource::Init(const string &dbPath)
{
    NativeRdb::RdbStoreConfig config(dbPath);
    ExternalOpenCall helper;
    int errCode = 0;
    shared_ptr<NativeRdb::RdbStore> store = NativeRdb::RdbHelper::GetRdbStore(config, 1, helper, errCode);
    externalStorePtr_ = store;
    InitStepOne();
    InitStepTwo();
    InitStepThree();
    InitStepFour();
}

void ExternalSource::InitStepOne()
{
    externalStorePtr_->ExecuteSql(string("INSERT INTO files VALUES(1, ") +
        "'/storage/emulated/0//BaiduMap/cache/fake_baidu.jpg', 2160867, 1706950419, " +
        "1546937461, 'fake_baidu', 'fake_baidu.jpg', 0, NULL, 2080527857, 1, 65537, 3968, 2976, 0, 0, 0, 0)");
    externalStorePtr_->ExecuteSql(string("INSERT INTO files VALUES(2, ") +
        "'/storage/emulated/0/Pictures/album1/album1.jpg', 3091817, 1706950426, 1546937461, 'album1', 'album1.jpg', " +
        "0, NULL, -1266858078, 1, 65537, 3968, 2976, 0, 0, 0, 0)");
    externalStorePtr_->ExecuteSql(string("INSERT INTO files VALUES(3,") +
        "'/storage/emulated/0/Pictures/album2/album2.jpg', 3621508, 1706950426, 1546937461, 'album2', 'album2.jpg', " +
        "0, NULL, -1266858077, 1, 65537, 3968, 2976, 0, 0, 0, 0)");
    externalStorePtr_->ExecuteSql(string("INSERT INTO files VALUES(4,") +
        "'/storage/emulated/0/Pictures/favorite.jpg', 7440437, 1706950426, 1546937461, 'favorite', 'favorite.jpg', " +
        "0, NULL, -1617409521, 1, 65537, 5120, 3840, 0, 1, 0, 0)");
    externalStorePtr_->ExecuteSql(string("INSERT INTO files VALUES(5,") +
        "'/storage/emulated/0/Pictures/no_exif.png', 234265, 1706950426, 1541215543, 'no_exif', 'no_exif.png', " +
        "0, NULL, -1617409521, 1, 65537, 1024, 768, 0, 0, 0, 0, 0)");
    externalStorePtr_->ExecuteSql(string("INSERT INTO files VALUES(6,") +
        "'/storage/emulated/0/Pictures/orientation.jpg', 2781577, 1706950426, 1706950486, 'orientation', " +
        "'orientation.jpg', 270, NULL, -1617409521, 1, 65537, 3264, 2448, 0, 0, 0, 0)");
    externalStorePtr_->ExecuteSql(string("INSERT INTO files VALUES(7,") +
        "'/storage/emulated/0/Pictures/user_common.jpg', 4330676, 1706950426, 1706950476, 'user_common', " +
        "'user_common.jpg', 0, NULL, -1617409521, 1, 65537, 3968, 2976, 0, 0, 0, 0)");
    externalStorePtr_->ExecuteSql(string("INSERT INTO files VALUES(8,") +
        "'/storage/emulated/0/Pictures/clone.jpg', 5431183, 1706950426, 1546937461, 'clone', 'clone.jpg', " +
        "0, NULL, -1617409521, 1, 65537, 5120, 3840, 0, 0, 0, 0)");
    externalStorePtr_->ExecuteSql(string("INSERT INTO files VALUES(9,") +
        "'/storage/emulated/0/Movies/common.mp4', 192844, 1706950426, 1698033985, 'common', 'common.mp4', " +
        "0, 6042, -1730634595, 3, 65537, 320, 240, 0, 0, 0, 0)");
    externalStorePtr_->ExecuteSql(string("INSERT INTO files VALUES(10,") +
        "'/storage/emulated/0/Movies/4K.mp4', 26162975, 1706950426, 1703905398, '4K', '4K.mp4', " +
        "90, 5099, -1730634595, 3, 65537, 3840, 2160, 0, 0, 0, 0)");
}

void ExternalSource::InitStepTwo()
{
    externalStorePtr_->ExecuteSql(string("INSERT INTO files VALUES(11,") +
        "'/storage/emulated/0/Movies/1080p.mp4', 6626209, 1706950426, 1703905397, '1080p', '1080p.mp4', " +
        "90, 4551, -1730634595, 3, 65537, 1920, 1080, 0, 0, 0, 0)");
    externalStorePtr_->ExecuteSql(string("INSERT INTO files VALUES(12,") +
        "'/storage/emulated/0/DCIM/Camera/camera_sync.jpg', 2808831, 1706950426, 1546937461, 'camera_sync', " +
        "'camera_sync.jpg', 0, NULL, -1739773001, 1, 65537, 3968, 2976, 0, 0, 0, 0)");
    externalStorePtr_->ExecuteSql(string("INSERT INTO files VALUES(13,") +
        "'/storage/emulated/0/DCIM/Sgame/fake_game.jpg', 3871481, 1706950426, 1546937461, 'fake_game', " +
        "'fake_game.jpg', 0, NULL, -1288103917, 1, 65537, 3968, 2976, 0, 0, 0, 0)");
    externalStorePtr_->ExecuteSql(string("INSERT INTO files VALUES(14,") +
        "'/storage/emulated/0/CTRIP/avatar/fake_garbage_ctrip.jpg', 3184187, 1706950426, 1546937461, " +
        "'fake_garbage_ctrip', 'fake_garbage_ctrip.jpg', 0, NULL, 876554266, 1, 65537, 3968, 2976, 0, 0, 0, 0)");
    externalStorePtr_->ExecuteSql(string("INSERT INTO files VALUES(15,") +
        "'/storage/emulated/0/tencent/MicroMsg/WeiXin/fake_wechat.jpg', 2419880, 1706950426, 1432973383, " +
        "'fake_wechat', 'fake_wechat.jpg', 0, NULL, -924335728, 1, 65537, 3968, 2976, 0, 0, 0, 0)");
    externalStorePtr_->ExecuteSql(string("INSERT INTO files VALUES(21,") +
        "'/storage/emulated/0/DCIM/Camera/camera_not_sync.jpg', 1780813, 1706950561, 1546937461, 'camera_not_sync', " +
        "'camera_not_sync.jpg', 0, NULL, -1739773001, 1, 65537, 3264, 2448, 0, 0, 0, 0)");
    externalStorePtr_->ExecuteSql(string("INSERT INTO files VALUES(22,") +
        "'/storage/emulated/0/CTRIP/avatar/not_sync_garbage_ctrip.jpg', 2767543, 1706950561, 1546937461, " +
        "'not_sync_garbage_ctrip', 'not_sync_garbage_ctrip.jpg', 0, NULL, 876554266, 1, 65537, 3264, 2448, " +
        "0, 0, 0, 0)");
    externalStorePtr_->ExecuteSql(string("INSERT INTO files VALUES(23,") +
        "'/storage/emulated/0/tencent/MicroMsg/WeiXin/not_sync_weixin.jpg', 3503265, 1706950561, 1546937461, " +
        "'not_sync_weixin', 'not_sync_weixin.jpg', 0, NULL, -924335728, 1, 65537, 3968, 2976, 0, 0, 0, 0)");
}

void ExternalSource::InitStepThree()
{
    externalStorePtr_->ExecuteSql(string("INSERT INTO files VALUES(16,") +
        "'/storage/emulated/0/A/media/Rocket/test/a_media_normal_video.mp4', 10865209, 1708600079, 1708600079," +
        "'a_media_normal_video', 'a_media_normal_video.mp4', 0, 52221, -1122816831, 3, 65537, 352, 640, 0, 0, 0, 0)");
    externalStorePtr_->ExecuteSql(string("INSERT INTO files VALUES(17,") +
        "'/storage/emulated/0/A/media/Rocket/test/a_media_favorite.mp4', 10865209, 1708600079, 1708600079," +
        "'a_media_favorite', 'a_media_favorite.mp4', 0, 52221, -1122816831, 3, 65537, 352, 640, 0, 1, 0, 0)");
    externalStorePtr_->ExecuteSql(string("INSERT INTO files VALUES(18,") +
        "'/storage/emulated/0/A/media/Baidu/a_media_favorite_baidu.mp4', 10865209, 1708600079, 1708600079," +
        "'a_media_favorite_baidu', 'a_media_favorite_baidu.mp4', 0, 52221, -1122816831, 3, 65537, 352, 640," +
        "0, 1, 0, 0)");
    externalStorePtr_->ExecuteSql(string("INSERT INTO files VALUES(19,") +
        "'/storage/emulated/0/A/media/Baidu/a_media_normal_image.jpg', 418436, 1708602162, 1708602162," +
        "'a_media_normal_image', 'a_media_normal_image.jpg', 0, 0, -1960413976, 1, 65537, 2000, 3556, 0, 0, 0, 0)");
    externalStorePtr_->ExecuteSql(string("INSERT INTO files VALUES(20,") +
        "'/storage/emulated/0/A/media/Baidu/a_media_zero_size.jpg', 0, 1708602162, 1708602162," +
        "'a_media_zero_size', 'a_media_zero_size.jpg', 0, 0, -1960413976, 1, 65537, 0, 0, 0, 0, 0, 0)");
}

void ExternalSource::InitStepFour()
{
    externalStorePtr_->ExecuteSql(string("INSERT INTO files VALUES(24,") +
        "'/storage/emulated/0/Music/audio1.mp3', 10865209, 1708600079, 1708600079," +
        "'a_media_normal_video', 'audio1.mp3', 0, 52221, -1122816831, 2, 65537, 352, 640, 0, 0, 0, 0)");
}
} // namespace Media
} // namespace OHOS