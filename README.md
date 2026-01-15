# EverythingClone – Project Structure & File Responsibilities

> 本项目是一个 **Windows 平台 Everything 风格的本地文件索引与搜索工具**。
> 架构目标是：**高性能、可扩展、可逐步演进到 NTFS / USN 级别实现**。

EverythingClone/
├─ CMakeLists.txt
├─ README.md
│
├─ include/
│   ├─ core/
│   │   ├─ indexer/
│   │   │   ├─ indexer.h
│   │   │   ├─ indexer_types.h        ← 核心数据结构
│   │   │   ├─ path_builder.h
│   │   │   ├─ index_update.h
│   │   │   └─ index_snapshot.h
│   │   │
│   │   ├─ mft/
│   │   │   ├─ mft_reader.h
│   │   │   ├─ ntfs_structs.h         ← NTFS 原始结构
│   │   │   ├─ attribute_parser.h
│   │   │   └─ frn_mapper.h
│   │   │
│   │   ├─ usn/
│   │   │   ├─ usn_watcher.h
│   │   │   ├─ usn_event.h
│   │   │   ├─ usn_parser.h
│   │   │   └─ usn_state.h
│   │   │
│   │   ├─ search/
│   │   │   ├─ search_engine.h
│   │   │   ├─ token_index.h
│   │   │   ├─ prefix_index.h
│   │   │   └─ result_ranker.h
│   │   │
│   │   └─ storage/
│   │       ├─ index_store.h
│   │       ├─ mmap_file.h
│   │       └─ snapshot_loader.h
│   │
│   ├─ gui/
│   │   ├─ main_window.h
│   │   ├─ search_box.h
│   │   ├─ file_list_view.h
│   │   └─ ui_controller.h
│   │
│   └─ utils/
│       ├─ win_handle.h
│       ├─ logger.h
│       ├─ unicode.h
│       ├─ thread_pool.h
│       └─ scope_guard.h
│
├─ src/
│   ├─ core/
│   │   ├─ indexer/
│   │   │   ├─ indexer.cpp
│   │   │   ├─ path_builder.cpp
│   │   │   ├─ index_update.cpp
│   │   │   └─ index_snapshot.cpp
│   │   │
│   │   ├─ mft/
│   │   │   ├─ mft_reader.cpp
│   │   │   ├─ attribute_parser.cpp
│   │   │   └─ frn_mapper.cpp
│   │   │
│   │   ├─ usn/
│   │   │   ├─ usn_watcher.cpp
│   │   │   ├─ usn_parser.cpp
│   │   │   └─ usn_state.cpp
│   │   │
│   │   ├─ search/
│   │   │   ├─ search_engine.cpp
│   │   │   ├─ token_index.cpp
│   │   │   ├─ prefix_index.cpp
│   │   │   └─ result_ranker.cpp
│   │   │
│   │   └─ storage/
│   │       ├─ index_store.cpp
│   │       ├─ mmap_file.cpp
│   │       └─ snapshot_loader.cpp
│   │
│   ├─ gui/
│   │   ├─ main_window.cpp
│   │   ├─ search_box.cpp
│   │   ├─ file_list_view.cpp
│   │   └─ ui_controller.cpp
│   │
│   ├─ utils/
│   │   ├─ logger.cpp
│   │   ├─ thread_pool.cpp
│   │   └─ unicode.cpp
│   │
│   └─ main.cpp
│
└─ resources/
    ├─ icons/
    └─ EverythingClone.rc


---

## 1. Core Layer（核心系统层）

核心层负责 **文件系统索引、增量更新、搜索与持久化**，完全不依赖 GUI。

---

### 1.1 `core/indexer/` — 索引核心（Single Source of Truth）

> 负责维护「文件索引的完整内存表示」，是整个系统的中枢。

#### `indexer.h / indexer.cpp`

* Indexer 的对外接口
* 协调：

  * 初始索引构建（MFT / fallback 扫描）
  * USN 增量更新
  * 搜索查询
* 保证线程安全
* 是系统中 **唯一可以修改索引状态的组件**

---

#### `indexer_types.h`

* 定义索引的核心数据结构：

  * `FileRecord`
  * 文件 ID / 父子关系
  * 文件属性（是否目录、大小、时间戳）
* 所有模块共享这些类型
* 防止类型定义分散在各个 cpp 中

---

#### `path_builder.h / path_builder.cpp`

* 负责将：

  * `file_id + parent_file_id`
    → 构建为完整路径字符串
* 支持递归路径重建
* 独立出来，避免路径逻辑污染索引逻辑

---

#### `index_update.h / index_update.cpp`

* 处理 USN 事件映射为索引操作：

  * 创建 / 删除 / 重命名 / 移动
* 封装增量更新规则
* 保证索引状态的一致性

---

#### `index_snapshot.h / index_snapshot.cpp`

* 索引快照管理
* 支持：

  * 索引持久化（磁盘）
  * 启动时快速恢复
