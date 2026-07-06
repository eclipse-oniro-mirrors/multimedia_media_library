遗留问题：
- photoRecord和albumRecord的路径storagePath、lpath匿名使用FileScanUtils::GarbleFilePath代替DfxUtils的函数
- MediaFileAccessUtils::GetAssetRealPath, no need to convert, path打印频繁，改成debug
- source_path要从相册拼，不能直接替换前缀，以防原路径不规范
- 补充相册删除审计日志，参考照片，可以使用album_subtype+lpath
- 补充最终结果审计日志（本次处理大于0才打，包括report的内容，和每个处理类型起始和结束id）