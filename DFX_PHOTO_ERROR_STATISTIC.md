# 图片错误统计 DFX 串讲文档

> 适用分支：`bugfix/dfx/add_file_size_check-merge`
> 功能：检测照片文件在磁盘上的实际状态与数据库记录（`MEDIA_SIZE`）之间的一致性，统计异常场景与数量并上报 HiSysEvent。

---

## 1. 功能概述

本 DFX 用于发现"图库 DB 有记录，但磁盘文件异常"的图片，并通过 HiSysEvent 上报各类异常的累计计数。共设计 **两条采集路径**，最终汇入同一上报事件 `MEDIALIB_PHOTO_ERROR_STATISTIC`：

| 路径 | 名称 | 触发方式 | 采集的错误子类型 |
|------|------|----------|------------------|
| A | 定时扫描统计（Scan） | 每 2 天定时，DB 批量扫描 | FILE_NOT_EXIST_THUMB_EXIST / FILE_NOT_EXIST_THUMB_NOT_EXIST / FILE_SIZE_ZERO / FILE_SMALLER_THAN_DB / FILE_LARGER_THAN_DB |
| B | 实时采集（Open） | 打开文件时创建空文件即记 1 次 | OPEN_CREATE_EMPTY_FILE |

---

## 2. 数据流

```
路径A(扫描)                            路径B(打开)
  │                                       │
  │ DfxPhotoErrorDao::QueryBatch          │ OpenAsset(非API10)
  │ (Photos表, 条件见§4)                   │ → 文件不存在→CreateDirectoryAndAsset
  ↓                                       │ → AddPhotoErrorDfx
  DfxPhotoErrorProcessor                   ↓
  ::GetPhotoErrorBatches                   DfxManager::HandlePhotoError
  │  ├ ResolveRealPath                     │  ├ Encode(OPEN_CREATE_EMPTY_FILE)
  │  ├ GetFileSize / GetMovingPhotoSize    │  └ DfxCollector::CollectPhotoError(内存map累加)
  │  ├ IsThumbnailExists(row.data)         │     + 异步 AuditLog
  │  ├ ClassifyPhotoError(exists,thumb)    │
  │  └ Encode → typeToCount++              │
  ↓                                       │
  PackPhotoErrors(packBatchSize=5) ←──────┘ CheckPhotoErrorInOpen 取出 collector 累计值
  │                                       
  ↓                                       
  DfxReporter::ReportPhotoError           
  → HiSysEvent: MEDIALIB_PHOTO_ERROR_STATISTIC
```

---

## 3. 统计时机

### 3.1 路径 A：定时扫描（每 2 天）

- 入口：`DfxManager::HandleTwoDayMissions()`（`dfx_manager.cpp:707`）
- 触发条件：`当前UTC秒 - 偏好键 LAST_TWO_DAY_REPORT_TIME > TWO_DAY(48h)`
- 执行方式：通过 `DfxWorker` 异步执行 `HandleCheckTwoDayTask`（`dfx_manager.cpp:662`），调用 `CheckPhotoError`（`dfx_manager.cpp:1721`）
- 关键常量：
  - `PHOTO_ERROR_SCAN_BATCH_SIZE = 500`（每批查询/扫描条数）
  - `PHOTO_ERROR_PACK_BATCH_SIZE = 5`（一次上报的 type 数组长度上限）
- 节流：上报成功后更新 `LAST_TWO_DAY_REPORT_TIME` 偏好。

### 3.2 路径 B：实时采集（打开即记录）

- 入口：`MediaLibraryAssetOperations::OpenAsset`（`medialibrary_asset_operations.cpp:1830`）
- 触发条件：**API < 10** 的打开路径中，目标文件不存在 → `CreateDirectoryAndAsset` 成功创建空文件后，调用 `AddPhotoErrorDfx`（`medialibrary_asset_operations.cpp:1807`）→ `DfxManager::HandlePhotoError`（`dfx_manager.cpp:1701`）
- 采集动作：
  1. 编码维度（子类型固定为 `OPEN_CREATE_EMPTY_FILE`）→ `DfxCollector::CollectPhotoError` 在内存 `photoErrorMap_` 中累加计数；
  2. 同时投递一个异步 `DfxTask` 写 `HiAudit` 审计日志（`LogPhotoError`，`dfx_manager.cpp:1687`）。
- 注意：此处只**采集累积**，**不立即上报**。上报发生在路径 A 下次触发时由 `CheckPhotoErrorInOpen` 一并打包上报。

> API10 路径（`fileAsset->GetPath()` 分支）不触发本 DFX。

---

## 4. 统计范围

### 4.1 扫描范围（路径 A 的 SQL）

`DfxPhotoErrorDao::QueryBatch`（`dfx_photo_error_dao.cpp:39`）执行：

