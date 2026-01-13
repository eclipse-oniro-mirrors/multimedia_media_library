/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef MEDIA_FILE_MONITOR_SERVICE_H
#define MEDIA_FILE_MONITOR_SERVICE_H

#include <string>

#include "iremote_broker.h"
#include "napi/native_api.h"

namespace OHOS::FileMonitorService {
const std::u16string DECLARE_INTERFACE_DESCRIPTOR = u"HDI.IServiceStatusListener.V1_0";

class CallbackData {
public:
    std::string path = "";
    napi_value getNapiVal(napi_env env)
    {
        napi_value result = nullptr;
        napi_status status = napi_create_object(env, &result);
        if (status != napi_ok) {
            return nullptr;
        }
        napi_value path = nullptr;
        status = napi_create_string_utf8(env, this->path.c_str(), NAPI_AUTO_LENGTH, &path);
        if (status != napi_ok) {
            return nullptr;
        }

        status = napi_set_named_property(env, result, "path", path);
        if (status != napi_ok) {
            return nullptr;
        }
        return nullptr;
    }
};

/**
 * 查询状态类型
 */

enum class SearchStatusType {
    IndexStatus = 0,
};

/**
 *  状态返回值
 */
enum class FileMonitorStatus {
    Error = 0,
    IndexBusy = 1,
    IndexIdle = 2,
};

enum BinType {
    BIN_01 = 1,
    BIN_10 = 2,
    BIN_11 = 3
};

/**
 *  路径类型
 */
enum PathType {
    PUBLIC = 0,
    SANDBOX,
    CLOUD,
    GROUP,
    DISK_SEARCH,
    UNKNOWN,
    HMDFS
};

// 通信错误码
enum ErrorCode {
    PROXY_OK                            = 0,
    PROXY_ERROR                         = 1,
    PROXY_PERMSSION_DENIED              = 2,
    ASYN_CONTINUE                       = 3,
    GET_IPC_OBJECT_FAILED               = 1000,
    SET_DESCRIPTOR_FAILED               = 1001,
    SET_REQUEST_INFO_FAILED             = 1002,
    CHECK_DESCRIPTOR_FAILED             = 1003,
    BANCH_SIZE_EXCEED_LIMIT             = 1004,
    HANDLE_REGISTER_REQUEST_FAILED      = 1005,
    HANDLE_SEARCH_REQUEST_FAILED        = 1006,
    HANDLE_UPDATE_REQUEST_FAILED        = 1007,
    SET_REQUEST_FAILED                  = 1008,
    IS_SCANNING_USER_FAILED             = 1009,
    PROXY_NO_RIGHT                      = 1010,
    EMPTY_DATA                          = 1011,
    USER_LOCK_YET                       = 1012,
    SET_ABSTRACT_FAILED                 = 1013,
    SET_FILE_LOCK_FAILED                = 1014,
    INVALED_PATH                        = 1015,
    STAT_ERROR                          = 1016,
    // DB错误码
    DB_STORE_NULL                       = 10001,
    DB_RESULTSET_NULL                   = 10002,
    DB_PARAM_EMPTY                      = 10003,
    DB_PARAM_ERROR                      = 10004,
    DB_EXECUT_ERROR                     = 10005,
    DB_EXECUT_NULL                      = 10006,
    DB_EXECUT_NO_DATA                   = 10007,
    E_SQLITE_CONSTRAINT                 = 27394115,
    // 智慧文件夹错误码
    SMART_FOLDER_RELATION_EXIST         = 11001,
    SMART_FORDER_RECOMMEND_FAILED       = 11002,
};

/**
 * u盘检索数据
 */
struct DiskData {
    std::string fileName = "";
    std::string fileUri = "";
};

struct BaseCallbackContext {
    napi_env env = nullptr;
    napi_ref callback_ref = nullptr;
    napi_threadsafe_function tsFn;
};

/**
 * u盘检索结果
 */
struct DiskResult {
    ErrorCode errorCode = ErrorCode::PROXY_ERROR;
    std::string traceId = "";
    std::vector<DiskData> diskDataVec {};

    uint64_t lastSizeAsynSendData = 0; //上一次异步发送数据时diskDataVec大小
    int64_t lastTimeAsynSendData  = 0; //上一次异步发送数据时间，毫秒级,默认当前时间
    uint32_t count;
};

struct DiskCallbackData {
    DiskResult diskResult;
};

struct DiskCallbackContext : public BaseCallbackContext {
    DiskCallbackData data;
};

class IPCCallback : public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"file_monitor_service.IPCCallback");
    enum {
        FILES_CALLBACK = 1,
        CANCEL_MONITOR = 2,
        DISK_REPLY = 3, //磁盘暴力搜索回复
        FILE_CHANGE = 4,
    };
    virtual int32_t SendOpenFile(const std::string &path) = 0;
    virtual int32_t CancelMonitor() = 0;
    virtual bool SendDiskResult(DiskResult &diskResult) = 0;
};

/**
 * u盘检索参数
 */
