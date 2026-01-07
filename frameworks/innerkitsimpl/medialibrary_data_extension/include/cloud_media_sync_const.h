/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_SYNC_CONST_H
#define OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_SYNC_CONST_H

#include <string>
#include <map>

#include "media_column.h"
#include "photo_album_column.h"

namespace OHOS::Media::CloudSync {
const std::string FILE_ATTRIBUTES = "attributes";
const std::string FILE_FIX_VERSION = "fix_version";
const std::string FILE_LOCAL_ID = "local_id";
constexpr int MILLISECOND_TO_SECOND = 1000;
constexpr int BATCH_LIMIT_SIZE = 500;
const std::string FILE_PROPERTIES = "properties";
const uint32_t NO_ORIENTATION = 0;
const std::string THUMB_SUFFIX = "THM";
const std::string FILE_THUMBNAIL = "thumbnail";
const std::string FILE_LCD = "lcdThumbnail";
const std::string FILE_RAW = "raw";
const std::string FILE_EDIT_DATA = "editData";
const std::string FILE_EDIT_DATA_CAMERA = "editDataCamera";
const std::string FILE_CONTENT = "content";
const std::string FILE_NAME = "fileName";
const std::string FILE_SIZE = "size";
const std::string FILE_ROTATION = "rotate";
const std::string FILE_TYPE = "fileType";
const std::string FILE_SOURCE_PATH = "sourcePath";
const std::string LCD_SUFFIX = "LCD";
const std::string THUMB_EX_SUFFIX = "THM_EX/THM";
const std::string LCD_EX_SUFFIX = "THM_EX/LCD";
const std::string TMP_SUFFIX = ".temp.download";
const std::string SCREENSHOT_ALBUM_PATH = "/storage/emulated/0/Pictures/Screenshots/";
const std::string DEFAULT_HIDE_ALBUM_CLOUDID = "default-album-4";
const std::string DEFAULT_SCREENSHOT_LPATH_EN = "/Pictures/Screenshots";
const std::string SCREEN_SHOT_AND_RECORDER_EN = ".Screenshots";
const std::string DEFAULT_SCREENSHOT_CLOUDID = "default-album-2";
const std::string DEFAULT_SCREENRECORDS_LPATH = "/Pictures/Screenrecords";
const std::string PHOTO_CLOUD_PATH_URI = "/storage/cloud/files/";
const std::string PHOTO_MEDIA_PATH_URI = "/storage/media/local/files/";
const std::string ALBUM_PATH = "localPath";
const std::string ALBUM_PROPERTIES = "properties";
const std::string ALBUM_NAME = "albumName";

const int32_t ORIENTATION_NORMAL = 1;
const int32_t ORIENTATION_ROTATE_90 = 6;
const int32_t ORIENTATION_ROTATE_180 = 3;
const int32_t ORIENTATION_ROTATE_270 = 8;

const std::string MEDIA_ALBUM_NAME_EN = "album_name_en";
const std::string MEDIA_DUAL_ALBUM_NAME = "dual_album_name";
const std::string MEDIA_ALBUM_PRIORITY = "priority";

const int32_t ROTATE_ANGLE_0 = 0;
const int32_t ROTATE_ANGLE_90 = 90;
const int32_t ROTATE_ANGLE_180 = 180;
const int32_t ROTATE_ANGLE_270 = 270;

const std::map<int32_t, int32_t> FILE_ROTATIONS = {
    {ORIENTATION_NORMAL, ROTATE_ANGLE_0},
    {ORIENTATION_ROTATE_90, ROTATE_ANGLE_90},
    {ORIENTATION_ROTATE_180, ROTATE_ANGLE_180},
    {ORIENTATION_ROTATE_270, ROTATE_ANGLE_270},
};

/* hash*/
const int32_t HASH_VLAUE = 31;

/* file type */
enum {
    FILE_TYPE_IMAGE = 1,
    FILE_TYPE_VIDEO = 4,
    FILE_TYPE_LIVEPHOTO = 9,
};

enum class DataType : int32_t {
    INT,
    LONG,
    DOUBLE,
    STRING,
};

enum {
    DEFAULT_VALUE = -1,
    TRUE_VALUE = 1,
    FALSE_VALUE = 0,
    CACHE_VIDEO_NUM = 100,
    // ROTATE_ANGLE_0 = 0,
    TO_MILLISECONDS = 1000,
    LIMIT_SIZE = 1000,
};
enum CloudOperationType {
    /* upload */
    FILE_CREATE,
    FILE_DELETE,
    FILE_METADATA_MODIFY,
    FILE_DATA_MODIFY,
    FILE_COPY,
    /* download */
    FILE_DOWNLOAD,
    /*clean*/
    FILE_CLEAN
};

enum CloudAlbumOperationType {
    /* upload */
    PHOTO_ALBUM_CREATE,
    PHOTO_ALBUM_METADATA_MODIF,
    PHOTO_ALBUM_DELETE
};

enum AlbumType : int32_t {
    NORMAL,
    SHARE,
    SOURCE = 2048,
};

enum LogicType : int32_t {
    PHYSICAL,
    LOGICAL,
};

struct MediaAlbumPluginRowData {
    std::string lpath;
    std::string albumName;
    std::string albumNameEn;
    std::string bundleName;
    std::string cloudId;
    std::string dualAlbumName;
    int32_t priority;
};

const std::vector<std::string> QUERY_ALBUM_COLUMNS = {
    PhotoAlbumColumns::ALBUM_ID,
    PhotoAlbumColumns::ALBUM_TYPE,
    PhotoAlbumColumns::ALBUM_NAME,
    PhotoAlbumColumns::ALBUM_LPATH,
    PhotoAlbumColumns::ALBUM_CLOUD_ID,
    PhotoAlbumColumns::ALBUM_SUBTYPE,
    PhotoAlbumColumns::ALBUM_DATE_ADDED,
    PhotoAlbumColumns::ALBUM_DATE_MODIFIED,
    PhotoAlbumColumns::ALBUM_BUNDLE_NAME,
    PhotoAlbumColumns::ALBUM_LOCAL_LANGUAGE,
    PhotoAlbumColumns::COVER_URI_SOURCE,
    PhotoAlbumColumns::COVER_CLOUD_ID,
};

const std::vector<std::string> MEDIA_CLOUD_SYNC_COLUMNS = {
    PhotoColumn::MEDIA_FILE_PATH,
    PhotoColumn::MEDIA_TITLE,
    PhotoColumn::MEDIA_SIZE,
    PhotoColumn::MEDIA_NAME,
    PhotoColumn::MEDIA_TYPE,
    PhotoColumn::MEDIA_MIME_TYPE,
    PhotoColumn::MEDIA_DEVICE_NAME,
    PhotoColumn::MEDIA_DATE_ADDED,
    PhotoColumn::MEDIA_DATE_MODIFIED,
    PhotoColumn::MEDIA_DATE_TAKEN,
    PhotoColumn::MEDIA_DURATION,
    PhotoColumn::MEDIA_IS_FAV,
    PhotoColumn::MEDIA_DATE_TRASHED,
    PhotoColumn::MEDIA_HIDDEN,
    PhotoColumn::PHOTO_HIDDEN_TIME,
    PhotoColumn::MEDIA_RELATIVE_PATH,
    PhotoColumn::MEDIA_VIRTURL_PATH,
    PhotoColumn::PHOTO_META_DATE_MODIFIED,
    PhotoColumn::PHOTO_ORIENTATION,
    PhotoColumn::PHOTO_LATITUDE,
    PhotoColumn::PHOTO_LONGITUDE,
    PhotoColumn::PHOTO_HEIGHT,
    PhotoColumn::PHOTO_WIDTH,
    PhotoColumn::PHOTO_SUBTYPE,
    PhotoColumn::PHOTO_BURST_COVER_LEVEL,
    PhotoColumn::PHOTO_BURST_KEY,
    PhotoColumn::PHOTO_DATE_YEAR,
    PhotoColumn::PHOTO_DATE_MONTH,
    PhotoColumn::PHOTO_DATE_DAY,
    PhotoColumn::PHOTO_USER_COMMENT,
    PhotoColumn::PHOTO_THUMB_STATUS,
    PhotoColumn::PHOTO_SYNC_STATUS,
    PhotoColumn::PHOTO_SHOOTING_MODE,
    PhotoColumn::PHOTO_SHOOTING_MODE_TAG,
    PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE,
    PhotoColumn::PHOTO_HDR_MODE,
    PhotoColumn::PHOTO_VIDEO_MODE,
    PhotoColumn::PHOTO_FRONT_CAMERA,
    PhotoColumn::PHOTO_DETAIL_TIME,
    PhotoColumn::PHOTO_EDIT_TIME,
    PhotoColumn::PHOTO_ORIGINAL_SUBTYPE,
    PhotoColumn::PHOTO_COVER_POSITION,
    PhotoColumn::MOVING_PHOTO_EFFECT_MODE,
    PhotoColumn::PHOTO_OWNER_ALBUM_ID,
    PhotoColumn::PHOTO_ORIGINAL_ASSET_CLOUD_ID,
    PhotoColumn::PHOTO_SOURCE_PATH,
    PhotoColumn::SUPPORTED_WATERMARK_TYPE,
    PhotoColumn::PHOTO_STRONG_ASSOCIATION,
    /* keep cloud_id at the last, so RecordToValueBucket can skip it*/
    MediaColumn::MEDIA_ID,
    PhotoColumn::PHOTO_CLOUD_ID,
};

const std::vector<std::string> ALBUM_LOCAL_QUERY_COLUMNS = {
    Media::PhotoAlbumColumns::ALBUM_ID,
    Media::PhotoAlbumColumns::ALBUM_NAME,
    Media::PhotoAlbumColumns::ALBUM_TYPE,
    Media::PhotoAlbumColumns::ALBUM_DIRTY,
    Media::PhotoAlbumColumns::ALBUM_LPATH,
    Media::PhotoAlbumColumns::ALBUM_CLOUD_ID,
};

const std::vector<std::string> ON_UPLOAD_COLUMNS = {
    Media::PhotoColumn::MEDIA_FILE_PATH,
    Media::PhotoColumn::MEDIA_DATE_MODIFIED,
    Media::PhotoColumn::PHOTO_META_DATE_MODIFIED,
    Media::MediaColumn::MEDIA_ID,
    Media::PhotoColumn::PHOTO_CLOUD_ID,
    Media::PhotoColumn::PHOTO_DIRTY,
};

const std::vector<std::string> PHOTOS_SYNC_COLUMN_STRING = {
    // Add Photos fields here. such as PhotoColumn::PHOTO_MEDIA_SUFFIX,
};

enum CheckFlag : int32_t {
    CHECKED,
    NEED_CHECK,
};

enum ErrorType {
    TYPE_UNKNOWN = 0,
    TYPE_NOT_NEED_RETRY,  // 不需要重试的错误类型
    TYPE_NEED_UPLOAD,     // 需要重新上传
    TYPE_MAX,
};

enum ServerErrorCode {
    ACCESS_DENIED = 403,            // 没有权限访问记录、数据库等
    ATOMIC_ERROR = 400,             // 原子批处理操作失败
    AUTHENTICATION_FAILED = 401,    // 身份验证失败
    AUTHENTICATION_REQUIRED = 421,  // 没有验证身份，但请求需要身份验证
    BAD_REQUEST = 400,              // 无效的请求
    CONFLICT = 409,                 // recordChangeTag值过期
    EXISTS = 409,                   // 创建的资源已存在
    INTERNAL_ERROR = 500,           // 内部错误
    NOT_FOUND = 404,                // 资源未找到
    QUOTA_EXCEEDED = 413,  // 如果访问公共数据库，您超出了应用程序的配额。访问私有数据库时，超出用户的iCloud配额
    THROTTLED = 429,       // 请求被停止了，稍后重试
    TRY_AGAIN_LATER = 503,             // 内部错误，稍后重试
    VALIDATING_REFERENCE_ERROR = 412,  // 请求违反了验证引用约束
    ZONE_NOT_FOUND = 404,              // 没有找到请求中指定的区域
    UID_EMPTY = 1003,                  // 云空间未登录，userid为空
    RESPONSE_EMPTY = 1004,             // 服务端返回为空
    RESPONSE_NOT_OK = 1005,            // 服务端返回结果错误
    NO_NETWORK = 1006,                 // 云空间没有网络
    GRS_NULL = 1007,                   // grs为空
    NETWORK_ERROR = 1008,              // 云空间网络异常
    ERROR_PARAM = 1009,                // 参数错误
    GET_AT_FAIL = 1010,                // 获取AT失效
    ASSET_NOT_EXIST = 1011,            // Asset不存在
    DELETE_ASSET_FAIL = 1012,          // 删除Asset失败
    SWITCH_OFF = 1013,                 // 同步开关关闭
    RESOURCE_INVALID = 1014,           // 资源重复，需要调用接口重新生成recordid
    ERROR_ANTI_DELETE = 1015,          // 防反删
    NOT_SUPPORT_APP = 1016,            // 应用不支持同步
    FILE_NOT_EXIST = 1017,             // 上传文件不存在
    READ_FILE_ERROR = 1018,            // 读取文件失败
    RESPONSE_TIME_OUT = 1019,          // 响应超时
    REQUEST_INHIBITED = 1020,          // 不允许
    INVALID_LOCK_PARAM = 1021,         // 锁参数不对
    RENEW_RESOURCE = 1024,             // 需要重置id，并重新上传
    ALBUM_NOT_EXIST = 1025,            // 相册不存在