```sql
SELECT file_id, data, storage_path, file_source_type, media_type,
       south_device_type, position, size, subtype,
       moving_photo_effect_mode, original_subtype
FROM Photos
WHERE file_id > ?            -- 游标分页（lastFileId）
  AND sync_status = 0
  AND clean_flag = 0
  AND time_pending = 0
  AND is_temp = 0
  AND position IN (1, 3)    -- LOCAL=1, LOCAL_AND_CLOUD=3
ORDER BY file_id LIMIT ?
```

即只统计**本地侧（LOCAL / LOCAL_AND_CLOUD）、非同步中、非回收站、非 pending、非临时**的照片；纯云端（CLOUD=2）不在扫描范围。

### 4.2 路径解析

`DfxPhotoErrorHelper::ResolveRealPath`（`dfx_photo_error_helper.cpp:60`）决定每条记录用哪个路径取磁盘大小：

| fileSourceType | 真实路径取值 |
|----------------|--------------|
| FILE_MANAGER(1) / MEDIA_HO_LAKE(3) | `storage_path` |
| 其它（MEDIA=0、PERIPHERAL=2、…） | `data` |

若解析得到的路径为空（如 FILE_MANAGER 但 storage_path 为空），该行**跳过不统计**。

### 4.3 磁盘大小获取

- 普通照片：`MediaFileUtils::GetFileSize(path)`
- 动态照片（`MovingPhotoFileUtils::IsMovingPhoto` 判定为真）：`MovingPhotoFileUtils::GetMovingPhotoSize(path)`（主图+视频合并大小）

### 4.4 缩略图残存检查

当文件不存在（`!exists`）时，`ProcessRow` 调用 `PhotoFileUtils::IsThumbnailExists(row.data)` 检查缩略图是否残存：

- **检查路径**：始终使用 `row.data`（即 `MEDIA_FILE_PATH` 列），而非 `ResolveRealPath` 解析后的路径。缩略图目录由 `data` 路径派生（`.thumbs/{相对路径}/LCD.jpg` 或 `THM.jpg`），与文件解析路径可能不同（FILE_MANAGER / MEDIA_HO_LAKE 场景下文件在 `storage_path`，缩略图仍按 `data` 派生）。
- **惰性求值**：仅在 `!exists` 时才调 `IsThumbnailExists`，避免对一致文件产生多余 I/O。
- **结果传入** `ClassifyPhotoError` 的 `thumbExists` 参数，用于区分 `FILE_NOT_EXIST_THUMB_EXIST` 与 `FILE_NOT_EXIST_THUMB_NOT_EXIST`。

---

## 5. 统计内容

### 5.1 错误子类型（`PhotoErrorType`）

定义见 `dfx_photo_error_helper.h:29`：

| 值 | 枚举 | 判定（`ClassifyPhotoError`） |
|----|------|------------------------------|
| 0 | CONSISTENT | 文件存在且 diskSize == dbSize（一致，不统计/不上报） |
| 1 | FILE_NOT_EXIST_THUMB_EXIST | 文件不存在且缩略图残存（与旧 `PHOTO_MISS_TYPE=1` 定义值一致） |
| 2 | FILE_NOT_EXIST_THUMB_NOT_EXIST | 文件不存在且缩略图亦不存在 |
| 3 | FILE_SIZE_ZERO | 文件存在但 diskSize == 0 |
| 4 | FILE_SMALLER_THAN_DB | 文件存在且 diskSize < dbSize |
| 5 | FILE_LARGER_THAN_DB | 文件存在且 diskSize > dbSize |
| 6 | OPEN_CREATE_EMPTY_FILE | 打开时创建空文件（仅路径 B） |

`ClassifyPhotoError` 签名：`ClassifyPhotoError(bool fileExists, bool thumbExists, int64_t diskSize, int64_t dbSize)`

判定顺序（优先级从高到低）：
1. `!fileExists` → 按 `thumbExists` 分流：`true` → `FILE_NOT_EXIST_THUMB_EXIST`，`false` → `FILE_NOT_EXIST_THUMB_NOT_EXIST`
2. `diskSize == 0` → `FILE_SIZE_ZERO`
3. `diskSize < dbSize` → `FILE_SMALLER_THAN_DB`
4. `diskSize > dbSize` → `FILE_LARGER_THAN_DB`
5. 以上均不满足 → `CONSISTENT`

> `FILE_NOT_EXIST_THUMB_EXIST=1` 与退役的旧 `PHOTO_MISS_TYPE=1`（"缩略图在、原文件丢"）定义值一致，保证迁移前后码值连续。

### 5.2 维度与编码规则

每条异常最终编码成一个 `int32_t typeCode`，由 5 个维度按位权相加：

```
typeCode = fileSourceType   × 1,000,000
         + southDeviceType  ×   100,000   // 归一化后
         + position         ×    10,000
         + mediaType         ×     1,000
         + errorSubtype      ×         1
```

