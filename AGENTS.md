# 媒体库框架指引

## 项目定位

本仓库对应 OpenHarmony `foundation/multimedia/media_library`，是系统级媒体资产管理框架，统一管理图片、视频、音频的元数据和相册，提供媒体扫描、缩略图、云同步、备份恢复、MTP、权限校验和跨语言公开接口。优先按这些目录定位问题：

- `frameworks/innerkitsimpl/medialibrary_data_extension/`：DataShare 扩展服务，资产/相册/文件/照片/音频/PTP 操作、命令和 URI 分发、TLV 序列化、脏数据、封面刷新等核心业务主链路。
- `frameworks/innerkitsimpl/media_library_manager/`、`frameworks/innerkitsimpl/media_library_helper/`：`MediaLibraryManager`、`MediaLibraryExtendManager`、`MediaFileUri`、`MediaVolume`、自定义恢复等内部实现与 helper 工具。
- `frameworks/innerkitsimpl/media_library_cloud_sync/`：云媒体数据客户端、MDK record、cloud photo/album handler、数据转换和云同步线程限流。
- `frameworks/innerkitsimpl/media_library_camera_helper/`、`frameworks/innerimpl/camera/`：相机联动、`MediaPhotoAssetProxy`、拍照落盘。
- `frameworks/innerkitsimpl/media_permission_helper/`、`services/media_permission/`：权限校验、URI 权限、敏感操作。
- `frameworks/innerkitsimpl/media_share_album_lite/`、`services/media_share_album_service/`：共享相册数据客户端和服务。
- `frameworks/innerkitsimpl/analysis_data_kits/`、`services/media_analysis_data_manager/`、`services/media_analysis_extension/`：分析数据 kit、分析数据管理、分析扩展。
- `interfaces/inner_api/`、`interfaces/kits/c/`、`interfaces/kits/js/`、`interfaces/kits/cj/`、`frameworks/ani/`、`frameworks/js/`、`frameworks/native/`：内部 Native API、C API、JS/NAPI、CJ、ANI、Taihe 等多语言绑定，以及原生资产管理实现。
- `services/media_rdbstore/`、`services/media_scanner/`、`services/media_file_monitor/`、`services/media_fuse/`、`services/media_file_management/`、`services/media_thumbnail/`、`services/media_albums_manager/`、`services/media_cloud_sync_service/`、`services/media_mtp/`、`services/media_backup_extension/`、`services/media_custom_restore/`、`services/media_refresh/`、`services/media_facard/`、`services/media_kv_db/`、`services/media_notification/`、`services/media_assets_manager/`：各业务子服务实现。
- `tools/medialibrary_scanner/`、`tools/medialibrary_tool/`、`MediaLibraryExt/`：扫描工具、媒体工具 HAP 和媒体库扩展 HAP。
- `frameworks/innerkitsimpl/test/fuzztest/`、`frameworks/innerkitsimpl/test/unittest/`、`common/`、`frameworks/utils/`：fuzz 目标、单元测试、公共数据和通用工具（错误码、日志、权限、exif、隐私）。

## 典型工作流

1. 先判断改动场景，按下文知识路由定位代码锚点和验证目标；一个任务跨多个场景时，按影响面同时读取多个入口。
2. 定位公开接口和内部实现边界：先看 `interfaces/inner_api/media_library_helper/include/`、`interfaces/kits/c/`、`interfaces/kits/js/include/`，再看 `frameworks/innerkitsimpl/` 和 `frameworks/native/`、`frameworks/js/`。
3. 改动涉及公开 API、错误码、DataShare URI/命令、权限、云同步、TLV/Parcel、数据库 schema 或跨进程传输时，先确认生命周期、安全边界、错误码映射和 API 兼容性。
4. 小步修改，就近复用项目已有宏、错误码、日志和测试资源。
5. 按下文验证矩阵运行最近的测试目标。
6. 最终回复要写明完成的验证、未覆盖的 XTS 缺口，以及是否已提交或 push。
7. 提交和 push 前按下文"提交和推送"要求完成检查。

## 依赖和接口边界

本仓对外依赖在 `bundle.json` 中声明，常见跨子系统边界包括：