    // networkkit 返回的错误
    NET_KIT_HTTP_ERROR = 10001,          // networkkit http错误，例如 服务端主动关闭拒绝
    NET_KIT_INTERNAL_ERROR = 10002,      // networkkit 内部错误
    NET_KIT_INTERRUPTED_ERROR = 10003,   // networkkit 中断错误，业务取消了上传下载
    NET_KIT_NETWORK_ERROR = 10004,       // networkkit 网络错误
    NET_KIT_NETWORK_IO_ERROR = 10005,    // networkkit IO错误
    NET_KIT_PARAMS_CHECK_ERROR = 10006,  // networkkit 参数错误
    NET_KIT_UNKOWN_ERROR = 10007,        // networkkit 未知错误
};

enum ErrorDetailCode {
    LACK_OF_PARAM = 4001,                     // 缺少入参
    PARAM_INVALID = 4002,                     // 参数校验失败
    PARAM_VALUE_NOT_SUPPORT = 4003,           // 参数不支持
    PARAM_EXPIRED = 4004,                     // 入参已失效
    SITE_NOT_FOUND = 4005,                    // 站点信息未知
    NON_SUPPORT_CHARACTER_INCLUDED = 4006,    // 包含有不支持的字符
    ILLEGAL_CHARACTER_INCLUDED = 4007,        // 包含有非法的字符
    PARAMETER_LENGTH_BEYOND_LIMIT = 4008,     // 参数长度超过限制
    PARENTFOLDER_NOT_FOUND = 4009,            // 父目录不存在
    TOKEN_CFG_INVALID = 4010,                 // token配置项无效
    USER_NOT_AUTHORIZED = 4011,               // 用户没有授权
    APP_NOT_AUTHORIZED = 4012,                // App没有授权
    TICKET_INVALID = 4013,                    // ticket无效
    GRANT_CANCEL = 4014,                      // 取消授权
    LOGIN_FAILED = 4015,                      // 登陆失败
    SESSION_TIMEOUT = 4016,                   // 会话过期
    FORCE_RELOGIN = 4017,                     // 强制重新登陆账号
    FLOW_ID_NOT_MATCH = 4020,                 // 越权
    DATA_MIGRATING = 4031,                    // 数据割接未完成
    SERVICE_NOT_SUPPORT = 4032,               // 服务未开放
    AGREEMENT_NOT_SIGNED = 4033,              // 协议未签署
    CROSS_SITE_NOT_SUPPORT = 4034,            // 跨站点功能限制
    INSUFFICIENT_SCOPE = 4035,                // Scope校验失败
    INSUFFICIENT_PERMISSION = 4036,           // 无此操作权限
    OPERATION_NOT_COMPLETE = 4037,            // 由于错误无法完成操作
    OUTER_SERVICE_ERROR = 4038,               // 外部服务错误且无法恢复
    SPACE_FULL = 4039,                        // 用户空间不足
    CONTENT_NOT_FIND = 4041,                  // 内容未发现
    CONTENT_UNAVAILABLE = 4042,               // 内容解析失败
    CHANNEL_NOT_FOUND = 4043,                 // 订阅未发现
    THUMBNAIL_NOT_FOUND = 4044,               // 缩略图不存在
    SHARE_LINK_NOT_FOUND = 4045,              // 分享链接不存在
    TEMP_DATA_INVALID = 4046,                 // 临时数据无效
    FILE_NOT_FOUND = 4047,                    // 实体数据不存在
    APP_NOT_EXISTS = 4048,                    // APP不存在
    CATEGORY_NOT_EXISTS = 4049,               // 分类不存在
    SHARE_CONTENT_NOT_EXISTS = 4050,          // 分享内容不存在
    THUMBNAIL_GENERATE_FAILED = 4051,         // 缩略图生成失败
    VERSION_CONFLICT = 4090,                  // 版本冲突
    LOCK_FAILED = 4091,                       // 获取锁失败
    SILENCE_USER_FAILED = 4092,               // 沉默用户拒绝失败
    FILE_USING_FORBIDDEN_OP = 4093,           // 文件正在使用禁止操作
    LOCK_BY_DEL_DEVICE = 4094,                // 设备因按设备删除已锁定
    EXPIRE_APP_CLEANUP = 4095,                // 超期模块清理中
    CURSOR_EXPIRED = 4100,                    // 游标过期
    TEMP_DATA_CLEARD = 4101,                  // 临时数据已经被清理
    CLOUD_DATA_UPDATED = 4121,                // 云端数据已被更新
    FORCE_UPDATE_CLIENT = 4161,               // 新商业模式下，老端不支持，强制更新
    USER_REQUEST_TOO_MANY = 4291,             // 用户请求太频繁
    APP_REQUEST_TOO_MANY = 4292,              // 应用请求太频繁
    FLOW_CONTROL = 4293,                      // 用户请求流控
    USER_REQUEST_ERROR_TOO_MANY = 4294,       // 用户请求错误次数超过限制
    USERDATA_EXCEEDS_UPPER_THRESHOLD = 4296,  // 用户上传数据超过阈值
    PARTIAL_FILE_NOT_SUPPORT_SHARE = 4905,    // 部分文件不支持分享
    PARENTFOLDER_TRASHED = 4906,              // 父目录被删除到回收站
    ACCOUNT_NAME_WRONG = 4907,                // 用户账号错误
    CIPHER_INVALID = 4908,                    // 用户秘钥已经失效
    DUPLICATED_ID = 4909,                     // id已存在
    VUDID_IMEI_INVALID = 4910,                // VUDID转换IMEI失败
    FILE_NOT_SUPPORT_SHARE = 4911,            // 所有文件不支持分享
    EXTERNAL_LINK_NOT_AUTHORIZED = 4912,      // 外部链接没有授权
    ORIGINAL_NOT_EXSIT = 4913,                // 源文件不存在
    NO_CHECK_RESULT = 4914,                   // 无检查结果
    CHECK_FAILED = 4915,                      // 检查失败
    BUCKET_NOT_EXISTS = 4916,                 // 桶不存在
    RECORD_DATA_LOST = 4917,                  // Record记录丢失
    MODIFY_NORMAL_DATA = 4918,  // 数据零丢失补全修复时，校验失败，正在修复的记录为正常的记录
    DIVIDE_NOT_SUPPORT = 4919,                           // 不支持差分
    BUCKET_NOT_IN_SAME_REGION = 4920,                    // 桶不同源
    FILE_REFERENCED = 4921,                              // File被引用，不能删除
    SLICE_OBJECT_LOST = 4923,                            // 分片丢失
    ABNORMAL_DOWNLOAD = 4931,                            // 用户异常下载
    SAME_FILENAME_NOT_ALLOWED = 4932,                    // 禁止重复文件名
    CANNOT_USE_SERVICE = 4933,                           // 无法为此用户提供服务
    FILES_NUM_BEYOND_LIMIT = 4934,                       // 下载文件数超过限制
    FILES_SIZE_BEYOND_LIMIT = 4935,                      // 下载文件大小超过限制
    TIER_BEYOND_LIMIT = 4936,                            // 层级超过限制
    HISTORYVERSIONS_BEYOND_LIMIT = 4937,                 // 历史版本数超过限制
    COPY_FORBIDDEN = 4938,                               // 此文件禁止拷贝
    USER_SUSPEND_SERVICE = 4939,                         // 用户停用服务
    FILE_VERSION_CONFLICT = 4940,                        // 文件版本冲突
    REAL_NAME_AUTHENTICATION_FAILED = 4941,              // 实名认证失败
    SHARE_LINK_EXPIRED = 4942,                           // 分享链接已经过期
    RECEIVER_BEYOND_LIMIT = 4943,                        // 分享人数达到上限
    CONTENT_SHARE_NOT_ALLOWED = 4944,                    // 内容禁止分享
    DATA_NUMBER_BEYOND_LIMIT = 4945,                     // 云端数据超过限制
    FORBIDDEN_USER = 4946,                               // 禁止用户操作
    PARAM_CAN_NOT_UPDATE = 4947,                         // 参数禁止更新
    HORIZONTAL_PRIVILEGE_ESCALATION = 4948,              // 存在横向越权
    OPERATION_FORBIDDEN_IN_RECYCLE = 4949,               // 禁止在回收站进行此操作
    FILENAME_LENGTH_BEYOND_LIMIT = 4950,                 // 文件名超过上限
    OPERATION_FORBIDDEN_DELETE_BACKUP_EXIST = 4951,      // 禁止删除操作，存在clearTime内的备份记录
    TEMP_KEY_EXPIRED = 4952,                             // 临时秘钥已过期
    USER_SHARE_NOT_ALLOWED = 4953,                       // 用户被封禁，禁止分享
    USER_SHARE_PRIVILEGE_LIMITED = 4954,                 // 用户没有权益
    CONTENT_COPYRIGHT_LIMIT = 4955,                      // 版权文件禁止下载
    RISK_MANAGE_FAILED = 4956,                           // 风控失败
    SHAREING_TIMES_EXCEEDS_LIMITS = 4957,                // 分享次数超出了限制
    TASK_RUNNING = 4959,                                 // 任务正在执行
    SERVER_VERSION_UNAVAILABLE = 4960,                   // 服务版本号不可用
    APP_STATUS_ABNORMAL = 4961,                          // APP状态不正常
    CATEGORY_STATUS_ABNORMAL = 4962,                     // 分类状态不正常
    CATEGORY_APP_ASSOCIATIONS_EXISTS = 4963,             // 存在应用和分类关系
    FUNCTION_NOT_SUPPORT = 4964,                         // 功能暂不支持
    MEMBER_KINSHIP_EXISTS = 4965,                        // 成员关系已存在
    MEMBER_NUMBER_LIMIT = 4966,                          // 成员人数已达上限
    USER_IS_JOINED = 4967,                               // 用户已经加入群组
    USER_BE_FROZEN = 4968,                               // 用户被冻结
    INVITER_LINK_EXPIRED = 4969,                         // 邀请码或链接已过期
    RISK_SCANNING = 4970,                                // 分享内容审核中禁止访问
    ALBUM_APPLICATION_NUMBER_LIMIT = 4971,               // 共享相册申请加入次数超过上限
    INVITE_CODE_ERROR = 4972,                            // 邀请码错误
    INVITER_LINK_USED = 4973,                            // 邀请码或链接已使用过
    APPLICANT_IS_EXIST = 4974,                           // 记录已存在
    APPLICANT_NOT_FOUND = 4975,                          // 记录不存在
    APPLICANT_IS_EXPIRED = 4976,                         // 记录已失效
    DATA_CLEARED_FORBIDDEN = 4977,                       // 云侧数据清理禁止访问
    APPEAL_ACCOUNT_FORBIDDEN = 4978,                     // 人工封禁不支持申诉
    APPLICANT_IS_PROCESSED = 4979,                       // 申请已经处理过
    RESOURCE_NOT_MATCH = 4980,                           // 备份记录资源归属不匹配
    RISKFILE_FORBIDDEN_DOWN = 4981,                      // 文件违规禁止下载
    CONTENT_IS_EXPIRED = 4982,                           // 内容已过期
    BATCH_IS_EXPIRED = 4983,                             // 批次失效
    USER_NOT_REALNAME = 4984,                            // 用户未实名认证
    CONTENT_ALL_RISK = 4985,                             // 内容全部违规
    BUSINESS_MODEL_CHANGE_DATA_UPLOAD_FORBIDDEN = 4988,  // 新商业模式下，基础用户不允许调用
    CLOUD_DATA_CLEARING = 4989,                          // 用户数据清理中，禁止数据操作
    USER_DATA_MAX_LIMIT = 4990,                          // 用户数据达到规格上限
    PARNENT_NOT_EXIST = 4991,                            // 文件父目录不存在
    SERVER_IS_BUSY = 5001,                               // 服务器资源不够
    OUTER_SERVICE_UNAVAILABLE = 5002,                    // 外部服务不可用
    OUTER_SERVICE_BUSY = 5003,                           // 外部服务忙
    DATABASE_UNAVAILABLE = 5004,                         // 数据库操作失败
    RESOURCE_LOCKED = 5005,                              // 资源被锁住，暂时禁止操作
    SERVER_TEMP_ERROR = 5006,                            // 服务器临时错误
    SERVER_UPDATING = 5007,                              // 服务器升级中
    RESOURCE_TRASHING = 5008,                            // 资源GC中，暂时禁止操作
    SERVICE_UNAVAILABLE = 5030,                          // 服务不可用
    NSP_FLOW_CONTROL = 5031,                             // NSP_FLOW_CONTROL
    TEMP_ERROR_RETRY = 5040,                             // 触发端侧重试错误码
};

enum CloudSyncServiceErrCode {
    E_OK = 0,
    E_SEVICE_DIED,
    E_INVAL_ARG,
    E_BROKEN_IPC,
    E_SA_LOAD_FAILED,
    E_SERVICE_DESCRIPTOR_IS_EMPTY,
    E_PERMISSION_DENIED,
    E_PERMISSION_SYSTEM,
    E_GET_PHYSICAL_PATH_FAILED,
    E_GET_TOKEN_INFO_ERROR,
    E_SYNCER_NUM_OUT_OF_RANGE,
    E_SYNC_FAILED_BATTERY_LOW,
    E_SYNC_FAILED_BATTERY_TOO_LOW,
    E_SYNC_FAILED_NETWORK_NOT_AVAILABLE,
    E_GET_NETWORK_MANAGER_FAILED,
    E_DELETE_FAILED,
    E_NO_SUCH_FILE,
    E_RENAME_FAIL,
    E_SYSTEM_LOAD_OVER,
    E_EXCEED_MAX_SIZE,
    E_ILLEGAL_URI,

