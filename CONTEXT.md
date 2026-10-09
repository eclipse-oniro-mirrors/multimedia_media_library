# 媒体库（Media Library）

管理设备上照片/视频资产的元数据（`Photos` 表）、文件本体与缩略图，并提供云同步、克隆恢复、DFX 上报等能力的子系统。

## 语言

### 诊断与上报

**DFX**：
媒体库的诊断/可观测性子系统，由 `DfxCollector` → `DfxAnalyzer` → `DfxReporter` → `DfxManager`（含 `DfxWorker`/`DfxTimer`）构成，通过 `HiSysEventWrite` 把统计/故障事件上报到 hiview 云端分析平台；按 5 分钟 / 6 小时 / 12 小时 / 1 天 / 2 天 / 1 周 六档节拍调度。
_Avoid_: 监控、埋点（口语化、不精确）

**异常类型（PhotoErrorType）**：
数据库与文件不一致的分类，经 `MEDIALIB_PHOTO_ERROR_STATISTIC` 事件、按 2 天节拍上报；type 码以维度组合编码。子类型分两类观测：**状态型**（缺文件按缩略图是否残存再分两子类、大小不一致，由 2 天库存扫描发现；缩略图残存的缺文件子类与退役的旧 `PHOTO_MISS_TYPE=1` 语义连续）与**访问补救型**（打开时为掩盖缺失而补建空文件等副作用，由调用点采集并入同一 2 天统计）；同一缺失症状可被两类各计一次（分开计数，非互斥）。
_Avoid_: 错误码（混淆概念）

### 资产状态

**对外可见资产（externally-visible asset）**：
满足 `sync_status = 0 AND clean_flag = 0 AND time_pending = 0 AND is_temp = 0` 的 `Photos` 记录——已提交、未标记清理、未在途、非临时；只有这类资产对用户可见，DFX 一致性检查只圈定该范围。
_Avoid_: 正常资产、有效资产（含义模糊）

**动态照片（moving photo）**：
`subtype = 3 (MOVING_PHOTO)` 的照片，磁盘上由三个文件组成：image（主路径，由 `data`/`storage_path` 解析）、video、extradata；总大小 = 三者之和（`GetMovingPhotoSize`）。`Photos` 表无 extradata 列。
_Avoid_: 动图、Live Photo（前者泛指动画，后者是另一种打包格式）

### 资产维度

**文件状态（position）**：
资产字节当前的存在位置——`1`=本地（仅本地有原图）、`2`=纯云（仅云端有）、`3`=端云（本地与云端皆有）。
_Avoid_: 位置（歧义）

**南向设备类型（south_device_type）**：
本地资产的**来源**（从哪同步来）——`0`=本地原生（含 `VISIT(-1)` 归一）、`1`=云、`2`=家庭存储（HDC）。用于统计"哪类来源的资产出了问题"。
_Avoid_: 设备类型（泛化）、来源（与 file_source_type 混淆）

**照片来源类型（file_source_type）**：
资产文件的存储来源，决定主文件路径解析——`0`=MEDIA（主文件在 `data`）、`1`=FILE_MANAGER（主文件在 `storage_path`）、`3`=MEDIA_HO_LAKE（主文件在 `storage_path`）。
_Avoid_: 来源类型（与 south_device_type 混淆）

**媒体类型（media_type）**：
资产内容类型——`1`=图片、`2`=视频。
_Avoid_: 类型（泛化）

### 时间信息（维测）

DFX 维测中的时间信息来自**两个来源**：数据库（下列 5 列，`BIGINT`/ms）与文件（`stat`/`lstat` 的 atime/ctime/mtime）。两来源独立，勿混。

**DB 时间列（date_added / date_taken / date_modified / meta_date_modified / change_time）**：
`Photos` 表的时间列——`date_added`←文件 `st_atim`（入库）、`date_modified`←`st_mtim`（内容修改）、`date_taken`←EXIF 拍摄时间、`meta_date_modified`←元数据级变更、`change_time`←综合变更触发列（升级时由 `date_modified` 播种）。
_Avoid_: 与文件 atime/ctime/mtime 混淆

**文件 atime / ctime / mtime（filesystem）**：
磁盘字节层时间，取自 `struct stat` 的 `st_atim`/`st_ctim`/`st_mtim`——atime=访问、ctime=inode 变更、mtime=内容修改；与 DB 列 `change_time` 无关（atime 与 DB `date_added` 的入库快照对应）。
_Avoid_: 与 DB change_time 混淆