- 数据存储和文件：`data_share`、`relational_store`、`kv_store`、`preferences`、`storage_service`、`app_file_service`、`file_api`、`e2fsprogs`、`libfuse`。
- 运行时和 API：`ability_runtime`、`ability_base`、`napi`、`node`、`ipc`、`ffrt`、`eventhandler`、`samgr`、`safwk`、`init`、`ets_frontend`、`ets_runtime`、`runtime_core`。
- 图像和媒体：`image_framework`、`player_framework`、`media_foundation`、`image_effect`、`graphic_2d`、`graphic_surface`、`drivers_interface_display`。
- 安全和权限：`access_token`、`huks`、`openssl`、`security_component_manager`、`os_account`。
- 云和分布式：`dfs_service`、`drivekit_native`、`device_manager`、`wifi`、`netmanager_base`、`cellular_data`。
- 系统和电源：`power_manager`、`battery_manager`、`thermal_manager`、`memory_utils`、`memmgr`、`device_standby`、`background_task_mgr`、`resource_schedule_service`、`qos_manager`。
- 第三方和工具：`libexif`、`libxml2`、`zlib`、`jsoncpp`、`cJSON`、`libuv`、`icu`、`i18n`、`c_utils`、`bounds_checking_function`、`bundle_framework`、`common_event_service`、`form_fwk`、`hiappevent`、`hicollie`、`hilog`、`hisysevent`、`hitrace`、`resource_management`、`camera_framework`、`core_service`、`usb_manager`、`drivers_interface_usb`、`window_manager`。

改动触达上述依赖的接口、枚举、buffer 语义、错误码或能力查询时，不要只在本仓内闭环；需要检查依赖方公开头文件、运行时能力和调用方假设，并在提交说明中写明跨仓影响和验证方式。

## 验证

按改动范围选择最近的测试目标。涉及对外接口或 API 行为时，还需要验证对应 XTS 用例。不要声称已完成完整验证；应记录缺失原因和仍需补充的验证项，并等待人工确认是否继续 push。

### 编译命令

```bash
NextBuild --cache ./build_system.sh --abi-type generic_generic_arm_64only --device-type general_all_phone_standard --ccache --build-variant root --gn-flags=--export-compile-commands -j50 --gn-args use_cfi=false --gn-args allow_sanitize_debug=true --disable-post-build --gn-args fwk_no_hidden=true --build-target medialibrary_data_extension media_library_manager media_library_cloud_sync media_library_common media_library_client media_library_handler media_permission_helper media_library_camera_helper media_share_album_lite analysis_data_kits medialibrary userfilemanager photoaccesshelpernative photopickercomponent albumpickercomponent sendablephotoaccesshelper recentphotocomponent native_media_asset_manager cj_photoaccesshelper_ffi media_library_ani medialibrary_ext_hap media_backup_package media_mtp_package scanner mediatool
```

完整目标列表见 `bundle.json` 的 `build.group_type`（`fwk_group` 为 API 绑定层，`service_group` 为服务/工具层，`base_group` 为参数配置）。按改动范围选取子集编译即可。

### 构建产物

| 产物 | 说明 |
|---|---|
| `libmedialibrary_data_extension.z.so` | DataShare 扩展服务（资产/相册/文件/照片/音频/PTP 操作主入口） |
| `libmedialibrary.z.so` | JS NAPI 公共接口库（FileAsset/PhotoAlbum/FetchFileResult） |
| `libuserfilemanager.z.so` | UserFileManager NAPI 接口库 |
| `libphotoaccesshelpernative.z.so` | PhotoAccessHelper 底层 NAPI 接口库 |
| `libphotopickercomponent.z.so` / `libalbumpickercomponent.z.so` / `librecentphotocomponent.z.so` | Picker 组件库 |
| `libsendablephotoaccesshelper.z.so` | Sendable 跨线程 PhotoAccessHelper 库 |
| `libnative_media_asset_manager.z.so` | C API 原生资产管理库 |
| `libcj_photoaccesshelper_ffi.z.so` | 仓颉（CJ）FFI 绑定库 |
| `libmedia_library_ani` 系列 | ANI/Taihe 绑定库 |
| `libmedia_library_manager.z.so` | `MediaLibraryManager`/`MediaLibraryExtendManager` 内部实现 |
| `libmedia_library_cloud_sync.z.so` | 云同步客户端（MDK record、cloud photo/album handler） |
| `libmedia_library_handler.z.so` | 媒体库 handler |
| `libmedia_library_common.z.so` / `libmedia_library_client.z.so` | 公共工具和客户端 IPC |
| `libmedia_permission_helper.z.so` | 权限校验 helper |
| `libmedia_library_camera_helper.z.so` | 相机联动 helper（`MediaPhotoAssetProxy`、拍照落盘） |
| `libmedia_share_album_lite.z.so` | 共享相册数据客户端 |
| `libanalysis_data_kits.z.so` | 分析数据 kit |
| `libmediabackup.z.so` | 备份扩展 |
| `medialibrary_ext_hap` | 媒体库扩展 HAP |
| `media_backup_package` / `media_mtp_package` | 备份扩展 HAP / MTP HAP |
| `scanner` / `mediatool` | 扫描工具 / 媒体工具二进制 |