    /* data syncer */
    E_CLOUD_SDK,
    E_RDB,
    E_CONTEXT,
    E_STOP,
    E_PENDING,
    E_SCHEDULE,
    E_ASYNC_RUN,
    E_PATH,
    E_DATA,
    E_NOTIFY,
    E_UNKNOWN,
    E_CLOUD_STORAGE_FULL,
    E_LOCAL_STORAGE_FULL,
    E_BUSINESS_MODE_CHANGED,
    E_OSACCOUNT,
    E_USER_LOCKED,
    E_GET_SIZE_ERROR,

    E_IPC_READ_FAILED,
    E_IPC_WRITE_FAILED,
    E_SOFTBUS_SESSION_FAILED,
    E_GET_DEVICE_ID,
    E_GET_USER_ID,
    E_NULLPTR,
    /* session */
    E_CREATE_SESSION,
    E_OPEN_SESSION,
    E_WAIT_SESSION_OPENED,
    E_FILE_NOT_EXIST,
    E_SEND_FILE,
    E_MEMORY,

    /* eventhandler */
    E_EVENT_HANDLER,
    E_ZIP,

    /* download timeout */
    E_TIMEOUT,

    E_SOURCE_BASIC = 10000,

    E_THM_SOURCE_BASIC = E_SOURCE_BASIC + 1000,
    E_THM_SIZE_IS_ZERO = E_THM_SOURCE_BASIC + 201,
    E_THM_IS_TOO_LARGE = E_THM_SIZE_IS_ZERO + 1,

