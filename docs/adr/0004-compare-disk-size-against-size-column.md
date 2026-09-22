# 磁盘 size 对比 `size` 列而非 `local_asset_size`

size 不一致检测比 DB 的 `size` 列（常规 `GetFileSize`、动态 `GetMovingPhotoSize` 对 `size`）。

`local_asset_size` 名字像首选，但有历史脏数据缺口——`media_update_local_asset_size_task` 修复任务的存在即证据（`local_asset_size=0` 而 `size` 正常的记录要靠后台任务补），用它会大量假阳性。`size` 是源真相、可靠填充；且动态照片的 `size` 列就是 `GetMovingPhotoSize`（图+视+extradata 之和）写入的（`photo_custom_restore_operation.cpp:954`），磁盘侧 `GetMovingPhotoSize` 对 `size` 是同口径对同口径。`local_asset_size` 本就从 `size` 派生（修复任务方向 `size`→`local_asset_size`），无增益。

边界：DB `size=0` 而磁盘非 0 → 记 `err=2/3`（DB 错了，是真不一致，非假阳性）；DB `size=0` 且磁盘 0 → 第 ⑥ 步已判 `err=1`（size 为 0）短路，不会进此步。