* 为未来的 mmap / crash recovery 做准备

---

---

### 1.2 `core/mft/` — NTFS 主文件表（MFT）读取

> 负责从 NTFS 的 `$MFT` 中读取完整文件列表。

#### `mft_reader.h / mft_reader.cpp`

* 读取 NTFS `$MFT`
* 枚举所有文件记录
* 提取：

  * File Reference Number (FRN)
  * 父目录 FRN
  * 文件名 / 目录标志

---

#### `ntfs_structs.h`

* 定义 NTFS 内部结构：

  * `FILE_RECORD_HEADER`
  * `ATTRIBUTE_HEADER`
  * `$FILE_NAME` 等
* 严格按磁盘格式定义
* 只包含结构，不包含逻辑

---

#### `attribute_parser.h / attribute_parser.cpp`

* 解析 MFT 中的属性流
* 从 `$FILE_NAME` / `$DATA` 等属性中提取信息
* 与 NTFS 结构解耦

---

#### `frn_mapper.h / frn_mapper.cpp`

* FRN（File Reference Number）相关处理
* 提供：

  * FRN → 索引 ID 映射
  * 父子关系构建辅助

---

---

### 1.3 `core/usn/` — USN Journal 增量监听

> 实时监听 NTFS 文件系统变化。

#### `usn_watcher.h / usn_watcher.cpp`

* 打开卷句柄
* 读取 USN Journal
* 持续监听文件变化事件

---

#### `usn_event.h`

* 定义 USN 事件抽象结构：

  * 创建 / 删除 / 重命名 / 移动
* 作为 Indexer 的输入

---

#### `usn_parser.h / usn_parser.cpp`

* 解析原始 `USN_RECORD`
* 兼容不同 USN 版本
* 转换为 `UsnEvent`

---

#### `usn_state.h / usn_state.cpp`

* 维护 USN 游标状态
* 处理：

  * 程序重启恢复
  * Journal 回绕
* 保证增量更新不丢事件

---

---

### 1.4 `core/search/` — 搜索引擎

> 提供 Everything 风格的极速字符串搜索。

#### `search_engine.h / search_engine.cpp`

* 搜索系统入口
* 接收查询字符串
* 调用底层索引结构
* 返回排序后的结果

---

#### `token_index.h / token_index.cpp`

* 将文件名拆分为 token
* 建立反向索引（token → file_id）
* 支持模糊匹配 / 子串匹配

---

#### `prefix_index.h / prefix_index.cpp`

* 前缀索引（Trie / Radix Tree）
* 优化前缀搜索性能
* Everything 风格即时响应的关键

---

#### `result_ranker.h / result_ranker.cpp`

* 搜索结果排序
* 规则示例：

  * 完全匹配优先
  * 文件名优先于路径
  * 最近访问权重

---

---

### 1.5 `core/storage/` — 存储与内存映射

> 提供高性能、低内存的索引存储。

#### `index_store.h / index_store.cpp`

* 管理索引的磁盘表示
* 定义序列化 / 反序列化格式

---

#### `mmap_file.h / mmap_file.cpp`

* Windows Memory Mapped File 封装
* 支持超大索引快速加载
* Everything 启动速度的核心技术

---

#### `snapshot_loader.h / snapshot_loader.cpp`

* 启动时加载索引快照
* 与 Indexer 协作恢复状态

---

---

## 2. GUI Layer（用户界面层）

> 只负责显示和交互，不包含任何索引逻辑。

---

### `gui/main_window.*`

* 主窗口创建
* 消息循环
* 协调子控件

---

### `gui/search_box.*`

* 搜索输入框
* 处理文本变化
* 触发搜索请求

---

### `gui/file_list_view.*`

* 显示搜索结果
* ListView 封装
* 虚拟列表支持（大结果集）

---

### `gui/ui_controller.*`

* GUI 与 Core 的桥梁
* 线程切换
* 防止 UI 阻塞

---

---

## 3. Utils Layer（通用工具）

> 与业务无关的基础设施。

---

### `utils/win_handle.*`

* Windows HANDLE RAII 封装
* 防止资源泄漏

---

### `utils/logger.*`

* 日志系统
* 支持调试 / 性能分析

---

### `utils/unicode.*`

* UTF-16 / UTF-8 转换
* Windows 字符串工具

---

### `utils/thread_pool.*`

* 线程池
* 后台索引 / 搜索任务调度

---

### `utils/scope_guard.h`

* RAII 辅助工具
* 自动回滚 / 清理

---

## 4. Entry Point

### `main.cpp`

* 程序入口
* 初始化：

  * Common Controls
  * 主窗口
  * 核心模块

---

## 5. 设计原则总结

* **单一职责**
* **模块边界清晰**
* **可替换实现**
* **长期可维护**

---

## 6. 项目目标

> 本项目目标不是“复刻 Everything 的全部细节”，
> 而是 **在工程结构与核心原理层面，逐步逼近真实 Everything**。