struct DiskSearchModel {
    std::string searchValue = "";
    std::string searchUri = "";
    bool isAsync {false}; // 是否异步调用
    std::string traceId = "";
    bool isInterrupt {false}; //是否中断搜索任务

    std::string searchPath = "";
    std::vector<std::string> searchPathVec {};
    PathType pathType;
    int32_t currentUid = 0;
    std::shared_ptr<DiskCallbackContext> context = nullptr;
    sptr<IPCCallback> ipcCallBack = nullptr;
    std::string tabId = ""; // traceId拆分得到
    int64_t timeStamp = 0; //traceId拆分得到
};

struct FileMsgModel {
    int32_t uid;
    int32_t id;
    std::string inode;
    std::string fileUri;
    std::string oldFileUri;
    int64_t viewTime;
    std::string appName;
    int32_t opType;
    uint32_t cliType;
    bool isContentChange;
    bool isFile;
    bool isListener;
    bool isRecycled = false;
    bool isConsumed = false;
    bool isLocalOp = true;
    bool isFileNameToFusion = false;
    bool isTFCard = false;
    BinType isMediaData = BinType::BIN_01;
    BinType isHOData = BinType::BIN_01;
    bool hasInsertFileMsgTable = false;
    bool hasInsertFileInfoTable = false;
    int64_t size {0};
    int64_t modifyDate {0};
    int64_t changeDate {0};
    int64_t createDate {0};
    int32_t openSource {0};
};

// 来源： 扫盘/监听
constexpr uint64_t MSG_TYPE_SOURCE_SCAN             = 0x00000001;
constexpr uint64_t MSG_TYPE_SOURCE_LISTENER         = 0x00000002;
constexpr uint64_t MSG_TYPE_SOURCE_ALL              = 0x00000003;
// 是否关注内容
constexpr uint64_t MSG_TYPE_TYPE_FILENAME           = 0x00000004;
constexpr uint64_t MSG_TYPE_TYPE_CONTENT            = 0x00000008;
constexpr uint64_t MSG_TYPE_TYPE_CONTENT_ALL        = 0x0000000C;
// 文件类型: 目录/文件
constexpr uint64_t MSG_TYPE_FILE_TYPE_FOLDER        = 0x00000010;
constexpr uint64_t MSG_TYPE_FILE_TYPE_FILE          = 0x00000020;
constexpr uint64_t MSG_TYPE_FILE_TYPE_ALL           = 0x00000030;
// 操作类型： 增/删/改
constexpr uint64_t MSG_TYPE_OPTION_INSERT           = 0x00000040;
constexpr uint64_t MSG_TYPE_OPTION_UPDATE           = 0x00000080;
constexpr uint64_t MSG_TYPE_OPTION_DELETE           = 0x00000100;
constexpr uint64_t MSG_TYPE_OPTION_VISIT            = 0x00000200;
constexpr uint64_t MSG_TYPE_OPTION_DELETE_VISIT     = 0x00000400;
constexpr uint64_t MSG_TYPE_OPTION_ALL              = 0x00000FC0;
// 文件用户/目录用户/沙箱目录/云盘目录
constexpr uint64_t MSG_TYPE_FOLDER_USER_DATA        = 0x00001000;
constexpr uint64_t MSG_TYPE_FOLDER_CLOUD            = 0x00002000;
constexpr uint64_t MSG_TYPE_FOLDER_SANBOX_1         = 0x00004000;
constexpr uint64_t MSG_TYPE_FOLDER_SANBOX_2         = 0x00008000;
constexpr uint64_t MSG_TYPE_FOLDER_ALL              = 0x000FF000;
// 是否在回收站
constexpr uint64_t MSG_TYPE_IS_RECYCLED             = 0x00100000;
constexpr uint64_t MSG_TYPE_IS_RECYCLED_NO          = 0x00200000;
constexpr uint64_t MSG_TYPE_IS_RECYCLED_ALL         = 0x00300000;
// 消费对象
constexpr uint64_t MSG_TYPE_IS_CLI_INDEX_INSERT     = 0x00400000;
constexpr uint64_t MSG_TYPE_IS_CLI_FILE_MGR         = 0x00800000;
constexpr uint64_t MSG_TYPE_IS_CLI_COMMON           = 0x01000000;
constexpr uint64_t MSG_TYPE_IS_CLI_ALL              = 0x01C00000;
// 是本地or云上下行的数据（主要针对云盘数据）
constexpr uint64_t MSG_TYPE_IS_LOCAL_OP_NO          = 0x02000000;
constexpr uint64_t MSG_TYPE_IS_LOCAL_OP             = 0x04000000;
constexpr uint64_t MSG_TYPE_IS_LOCAL_OP_ALL         = 0x06000000;
// 是否TF卡数据
constexpr uint64_t MSG_TYPE_IS_TF_DATA_NO           = 0x08000000;
constexpr uint64_t MSG_TYPE_IS_TF_DATA              = 0x10000000;
constexpr uint64_t MSG_TYPE_IS_TF_DATA_ALL          = 0x18000000;
// 是否媒体库相关数据
constexpr uint64_t MSG_TYPE_IS_MEDIA_DATA_NO        = 0x20000000;
constexpr uint64_t MSG_TYPE_IS_MEDIA_DATA           = 0x40000000;
constexpr uint64_t MSG_TYPE_IS_MEDIA_DATA_ALL       = 0x60000000;
// 是否HO_DATA相关数据
constexpr uint64_t MSG_TYPE_IS_HO_DATA_NO           = 0x80000000;
constexpr uint64_t MSG_TYPE_IS_HO_DATA              = 0x100000000;
constexpr uint64_t MSG_TYPE_IS_HO_DATA_ALL          = 0x180000000;
// 所有类型
constexpr uint64_t MSG_TYPE_NULL                    = 0x00000000;
constexpr uint64_t MSG_TYPE_DEFAULT                 = 0x1ffffffff;