实现见 `DfxPhotoErrorHelper::EncodePhotoErrorType`（`dfx_photo_error_helper.cpp:49`），基数常量见 `dfx_photo_error_helper.h:38`。

#### 5.2.1 各维度取值

| 维度 | 基数 | 取值（枚举 → 数值） |
|------|------|---------------------|
| fileSourceType | 1,000,000 | MEDIA=0, FILE_MANAGER=1, PERIPHERAL=2, MEDIA_HO_LAKE=3, TEMP_FILE_MANAGER=4, MEDIA_SHARE_ALBUM=5 |
| southDeviceType | 100,000 | **SOUTH_DEVICE_VISIT(-1)→归一化为0**, SOUTH_DEVICE_NULL=0, SOUTH_DEVICE_CLOUD=1, SOUTH_DEVICE_HDC=2 |
| position | 10,000 | LOCAL=1, CLOUD=2, LOCAL_AND_CLOUD=3（扫描范围仅含 1、3） |
| mediaType | 1,000 | MEDIA_TYPE_FILE=0, MEDIA_TYPE_IMAGE=1, MEDIA_TYPE_VIDEO=2, MEDIA_TYPE_AUDIO=3 … |
| errorSubtype | 1 | 见 §5.1 |

> 归一化：`NormalizeSouthDeviceType` 把 VISIT(-1) 折叠为 NULL(0)，避免负数污染编码（`dfx_photo_error_helper.cpp:38`）。其余维度当前为直通。

#### 5.2.2 编码示例

- 本地 MEDIA 图片、文件不存在且缩略图残存：`fst=0, sd=0, pos=1, mt=1, err=1` → `0×1e6 + 0×1e5 + 1×1e4 + 1×1e3 + 1 = 11001`
- 本地 MEDIA 图片、文件不存在且缩略图亦丢：`fst=0, sd=0, pos=1, mt=1, err=2` → `0×1e6 + 0×1e5 + 1×1e4 + 1×1e3 + 2 = 11002`
- MEDIA_HO_LAKE、HDC、LOCAL_AND_CLOUD、VIDEO、文件大于 DB：`3×1e6 + 2×1e5 + 3×1e4 + 2×1e3 + 5 = 3232005`

### 5.3 打包（Pack）

`PackPhotoErrors`（`dfx_photo_error_helper.cpp:83`）将 `map<int32_t typeCode, int32_t count>` 按升序、每 `packBatchSize` 个 type 切成一个 `PhotoErrorCount{types[], counts[]}`，形成多包。`packBatchSize` 默认 5（与 HiSysEvent 数组 `arrsize: 5` 对齐）。

---

## 6. 上报方式

### 6.1 事件定义

`hisysevent.yaml:180-183`：

```yaml
MEDIALIB_PHOTO_ERROR_STATISTIC:
  __BASE: {type: STATISTIC, level: CRITICAL, desc: ..., preserve: true}
  PHOTO_ERROR_TYPE:  {type: INT32, arrsize: 5, desc: User image fault type}
  PHOTO_ERROR_COUNT: {type: INT32, arrsize: 5, desc: Number of user image fault types}
```

### 6.2 上报实现

`DfxReporter::ReportPhotoError`（`dfx_reporter.cpp:710`）调用 `HiSysEventWrite`，事件域 `MEDIA_LIBRARY`、事件名 `MEDIALIB_PHOTO_ERROR_STATISTIC`、类型 `STATISTIC`，参数为数组对。

### 6.3 上报编排（`CheckPhotoError`，`dfx_manager.cpp:1721`）

1. `CheckPhotoErrorInScan`：构造 `DfxPhotoErrorProcessor`，`GetPhotoErrorBatches(500, 5)` 完整扫描后逐包上报；
2. `CheckPhotoErrorInOpen`：从 `DfxCollector::GetPhotoError()` 取出累计的 openMap（取出即清空），`PackPhotoErrors` 后逐包上报。

---

## 7. 中断保护机制

扫描循环（`dfx_photo_error_processor.cpp:36`）每轮检查 `MedialibrarySubscriber::IsCurrentStatusOn()`：

- 中途变为 false → 立即跳出循环，`completed=false`，`GetPhotoErrorBatches` 返回空 `{}`，**丢弃已累计的部分结果，本周期不上报**，等下次周期重新完整扫描。
- 目的：避免只上报"部分扫描"的计数造成统计口径不一致。日志：`"Scan interrupted, discard partial stat. lastFileId=%d"`。

---

## 8. 关键代码位置

