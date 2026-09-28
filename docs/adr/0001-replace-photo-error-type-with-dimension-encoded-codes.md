# 维度编码 type 码取代 PHOTO_MISS_TYPE

`MEDIALIB_PHOTO_ERROR_STATISTIC` 原只上报单维度 `PHOTO_MISS_TYPE=1`（缩略图在、原文件丢）。为按来源/出处/位置/媒体类型诊断 DB-vs-文件不一致，决定**取代**（非共存）：退役 `PHOTO_MISS_TYPE=1`，引入维度组合编码的 type 码（file_source_type / south_device_type / position / media_type / 异常子类）。

取代而非共存，是为避免同一现象被两条 type 码双计、消费方口径割裂；迁移成本仅一张 type 码映射表。消费方按新编码（type ≥ 1100）解码，旧码 1 不再发送。

> **勘误（2026-09）**：初版将 `FILE_NOT_EXIST=1` 视为与旧 `PHOTO_MISS_TYPE=1` 语义连续，但 `PHOTO_MISS_TYPE=1` 专指"缩略图在、原文件丢"，而 `FILE_NOT_EXIST=1` 不分缩略图残存与否，二者语义并不一致。已将 `FILE_NOT_EXIST` 拆为 `FILE_NOT_EXIST_THUMB_EXIST=1`（缩略图残存，与旧 `PHOTO_MISS_TYPE=1` 定义值一致）与 `FILE_NOT_EXIST_THUMB_NOT_EXIST=2`（缩略图亦丢），其余子类顺延。`ClassifyPhotoError` 新增 `bool thumbExists` 参数，由 `ProcessRow` 调 `PhotoFileUtils::IsThumbnailExists(data)` 提供。
