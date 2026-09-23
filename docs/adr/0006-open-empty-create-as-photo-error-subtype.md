# 打开补建空文件作为 PhotoErrorType 访问补救型子类型

`MediaLibraryAssetOperations::OpenAsset` 非 API10 分支在真实文件缺失时调用 `CreateDirectoryAndAsset` 补建空文件——这一**补救副作用**此前零 DFX。决定把它建模为 `PhotoErrorType` 新子类型 `OPEN_CREATE_EMPTY_FILE`，**复用** `MEDIALIB_PHOTO_ERROR_STATISTIC` 事件与 `EncodePhotoErrorType` 维度编码，经新 `DfxCollector::CollectPhotoError` 累加（内存 map，内联于调用点，无同步 I/O）；2 天 `CheckPhotoError` 单独 drain 并以**独立的 `ReportPhotoError` 发射**（不与 scan 的 0–4 子类型批混批），审计走 `HiAudit` 逐次 CSV。触发点在 `CreateDirectoryAndAsset` 成功之后（副作用完成才计）；调用点内联做廉价 collect、把 audit 入队 DfxWorker 异步写（不阻塞 open）。

复用而非新建独立 STATISTIC 事件，是为避免同类现象两套上报、消费方口径割裂；子类型码天然与 `FILE_NOT_EXIST=1` 区分实现分开计数。piggyback 2 天而非新更快周期，是因为该信号按"周期性趋势聚合"定位、非准实时告警，零新周期接线代价最低。

## 考虑过的备选

- **新建独立 STATISTIC 事件**（如 `MEDIALIB_OPEN_EMPTY_FILE_STATISTIC`，自带维度字段）：语义最干净，但要改 hisysevent.yaml + 新 reporter/collector/周期接线，面最大，且与 PhotoErrorType 体系并行产生"同类两套"。
- **仅 HiAudit 审计、不做 hiview 周期统计**：最轻，但云端无周期维度数，"周期性计数"诉求不满足。

## 后果

- **=5 与 =1 非互斥**：同一对外可见缺失资产，会被首开补建计 =5（访问补救型）且 2 天库存扫描计 =1（状态型）各一次——有意为之，两类回答不同运维问题（访问影响 vs 库存盘点）。
- **audit happenTime 有延迟**：审计由 worker 异步写，`happenTime` 是写入时刻非补建时刻，毫秒~秒级偏移，已接受。
- **southDeviceType 维度降级**：`PHOTO_SOUTH_DEVICE_TYPE` 不在 `FileAsset` 的 member map，该子类型码的南向来源维度恒 0；如需真值，须先注册 member + 加列（未在本决策范围）。
- **去重依赖空文件留存**：首开补建后后续打开命中空壳跳过此分支，故 2 天窗内按资产去重；若空文件被中间删除则可能重复计数（边界，接受）。
