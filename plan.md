# 动态内存管理模拟器 - 设计文档

## 一、项目概述

本项目实现一个模拟操作系统动态内存分配和释放的程序，包含内存申请、释放、碎片检测、多种分配策略等功能。

**参考项目**：
- [davidsauntson/memory-alloc-list-sim](https://github.com/davidsauntson/memory-alloc-list-sim) - 架构参考
- [GeeksforGeeks Memory Management](ht
- tps://www.geeksforgeeks.org/dsa/program-first-fit-algorithm-memory-management/) - 算法参考

---

## 二、功能需求

### 2.1 基本要求

| 编号 | 功能 | 描述 |
|------|------|------|
| B1 | 内存模拟 | 创建固定大小的内存池（如 1024 字节） |
| B2 | 内存分配 | 用户申请指定大小的内存块，从内存池分配 |
| B3 | 内存释放 | 用户释放先前申请的内存块 |
| B4 | 状态显示 | 显示当前内存使用情况（已分配/未分配） |

### 2.2 扩展要求

| 编号 | 功能 | 描述 |
|------|------|------|
| E1 | 碎片分析 | 分析内存池碎片情况，计算碎片率 |
| E2 | 内存合并 | 释放时自动合并相邻空闲块 |
| E3 | 多种策略 | First Fit、Best Fit、Worst Fit |
| E4 | 历史记录 | 记录分配/释放的时间、大小、位置 |

---

## 三、技术设计

### 3.1 核心数据结构

使用**双向链表**管理内存块，每个节点代表一个内存块（已分配或空闲）。

```
┌─────────────────────────────────────────────────────────┐
│                    内存池 (1024 bytes)                   │
├─────────┬─────────────┬─────────┬─────────────┬─────────┤
│ Block 1 │   Block 2   │ Block 3 │   Block 4   │ Block 5 │
│ 已分配   │    空闲      │ 已分配   │    空闲      │ 已分配  │
│ 100B    │    200B     │  150B   │    300B     │  274B   │
└─────────┴─────────────┴─────────┴─────────────┴─────────┘
     ↑           ↑           ↑           ↑           ↑
   Node 1 ←→  Node 2  ←→  Node 3  ←→  Node 4  ←→  Node 5
```

### 3.2 类设计

#### （1）MemoryBlock - 内存块

```cpp
// 内存块结构
class MemoryBlock {
private:
    int id;              // 块ID（分配时的进程/请求ID）
    size_t startAddr;    // 起始地址
    size_t size;         // 块大小
    bool isFree;         // 是否空闲

    MemoryBlock* prev;   // 前一个块
    MemoryBlock* next;   // 后一个块

public:
    // 构造、析构
    // Getter/Setter
    // 打印块信息
};
```

#### （2）MemoryPool - 内存池管理器

```cpp
// 分配策略枚举
enum class AllocStrategy {
    FIRST_FIT,   // 首次适应
    BEST_FIT,    // 最佳适应
    WORST_FIT    // 最差适应
};

// 内存池管理器
class MemoryPool {
private:
    size_t totalSize;           // 内存池总大小
    MemoryBlock* head;          // 链表头
    AllocStrategy strategy;     // 当前分配策略
    int nextBlockId;            // 下一个块ID

    // 查找算法
    MemoryBlock* findFirstFit(size_t size);
    MemoryBlock* findBestFit(size_t size);
    MemoryBlock* findWorstFit(size_t size);

    // 合并相邻空闲块
    void mergeAdjacentFreeBlocks(MemoryBlock* block);

public:
    // 构造函数：初始化内存池
    MemoryPool(size_t size);
    ~MemoryPool();

    // 核心操作
    int allocate(size_t size);          // 分配内存，返回块ID
    bool deallocate(int blockId);       // 释放内存

    // 策略设置
    void setStrategy(AllocStrategy s);

    // 状态查询
    void displayStatus();               // 显示内存状态
    double getFragmentationRate();      // 获取碎片率
};
```

#### （3）HistoryRecord - 历史记录

```cpp
// 操作类型
enum class OperationType {
    ALLOCATE,    // 分配
    DEALLOCATE   // 释放
};

// 历史记录条目
struct HistoryRecord {
    int recordId;            // 记录ID
    OperationType type;      // 操作类型
    int blockId;             // 块ID
    size_t size;             // 大小
    size_t address;          // 地址
    std::string timestamp;   // 时间戳
};

// 历史管理器
class HistoryManager {
private:
    std::vector<HistoryRecord> records;  // 历史记录列表
    int nextRecordId;

public:
    void addRecord(OperationType type, int blockId, size_t size, size_t addr);
    void displayHistory();
    void clearHistory();
};
```

### 3.3 分配算法

#### First Fit（首次适应）
```
从链表头开始，找到第一个足够大的空闲块
优点：速度快
缺点：可能在低地址产生碎片
```

#### Best Fit（最佳适应）
```
遍历所有空闲块，找到最小的足够大的块
优点：减少浪费
缺点：速度慢，产生小碎片
```

#### Worst Fit（最差适应）
```
遍历所有空闲块，找到最大的块
优点：剩余空间可能仍可用
缺点：大块很快被用完
```

### 3.4 碎片率计算

```
碎片率 = (空闲块数量 - 1) / 空闲块数量 × 100%   （当空闲块 > 1 时）

或者更精确的：
外部碎片率 = 1 - (最大空闲块大小 / 总空闲空间) × 100%
```

---

## 四、项目结构

```
ProjectB/
├── CMakeLists.txt              # CMake 构建配置
├── plan.md                     # 本设计文档
├── .gitignore                  # Git 忽略文件
│
├── include/                    # 头文件目录
│   ├── MemoryBlock.h           # 内存块类
│   ├── MemoryPool.h            # 内存池管理器
│   ├── HistoryManager.h        # 历史记录管理
│   └── Utils.h                 # 工具函数
│
├── src/                        # 源文件目录
│   ├── main.cpp                # 主程序入口
│   ├── MemoryBlock.cpp         # 内存块实现
│   ├── MemoryPool.cpp          # 内存池实现
│   ├── HistoryManager.cpp      # 历史记录实现
│   └── Utils.cpp               # 工具函数实现
│
└── docs/                       # 文档目录（可选）
    └── screenshots/            # 运行截图
```

---

## 五、用户界面设计

```
╔══════════════════════════════════════════════════════════╗
║           动态内存管理模拟器 v1.0                          ║
╠══════════════════════════════════════════════════════════╣
║  内存池大小: 1024 字节    当前策略: First Fit             ║
╠══════════════════════════════════════════════════════════╣
║  [1] 分配内存                                             ║
║  [2] 释放内存                                             ║
║  [3] 显示内存状态                                         ║
║  [4] 显示碎片分析                                         ║
║  [5] 切换分配策略                                         ║
║  [6] 查看历史记录                                         ║
║  [0] 退出程序                                             ║
╠══════════════════════════════════════════════════════════╣
║  请选择操作:                                              ║
╚══════════════════════════════════════════════════════════╝
```

### 内存状态可视化

```
内存状态 (已用: 450/1024 字节, 使用率: 43.9%)
┌────────────────────────────────────────────────────────┐
│███████░░░░░░░░░░████████░░░░░░░░░░░░░░░░░░░░░░░████████│
└────────────────────────────────────────────────────────┘
 [ID:1]    [空闲]     [ID:2]        [空闲]          [ID:3]
 100B      200B       150B          300B            274B

块详情:
+------+--------+--------+------+--------+
| ID   | 起始   | 大小   | 状态 | 结束   |
+------+--------+--------+------+--------+
| 1    | 0      | 100    | 已用 | 99     |
| -    | 100    | 200    | 空闲 | 299    |
| 2    | 300    | 150    | 已用 | 449    |
| -    | 450    | 300    | 空闲 | 749    |
| 3    | 750    | 274    | 已用 | 1023   |
+------+--------+--------+------+--------+
```

---

## 六、实现步骤

### 阶段一：基础框架
1. 创建 CMake 项目结构
2. 实现 MemoryBlock 类
3. 实现 MemoryPool 基础功能（初始化、First Fit 分配、释放）
4. 实现简单的状态显示

### 阶段二：核心功能
5. 实现 Best Fit 和 Worst Fit 算法
6. 实现内存合并功能
7. 实现碎片分析

### 阶段三：扩展功能
8. 实现 HistoryManager 历史记录
9. 完善用户界面
10. 添加可视化显示

### 阶段四：测试优化
11. 测试各种边界情况
12. 优化代码，添加注释
13. 编写使用说明

---

## 七、测试用例

| 测试场景 | 操作序列 | 预期结果 |
|----------|----------|----------|
| 基本分配 | 分配 100B | 成功，返回 ID=1 |
| 连续分配 | 分配 100B, 200B, 300B | 三个块依次排列 |
| 释放合并 | 分配 A,B,C → 释放 B | B 变为空闲 |
| 相邻合并 | 分配 A,B,C → 释放 A,B | A,B 合并为一个空闲块 |
| 分配失败 | 内存池满后再分配 | 返回失败 |
| First Fit | 有多个空闲块时分配 | 选择第一个够大的 |
| Best Fit | 有多个空闲块时分配 | 选择最小的够大的 |
| Worst Fit | 有多个空闲块时分配 | 选择最大的 |

---

## 八、参考资料

1. [Operating System Concepts - Memory Management](https://www.os-book.com/)
2. [GeeksforGeeks - Memory Allocation Algorithms](https://www.geeksforgeeks.org/partition-allocation-methods-in-memory-management/)
3. [davidsauntson/memory-alloc-list-sim](https://github.com/davidsauntson/memory-alloc-list-sim)