### 验证命令

| 场景 | 命令 | 说明 |
|---|---|---|
| 全量编译 | 见上方编译命令 | 编译全部目标 |
| 增量编译 | 去掉 `--cache` 参数 | 仅编译变更文件 |
| 清理后编译 | 在编译命令前加 `clean` | 清理 build 目录后重新编译 |
| 单元测试 | 编译 `test` 目标后执行 | 见 `frameworks/innerkitsimpl/test/` 下各模块单元测试 |
| 模糊测试 | 编译 `media_library_fuzztest` 目标后执行 | 见 `frameworks/innerkitsimpl/test/fuzztest/` 下各 fuzz 目标 |

任务级验证参考：

| 改动类型 | 近端验证 | 额外要求 |
|---|---|---|
| 文档、知识路由、注释 | 检查链接、路径、术语和代码锚点是否存在 | 不改行为时通常不需要额外验证 |
| C++ 内部实现 | 对应模块最近单元测试 | 关注错误码、日志、资源释放和异常路径 |
| 公开 API 或多语言绑定 | 对应 JS/C API/CJ/ANI 单元测试 | 必须验证或说明对应 XTS；检查错误码和默认值兼容 |
| DataShare 操作、URI/命令分发、TLV | 对应 datashare/操作类单元测试 + 相关 fuzz | 补跑相关 fuzz，覆盖畸形 URI、空 bucket、越界参数和异常 TLV |
| 云同步、MDK record、备份恢复 | 对应 cloud/backup 单元测试 + 相关 fuzz | 条件不具备时记录缺口并等待人工确认 |
| 扫描、文件监听、FUSE、文件管理 | 对应 scanner/file_monitor 单元测试 | 关注文件系统行为和 inotify 路径覆盖 |
| 数据库 schema、RDB store、升级迁移 | 对应 rdbstore/upgrade 单元测试 | 重点检查升级兼容、字段映射和脏数据；新增版本号需补跑版本覆盖对比验证，确认版本号连续无缺口、兜底逻辑覆盖全区间 |
| MTP/PTP、USB | 对应 MTP 单元测试 + fuzz | 关注协议处理和数据同步路径 |
| 权限、URI 权限、敏感操作、安全修复 | 对应单元测试 + 相关 fuzz | 重点检查越权、绕过、fd 泄漏和跨进程传输 |

XTS 用例不在本仓完整维护。涉及公开 API、错误码、默认值、权限、异常类型、跨语言行为或兼容性时，必须查 OpenHarmony XTS 仓、CI 配置或团队用例映射；查不到时，在最终回复中明确写"XTS 目标未确认"，并列出已跑的本仓单元测试/fuzz 和需要人工补充确认的 API 场景。

## 提交和推送

以下为 Agent 提交约定（与人工提交风格可能不同，人工提交按团队现有规范执行）。提交建议使用 `git commit -s` 自动生成 `Signed-off-by`，其姓名和邮箱来自 `git config user.name` 与 `git config user.email`，格式类似 `Signed-off-by: your-name <your-name@example.com>`。同时在 commit message 末尾额外空一行写入 `Co-Authored-By: Agent`：

```text
<type>(<scope>): <summary>

<body，可选>

Signed-off-by: <name> <email>

Co-Authored-By: Agent
```

没有明确项目要求时，`type` 优先使用 `fix`、`feat`、`refactor`、`test`、`docs`、`build`，`scope` 使用模块名或目录名。若关联 issue、缺陷单或需求单，在 body 中写清编号和影响范围。

### Issue、PR 与门禁闭环

