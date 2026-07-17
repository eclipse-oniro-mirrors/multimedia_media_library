# 审计日志新增 mediaType 字符串列

## 背景

`AuditLog.type`（int32_t）是一个被多处复用的通用整数桶，语义随调用路径而变：用户行为删除路径（`dfx_manager`）存 `DfxType` 删除场景（如 `TRASH_PHOTO`），离线照片清理路径存资产 `MediaType`，离线相册清理路径存 `albumSubtype`，云下载存 `downloadType`，云同步相册存 `albumType`。因此仅凭 `type` 列无法可靠反推出被删资产的媒体类型——在主删除路径上媒体类型根本未被记录。

## 决策

在审计 CSV 末尾新增 `mediaType` 字符串列（置于 `albumName` 之后），与既有 `int32_t type` 列**并存**，不替换、不复用。其值取 `MediaType` 枚举名去掉 `MEDIA_TYPE_` 前缀（`IMAGE`/`VIDEO`/`AUDIO`/`PHOTO`…），越界值记 `UNKNOWN`。仅在删除**媒体资产**时填充；相册删除不填（相册非媒体资产）。`Write()` 保持「统一序列化所有字段」的现状，是否填 `mediaType` 由调用方负责，`Write()` 不按 `operationType` 做条件序列化。`MEDIA_TYPE_PHOTO` 与 `MEDIA_TYPE_IMAGE` 保持为两个独立字符串（照片表两者均会出现）。

## 原因

保留 `int type` 以向后兼容既有审计日志与可能的下游消费者；`mediaType` 字符串提供自描述的资产媒体类型，消除对被复用的 `type` 列的歧义推断。在主删除路径（`dfx_manager`）上，资产媒体类型此前根本不存在，`mediaType` 是真正新增的信息，须由调用方/URI/DB 显式提供，而不能从既有 `type` 列推导。

## 备选方案（未采纳）

- **替换 `int type` 列**：将所有调用点迁移到新字段并最终移除 `type`。被否决——改动面大、破坏既有 CSV 语义与向后兼容，且 `type` 在非媒体场景（相册子类型等）仍有用途。
- **归一化 PHOTO→IMAGE**：把 `MEDIA_TYPE_PHOTO` 与 `MEDIA_TYPE_IMAGE` 都映射为 `IMAGE`，仅输出 `IMAGE`/`VIDEO`/`AUDIO`。被否决——会静默合并两种实际共存的类型，引入归一化策略且丢失 `PHOTO` 这一区分。
- **`Write()` 按 `operationType` 条件序列化**：仅当 `operationType=="DELETE"` 时输出 `mediaType` 列。被否决——破坏 `Write()` 统一序列化所有字段的契约，并产生每行列数不一致的 CSV，损害下游解析。

## 取值与排序约定

- 列位置：`albumName` 之后（末尾追加，不移动既有列）。
- 转换函数：新增 `MediaType` 枚举名（去前缀）→ string 的转换；越界值返回 `UNKNOWN`。
- 调用方职责：仅删除媒体资产时设置 `auditLog.mediaType`；相册删除与新增/下载等非删除场景留空。

## 调用方覆盖范围（dfx_manager 删除路径）

`mediaType` 的取数遵循「能在既有数据流中拿到就填、不新增查询」的原则，因此四条
`HandleDeleteBehavior` 删除路径的覆盖并不对等：

- `TRASH_PHOTO`：扩展 `GetFilesParams` 既有照片表查询（加 `MEDIA_TYPE` 列），不新增查询。
- `DELETE_LOCAL_ASSETS_PERMANENTLY`：直接取 `fileAsset->GetMediaType()`，不新增查询。
- `ALBUM_DELETE_ASSETS`：扩展 `HandlePhotosResultSet` 既有查询读取 `MEDIA_TYPE`（照片表已选该列），
  不新增查询；`HandleAudiosResultSet` 暂不扩展（音频表查询未选 `MEDIA_TYPE`），故音频删除行的
  `mediaType` 落为 `UNKNOWN`。
- `ALBUM_REMOVE_PHOTOS`：该路径仅有 URI/id 字符串、执行一次 `date_trashed` 更新，无结果集与
  `FileAsset`，照片 URI 也不编码 IMAGE/VIDEO；不新增查询则无法取到 `mediaType`，`LogDelete` 对未取到者
  直接 `MediaTypeToString` → `UNKNOWN`。

即：`ALBUM_REMOVE_PHOTOS` 与 `ALBUM_DELETE_ASSETS` 的音频分支无法在不新增查询的前提下取到真实
`mediaType`，统一落为 `UNKNOWN`（转换器对未取到/越界值的默认）。这是有意取舍，而非疏漏——若后续要求
这些路径覆盖真实值，需为其单独引入 `MEDIA_TYPE` 查询。

## 性能原则（贯穿全程）

`mediaType` 的取数遵循一条高于补全性的硬约束：**维测（审计）不得劣化删除路径性能**。因此：

- 一律不为审计新增 DB 查询；`mediaType` 只能在既有数据流中顺带取得，取不到即记 `UNKNOWN`
  （转换器对未取到/越界值的默认，`LogDelete` 直接 `MediaTypeToString`，不做额外留空处理）。
- 据此否决了「`LogDelete` 内按 photo id 查 `MEDIA_TYPE`」（会在每条用户删除的异步任务里加一次读）
  与「云端删前查本地行 `MEDIA_TYPE`」两个备选；`ALBUM_REMOVE_PHOTOS` 取不到而落 `UNKNOWN` 亦同源此原则。
- 调用方负责取数、`Write()` 保持统一序列化，也是为了避免把 DB 访问塞进审计写入路径。

补全性让位给性能：审计行偶尔出现 `UNKNOWN` 可接受，给删除热路径加查询不可接受。

## 离线与云端删除路径

- 离线照片清理（`media_file_manager_offline_cleanup_task.cpp` 的 `DELETE PHOTO`）：已有
  `photo.mediaType`（int），直接转换填入。
- 离线相册清理（同文件的 `DELETE ALBUM`）：相册非媒体资产，`mediaType` 留空。
- 云端按 cloudId 删除照片（`cloud_media_photos_dao.cpp` 的 `DeleteLocalByCloudId`）：将入参改为
  由外层传入 `CloudMediaPullDataDto`，从 `basicFileType` 推导 `mediaType`——`FILE_TYPE_VIDEO→"VIDEO"`，
  其它具体类型→`"IMAGE"`，**`basicFileType` 缺失（−1）→`"UNKNOWN"`**。
  此处**刻意偏离** DB 写入路径的盲三元 `FILE_TYPE_VIDEO ? VIDEO : IMAGE` 习语（该习语会把 −1/LIVEPHOTO
  默认成 IMAGE），因为审计应忠实记录"未知"，而非继承 DB 写入必须填值的默认；代价是若云端删除记录经常
  不带 `fileType`，云删除行会较多出现 `UNKNOWN`——这正确暴露上游缺口，而非掩盖。
