# type 码 place-value 编码

type 码 = `fst×FILE_SOURCE_TYPE_BASE + norm(sd)×SOUTH_DEVICE_TYPE_BASE + pos×POSITION_BASE + mt×MEDIA_TYPE_BASE + err×ERROR_SUBTYPE_BASE`，各基数定义为 `PhotoErrorEncodeBase` 枚举常量（`FILE_SOURCE_TYPE_BASE=100000`、`SOUTH_DEVICE_TYPE_BASE=10000`、`POSITION_BASE=1000`、`MEDIA_TYPE_BASE=100`、`ERROR_SUBTYPE_BASE=1`）。**用枚举原值**（fst=3 编 3，不重映射），与仓库 `MODULE_OFFSET=100000` 家风一致、码可人解。`err` 取 `PhotoErrorType` 枚举原值（`CONSISTENT=0` 不编码、`FILE_NOT_EXIST=1`…`FILE_LARGER_THAN_DB=4`；`FILE_NOT_EXIST=1` 与旧 `PHOTO_MISS_TYPE=1` 语义连续）。维度顺序 `fst > sd > pos > mt > err` 选定，是因按 type 码排序打包时同来源/出处/位置天然聚拢，契合"按来源诊断"的划分意图。

**每个维度预留归一化函数**（目前仅 south_device_type 有 -1→0，其余 passthrough，留扩展点）。

## 脆性假设

各维度值域保持单位数（<10）。当前实际值域下无碰撞（max = 323204，int32 装得下）：

| 维度 | 值域 | 基数 | max | 上邻基数/本基数 | 碰撞 |
|------|------|------|-----|------------------|------|
| fst | {0,1,3} | 100000 | 3 | — | 无 |
| sd | {0,1,2} | 10000 | 2 | 10 | 无 |
| pos | {1,3} | 1000 | 3 | 10 | 无 |
| mt | {1,2} | 100 | 2 | 10 | 无 |
| err | {0,1,2,3,4} | 1 | 4 | 100 | 无 |

最小上报码 = `fst0/sd0/pos1/mt1/err1` = 1101（`CONSISTENT=0` 不上报，故 err 从 1 起）。若未来某维度枚举扩到 ≥10 会撞位（如 media_type=10 → 10×10000=100000 撞 fst=1），需届时重审编码或改 bitfield。
