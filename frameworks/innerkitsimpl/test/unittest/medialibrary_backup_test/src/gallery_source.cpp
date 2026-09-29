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

#include "gallery_source.h"

#include "backup_const_map.h"

namespace OHOS {
namespace Media {
using namespace std;

const string GalleryOpenCall::CREATE_GALLERY_MEDIA = string("CREATE TABLE IF NOT EXISTS gallery_media ") +
    " (_id INTEGER PRIMARY KEY AUTOINCREMENT, local_media_id INTEGER, _data TEXT COLLATE NOCASE," +
    "_size INTEGER,  date_added INTEGER, date_modified INTEGER, title TEXT, description TEXT, _display_name TEXT, " +
    "orientation INTEGER, bucket_id TEXT, duration INTEGER, media_type INTEGER, storage_id INTEGER, width INTEGER, " +
    "height INTEGER, is_hw_favorite INTEGER,  relative_bucket_id TEXT, showDateToken INTEGER, recycleFlag INTEGER, " +
    "recycledTime INTEGER, sourcePath TEXT, is_hw_burst INTEGER DEFAULT 0, hash TEXT, " +
    "special_file_type INTEGER DEFAULT 0, first_update_time INTEGER, datetaken INTEGER, detail_time TEXT, " +
    "localThumbPath TEXT, localBigThumbPath TEXT, thumbType INTEGER DEFAULT 0, uniqueId TEXT, hdc_unique_id TEXT, " +
    "latitude DOUBLE, longitude DOUBLE, story_id TEXT, portrait_id TEXT, albumId TEXT);";

const string GalleryOpenCall::CREATE_GARBAGE_ALBUM = string("CREATE TABLE IF NOT EXISTS garbage_album") +
    "(app_name TEXT, cache_dir TEXT, nick_name TEXT, nick_dir TEXT, type INTEGER, relative_bucket_id TEXT);";

const string GalleryOpenCall::CREATE_GALLERY_ALBUM = string("CREATE TABLE IF NOT EXISTS gallery_album ") +
    "(albumId TEXT PRIMARY KEY NOT NULL, albumName TEXT, relativeBucketId TEXT, createTime INTEGER, lPath TEXT, " +
    "source TEXT,photoNum INTEGER DEFAULT 0, totalSize INTEGER DEFAULT 0, garbage INTEGER DEFAULT 0, " +
    "dirty INTEGER DEFAULT 0, hide INTEGER DEFAULT 0, emptyShow INTEGER DEFAULT 0, sortIndex INTEGER DEFAULT 0, " +
    "uploadStatus INTEGER DEFAULT 0,cloud INTEGER DEFAULT 1,sideStatus INTEGER DEFAULT 0, timeStamp INTEGER, " +
    "hdcUploadStatus INTEGER, hdc INTEGER DEFAULT 1);";

const string GalleryOpenCall::CREATE_GALLERY_MERGE_TAG = string("CREATE TABLE IF NOT EXISTS merge_tag ") +
    "(tag_id TEXT, group_tag TEXT, tag_name TEXT, user_operation INTEGER, rename_operation TEXT);";

const string GalleryOpenCall::CREATE_GALLERY_MERGE_FACE = string("CREATE TABLE IF NOT EXISTS merge_face ") +
    "(hash TEXT, face_id TEXT, tag_id TEXT NOT NULL, scale_x REAL NOT NULL, scale_y REAL NOT NULL, " +
    "scale_width REAL NOT NULL, scale_height REAL NOT NULL, landmarks BLOB, prob REAL, " +
    "yaw REAL NOT NULL, pitch REAL NOT NULL, roll REAL NOT NULL, total_face INTEGER NOT NULL);";

const string GalleryOpenCall::CREATE_GALLERY_FACE = string("CREATE TABLE IF NOT EXISTS face ") +
    "(hash TEXT, face_id TEXT, tag_id TEXT NOT NULL, scale_x REAL NOT NULL, scale_y REAL NOT NULL, " +
    "scale_width REAL NOT NULL, scale_height REAL NOT NULL, landmarks BLOB, prob REAL, " +
    "yaw REAL NOT NULL, pitch REAL NOT NULL, roll REAL NOT NULL, total_face INTEGER NOT NULL);";

const string GalleryOpenCall::CREATE_T_STORY_ALBUM = string("CREATE TABLE IF NOT EXISTS t_story_album ") +
    "(story_id TEXT, date TEXT, name TEXT, min_datetaken INTEGER, max_datetaken INTEGER, project_id TEXT, " +
    "cover_id INTEGER, album_type INTEGER, album_scene INTEGER, typeface_name TEXT, remarks TEXT, " +
    "generatedtime INTEGER, tv_video_path TEXT, cluster_type TEXT NOT NULL, cluster_sub_type TEXT NOT NULL, " +
    "cluster_condition TEXT NOT NULL, story_version TEXT NOT NULL, displayable INTEGER NOT NULL, " +
    "insert_pic_count INTEGER, remove_pic_count INTEGER, share_screenshot_count INTEGER, share_cover_count INTEGER, " +
    "rename_count INTEGER, change_cover_count INTEGER);";

const string GalleryOpenCall::CREATE_T_STORY_ALBUM_SUGGESTION =
    string("CREATE TABLE IF NOT EXISTS t_story_album_suggestion") +
    "(story_id TEXT, suggest_name TEXT, suggest_start_time INTEGER, suggest_end_time INTEGER, cluster_type TEXT);";

const string GalleryOpenCall::CREATE_T_VIDEO_SEMANTIC_ANALYSIS =
    string("CREATE TABLE IF NOT EXISTS t_video_semantic_analysis") +
    "(hash TEXT, category_id INTEGER, confidence_probability REAL, sub_category INTEGER, sub_confidence_prob REAL, " +
    "sub_label INTEGER, sub_label_prob REAL, sub_label_type INTEGER, tracks TEXT);";

int GalleryOpenCall::OnCreate(NativeRdb::RdbStore &store)
{
    int ret = 0;
    ret += store.ExecuteSql(CREATE_GALLERY_MEDIA);
    ret += store.ExecuteSql(CREATE_GARBAGE_ALBUM);
    ret += store.ExecuteSql(CREATE_GALLERY_ALBUM);
    ret += store.ExecuteSql(CREATE_GALLERY_MERGE_TAG);
    ret += store.ExecuteSql(CREATE_GALLERY_MERGE_FACE);
    ret += store.ExecuteSql(CREATE_GALLERY_FACE);
    ret += store.ExecuteSql(CREATE_T_STORY_ALBUM);
    ret += store.ExecuteSql(CREATE_T_STORY_ALBUM_SUGGESTION);
    ret += store.ExecuteSql(CREATE_T_VIDEO_SEMANTIC_ANALYSIS);
    return ret;
}

int GalleryOpenCall::OnUpgrade(NativeRdb::RdbStore &store, int oldVersion, int newVersion)
{
    return 0;
}

void GallerySource::Init(const string &dbPath)
{
    NativeRdb::RdbStoreConfig config(dbPath);
    GalleryOpenCall helper;
    int errCode = 0;
    shared_ptr<NativeRdb::RdbStore> store = NativeRdb::RdbHelper::GetRdbStore(config, 1, helper, errCode);
    galleryStorePtr_ = store;
    InitGalleryMediaOne();
    InitGalleryMediaTwo();
    InitGalleryMediaThree();
    InitGalleryMediaFour();
    InitGalleryMediaFive();
    InitGalleryMediaSix();
    InitGarbageAlbum();
    InitGalleryAlbumOne();
    InitGalleryAlbumTwo();
    InitGalleryMergeTag();
    InitGalleryMergeFace();
    InitGalleryFace();
    InitTStoryAlbum();
    InitTStoryAlbumSuggestion();
    InitTVideoSemanticAnalysis();
}

void GallerySource::InitGalleryMediaOne()
{
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_media VALUES(1, 1, ") +
        "'/storage/emulated/0//BaiduMap/cache/fake_baidu.jpg', 2160867, 1706950419, 1546937461," +
        "'fake_baidu', NULL, 'fake_baidu.jpg', 0, 2080527857, 0, 1, 65537, 3968, 2976, NULL, " +
        "-1492241466, 1495954569032, 0, 0, '/storage/emulated/0//BaiduMap/cache/fake_baidu.jpg', 0, NULL, 0, "+
        "1495970415377, 1495970415377, '2024:09:06 17:00:00', NULL, NULL, 0, NULL, NULL, " +
        "0, 0, NULL, NULL, NULL)");
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_media VALUES(2, 16, ") +
        "'/storage/emulated/0/DCIM/Camera/camera_sync.jpg', 2808831, 1706950426, 1546937461," +
        "'camera_sync', NULL, 'camera_sync.jpg', 0, -1739773001, 0, 1, 65537, 3968, 2976, NULL, " +
        "2064266562, 1496025408734, 0, 0, '/storage/emulated/0/DCIM/Camera/camera_sync.jpg', 0, NULL, 0, "+
        "1495970415377, 1495970415377, '2024:09:06 17:00:00', NULL, NULL, " +
        "0, 0, NULL, NULL, NULL)");
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_media VALUES(3, 2, ") +
        "'/storage/emulated/0/Pictures/album1/album1.jpg', 3091817, 1706950426, 1546937461," +
        "'album1', NULL, 'album1.jpg', 0, -1266858078, 0, 1, 65537, 3968, 2976, NULL, " +
        "1999934381, 1495970415377, 0, 0, '/storage/emulated/0/Pictures/album1/album1.jpg', 0, NULL, 0, "+
        "1495970415377, 1495970415377, '2024:09:06 17:00:00', NULL, NULL, " +
        "0, 0, NULL, NULL, NULL)");
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_media VALUES(4, 3, ") +
        "'/storage/emulated/0/Pictures/album2/album2.jpg', 3621508, 1706950426, 1546937461," +
        "'album2', NULL, 'album2.jpg', 0, -1266858077, 0, 1, 65537, 3968, 2976, NULL, " +
        "1999934382, 1495973952757, 0, 0, '/storage/emulated/0/Pictures/album2/album2.jpg', 0, NULL, 0, "+
        "1495970415377, 1495970415377, '2024:09:06 17:00:00', NULL, NULL, " +
        "0, 0, NULL, NULL, NULL)");
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_media VALUES(5, 4, ") +
        "'/storage/emulated/0/Pictures/favorite.jpg', 7440437, 1706950426, 1546937461," +
        "'favorite', NULL, 'favorite.jpg', 0, -1617409521, 0, 1, 65537, 5120, 3840, 1, " +
        "218866788, 1495957457427, 0, 0, '/storage/emulated/0/Pictures/favorite.jpg', 0, NULL, 0, "+
        "1495970415377, 1495970415377, '2024:09:06 17:00:00', NULL, NULL, " +
        "0, 0, NULL, NULL, NULL)");
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_media VALUES(6, -4, ") +
        "'/storage/emulated/0/Pictures/hiddenAlbum/bins/0/" +
        "GF6DA7BRGAYXYMJXGA3DSNJQGQ4TEMBTGF6GQ2LEMRSW47BONJYGO7DJNV', 2716337, 1706950426, " +
        "1546937461, 'hidden', NULL, 'hidden.jpg', 0, -1617409521, 0, 1, 65537, 3968, 2976, NULL, " +
        "659449306, 1495961420646, 0, 0, 'GF6DA7BRGAYXYMJXGA3DSNJQGQ4TEMBTGF6GQ2LEMRSW47BONJYGO7DJNV', 0, NULL, 0, "+
        "1495970415377, 1495970415377, '2024:09:06 17:00:00', NULL, NULL, " +
        "0, 0, NULL, NULL, NULL)");
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_media VALUES(7, 5, ") +
        "'/storage/emulated/0/Pictures/no_exif.png', 234265, 1706950426, 1541215543," +
        "'no_exif', NULL, 'no_exif.png', 0, -1617409521, 0, 1, 65537, 1024, 768, NULL, " +
        "218866788, 1541215543000, 0, 0, '/storage/emulated/0/Pictures/no_exif.png', 0, NULL, 0, "+
        "1495970415377, 1495970415377, '2024:09:06 17:00:00', NULL, NULL, " +
        "0, 0, NULL, NULL, NULL)");
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_media VALUES(8, 6, ") +
        "'/storage/emulated/0/Pictures/orientation.jpg', 2781577, 1706950426, 1706950486," +
        "'orientation', NULL, 'orientation.jpg', 270, -1617409521, 0, 1, 65537, 3264, 2448, NULL, " +
        "218866788, 1495962070277, 0, 0, '/storage/emulated/0/Pictures/orientation.jpg', 0, NULL, 0, "+
        "1495970415377, 1495970415377, '2024:09:06 17:00:00', NULL, NULL, " +
        "0, 0, NULL, NULL, NULL)");
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_media VALUES(9, 0, ") +
        "'/storage/emulated/0/Pictures/.Gallery2/recycle/bins/0/\
        GF6DA7BRGA2HYMJXGA3DSNJQGQ4DENJXGN6HI4TBONUGKZD', 2454477, 1706950426, " +
        "1546937461, 'trashed', NULL, 'trashed.jpg', 0, -1617409521, 0, 1, 65537, 3968, 2976, NULL, " +
        "218866788, 1495959683996, 2, 1706950482573, 'GF6DA7BRGA2HYMJXGA3DSNJQGQ4DENJXGN6HI4TBONUGKZD', 0, NULL, 0, " +
        "1495970415377, 1495970415377, '2024:09:06 17:00:00', NULL, NULL, " +
        "0, 0, NULL, NULL, NULL)");
}

void GallerySource::InitGalleryMediaTwo()
{
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_media VALUES(10, 7, ") +
        "'/storage/emulated/0/Pictures/user_common.jpg', 4330676, 1706950426, 1706950476," +
        "'user_common', 'user_comment', 'user_common.jpg', 0, -1617409521, 0, 1, 65537, 3968, 2976, NULL, " +
        "218866788, 1495962762035, 0, 0, '/storage/emulated/0/Pictures/user_common.jpg', 0, NULL, 0, "+
        "1495970415377, 1495970415377, '2024:09:06 17:00:00', NULL, NULL, " +
        "0, 0, NULL, NULL, NULL)");
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_media VALUES(11, -3, ") +
        "'/storage/emulated/0/Pictures/clone.jpg', 5431183, 1706950426, 1546937461," +
        "'clone', NULL, 'clone.jpg', 0, -1617409521, 0, 1, 65537, 5120, 3840, NULL, " +
        "218866788, 1496056464221, 0, 0, '/storage/emulated/0/Pictures/clone.jpg', 0, NULL, 0, "+
        "1495970415377, 1495970415377, '2024:09:06 17:00:00', NULL, NULL, " +
        "0, 0, NULL, NULL, NULL)");
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_media VALUES(15, 13, ") +
        "'/storage/emulated/0/DCIM/Sgame/fake_game.jpg', 3871481, 1706950426, 1546937461," +
        "'fake_game', NULL, 'fake_game.jpg', 0, -1288103917, 0, 1, 65537, 3968, 2976, NULL, " +
        "-1858129624, 1495970384215, 0, 0, '/storage/emulated/0/DCIM/Sgame/fake_game.jpg', 0, NULL, 0, "+
        "1495970415377, 1495970415377, '2024:09:06 17:00:00', NULL, NULL, " +
        "0, 0, NULL, NULL, NULL)");
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_media VALUES(16, 14, ") +
        "'/storage/emulated/0/CTRIP/avatar/fake_garbage_ctrip.jpg', 3184187, 1706950426, 1546937461," +
        "'fake_garbage_ctrip', NULL, 'fake_garbage_ctrip.jpg', 0, 876554266, 0, 1, 65537, 3968, 2976, NULL, " +
        "-1457303569, 1495969289098, 0, 0, '/storage/emulated/0/CTRIP/avatar/fake_garbage_ctrip.jpg', 0, NULL, 0, "+
        "1495970415377, 1495970415377, '2024:09:06 17:00:00', NULL, NULL, " +
        "0, 0, NULL, NULL, NULL)");
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_media VALUES(17, 15, ") +
        "'/storage/emulated/0/tencent/MicroMsg/WeiXin/fake_wechat.jpg', 2419880, 1706950426, 1432973383," +
        "'fake_wechat', NULL, 'fake_wechat.jpg', 0, -924335728, 0, 1, 65537, 3968, 2976, NULL, " +
        "-1803300197, 1432973383179, 0, 0, '/storage/emulated/0/tencent/MicroMsg/WeiXin/fake_wechat.jpg', " +
        "0, NULL, 0, 1495970415378, 1495970415379, '2024:09:06 17:00:01', NULL, NULL, " +
        "0, 0, NULL, NULL, NULL)");
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_media VALUES(12, 9, ") +
        "'/storage/emulated/0/Movies/common.mp4', 192844, 1706950426, 1698033985," +
        "'common', NULL, 'common.mp4', 0, -1730634595, 6042, 3, 65537, 320, 240, NULL, " +
        "1989707826, 1240601085000, 0, 0, '/storage/emulated/0/Movies/common.mp4', 0, NULL, 0, " +
        "1495970415377, 1495970415377, '2024:09:06 17:00:00', NULL, NULL, " +
        "0, 0, NULL, NULL, NULL)");
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_media VALUES(13, 10, ") +
        "'/storage/emulated/0/Movies/4K.mp4', 26162975, 1706950426, 1703905398," +
        "'4K', NULL, '4K.mp4', 0, -1730634595, 5099, 3, 65537, 3840, 2160, NULL, " +
        "1989707826, 1703905326000, 0, 0, '/storage/emulated/0/Movies/4K.mp4', 0, NULL, 0, " +
        "1495970415377, 1495970415377, '2024:09:06 17:00:00', NULL, NULL, " +
        "0, 0, NULL, NULL, NULL)");
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_media VALUES(14, 11, ") +
        "'/storage/emulated/0/Movies/1080p.mp4', 6626209, 1706950426, 1703905397," +
        "'1080p', NULL, '1080p.mp4', 0, -1730634595, 4551, 3, 65537, 1920, 1080, NULL, " +
        "1989707826, 1703905305000, 0, 0, '/storage/emulated/0/Movies/1080p.mp4', 0, NULL, 0, " +
        "1495970415377, 1495970415377, '2024:09:06 17:00:00', NULL, NULL, " +
        "0, 0, NULL, NULL, NULL)");
}

void GallerySource::InitGalleryMediaThree()
{
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_media VALUES(18, 16, ") +
        "'/storage/emulated/0/A/media/Rocket/test/a_media_normal_video.mp4', 10865209, 1708600079, 1708600079," +
        "'a_media_normal_video', 'NULL', 'a_media_normal_video.mp4', 0, -1122816831, 52221, 3, 65537, 352, 640, " +
        "NULL, 1264692236, 1708600079000, 0, 0, '/storage/emulated/0/A/media/Rocket/test/a_media_normal_video.mp4', " +
        "0, NULL, 0, 1495970415377, 1495970415377, '2024:09:06 17:00:00', NULL, NULL, " +
        "0, 0, NULL, NULL, NULL)");
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_media VALUES(19, 17, ") +
        "'/storage/emulated/0/A/media/Rocket/test/a_media_favorite.mp4', 10865209, 1708600079, 1708600079," +
        "'a_media_favorite', 'NULL', 'a_media_favorite.mp4', 0, -1122816831, 52221, 3, 65537, 352, 640, 1, " +
        "1264692236, 1708600079000, 0, 0, '/storage/emulated/0/A/media/Rocket/test/a_media_favorite.mp4', 0, NULL, " +
        "0, 1495970415377, 1495970415377, '2024:09:06 17:00:00', NULL, NULL, " +
        "0, 0, NULL, NULL, NULL)");
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_media VALUES(20, 18, ") +
        "'/storage/emulated/0/A/media/Baidu/a_media_favorite_baidu.mp4', 10865209, 1708600079, 1708600079," +
        "'a_media_favorite_baidu', 'NULL', 'a_media_favorite_baidu.mp4', 0, -1122816831, 52221, 3, 65537, 352, 640," +
        "1, 1264692236, 1708600079000, 0, 0, '/storage/emulated/0/A/media/Baidu/a_media_favorite_baidu.mp4', " +
        "0, NULL, 0, 1495970415377, 1495970415377, '2024:09:06 17:00:00', NULL, NULL, " +
        "0, 0, NULL, NULL, NULL)");
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_media VALUES(21, 19, ") +
        "'/storage/emulated/0/A/media/Baidu/a_media_normal_image.jpg', 418436, 1708602162000, 1708602162," +
        "'a_media_normal_image', 'NULL', 'a_media_normal_image.jpg', 0, -1960413976, 0, 1, 65537, 2000, 3556, NULL, " +
        "1264692236, 1708602162000, 0, 0, '/storage/emulated/0/A/media/Baidu/a_media_normal_image.jpg', 0, NULL, 0, " +
        "1495970415377, 1495970415377, '2024:09:06 17:00:00', NULL, NULL, " +
        "0, 0, NULL, NULL, NULL)");
    galleryStorePtr_->ExecuteSql("INSERT INTO gallery_media VALUES(22, 20, \
        '/storage/emulated/0/A/media/Baidu/a_media_normal_image1.jpg', \
        418436, 1708602162000, 1708602162,'a_media_normal_image1', 'NULL', \
        'a_media_normal_image1.jpg', 0, -1960413976, 0, 1, 65537, \
        2000, 3556, NULL, 2, 1708602162000, 0, 0, '/storage/emulated/0/A/media/Baidu/a_media_normal_image1.jpg', \
        0, NULL, 0, 1495970415377, 1495970415377, '2024:09:06 17:00:00', NULL, NULL, \
        0, 0, NULL, NULL, NULL)");
    galleryStorePtr_->ExecuteSql("INSERT INTO gallery_media VALUES(23, 21, \
        '/storage/emulated/0/A/media/Baidu/a_media_normal_video1.mp4', \
        418436, 1708602162000, 1708602162,'a_media_normal_video1', 'NULL', \
        'a_media_normal_video1.mp4', 0, -1960413976, 0, 1, 65537, \
        2000, 3556, NULL, 3, 1708602162000, 0, 0, '/storage/emulated/0/A/media/Baidu/a_media_normal_video1.mp4', \
        0, NULL, 0, 1495970415377, 1495970415377, '2024:09:06 17:00:00', NULL, NULL, \
        0, 0, NULL, NULL, NULL)");
    galleryStorePtr_->ExecuteSql("INSERT INTO gallery_media VALUES(24, 22, '/DCIM/Camera/camera1.jpg', \
        418436, 1708602162000, 1708602162,'camera1', 'NULL', 'camera1.jpg', 0, -1960413976, 0, 1, 65537, \
        2000, 3556, NULL, 3, 1708602162000, 0, 0, '/DCIM/Camera/camera1.jpg', 0, NULL, 0, \
        1495970415377, 1495970415377, '2024:09:06 17:00:00', NULL, NULL, \
        0, 0, NULL, NULL, NULL)");
    galleryStorePtr_->ExecuteSql("INSERT INTO gallery_media VALUES(25, 23, '/Screenshots/screenshots1.mp4', \
        418436, 1708602162000, 1708602162,'screenshots1', 'NULL', 'screenshots1.mp4', 0, -1960413976, 0, 1, 65537, \
        2000, 3556, NULL, 3, 1708602162000, 0, 0, '/Screenshots/screenshots1.mp4', 0, NULL, 0, \
        1495970415377, 1495970415377, '2024:09:06 17:00:00', NULL, NULL, \
        0, 0, NULL, NULL, NULL)");
    galleryStorePtr_->ExecuteSql("INSERT INTO gallery_media VALUES(26, 24, '/Screenrecorder/screenrecorder1.jpg', \
        418436, 1708602162000, 1708602162,'screenrecorder1', 'NULL', 'screenrecorder1.jpg', \
        0, -1960413976, 0, 1, 65537, 2000, 3556, NULL, 3, 1708602162000, 0, 0, '/Screenrecorder/screenrecorder1.jpg', \
        0, NULL, 0, 1495970415377, 1495970415377, '2024:09:06 17:00:00', NULL, NULL, \
        0, 0, NULL, NULL, NULL)");
}

void GallerySource::InitGalleryMediaFour()
{
    galleryStorePtr_->ExecuteSql("INSERT INTO gallery_media VALUES(27, 25, '/" +
        GetDUALBundleName() + " Share/" + GetDUALBundleName() + "Share1.jpg', 418436, 1708602162000, 1708602162, '" +
        GetDUALBundleName() + "Share1', 'NULL', '" + GetDUALBundleName() +
        "Share1.jpg', 0, -1960413976, 0, 1, 65537, 2000, 3556, NULL, 3, 1708602162000, 0, 0, 'Share1.jpg', " +
        "0, NULL, 0, 1495970415377, 1495970415377, '2024:09:06 17:00:00', NULL, NULL, " +
        "0, 0, NULL, NULL, NULL)");
    galleryStorePtr_->ExecuteSql("INSERT INTO gallery_media VALUES(28, 26, '/DCIM/Camera/camera2.jpg', \
        418436, 1708602162000, 1708602162,'camera2', 'NULL', 'camera2.jpg', 0, -1960413976, 0, 1, 65537, \
        2000, 3556, NULL, 3, 1708602162000, 0, 0, '/DCIM/Camera/camera2.jpg', 0, NULL, 0, \
        1495970415377, 1495970415377, '2024:09:06 17:00:00', NULL, NULL, \
        0, 0, NULL, NULL, NULL)");
    galleryStorePtr_->ExecuteSql("INSERT INTO gallery_media VALUES(29, 27, '/Screenshots/screenshots2.jpg', \
        418436, 1708602162000, 1708602162,'screenshots2', 'NULL', 'screenshots2.jpg', 0, -1960413976, 0, 1, 65537, \
        2000, 3556, NULL, 3, 1708602162000, 0, 0, '/Screenshots/screenshots2.jpg', 0, NULL, 0, \
        1495970415377, 1495970415377, '2024:09:06 17:00:00', NULL, NULL, \
        0, 0, NULL, NULL, NULL)");
    galleryStorePtr_->ExecuteSql("INSERT INTO gallery_media VALUES(30, 28, '/Screenrecorder/screenrecorder2.mp4', \
        418436, 1708602162000, 1708602162,'screenrecorder2', 'NULL', 'screenrecorder2.mp4', 0, \
        -1960413976, 0, 1, 65537, 2000, 3556, NULL, 3, 1708602162000, 0, 0, '/Screenrecorder/screenrecorder2.mp4', \
        0, NULL, 0, 1495970415377, 1495970415377, '2024:09:06 17:00:00', NULL, NULL, \
        0, 0, NULL, NULL, NULL)");
    galleryStorePtr_->ExecuteSql("INSERT INTO gallery_media VALUES(31, 29, '/" + GetDUALBundleName() + " Share/" +
        GetDUALBundleName() + "Share2.jpg', 418436, 1708602162000, 1708602162,'" + GetDUALBundleName() +
        "Share2', 'NULL', '" + GetDUALBundleName() + "Share2.jpg', 0, -1960413976, 0, 1, 65537, \
        2000, 3556, NULL, 3, 1708602162000, 0, 0, 'Share2.jpg', 0, NULL, 0, \
        1495970415377, 1495970415377, '2024:09:06 17:00:00', NULL, NULL, \
        0, 0, NULL, NULL, NULL)");
    // for portrait restoration
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_media VALUES(32, 55950, ") +
        "'/storage/emulated/0/Pictures/NULL/NULL_1.jpg', 216446, 1722851511, 1717550867," +
        "'NULL_1', NULL, 'NULL_1.jpg', 0, 991881755, 0, 1, 65537, 1860, 1200, NULL, " +
        "500954022, 1643958981000, 0, 0, '/storage/emulated/0/Pictures/NULL/NULL_1.jpg', 0, '5659', 0, " +
        "1495970415377, 1495970415377, '2024:09:06 17:00:00', NULL, NULL, " +
        "0, 0, NULL, NULL, NULL)");
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_media VALUES(33, 55979, ") +
        "'/storage/emulated/0/Pictures/B/B_1.jpg', 4567535, 1722913178, 1715679284," +
        "'B_1', NULL, 'B_1.jpg', 0, -1608790897, 0, 1, 65537, 5184, 3456, NULL, " +
        "-586242982, 1434362029810, 0, 0, '/storage/emulated/0/Pictures/B/B_1.jpg', 0, '1c05', 0, " +
        "1495970415377, 1495970415377, '2024:09:06 17:00:00', NULL, NULL, " +
        "0, 0, NULL, NULL, NULL)");
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_media VALUES(34, 57139, ") +
        "'/storage/emulated/0/Pictures/B/B_2.jpg', 4369271, 1723044589, 1720179455," +
        "'B_2', NULL, 'B_2.jpg', 0, -1035207676, 0, 1, 65537, 5184, 3456, NULL, " +
        "-370356529, 1434363463590, 0, 0, '/storage/emulated/0/Pictures/B/B_2.jpg', 0, '23f2', 0, " +
        "1495970415377, 1495970415377, '2024:09:06 17:00:00', NULL, NULL, " +
        "0, 0, NULL, NULL, NULL)");
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_media VALUES(35, 57063, ") +
        "'/storage/emulated/0/Pictures/Y/Y_1.jpg', 2971707, 1723044589, 1720190486," +
        "'Y_1', NULL, 'Y_1.jpg', 0, -1035207678, 0, 1, 65537, 3968, 2976, NULL, " +
        "-370356531, 1491269475663, 0, 0, '/storage/emulated/0/Pictures/Y/Y_1.jpg', 0, '3275', 0, " +
        "1495970415377, 1495970415377, '2024:09:06 17:00:00', NULL, NULL, " +
        "0, 0, NULL, NULL, NULL)");
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_media VALUES(36, 57064, ") +
        "'/storage/emulated/0/Pictures/A/A_1.jpg', 2971707, 1723044589, 1720190486," +
        "'A_1', NULL, 'A_1.jpg', 0, -1035207678, 0, 1, 65537, 3968, 2976, NULL, " +
        "-370356531, 1491269475663, 0, 0, '/storage/emulated/0/Pictures/A/A_1.jpg', 0, '3280', 0, " +
        "1495970415377, 1495970415377, '2024:09:06 17:00:00', NULL, NULL, " +
        "0, 0, NULL, NULL, NULL)");
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_media VALUES(37, 57065, ") +
        "'/storage/emulated/0/Pictures/A/A_2.jpg', 2971707, 1723044589, 1720190486," +
        "'A_2', NULL, 'A_2.jpg', 0, -1035207678, 0, 1, 65537, 3968, 2976, NULL, " +
        "-370356531, 1491269475663, 0, 0, '/storage/emulated/0/Pictures/A/A_2.jpg', 0, '3281', 0, " +
        "1495970415377, 1495970415377, '2024:09:06 17:00:00', NULL, NULL, " +
        "0, 0, NULL, NULL, NULL)");
}

void GallerySource::InitGalleryMediaFive()
{
    // duplicate data
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_media VALUES(36, 30, ") +
        "'/storage/emulated/0/A/media/Rocket/test/duplicate_data.mp4', 10865209, 1708600079, 1708600079," +
        "'duplicate_data', 'NULL', 'duplicate_data.mp4', 0, -1122816831, 52221, 3, 65537, 352, 640, " +
        "NULL, 1264692236, 1708600079000, 0, 0, '/storage/emulated/0/A/media/Rocket/test/duplicate_data.mp4', " +
        "0, NULL, 0, 1495970415377, 1495970415377, '2024:09:06 17:00:00', NULL, NULL, " +
        "0, 0, NULL, NULL, NULL)");
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_media VALUES(37, 31, ") +
        "'/storage/emulated/0/A/media/Rocket/test/duplicate_data.mp4', 10865209, 1708600079, 1708600079," +
        "'duplicate_data', 'NULL', 'duplicate_data.mp4', 0, -1122816831, 52221, 3, 65537, 352, 640, " +
        "NULL, 1264692236, 1708600079000, 0, 0, '/storage/emulated/0/A/media/Rocket/test/duplicate_data.mp4', " +
        "0, NULL, 0, 1495970415377, 1495970415377, '2024:09:06 17:00:00', NULL, NULL, " +
        "0, 0, NULL, NULL, NULL)");
}

void GallerySource::InitGalleryMediaSix()
{
    // duplicate data with case differences
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_media VALUES(38, 32, ") +
        "'/storage/emulated/0/A/media/Rocket/test/DUPLICATE_DATA_CASE.mp4', 10865209, 1708600079, 1708600079," +
        "'DUPLICATE_DATA_CASE', 'NULL', 'DUPLICATE_DATA_CASE.mp4', 0, -1122816831, 52221, 3, 65537, 352, 640, " +
        "NULL, 1264692236, 1708600079000, 0, 0, '/storage/emulated/0/A/media/Rocket/test/DUPLICATE_DATA_CASE.mp4', " +
        "0, NULL, 0, 1495970415377, 1495970415377, '2025:04:16 17:00:00', NULL, NULL, " +
        "0, 0, NULL, NULL, NULL)");
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_media VALUES(39, 33, ") +
        "'/storage/emulated/0/A/media/Rocket/test/duplicate_data_case.mp4', 10865209, 1708600079, 1708600079," +
        "'duplicate_data_case', 'NULL', 'duplicate_data_case.mp4', 0, -1122816831, 52221, 3, 65537, 352, 640, " +
        "NULL, 1264692236, 1708600079000, 0, 0, '/storage/emulated/0/A/media/Rocket/test/duplicate_data_case.mp4', " +
        "0, NULL, 0, 1495970415377, 1495970415377, '2025:04:16 17:00:00', NULL, NULL, " +
        "0, 0, NULL, NULL, NULL)");

    // NULL storage_id
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_media VALUES(40, 34, ") +
        "'/storage/emulated/0/A/media/Rocket/test/null_storage_id.mp4', 10865209, 1708600079, 1708600079," +
        "'null_storage_id', 'NULL', 'null_storage_id.mp4', 0, -1122816831, 52221, 3, NULL, 352, 640, " +
        "NULL, 1264692236, 1708600079000, 0, 0, '/storage/emulated/0/A/media/Rocket/test/null_storage_id.mp4', " +
        "0, NULL, 0, 1495970415377, 1495970415377, '2025:04:16 17:00:00', NULL, NULL, " +
        "0, 0, NULL, NULL, NULL)");
}

void GallerySource::InitGarbageAlbum()
{
    galleryStorePtr_->ExecuteSql(string("INSERT INTO garbage_album VALUES('baidu', '/BaiduMap/cache', ") +
        "null, null, 1, 1151084355);");
    galleryStorePtr_->ExecuteSql(string("INSERT INTO garbage_album VALUES('ctrip', '/CTRIP/avatar', ") +
        "null, null, 1, -1457303569);");
    galleryStorePtr_->ExecuteSql(string("INSERT INTO garbage_album VALUES(null, null, 'wechat', ") +
        "'/tencent/MicroMsg/WeiXin', 0, null);");
    galleryStorePtr_->ExecuteSql(string("INSERT INTO garbage_album VALUES(null, null, 'sgame', ") +
        "'/DCIM/Sgame', 0, null);");
    galleryStorePtr_->ExecuteSql("INSERT INTO garbage_album (nick_name, nick_dir) \
        VALUES ('天猫', '/TmallPic');");
}

void GallerySource::InitGalleryAlbumOne()
{
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_album (albumId, albumName,relativeBucketId, lPath) \
        VALUES ('test001', 'test001', '1', '/test001');"));
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_album (albumId, albumName,relativeBucketId, lPath) \
        VALUES ('test002', 'test002', '2', '/test002');"));
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_album (albumId, albumName,relativeBucketId, lPath) \
        VALUES ('test003', 'test003', '3', '/test003');"));
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_album (albumId, albumName,relativeBucketId, lPath) \
        VALUES ('test004', 'TmallPic', '4', '/TmallPic');"));
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_album (albumId, albumName,relativeBucketId, lPath) \
        VALUES ('test005', 'UCDownloads',  '5', '/UCDownloads');"));
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_album (albumId, albumName,relativeBucketId, lPath) \
        VALUES ('test006', 'xiaohongshu',  '6', '/xiaohongshu');"));
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_album (albumId, albumName,relativeBucketId, lPath) \
        VALUES ('test007', 'Douyin', '7', '/Douyin');"));
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_album (albumId, albumName,relativeBucketId, lPath) \
        VALUES ('test008', 'Weibo', '8', '/sina/weibo/save');"));
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_album (albumId, albumName,relativeBucketId, lPath) \
        VALUES ('test009', 'Camera', '9', '/DCIM/Camera');"));
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_album (albumId, albumName,relativeBucketId, lPath) \
        VALUES ('test010', 'Screenshots', '10', '/Screenshots');"));
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_album (albumId, albumName,relativeBucketId, lPath) \
        VALUES ('test011', 'Screenrecorder', '11', '/Screenrecorder');"));
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_album (albumId, albumName,relativeBucketId, lPath) \
        VALUES ('test012', '" + GetDUALBundleName() + " Share', '12', '/" + GetDUALBundleName() + " Share');"));
}

void GallerySource::InitGalleryAlbumTwo()
{
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_album (albumId, albumName,relativeBucketId, lPath) \
        VALUES ('test101', 'test101', '101', '/test101');"));
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_album (albumId, albumName,relativeBucketId, lPath) \
        VALUES ('test102', 'TmallPic', '102', '/TmallPic');"));
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_album (albumId, albumName,relativeBucketId, lPath) \
        VALUES ('test103', 'MTTT', '103', '/MTTT');"));
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_album (albumId, albumName,relativeBucketId, lPath) \
        VALUES ('test104', 'funnygallery', '104', '/funnygallery');"));
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_album (albumId, albumName,relativeBucketId, lPath) \
        VALUES ('test105', 'xiaohongshu', '105', '/xiaohongshu');"));
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_album (albumId, albumName,relativeBucketId, lPath) \
        VALUES ('test106', 'Douyin', '106', '/Douyin');"));
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_album (albumId, albumName,relativeBucketId, lPath) \
        VALUES ('test107', 'save', '107', '/sdcard/Pictures/sina');"));
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_album (albumId, albumName,relativeBucketId, lPath) \
        VALUES ('test108a', 'Weibo', '108a', '/sina/weibo/save');"));
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_album (albumId, albumName,relativeBucketId, lPath) \
        VALUES ('test108b', 'Weibo', '108b', '/sina/weibo/weibo');"));
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_album (albumId, albumName,relativeBucketId, lPath) \
        VALUES ('test108c', 'Weibo', '108c', '/sina/weibo/storage/photoalbum_save/weibo');"));
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_album (albumId, albumName,relativeBucketId, lPath) \
        VALUES ('test109', 'Camera', '109', '/DCIM/Camera');"));
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_album (albumId, albumName,relativeBucketId, lPath) \
        VALUES ('test110', 'Screenshots', '110', '/Screenshots');"));
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_album (albumId, albumName,relativeBucketId, lPath) \
        VALUES ('test111', 'Screenrecorder', '111', '/Screenrecorder');"));
    galleryStorePtr_->ExecuteSql(string("INSERT INTO gallery_album (albumId, albumName,relativeBucketId, lPath) \
        VALUES ('test112', '" + GetDUALBundleName() + " Share', '112', '/" + GetDUALBundleName() + " Share');"));
}

void GallerySource::InitGalleryMergeTag()
{
    galleryStorePtr_->ExecuteSql(string("INSERT INTO merge_tag (tag_id, group_tag, user_operation, \
        rename_operation) VALUES ('ser_1660318467974', 'ser_1660318467974', 0, 0)")); // without tag_name
    galleryStorePtr_->ExecuteSql(string("INSERT INTO merge_tag (tag_id, group_tag, tag_name, user_operation, \
        rename_operation) VALUES ('ser_1660239135682', 'ser_1660239135682', 'B', 0, 1)")); // 'B'
    galleryStorePtr_->ExecuteSql(string("INSERT INTO merge_tag (tag_id, group_tag, tag_name, user_operation, \
        rename_operation) VALUES ('ser_274602026436444', 'ser_274602026436444', 'Y', 1, 0)")); // 'Y', groupped
    galleryStorePtr_->ExecuteSql(string("INSERT INTO merge_tag (tag_id, group_tag, tag_name, user_operation, \
        rename_operation) VALUES ('ser_274602037407069', 'ser_274602026436444', 'Y', 0, 1)")); // 'Y', groupped
    galleryStorePtr_->ExecuteSql(string("INSERT INTO merge_tag (tag_id, group_tag, tag_name, user_operation, \
        rename_operation) VALUES ('ser_4970706812782', 'ser_4970706812782', 'A1', 0, 1)"));
    galleryStorePtr_->ExecuteSql(string("INSERT INTO merge_tag (tag_id, group_tag, tag_name, user_operation, \
        rename_operation) VALUES ('ser_4970715412782', 'ser_4970715412782', 'A2', 0, 1)"));
    galleryStorePtr_->ExecuteSql(string("INSERT INTO merge_tag (tag_id, group_tag, tag_name, user_operation, \
        rename_operation) VALUES ('ser_4970706812782|ser_4970715412782', 'ser_4970706812782|ser_4970715412782', \
        'A', 0, 1)"));
}

void GallerySource::InitGalleryMergeFace()
{
    galleryStorePtr_->ExecuteSql(string("INSERT INTO merge_face (hash, face_id, tag_id, scale_x, scale_y, \
        scale_width, scale_height, landmarks, prob, yaw, pitch, roll, total_face) VALUES ('5659', '0', \
        'ser_1660318467974', 0.6097, 0.5799, 0.0796, 0.1299, \
        x'95040000e6020000df040000e2020000ba040000fb0200009e04000026030000db04000022030000', NULL, 0.5064, 0.7402, \
        0.1916, 1)")); // without tag_name
    galleryStorePtr_->ExecuteSql(string("INSERT INTO merge_face (hash, face_id, tag_id, scale_x, scale_y, \
        scale_width, scale_height, landmarks, prob, yaw, pitch, roll, total_face) VALUES ('1c05', '0', \
        'ser_1660239135682', 0.5449, 0.2419, 0.1722, 0.3022, \
        x'0906000034020000d40600005b02000065060000c6020000dd050000f4020000a806000020030000', NULL, -5.5875, -7.4554, \
        -0.3196, 1)")); // 'B'
    galleryStorePtr_->ExecuteSql(string("INSERT INTO merge_face (hash, face_id, tag_id, scale_x, scale_y, \
        scale_width, scale_height, landmarks, prob, yaw, pitch, roll, total_face) VALUES ('23f2', '0', \
        'ser_1660239135682', 0.3797, 0.3199, 0.2172, 0.4048, \
        x'7f0400001f0300006b0500004b030000d6040000be0300005a0400001d0400001f05000047040000', NULL, -0.6532, -5.2244, \
        3.9188, 1)")); // 'B'
    galleryStorePtr_->ExecuteSql(string("INSERT INTO merge_face (hash, face_id, tag_id, scale_x, scale_y, \
        scale_width, scale_height, landmarks, prob, yaw, pitch, roll, total_face) VALUES ('3275', '0', \
        'ser_274602037407069', 0.5696, 0.4046, 0.0449, 0.0645, \
        x'7c04000079020000ab04000074020000920400008b02000085040000a7020000ab040000a3020000', NULL, 1.9596, -1.3064, \
        2.6128, 2)")); // 'Y'
    galleryStorePtr_->ExecuteSql(string("INSERT INTO merge_face (hash, face_id, tag_id, scale_x, scale_y, \
        scale_width, scale_height, landmarks, prob, yaw, pitch, roll, total_face) VALUES ('3275', '1', \
        'ser_274602026436444', 0.4325, 0.4294, 0.0473, 0.0746, \
        x'71030000a70200009e030000a502000086030000bc02000076030000d40200009b030000d3020000', NULL, 1.9596, -1.9596, \
        3.9188, 2)")); // 'Y'
    galleryStorePtr_->ExecuteSql(string("INSERT INTO merge_face (hash, face_id, tag_id, scale_x, scale_y, \
        scale_width, scale_height, landmarks, prob, yaw, pitch, roll, total_face) VALUES ('3280', '0', \
        'ser_4970706812782', 0.5696, 0.4046, 0.0449, 0.0645, \
        x'7c04000079020000ab04000074020000920400008b02000085040000a7020000ab040000a3020000', NULL, 1.9596, -1.3064, \
        2.6128, 2)"));
    galleryStorePtr_->ExecuteSql(string("INSERT INTO merge_face (hash, face_id, tag_id, scale_x, scale_y, \
        scale_width, scale_height, landmarks, prob, yaw, pitch, roll, total_face) VALUES ('3281', '1', \
        'ser_4970715412782', 0.4325, 0.4294, 0.0473, 0.0746, \
        x'71030000a70200009e030000a502000086030000bc02000076030000d40200009b030000d3020000', NULL, 1.9596, -1.9596, \
        3.9188, 2)"));
}

void GallerySource::InitGalleryFace()
{
    galleryStorePtr_->ExecuteSql(string("INSERT INTO face (hash, face_id, tag_id, scale_x, scale_y, \
        scale_width, scale_height, landmarks, prob, yaw, pitch, roll, total_face) VALUES ('5659', '0', \
        'ser_1660318467974', 0.6097, 0.5799, 0.0796, 0.1299, \
        x'95040000e6020000df040000e2020000ba040000fb0200009e04000026030000db04000022030000', 0, 0.5064, 0.7402, \
        0.1916, 1)")); // without tag_name
    galleryStorePtr_->ExecuteSql(string("INSERT INTO face (hash, face_id, tag_id, scale_x, scale_y, \
        scale_width, scale_height, landmarks, prob, yaw, pitch, roll, total_face) VALUES ('1c05', '0', \
        'ser_1660239135682', 0.5449, 0.2419, 0.1722, 0.3022, \
        x'0906000034020000d40600005b02000065060000c6020000dd050000f4020000a806000020030000', 0, -5.5875, -7.4554, \
        -0.3196, 1)")); // 'B'
    galleryStorePtr_->ExecuteSql(string("INSERT INTO face (hash, face_id, tag_id, scale_x, scale_y, \
        scale_width, scale_height, landmarks, prob, yaw, pitch, roll, total_face) VALUES ('23f2', '0', \
        'ser_1660239135682', 0.3797, 0.3199, 0.2172, 0.4048, \
        x'7f0400001f0300006b0500004b030000d6040000be0300005a0400001d0400001f05000047040000', 0, -0.6532, -5.2244, \
        3.9188, 1)")); // 'B'
    galleryStorePtr_->ExecuteSql(string("INSERT INTO face (hash, face_id, tag_id, scale_x, scale_y, \
        scale_width, scale_height, landmarks, prob, yaw, pitch, roll, total_face) VALUES ('3275', '0', \
        'ser_274602037407069', 0.5696, 0.4046, 0.0449, 0.0645, \
        x'7c04000079020000ab04000074020000920400008b02000085040000a7020000ab040000a3020000', 0, 1.9596, -1.3064, \
        2.6128, 2)")); // 'Y'
    galleryStorePtr_->ExecuteSql(string("INSERT INTO face (hash, face_id, tag_id, scale_x, scale_y, \
        scale_width, scale_height, landmarks, prob, yaw, pitch, roll, total_face) VALUES ('3275', '1', \
        'ser_274602026436444', 0.4325, 0.4294, 0.0473, 0.0746, \
        x'71030000a70200009e030000a502000086030000bc02000076030000d40200009b030000d3020000', 0, 1.9596, -1.9596, \
        3.9188, 2)")); // 'Y'
    galleryStorePtr_->ExecuteSql(string("INSERT INTO face (hash, face_id, tag_id, scale_x, scale_y, \
        scale_width, scale_height, landmarks, prob, yaw, pitch, roll, total_face) VALUES ('3280', '0', \
        'ser_4970706812782', 0.5696, 0.4046, 0.0449, 0.0645, \
        x'7c04000079020000ab04000074020000920400008b02000085040000a7020000ab040000a3020000', 0, 1.9596, -1.3064, \
        2.6128, 2)"));
    galleryStorePtr_->ExecuteSql(string("INSERT INTO face (hash, face_id, tag_id, scale_x, scale_y, \
        scale_width, scale_height, landmarks, prob, yaw, pitch, roll, total_face) VALUES ('3281', '1', \
        'ser_4970715412782', 0.4325, 0.4294, 0.0473, 0.0746, \
        x'71030000a70200009e030000a502000086030000bc02000076030000d40200009b030000d3020000', 0, 1.9596, -1.9596, \
        3.9188, 2)"));
}

void GallerySource::InitTStoryAlbum()
{
    galleryStorePtr_->ExecuteSql(string("INSERT INTO t_story_album (story_id, date, name, min_datetaken, \
        max_datetaken, project_id, cover_id, album_type, album_scene, typeface_name, remarks, generatedtime, \
        tv_video_path, cluster_type, cluster_sub_type, cluster_condition, story_version, displayable, \
        insert_pic_count, remove_pic_count, share_screenshot_count, share_cover_count, rename_count, \
        change_cover_count) VALUES ('1', 'date', 'pic', 1, 2, '123', 123, 321, 0, 'typ_name', 'marks', 10, 'e12', \
        'clu_type', 'clu_sub_type', 'clu_con', 'version', 1, 2, 3, 4, 5, 6, 7)"));
}

void GallerySource::InitTStoryAlbumSuggestion()
{
    galleryStorePtr_->ExecuteSql(string("INSERT INTO t_story_album_suggestion (story_id, suggest_name, \
        suggest_start_time, suggest_end_time, cluster_type) VALUES ('1', 'sug_name', 1, 2, 'clu_type')"));
}

void GallerySource::InitTVideoSemanticAnalysis()
{
    galleryStorePtr_->ExecuteSql(string("INSERT INTO t_video_semantic_analysis (hash, category_id, \
        confidence_probability, sub_category, sub_confidence_prob, sub_label, sub_label_prob, sub_label_type, \
        tracks) VALUES ('3275', 1, 9827, 103, 9827, 153, 9827, 0, 'beginFrame')"));
}
} // namespace Media
} // namespace OHOS