inline const std::vector<uint64_t> MSG_GROUPS = {MSG_TYPE_SOURCE_ALL, MSG_TYPE_TYPE_CONTENT_ALL,
    MSG_TYPE_FILE_TYPE_ALL, MSG_TYPE_OPTION_ALL, MSG_TYPE_FOLDER_ALL, MSG_TYPE_IS_RECYCLED_ALL, MSG_TYPE_IS_CLI_ALL,
    MSG_TYPE_IS_LOCAL_OP_ALL, MSG_TYPE_IS_TF_DATA_ALL, MSG_TYPE_IS_MEDIA_DATA_ALL, MSG_TYPE_IS_HO_DATA_ALL};

// 消息状态，1为未消费， 0为已消费， 用户可自定义其他类型
constexpr int32_t STATE_CONSUMED = 0;
constexpr int32_t STATE_UNCONSUMED = 1;

// 权限表消息类型
constexpr int32_t PERM_MSG_TYPE_APP_STOP = 0;
constexpr int32_t PERM_MSG_TYPE_CHANGE = 1;

struct PermMsgModel {
    int32_t uid;
    int32_t msgType;
    int32_t tokenID;
    int32_t changeType;
    std::string permissonName;
};

class FileChangeCallback {
public:
    FileChangeCallback() = default;
    virtual ~FileChangeCallback() = default;
    virtual int32_t OnFileChanged() = 0;
};

struct VisitDatetime {
    std::string path;
    int64_t viewDatetime;
};

struct FileMonitorContext : public BaseCallbackContext {
    CallbackData *data = nullptr;
};

class FileMonitorProxy {
public:
    // 通过获取的对象调用请求接口
    virtual ~FileMonitorProxy() {};
    virtual int32_t RegisteRequset(uint64_t type) = 0;
    virtual int32_t RegisteRequest(uint64_t type, std::shared_ptr<FileChangeCallback> callback) = 0;
    virtual int32_t UnregisteRequest() = 0;
    virtual int32_t SearchRequest(int32_t state, uint64_t type, std::vector<FileMsgModel> &outMsg) = 0;
    virtual int32_t SearchMonitorData(int32_t state, std::vector<FileMsgModel> &outMsgs,
        bool isStorage = false) = 0;
    virtual int32_t UpdateRequest(std::vector<int32_t> ids) = 0;
    virtual int32_t UpdateRequest(std::vector<int32_t> ids, int32_t state) = 0;
    virtual int32_t UpdateRequestV2(int32_t uid, std::vector<int32_t> ids, int32_t state) = 0;
    virtual int32_t UpdateRequestByState(int32_t uid, std::vector<int32_t> states, int32_t state) = 0;
    virtual int32_t UpdateAppNameRequest(uint64_t type, FileMsgModel &fileMsgModel) = 0;
    virtual int32_t UpdateVisitDatetime(uint64_t type, VisitDatetime &visitDatetime) = 0;

    virtual int32_t SearchPermMsgReq(
        int32_t saId, std::vector<int32_t> &outTokenIds, std::vector<PermMsgModel> &outMsgs) = 0;
    virtual DiskResult SearchDiskData(DiskSearchModel &diskSearchModel) = 0;
    virtual void SearchFileMonitorStatus(const SearchStatusType &searchStatusType,
        FileMonitorStatus &fileMonitorStatus) = 0;
    virtual int32_t GetDeviceTypeFromUuid(const std::string &tfUuid, int32_t &deviceType) = 0;
    virtual int32_t GetFilesStatus(const std::string &uri, const std::vector<std::string> &inputList,
        std::shared_ptr<FileMonitorContext> context, std::vector<std::string> &outputList, bool &isOccupy) = 0;
    virtual int32_t NotifyMonitor(const std::string &uri, const int32_t opFlag) = 0;
};

extern "C" FileMonitorProxy* CreateFileMontitorProxy(int32_t tableID);
extern "C" void RealseFileMonitorProxy(FileMonitorProxy* fileMonitorProxy);
} // OHOS::FileMonitorService
#endif