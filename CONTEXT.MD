遗留问题：
- 路径匿名使用FileScanUtils::GarbleFilePath
- MediaFileAccessUtils::GetAssetRealPath, no need to convert, path打印频繁，改成debug
- source_path要从相册拼，不能直接替换前缀，以防原路径不规范
- 补充相册删除审计日志
- 补充最终结果审计日志（本次处理大于0才打）