    E_LCD_SOURCE_BASIC = E_SOURCE_BASIC + 2000,
    E_LCD_SIZE_IS_ZERO = E_LCD_SOURCE_BASIC + 201,
    E_LCD_IS_TOO_LARGE = E_LCD_SIZE_IS_ZERO + 1,

    E_CONTENT_SOURCE_BASIC = E_SOURCE_BASIC + 3000,
    E_CONTENT_SIZE_IS_ZERO = E_CONTENT_SOURCE_BASIC + 201,
    E_CONTENT_COVERT_LIVE_PHOTO = E_CONTENT_SIZE_IS_ZERO + 1,
    E_CONTENT_RAW_SIZE_IS_ZERO = E_CONTENT_COVERT_LIVE_PHOTO + 1,

    E_FIELD_BASIC = 20000,

    E_DB_FIELD_BASIC = E_FIELD_BASIC + 1000,
    E_DB_SIZE_IS_ZERO = E_DB_FIELD_BASIC + 1,
    E_DB_ALBUM_NOT_FOUND = E_DB_SIZE_IS_ZERO + 1,
    E_DB_ALBUM_CLOUD_ID_IS_EMPTY = E_DB_ALBUM_NOT_FOUND + 1,

    E_DK_FIELD_BASIC = E_FIELD_BASIC + 2000,
    E_NO_ATTRIBUTES = E_DK_FIELD_BASIC + 1,
};

enum AlbumSource {
    ALBUM_FROM_LOCAL = 1,
    ALBUM_FROM_CLOUD = 2,
};

enum Clean {
    NOT_NEED_CLEAN = 0,
    NEED_CLEAN,
};

enum ThmLcdState {
    THM = 0b001,
    LCD = 0b010,
    THMLCD = 0b011,
};

enum PhotoPosition {
    POSITION_LOCAL = 1,
    POSITION_CLOUD,
    POSITION_BOTH,
};

enum StatsIndex {
    NEW_RECORDS_COUNT = 0,
    MERGE_RECORDS_COUNT = 1,
    META_MODIFY_RECORDS_COUNT = 2,
    FILE_MODIFY_RECORDS_COUNT = 3,
    DELETE_RECORDS_COUNT = 4,
};

enum class MediaTableType : uint32_t {
    TYPE_ASSET,
    TYPE_ALBUM,
};

static constexpr int STORAGE_MANAGER_MANAGER_ID = 5003;
static const std::string CLOUD_BASE_URI = "datashareproxy://com.huawei.hmos.clouddrive/";
static const std::string CLOUD_DATASHARE_URI = CLOUD_BASE_URI + "cloud_sp?Proxy=true";
static const std::string CLOUD_URI = CLOUD_DATASHARE_URI + "&key=useMobileNetworkData";
static const std::string CLOUD_AGING_URI = CLOUD_DATASHARE_URI + "&key=dataAgingPolicy";
static const std::string CLOUD_SYNC_URI = CLOUD_BASE_URI + "sync_module_data?Proxy=true";
static const std::string CLOUD_SYNC_SWITCH_URI = CLOUD_BASE_URI + "sync_switch&bundleName=com.huawei.hmos.photos";
static const std::string MOBILE_NETWORK_STATUS_ON = "1";
static const std::string SCREENRECORD_ALBUM_PATH = "/storage/emulated/0/Pictures/Screenrecords/";
static const std::string SOURCE_PATH_PERFIX = "/storage/emulated/0";
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_SYNC_CONST_H