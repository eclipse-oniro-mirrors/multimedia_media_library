# type 码 place-value 编码

type 码 = `fst×FILE_SOURCE_TYPE_BASE + norm(sd)×SOUTH_DEVICE_TYPE_BASE + pos×POSITION_BASE + mt×MEDIA_TYPE_BASE + err`，各基数定义为 `PhotoErrorEncodeBase` 枚举常量（`FILE_SOURCE_TYPE_BASE=1000000`、`SOUTH_DEVICE_TYPE_BASE=100000`、`POSITION_BASE=10000`、`MEDIA_TYPE_BASE=1000`）。为 `PhotoErrorType` err 位预留 3 位（0-990）的错误码。**用枚举原值**（fst=3 编 3，不重映射）。`err` 取 `PhotoErrorType` 枚举原值（`CONSISTENT=0` 不编码、`FILE_NOT_EXIST=1`…`FILE_LARGER_THAN_DB=4`；`FILE_NOT_EXIST=1` 与旧 `PHOTO_MISS_TYPE=1` 语义连续）。维度顺序 `fst > sd > pos > mt > err` 选定，是因按 type 码排序打包时同来源/出处/位置天然聚拢。

纯逻辑函数通过 `namespace DfxPhotoErrorHelper`（仿 `CheckSceneHelper`）组织，调用形如 `DfxPhotoErrorHelper::EncodePhotoErrorType(...)`。

**每个维度预留归一化函数**（目前仅 south_device_type 有 -1→0，其余 passthrough，留扩展点）。

## 脆性假设

fst/sd/pos/mt 各占 1 位（<10）、err 占 2 位（<100）。当前实际值域下无碰撞（max = 3232040，int32 装得下）：

| 维度 | 值域 | 基数 | max | 上邻基数/本基数 | 碰撞 |
|------|------|------|-----|------------------|------|
| fst | {0,1,3} | 1000000 | 3 | — | 无 |
| sd | {0,1,2} | 100000 | 2 | 10 | 无 |
| pos | {1,3} | 10000 | 3 | 10 | 无 |
| mt | {1,2} | 1000 | 2 | 10 | 无 |
| err | {0,1,2,3,4} | 1 | 4，支持后续增加啊 | 1000 | 无 |

最小上报码 = `fst0/sd0/pos1/mt1/err1` = 11010（`CONSISTENT=0` 不上报，故 err 从 1 起）。若未来 fst/sd/pos/mt 扩到 ≥10 或 err 扩到 ≥100 会撞位，需届时重审编码或改 bitfield。
