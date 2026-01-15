# 一、整体目标（一句话版）

> 这个 `core/` 目录的目标只有一个：
> **把 NTFS 上的真实文件 → 变成一个可实时更新、可瞬间搜索的内存索引。**

Everything 的本质就是一句话：

> **“文件系统 → 内存索引 → 极速搜索”**

---

# 二、模块划分总览（为什么这样拆）

```
core/
 ├─ indexer/   👉 索引的“中心大脑”
 ├─ mft/       👉 启动时：一次性扫描全盘
 ├─ usn/       👉 运行时：实时监听变化
 ├─ search/    👉 把索引变成搜索结果
 └─ storage/   👉 索引的保存与恢复
```

这 5 个模块分别回答 5 个问题：

| 模块      | 回答的问题             |
| ------- | ----------------- |
| mft     | 启动时：磁盘上“现在有什么文件？” |
| usn     | 运行时：文件“发生了什么变化？”  |
| indexer | 如何把文件信息组织成索引      |
| search  | 如何从索引中快速找到结果      |
| storage | 如何把索引保存 & 秒级恢复    |

---

# 三、整体运行流程（核心流程图）

下面这个流程图，**就是 Everything 的一生** 👇

```
┌──────────────┐
│ 程序启动     │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│ storage      │
│ IndexStore   │
│ 读取快照？   │
└──────┬───────┘
       │ yes
       ▼
┌──────────────┐
│ Snapshot     │
│ Loader       │
│ 恢复 Indexer │
└──────┬───────┘
       │
       ├───────────────┐
       │               │
       ▼               ▼
  （完成）        （若无快照）
                       │
                       ▼
              ┌────────────────┐
              │ mft_reader     │
              │ 扫描 $MFT      │
              └──────┬─────────┘
                     ▼
              ┌────────────────┐
              │ indexer        │
              │ 构建完整索引   │
              └──────┬─────────┘
                     ▼
              ┌────────────────┐
              │ usn_watcher    │
              │ 监听文件变化   │
              └──────┬─────────┘
                     ▼
              ┌────────────────┐
              │ index_update   │
              │ 实时更新索引   │
              └────────────────┘

任何时刻：
┌────────────────┐
│ search_engine  │ ← 用户输入
└────────────────┘
```

---

# 四、每个模块是干什么的（README 级说明）

## 1️⃣ `core/mft/` —— **启动时的“全量扫描”**

> 只在启动时（或首次运行）使用

### 作用

* 直接读取 NTFS 的 `$MFT`
* 构建**完整、准确的文件初始状态**

### 文件职责

| 文件                   | 作用               |
| -------------------- | ---------------- |
| `mft_reader.h`       | 顺序读取 $MFT 记录     |
| `ntfs_structs.h`     | NTFS 原始结构定义      |
| `attribute_parser.h` | 解析 FILE_NAME 等属性 |
| `frn_mapper.h`       | FRN → 父子关系映射     |

📌 **输出给谁？**
→ `indexer`

---

## 2️⃣ `core/usn/` —— **运行时的“增量更新”**

> 程序启动后，一直运行

### 作用

* 监听 NTFS 的 USN Journal
* 把系统事件转成“索引更新事件”

### 文件职责

| 文件              | 作用                         |
| --------------- | -------------------------- |
| `usn_watcher.h` | 监听 USN Journal             |
| `usn_parser.h`  | 解析 USN_RECORD              |
| `usn_event.h`   | 抽象事件（Create/Delete/Rename） |
| `usn_state.h`   | 记录 USN 处理进度                |

📌 **输出给谁？**
→ `index_update` → `indexer`

---

## 3️⃣ `core/indexer/` —— **整个系统的“中枢”**

> 所有数据最终都进入这里

### 作用

* 保存所有文件的“权威索引”
* 维护：

  * 文件 ID
  * 父子关系
  * 完整路径
  * 是否目录

### 文件职责

| 文件                 | 作用             |
| ------------------ | -------------- |
| `indexer.h`        | 索引主类           |
| `indexer_types.h`  | FileRecord 等结构 |
| `path_builder.h`   | 根据 FRN 构建路径    |
| `index_update.h`   | 应用 USN 事件      |
| `index_snapshot.h` | 索引快照结构         |

📌 **被谁使用？**
→ `search` / `storage`