| 模块 | 文件 | 说明 |
|------|------|------|
| 维度/分类/打包 | `frameworks/services/media_dfx/include/dfx_photo_error_helper.h`、`src/dfx_photo_error_helper.cpp` | Encode/Classify(exists,thumbExists,...)/Resolve/Pack |
| 扫描处理器 | `frameworks/services/media_dfx/include/dfx_photo_error_processor.h`、`src/dfx_photo_error_processor.cpp` | GetPhotoErrorBatches / ProcessRow（含 IsThumbnailExists 调用） |
| DB 查询 | `frameworks/services/media_dfx/include/dfx_photo_error_dao.h`、`src/dfx_photo_error_dao.cpp` | QueryBatch SQL |
| 缩略图检查 | `frameworks/innerkitsimpl/media_library_helper/include/photo_file_utils.h`、`src/photo_file_utils.cpp` | IsThumbnailExists（由 ProcessRow 调用） |
| 触发编排 | `frameworks/services/media_dfx/src/dfx_manager.cpp` | HandlePhotoError / CheckPhotoError / InScan / InOpen / HandleTwoDayMissions |
| 内存累计 | `frameworks/services/media_dfx/include/dfx_collector.h`、`src/dfx_collector.cpp` | CollectPhotoError / GetPhotoError |
| 上报 | `frameworks/services/media_dfx/src/dfx_reporter.cpp:710` | ReportPhotoError (HiSysEvent) |
| 打开触发点 | `frameworks/innerkitsimpl/medialibrary_data_extension/src/medialibrary_asset_operations.cpp:1807` | AddPhotoErrorDfx |
| 事件声明 | `hisysevent.yaml:180` | MEDIALIB_PHOTO_ERROR_STATISTIC |

---

## 9. 测试覆盖

测试位于 `frameworks/innerkitsimpl/test/unittest/medialibrary_dfx_test/`：

| 文件 | 覆盖点 |
|------|--------|
| `dfx_photo_error_helper_test.cpp` | 维度归一化（sd=-1→0）；编码往返 5 维度可解码；ResolveRealPath 全分支；ClassifyPhotoError 全分支（含 THUMB_EXIST / THUMB_NOT_EXIST 分流）+ 优先级；PackPhotoErrors 空/单/满包/多包/升序/自定义 batch |
| `dfx_photo_error_processor_test.cpp` | 空表无上报；文件不存在计为 FILE_NOT_EXIST_THUMB_NOT_EXIST（测试路径不在 ROOT_MEDIA_DIR 下，无缩略图）；解析路径为空的行被排除；同维度行计数累加；扫描中断丢弃部分结果 |
| `dfx_photo_error_open_test.cpp` | HandlePhotoError 采集 OPEN_CREATE_EMPTY_FILE；lake video 编码维度正确 |

---

## 10. 测试串讲验证要点

1. **统计时机**：路径 A 是否每 2 天触发一次（偏好节流）；路径 B 是否仅在 API<10 且文件不存在创建空文件时触发一次，API10 不触发。
2. **统计范围**：SQL 五个过滤条件（sync_status/clean_flag/time_pending/is_temp/position）是否生效；纯云端(position=2)不被扫描；FILE_MANAGER/MEDIA_HO_LAKE 用 storage_path、其它用 data；storage_path 为空时跳过。
3. **统计内容（分类正确性）**：
   - 不存在且缩略图残存 → FILE_NOT_EXIST_THUMB_EXIST（err=1，与旧 PHOTO_MISS_TYPE=1 一致）；
   - 不存在且缩略图亦丢 → FILE_NOT_EXIST_THUMB_NOT_EXIST（err=2）；
   - 存在但 0 字节 → FILE_SIZE_ZERO；
   - diskSize < dbSize → FILE_SMALLER_THAN_DB；
   - diskSize > dbSize → FILE_LARGER_THAN_DB；
   - 相等 → CONSISTENT（不上报）；
   - 动态照片按合并大小比对。
4. **缩略图检查**：仅在 `!exists` 时调 `IsThumbnailExists(row.data)`；始终用 `data` 路径而非解析后的 `storage_path`；惰性求值不浪费一致文件的 I/O。
5. **编码正确性**：5 维度按基数相加；sd=-1 归一化为 0；解码能还原各维度（见 helper_test 的 round-trip）。
6. **打包与上报**：每包最多 5 个 type（与 arrsize:5 对齐）；多包逐次 ReportPhotoError；open 与 scan 走同一事件。
7. **中断保护**：扫描中状态变 false，本周期返回空、不上报；下次周期从 file_id=0 重新完整扫描。
8. **Open 路径内存累加**：HandlePhotoError 只累加到 collector，不立即上报；在下次 CheckPhotoErrorInOpen 取出（取出即清空）后上报。
9. **审计日志**：路径 B 同时写一条 HiAudit（OPEN_EMPTY_CREATE），含脱敏 path/displayName/mediaType。
10. **边界**：空表无上报；解析路径为空的行被排除；同维度计数累加而非新增 type。