用户要求完成推送、Issue/PR 和门禁时，按以下流程推进；只要求某一步时按授权范围执行。下文 `<仓库>` 指上游 `owner/repo`，占位符须按实际替换。

1. **准备**：用 `git status --short`、`git remote -v`、`git branch --show-current` 核对工作区、fork、上游和分支；检查 `oh-gc --version`、`oh-gc auth status`。
2. **Issue**：用 `oh-gc issue list --search "<关键词>" --state all --repo <仓库>` 查重；需新建时执行 `oh-gc issue create --repo <仓库> --title "<标题>" --body "<说明>" --json`。说明包含问题、原因、修复范围和验证缺口；记录编号和链接，用于提交说明及 PR 关联。
3. **提交推送**：执行 `git diff --check`，用 `git add -- <本次文件>` 精确暂存、`git diff --cached` 复核，再执行 `git commit -s -F <提交说明文件>` 和 `git push -u <fork-remote> HEAD:refs/heads/<分支>`。按上文保留两个 trailer，用 `git log -1 --format=full` 核对 SHA 和签名。CRLF 文件用 `git -c core.whitespace=cr-at-eol diff --check` 检查。
4. **PR 创建与关联**：
   - 模板：`oh-gc file raw .gitcode/PULL_REQUEST_TEMPLATE.md <目标分支> --repo <仓库>`；存在时按模板填写，确认不存在时自行组织说明。
   - 创建：`oh-gc pr create --repo <仓库> --head <fork-owner>:<分支> --base <目标分支> --title "<标题>" --body "<说明>" --json`。
   - 关联：`oh-gc pr link <PR编号> <Issue编号> --repo <仓库> --json`。
   - 核对：分别执行 `oh-gc pr view`、`oh-gc pr files`、`oh-gc pr linked-issues`，均追加 `<PR编号> --repo <仓库> --json`，确认源仓库、分支、SHA、文件范围和关联结果。
5. **触发门禁**：确认 PR 已收到最新 SHA，再执行 `oh-gc pr comment <PR编号> --repo <仓库> --body 'start build'`；已有本轮构建时直接跟踪。用 `oh-gc pr comments <PR编号> --repo <仓库> --latest --limit 10 --full-body --json` 获取报告，核对报告对应的 SHA。
6. **修复重跑**：按具体 CodeCheck、编译或测试错误修复并验证，重复第 3 步向同一分支追加签名提交；用 `oh-gc pr update <PR编号> --repo <仓库> --body "<更新后的说明>"` 更新记录，再按第 5 步重跑，直到最新提交门禁通过。无法自行解决的阻塞须说明原因和待处理事项。

多行正文在 PowerShell 中用 `Get-Content -Raw` 读取后传给 `--body`；`oh-gc pr comments` 提供报告入口，具体错误需读取对应 CI 报告。

同一任务、同一影响范围内，沿用用户已确认的检视选择；新增影响范围则按下文确认。不得通过删测试、屏蔽检查或 `oh-gc pr review/test` 手工标记代替 CI，通过后不自动合并 PR。

最终提供 Issue/PR 链接、最新 SHA、门禁结果和验证缺口。准确区分实际通过、`IGNORE`、`NA` 和未执行；编译成功，或 `Upgrade only` 冒烟通过，均不代表新增用例已执行。

## 知识路由

改动前按场景定位对应代码锚点和验证目标。锚点分两层：目录锚点（宽定位，定位到目录而非具体文件）+ 功能锚点（grep 命中，类名/方法名/枚举/稳定文件名词干）；两层结合后，即使文件被改名、拆分或移动，锚点仍可命中。标 † 的功能锚点暂为文件名词干/关键词，待模块负责人确认为类名。