---

## 4️⃣ `core/search/` —— **用户能感知到的一切**

> Everything 的“脸”

### 作用

* 把索引变成搜索结果
* 极快匹配 + 排序

### 文件职责

| 文件                | 作用    |
| ----------------- | ----- |
| `search_engine.h` | 搜索入口  |
| `token_index.h`   | 关键词索引 |
| `prefix_index.h`  | 前缀匹配  |
| `result_ranker.h` | 排序规则  |

📌 **数据来源？**
→ `indexer`

### 🚧 TODO（搜索索引与匹配优化）

### 🔍 基于 Radix Tree 的高性能搜索索引

* [ ] **实现 Radix Tree（Compressed Trie）作为核心字符串索引结构**

  * 以完整文件路径 / 文件名为索引键
  * 相比普通 Trie 显著减少节点数量，降低内存占用
  * 为后续模糊搜索与正则匹配提供高效剪枝能力

* [ ] **前缀搜索（Prefix Search）**

  * Radix Tree 原生支持前缀匹配
  * 输入前缀后快速定位子树并 DFS 收集候选结果
  * 用于 Everything 风格的即时输入搜索体验

* [ ] **模糊搜索（Fuzzy / Subsequence Match）**

  * 在 Radix Tree 上进行 DFS，基于子序列匹配推进状态
  * 支持如 `sys32 → System32`、`abc → a_b_c.txt` 等匹配
  * 结合剪枝策略与评分规则（连续匹配、位置权重）提高性能与相关性

* [ ] **正则表达式搜索（Regex Search）**

  * 利用 Radix Tree 进行候选集裁剪
  * 对裁剪后的少量候选路径应用正则表达式匹配
  * 避免全量正则扫描带来的性能问题

* [ ] **搜索结果排序与打分（Ranking）**

  * 根据匹配类型（前缀 / 模糊 / 正则）进行差异化打分
  * 综合考虑：

    * 匹配连续性
    * 匹配起始位置
    * 文件名长度
    * 路径深度
  * 输出更符合用户直觉的搜索结果顺序

---

## 🧠 设计目标

> 构建接近 Everything 行为模型的搜索系统：
> **索引负责“快速缩小候选集”，匹配与排序负责“结果质量”**



---

## 5️⃣ `core/storage/` —— **秒级启动的秘密**

### 作用

* 把索引保存成磁盘文件
* 下次启动直接 mmap 恢复

### 文件职责

| 文件                  | 作用           |
| ------------------- | ------------ |
| `index_store.h`     | 管理快照文件       |
| `mmap_file.h`       | 内存映射         |
| `snapshot_loader.h` | 快照 → Indexer |

📌 **服务对象？**
→ `indexer`

---

# 五、一个完整“真实例子”贯穿所有模块

## 场景：

你电脑里有这个文件：

```
D:\Projects\Everything\README.md
```

---

### 🔹 启动时（MFT）

1. `mft_reader`

   * 读到一条 MFT 记录
2. `attribute_parser`

   * 解析出：

     * file_id = 12345
     * parent_id = 678
     * name = "README.md"
3. `frn_mapper`

   * 知道 678 对应 `Everything` 目录
4. `indexer`

   * 创建 `FileRecord`
   * 用 `path_builder` 生成完整路径

---

### 🔹 运行时（USN）

你把文件改名为 `README_CN.md`

1. NTFS 写入 USN
2. `usn_watcher` 捕获
3. `usn_parser` 解析
4. 转成：

```cpp
UsnEvent {
  action = Rename,
  file_id = 12345,
  name = L"README_CN.md"
}
```

5. `index_update`

   * 更新 Indexer 中对应记录

---

### 🔹 搜索时

你输入：

```
read
```

1. `search_engine`
2. `prefix_index` 找到 `read*`
3. `token_index` 匹配
4. `result_ranker` 排序
5. 返回路径

---

### 🔹 退出 & 再启动

* 退出时：

  * `index_store` 保存 snapshot
* 再启动：

  * `snapshot_loader` mmap 恢复
  * **不用再扫磁盘**

---

# 六、一句总结

> 本项目采用 **MFT 全量扫描 + USN 增量更新 + 内存索引 + mmap 快照** 的架构，
> 在保证索引准确性的同时，实现了百万级文件的瞬时搜索体验。