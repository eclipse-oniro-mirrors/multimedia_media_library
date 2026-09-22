# 三类结构：helper / dao / processor / manager（建议）

> 本 ADR 为**建议**（advisory），非强制约束——结构可随实现演进调整。

## 背景

"DB vs 文件不一致"DFX 需扫描 Photos 表（keyset 分批）→ per-row 检查文件状态 → 分类错误码 → 维度编码 → 打包多事件上报。既有的 `DfxMovingPhoto` / `CloudUploadChecker` 把查询+分类+上报揉在一个静态类里，monolithic 且纯逻辑不可单测。

## 决策

拆成 4 个文件对、各司其职：

- `dfx_photo_error_helper.{h,cpp}`：**纯逻辑**（无 DB/无文件 I/O）——`PhotoErrorType`/`PhotoErrorEncodeBase` 枚举、`PhotoErrorDimension`、`Normalize*`、`EncodePhotoErrorType`、`ResolveRealPath`、`ClassifyPhotoError`、`PackPhotoErrors(map, packBatchSize)`。函数置于 `namespace DfxPhotoErrorHelper`（仿 `CheckSceneHelper`，调用形如 `DfxPhotoErrorHelper::Func()`）。TDD seam，gtest 通过 base 反解 round-trip 断言（不依赖手算期望码）。
- `dfx_photo_error_dao.{h,cpp}`：**DB keyset 查询**——`PhotoErrorRow` struct + `DfxPhotoErrorDao::QueryBatch(lastFileId, limit)`，SQL `WHERE file_id > ? ... ORDER BY file_id LIMIT ?`，无 count 查询。
- `dfx_photo_error_processor.{h,cpp}`：**扫描循环+分类+打包**——`DfxPhotoErrorProcessor::GetPhotoErrorBatches(scanBatchSize, packBatchSize)` 驱动 dao 批取 → `ProcessRow(row)`（私有，调 utils + 文件 I/O）→ 累计 map → `PackPhotoErrors` 返回；持 `DfxPhotoErrorDao dao_` 成员。
- `dfx_manager.cpp`：**节拍+上报**——`CheckPhotoError` 调 `processor.GetPhotoErrorBatches(scan, pack)` 遍历 `ReportPhotoError`。

全部**非静态实例类**（仿 `MediaFileManagerOfflineCleanupDao`，便于将来 mock 注入）。扫描**单轮不跨周期**（keyset `file_id > ?` 是单轮内分批游标，不持久化）；中断用 `MedialibrarySubscriber::IsCurrentStatusOn()` per-batch，丢弃+从头重扫（详见 ADR 0002）。

## 取舍

- **纯逻辑外置 helper vs processor 内聚**：选外置——保 TDD seam（纯函数无依赖可直接断言），processor 只做薄编排（路径解析+stat+调 helper）。代价：helper 与 processor 分两文件。
- **实例 vs 静态**：选实例（仿 OfflineCleanupDao）——便于 mock 注入 + 与仓库 DAO 家风一致。代价：需实例化（无状态实例略多余，但可接受）。
- **cursor 跨周期 vs 单轮丢弃**：选单轮丢弃——ADR 0002 已定；典型规模秒级完成，无需跨周期状态。代价：大图库+短门控窗口理论上可能永不完成（罕见，设备夜间充电）。
- **vs monolithic（DfxMovingPhoto 形态）**：选拆分——SRP + 可测；代价：4 文件 vs 1。

## 影响

- 新增 3 文件对 + 1 测试；`dfx_manager.cpp` `CheckPhotoError` 重写；`dfx_reporter.h` 删旧 `PhotoErrorType` enum（ADR 0001 退役）。
- BUILD.gn 加 `media_dfx_source` + 测试 sources。