| 场景 | 目录锚点（宽定位） | 功能锚点（grep 命中） | 验证重点 |
|---|---|---|---|
| DataShare 扩展服务、资产/相册/文件/照片/音频/PTP 操作、命令和 URI 分发 | `frameworks/innerkitsimpl/medialibrary_data_extension/`（扩展服务 + 操作类）, `frameworks/innerkitsimpl/media_library_helper/`（命令/URI/枚举） | `MediaDataShareExtAbility`, `MediaLibraryDataManager`, `MediaLibraryCommand`, `MediaLibraryAssetOperations`, `MediaLibraryPhotoOperations`, `MediaLibraryAlbumOperations`, `MediaLibraryFileOperations`, `OperationObject`, `OperationType` | `medialibrary_datamanager_test`、`media_datashare_ext_ability_test`、`medialibrary_photo_operations_test`、`medialibrary_album_operation_test`、对应操作类 fuzz |
| JS/NAPI、PhotoAccessHelper、UserFileManager、Picker 组件、Sendable、错误码映射、API 兼容 | `interfaces/kits/js/`, `frameworks/js/` | `FileAsset`, `PhotoAlbum`, `FetchFileResult`, `PhotoAccessHelper`, `UserFileManager`, `Sendable` † | 对应 NAPI 单元测试和 XTS |
| C API、`MediaAssetManager`、`MovingPhoto`、`MediaAssetChangeRequest`、CJ、ANI、多语言绑定 | `interfaces/kits/c/`, `interfaces/kits/cj/`, `frameworks/ani/`, `frameworks/native/c_api/`, `frameworks/native/media_library_asset_manager/` | `MediaAssetManager`, `MovingPhoto`, `MediaAssetChangeRequest` | `media_library_asset_helper_capi_test`、`media_library_asset_manager_test`、`cj_photoaccesshelper_ffi` 测试、对应 XTS |
| 云媒体数据同步、MDK record、cloud photo/album handler、云下载上传、云增强、备份恢复、克隆、自定义恢复 | `frameworks/innerkitsimpl/media_library_cloud_sync/`, `interfaces/inner_api/native/cloud_sync/`, `services/media_cloud_sync_service/`, `services/media_cloud_enhancement/`, `services/media_backup_extension/`, `services/media_custom_restore/`, `frameworks/innerkitsimpl/media_library_manager/` | `cloud_media_photo_handler`, `mdk_record_photos_data`, `cloud_sync_data_convert`, `media_library_custom_restore` † | `media_library_cloud_sync_test`、`media_library_cloud_sync_service_test`、`medialibrary_cloud_enhancement_test`、`medialibrary_backup_test`、`medialibrary_restore_test`、`medialibrary_custom_restore_test`、`medialibrary_backup_clone_test`、对应 cloud fuzz |
| 媒体扫描、文件监听（inotify/lake）、文件管理、FUSE 文件系统 | `tools/medialibrary_scanner/`, `services/media_scanner/`, `services/media_file_monitor/`, `services/media_file_scan/`, `services/media_fuse/`, `services/media_file_management/`, `frameworks/services/media_file_manager/` | `MediaScanner`, `FileMonitor`, `MediaFuse`, `inotify`, `lake` † | `medialibrary_scanner_test`、`mediascanner_test`、`medialibrary_fuse_test`、`media_file_management_test`、`media_lake_load_test`、`media_lake_file_monitor_test` |
| RDB store、数据库 schema/常量、升级迁移、meta recovery | `services/media_rdbstore/`, `interfaces/inner_api/media_library_helper/include/` | `MediaLibraryRdbStore`, `medialibrary_db_const`, `media_upgrade`, `media_column`, `medialibrary_meta_recovery` † | `medialibrary_rdb_test`、`medialibrary_rdb_callback_test`、`medialibrary_rdb_utils_test`、`medialibrary_upgrade_schema_test`、`medialibrary_upgrade_schema_cover_record_test`、`medialibrary_meta_recovery_test` |
| MTP/PTP 协议、USB 媒体传输 | `services/media_mtp/`, `frameworks/innerkitsimpl/medialibrary_data_extension/src/`, `interfaces/inner_api/media_library_helper/include/` | `medialibrary_ptp_operations`, `ptp_medialibrary_manager_uri` † | `medialibrary_mtp_test`、`medialibrary_ptp_operations_test`、`mtp_native_test`、MTP 系列 fuzz |
| 缩略图、相册封面、封面刷新、共享相册 | `services/media_thumbnail/`, `services/media_refresh/`, `services/media_albums_manager/`, `services/media_share_album_service/`, `frameworks/innerkitsimpl/media_share_album_lite/`, `frameworks/services/media_albums_refresh/`, `frameworks/innerkitsimpl/medialibrary_data_extension/src/`, `interfaces/inner_api/media_library_helper/include/` | `Thumbnail`, `AlbumCover`, `ShareAlbum`, `medialibrary_all_album_refresh_processor`, `photo_album_column` † | `medialibrary_thumbnail_service_test`、`media_albums_refresh_test`、`medialibrary_album_cover_order_test`、`media_refresh_cover_order_test`、`media_library_share_test`、`media_share_album_lite_test` |
| 权限、URI 权限、敏感操作、安全校验 | `frameworks/services/media_permission/`（责任链处理器）, `services/media_permission/`（PermissionCheck 注册表 + 场景检查类）, `frameworks/utils/`（PermissionUtils/白名单/隐私管理器）, `frameworks/innerkitsimpl/medialibrary_data_extension/src/`（URI 权限/敏感操作） | `AbsPermissionHandler`, `PermissionCheck`, `VerifyPermissions`, `PermissionUtils`, `CheckCallerPermission`, `CheckPhotoCallerPermission`, `UriPermissionOperations`, `UriSensitiveOperations`, `MediaPrivacyManager` | `media_permission_check_test`、`media_permission_helper_test`、`medialibrary_uri_sensitive_operations_test`、`medialibrary_app_uri_permission_operations_test`、`get_self_permissions`、对应权限 fuzz |
| 分析数据、相册分析、分析扩展 | `frameworks/innerkitsimpl/analysis_data_kits/`, `services/media_analysis_data_manager/`, `services/media_analysis_extension/`, `interfaces/inner_api/analysis_data_kits/include/` | `AnalysisData`, `active_analysis`, `analysis_album` † | `media_analysis_data_manager_dto_test`、`media_analysis_data_service_test`、`media_analysis_extension_test`、`medialibrary_analysis_album_operation_test`、`medialibrary_analysis_progress_test` |
| TLV 序列化、Parcel、fuzz、安全解析、异常输入 | `frameworks/innerkitsimpl/medialibrary_data_extension/src/`（TLV 实现）, `frameworks/innerkitsimpl/test/fuzztest/`（fuzz 目标）, `frameworks/utils/include/`（错误码/工具）, `frameworks/client/media_ipc_common/`（IPC 序列化） | `TlvUtil`, `Unmarshal`, `Marshal`, `E_FAIL`, `medialibrary_errno` † | 对应 fuzz 目标、截断/畸形/超大输入样例、相关单元测试和 XTS |

