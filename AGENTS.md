# AGENTS.md

## 项目定位

OpenHarmony `multimedia/media_library` 组件，提供用户文件（图片、视频、音频）的统一管理能力，包括扫描、索引、元数据查询、缩略图生成、云同步及权限管控。

## 关键模块边界

| 目录 | 职责 |
|------|------|
| `services/` | 核心服务实现：扫描、相册管理、权限、缩略图、MTP、云同步、备份恢复等后端逻辑 |
| `frameworks/` | 框架层：NAPI/CAPI 绑定、IPC 客户端、DataAbility 扩展、工具类 |
| `interfaces/` | 对外接口定义：inner_api（系统内部）、kits/js（JS API）、kits/c（NDK C API） |
| `common/` | 跨模块共享的数据结构、IPC 公共定义、工具函数 |

## 不可修改的约束

- **公共 API 兼容性**：`interfaces/kits/` 下的 JS/C API 签名不得做 breaking change
- **BUILD.gn 结构**：各模块的 `BUILD.gn` 遵循 OpenHarmony 构建规范，target 名称和依赖关系不可随意变更
- **bundle.json**：组件名 `media_library`、子系统 `multimedia`、syscap 声明不可擅自修改
- **目录规范**：`services/`、`frameworks/`、`interfaces/` 的分层职责边界必须保持

## 变更后的验证路径

1. **编译验证**：确保全量编译通过（`BUILD.gn` 依赖无断裂）
2. **接口兼容**：公共头文件变更需确认向后兼容
3. **单元测试**：运行相关模块的测试用例
4. **代码规范**：符合 OpenHarmony 编码规范（命名、注释、许可证头）
