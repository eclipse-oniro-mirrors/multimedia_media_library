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

#ifndef OHOS_MEDIA_CLOUD_SYNC_SHARE_ALBUM_CLOUD_SYNC_TEST_UTILS_H
#define OHOS_MEDIA_CLOUD_SYNC_SHARE_ALBUM_CLOUD_SYNC_TEST_UTILS_H

#include <memory>
#include <string>
#include <vector>

#include "album_dao.h"
#include "cloud_media_data_handler.h"
#include "json_file_reader.h"
#include "photos_dao.h"
#include "rdb_store.h"

namespace OHOS::Media::CloudSync {
/**
 * 共享相册云资产上下行 TDD 公共工具.
 *
 * 数据来源(重要):
 * 本套用例全部使用真实抓取的共享相册/共享资产记录(见 resources/cloudsync_datafile/share_album),
 * 仅做了必要的最小改动: 3 条资产的 albumIds 统一指向相册记录的真实 cloudId;
 * 相册删除记录/改 lpath 记录由真实记录派生(各只改 deleted / localPath 一个字段).
 * 隔离约定(重要):
 * 用例结束后既要靠 DatabaseDataMock::Rollback() 按 id 区间回收, 也要靠
 * CleanShareTddData() 按真实 cloud_id / lpath 精确定点清理, 因此不会影响既有用例.
 */
class ShareAlbumCloudSyncTestUtils {
public:  // 场景与库表常量
    static constexpr int32_t SCENE_TYPE_NORMAL = 0;
    static constexpr int32_t SCENE_TYPE_SHARE = 1;
    static constexpr int32_t CLOUD_TYPE = 0;
    static constexpr int32_t USER_ID = 100;
    static constexpr int32_t ALBUM_TYPE_SHARE = 8192;
    static constexpr int32_t ALBUM_SUBTYPE_SHARE_GENERIC = 8193;
    static constexpr int32_t SHARE_TYPE_SHAREALBUM = 2;
    static constexpr int32_t UPLOAD_STATUS_OPEN = 1;
    // DirtyType 枚举值(medialibrary_type_const.h): SYNCED=0, NEW=1, MDIRTY=2, FDIRTY=3, DELETED=4, RETRY=5, SDIRTY=6
    static constexpr int32_t DIRTY_TYPE_NEW = 1;
    static constexpr int32_t DIRTY_TYPE_SYNCED = 0;
    static constexpr int32_t DIRTY_TYPE_SDIRTY = 6;
    static constexpr int32_t IS_SHARED_TRUE = 1;
    // FileSourceTypes::MEDIA_SHARE_ALBUM
    static constexpr int32_t FILE_SOURCE_TYPE_MEDIA_SHARE_ALBUM = 5;
    // 取值非法时的占位值(make_optional 的 value_or 默认值)
    static constexpr int32_t INVALID_INDEX = -1;
    // 上行通道要求的资产就绪状态: 缩略图已就绪 + LCD 已访问
    static constexpr int32_t THUMBNAIL_READY_FOR_UPLOAD = 3;
    static constexpr int32_t LCD_VISIT_TIME_FOR_UPLOAD = 2;
    // 模拟云侧下发异常错误码
    static constexpr int32_t MOCK_PULL_ERROR_CODE = -1;

public:  // 用例数据常量(全部取自真实抓取的共享相册/共享资产记录)
    static const std::string SHARE_ALBUM_CLOUD_ID;
    static const std::string SHARE_ALBUM_LPATH;
    static const std::string SHARE_ALBUM_NAME;
    static const std::string SHARE_ALBUM_OWNER;
    static const std::string SHARE_PHOTO_CLOUD_ID;
    static const std::string SHARE_PHOTO_DISPLAY_NAME;
    // 本地文件路径(attributes.data)中的文件名, 与展示名不同(生产对账按本地路径取名)
    static const std::string SHARE_PHOTO_DATA_FILE_NAME;
    static const std::string SHARE_PHOTO_OWNER_INFO;
    // 待上行用例使用的共享资产(真实记录第 2 条)
    static const std::string SHARE_PHOTO_UPLINK_CLOUD_ID;
    // 元数据上报与自动归属用例使用的共享资产(真实记录第 3 条, 初始不落库)
    static const std::string SHARE_PHOTO_META_CLOUD_ID;
    // 云侧下发的新相册名与派生迁移路径, 与 share_album_update*.json 保持一致
    static const std::string SHARE_ALBUM_UPDATED_NAME;
    static const std::string SHARE_ALBUM_MOVED_LPATH;
    // 必须为 TYPE_MDIRTY=2(3 是 TYPE_FDIRTY), 否则生产 PullUpdate/PullDelete 的脏保护分支不会生效
    static constexpr int32_t DIRTY_TYPE_MDIRTY = 2;
    static constexpr int64_t SHARE_DATE_DAY = 20260924;
    static constexpr int64_t SHARE_GROUP = 1790221829533;

public:  // 下行数据文件
    static const std::string SHARE_ALBUM_JSON;
    static const std::string SHARE_ALBUM_UPDATE_JSON;
    static const std::string SHARE_ALBUM_UPDATE_LPATH_JSON;
    static const std::string SHARE_ALBUM_DELETE_JSON;
    static const std::string SHARE_PHOTO_JSON;
    static const std::string SHARE_PHOTO_DELETE_JSON;
    static const std::string SHARE_PHOTO_META_JSON;
    // 下行共享相册(基础/SDIRTY/SYNCED 三个), 结果通过 stats 返回
    static int32_t DownLinkShareAlbums(std::vector<int32_t> &stats);
    // 下行指定 json 的共享相册记录(更新/删除等不同场景), 结果通过 stats 返回
    static int32_t DownLinkShareAlbumRecords(const std::string &jsonPath, std::vector<int32_t> &stats);
    // 通过 dentry 接口把共享资产写入媒体库(这条通路才真正落库, 与既有用例约定一致)
    static int32_t InsertSharePhotosByDentry();
    // 通过 dentry 接口下行指定 json 的共享资产(单个 json 只含一条资产时便于验证单条行为)
    static int32_t InsertSharePhotosByDentryFile(const std::string &jsonPath);
    // 通过 OnFetchRecords 走共享资产元数据通路, 结果通过 newData 返回
    static int32_t DownLinkSharePhotoMeta(std::vector<CloudMetaData> &newData, std::vector<int32_t> &stats);
    // 下行"云侧已删除"的共享资产, 用于验证删除通路
    static int32_t DownLinkDeletedSharePhoto(std::vector<int32_t> &stats);

public:  // handler 构造
    // 必须用 4 参构造, 否则 sceneType 默认 0 不会进入共享分支
    static std::shared_ptr<CloudMediaDataHandler> MakeHandler(const std::string &tableName, int32_t sceneType);
    static std::shared_ptr<CloudMediaDataHandler> MakeShareAlbumHandler();
    static std::shared_ptr<CloudMediaDataHandler> MakeSharePhotoHandler();

public:  // 数据查询
    static bool GetAlbumByCloudId(const std::string &cloudId, OHOS::Media::ORM::PhotoAlbumPo &album);
    static bool GetAlbumById(int32_t albumId, OHOS::Media::ORM::PhotoAlbumPo &album);
    static bool GetPhotoByCloudId(const std::string &cloudId, OHOS::Media::ORM::PhotosPo &photo);
    static int32_t GetAlbumIdByCloudId(const std::string &cloudId);
    // 按 cloudId 统计相册条数, 用于验证下行不会产生重复相册
    static int32_t CountAlbumsByCloudId(const std::string &cloudId);
    // 按相册内共享资产个数, 用于等待异步清理完成
    static int32_t CountShareAssetsOfAlbum(int32_t albumId);

public:  // 数据准备与清理
    static void UpdateAlbumDirty(const std::string &cloudId, int32_t dirty);
    // 显式建立"共享资产 -> 共享相册"的归属关系.
    // 生产侧 dentry 下行的相册归属依赖 CloudMediaPhotosDao 的进程内相册映射缓存,
    // 缓存里若已存在同 lpath 的源相册(历史运行残留), 资产会被归到那个相册而不是共享相册,
    // 导致相册维度的行为无法确定复现; 这里显式构造归属, 与既有用例直接构造库表状态的约定一致.
    static int32_t BindSharePhotosToShareAlbums();
    // 把共享资产置为待上行状态.
    // 元数据上行(dirty=SDIRTY)不要求物理文件; 文件新建上行(dirty=TYPE_NEW)还会要求真实原图与 THM/LCD 缩略图
    static void PrepareSharePhotoForUpload(const std::string &cloudId, int32_t dirty = DIRTY_TYPE_SDIRTY);
    // 按真实 cloud_id / lpath 精确定点清理本套用例涉及的共享相册与共享资产, 兜底避免污染既有用例
    static void CleanShareTddData();

public:  // 异步流程等待
    // 共享资产删除由 MediaAsyncWorker 异步执行, 必须轮询等待; 超时返回 false
    static bool WaitForShareAssetsRemoved(int32_t albumId);

private:
    static std::shared_ptr<NativeRdb::RdbStore> GetRdbStore();
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_SHARE_ALBUM_CLOUD_SYNC_TEST_UTILS_H
