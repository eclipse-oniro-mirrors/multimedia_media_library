# 每 2 天周期自适应多事件上报

4 维 × 异常子类展开至 108~144 个 (type, count) 对（按当前实际出现的枚举值计），而 HiSysEvent `PHOTO_ERROR_TYPE` 字段 `arrsize:5`。决定每周期收集**非零** (type, count)、按 type 码升序、每 5 个打包一事件、发 `ceil(非零数/5)` 个 `ReportPhotoError`；0 非零则**静默不发**。

否决固定轮转（首报延迟可达 ~44 天，对故障检测太慢）与降维（丢失来源诊断粒度）。代价：消费方需处理变长批；DFX 存活从事件流不可观测（用户取舍：省上行）。每个 type 码每周期最多出现在一个事件里（非零组合分区打包），hiview 按码聚合不会双计。

## 中断与门控

扫描中断检测用 `MedialibrarySubscriber::IsCurrentStatusOn()`（per-batch，**非**充电+熄屏门控）。中断 → 丢弃已扫 map、返回空（0 批，manager 不上报），下个 2 天 tick 从 `file_id=0` 从头重扫。**无** `LAST_TWO_DAY_REPORT_TIME` 回退（不立即重试，等下个 2 天 tick）。keyset `file_id > ?` 是单轮内分批游标，不跨周期持久化。