术语路由：

| 触发词 | 重点 |
|---|---|
| DataShare、`DatashareExtAbility`、asset operation、album operation、photo operation、file operation、audio operation、PTP、command、URI 分发 | DataShare 入口、URI/命令解析、操作类生命周期和脏数据 |
| `PhotoAccessHelper`、`UserFileManager`、napi、picker、`FileAsset`、`PhotoAlbum`、`FetchFileResult`、Sendable、错误码 | JS/NAPI 接口一致性、错误码映射、XTS 兼容 |
| `MediaAssetManager`、`MediaAssetChangeRequest`、`MovingPhoto`、C API、CJ、ANI、Taihe | 多语言接口一致性、ABI、错误码映射 |
| 云同步、cloud sync、MDK record、cloud download、cloud upload、cloud enhancement、备份、backup、恢复、restore、克隆、clone、自定义恢复、custom restore | 云数据转换、record 解析、下载/上传生命周期、恢复兼容 |
| scanner、扫描、inotify、file monitor、lake、FUSE、文件管理、file manager | 扫描策略、文件监听、FUSE 挂载、文件系统行为 |
| RDB、schema、`db_const`、升级、upgrade、迁移、migration、meta recovery、脏数据、dirty | 表结构、字段映射、升级兼容、脏数据处理 |
| MTP、PTP、USB、媒体传输 | 协议处理、设备管理、数据同步 |
| 缩略图、thumbnail、封面、cover、refresh、共享相册、share album、相册管理 | 缩略图生成、封面刷新、相册生命周期 |
| 权限、permission、URI 权限、敏感操作、sensitive、`app_uri_permission`、`urisensitive`、隐私、privacy | 权限校验、URI 授权、敏感操作、绕过防护 |
| 分析、analysis、`analysis_data`、相册分析、`active_analysis` | 分析数据流转、相册分析、扩展通信 |
| TLV、Parcel、unmarshal、fuzz、越界、溢出、截断、恶意输入 | 不可信输入、安全攻击面、fuzz 和跨进程传输 |

## 项目约束

不要做：

- 不要在 DataShare URI 解析、数据库查询、文件扫描、TLV 序列化或权限校验的热点路径中增加全量扫描、重复大内存拷贝、字符串格式化或高频 INFO 日志。
- 不要只改某一层语言绑定来改变公开行为；资产类型、相册类型、媒体列、错误码、权限校验、URI 语义会影响 JS、C API、Native、CJ、ANI、Taihe 等外部接口。
- 不要只改 DataShare stub 或操作类中的一个入口来声明资产/相册能力；`media_datashare_ext_ability.cpp`、`medialibrary_data_manager.cpp`、`medialibrary_command.cpp` 和各操作类需保持一致。
- 不要把 `medialibrary_kit_whitelist.json`、`heif_transcoding_checklist.json` 或 `userfilemanager_mimetypes.json` 当作普通业务路径分析；它们是能力白名单和配置，改动需确认产品裁剪矩阵。
- 不要把动态照片的视频部分当作脏数据清理。动态照片在 Photos 表仅记录图片部分，视频部分与图片同目录桶存储但不单独建记录；脏数据清理逻辑判断"桶内物理文件在 Photos 表无记录即删除"时，必须排除动态照片视频部分，否则升级后历史动态照片视频丢失、无法播放。
- 不要为新增特性修改已有字段的赋值范围或语义。新增特性（如子弹时刻需要视频时长）应新增专用字段，不得复用已有字段（如 duration）改变其历史取值含义，否则依赖该字段判断媒体类型的三方应用会受影响。
- 不要执行破坏性 git/文件操作或大范围机械重构，除非用户明确要求。

Ask before / 必须人工确认：

以下场景不是普通"建议确认"，而是 Agent 在修改、提交或 push 前必须通过的门禁。触发后要向用户或模块责任人说明影响面、已读代码锚点、计划改动和拟验证项，得到明确答复后再继续。

- 改公开 API/ABI、枚举值、结构体字段、错误码、默认值或 XTS 预期前，先确认兼容策略。
- 改 JS/C API/CJ/ANI/Taihe 绑定、生成代码或接口命名时，先确认所有语言入口是否需要同步。
- 改数据库 schema、字段、表结构或升级迁移逻辑时，先确认升级兼容和脏数据处理策略。新增数据库版本号必须排在现有版本号末尾，不得占用或跳过已有版本号；确认兜底逻辑覆盖所有版本区间。
- 改 DataShare URI 语义、命令分发、操作类接口或跨进程 IPC 协议时，先确认调用方假设和兼容。
- 改 `data_share`、`relational_store`、`ability_runtime`、`access_token`、`ipc`、`image_framework`、`camera_framework` 等跨仓接口或 buffer 语义时，先确认依赖方公开头文件、调用方假设和跨仓验证方式。
- 改云同步、MDK record、备份恢复、克隆逻辑时，先确认生命周期、数据转换和冲突处理。
- 改权限校验、URI 权限、敏感操作或安全修复时，先确认攻击面、绕过防护、兼容策略和回归用例。
- 改 FUSE 挂载、文件监听、扫描策略或文件系统行为时，先确认 fallback 策略。
- 改脏数据清理逻辑（判断条件、启动时机、处理策略）时，先确认对动态照片视频部分、云增强复合图、一键还原和接续编辑等附属文件的影响，避免误删历史资产。
- 改上述行为时，要同步检查错误码和各语言接口映射，包括 `frameworks/utils/include/medialibrary_errno.h`、`frameworks/utils/include/medialibrary_client_errno.h`、`interfaces/inner_api/media_library_helper/include/media_library_error_code.h`、`interfaces/kits/js/include/napi_error.h` 以及对应 C API/CJ/NAPI 适配代码。

C++ 改动优先复用附近的项目宏、错误码和日志习惯，包括 `frameworks/utils/include/medialibrary_errno.h`（`E_FAIL`、`E_DB_FAIL`、`E_INVALID_VALUES` 等）、`frameworks/utils/include/medialibrary_common_log.h`（日志宏）和 `frameworks/utils/include/medialibrary_common_utils.h`。

## 完成定义

Agent 最终回复必须包含：

- 修改的文件、行为影响面和明确未修改的关键文件。
- 已执行的单元测试、fuzz、XTS 验证命令；未执行时说明原因。
- XTS 目标无法确认时，列出缺口和需要人工确认的问题。
- 若涉及提交或 push，说明 commit message 是否包含 `Signed-off-by` 和 `Co-Authored-By: Agent`。
