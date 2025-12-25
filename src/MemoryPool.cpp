// MemoryPool.cpp
#include "MemoryPool.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <limits>

namespace mem {

MemoryPool::MemoryPool(std::size_t totalSize, AllocationStrategy strategy)
    : m_totalSize(totalSize)
    , m_head(std::make_unique<MemoryBlock>(0, totalSize, true, MemoryBlock::INVALID_ID))
    , m_strategy(strategy)
    , m_nextBlockId(1)
{
}

MemoryPool::~MemoryPool() {
    cleanup();
}

MemoryPool::MemoryPool(MemoryPool&& other) noexcept
    : m_totalSize(other.m_totalSize)
    , m_head(std::move(other.m_head))
    , m_strategy(other.m_strategy)
    , m_nextBlockId(other.m_nextBlockId)
{
    other.m_totalSize = 0;
}

MemoryPool& MemoryPool::operator=(MemoryPool&& other) noexcept {
    if (this != &other) {
        m_totalSize = other.m_totalSize;
        m_head = std::move(other.m_head);
        m_strategy = other.m_strategy;
        m_nextBlockId = other.m_nextBlockId;
        other.m_totalSize = 0;
    }
    return *this;
}

void MemoryPool::cleanup() {
    // unique_ptr 自动管理内存，只需重置即可
    m_head.reset();
}

// =====================================================================
//                         核心操作函数
// =====================================================================

/**
 * @brief 内存分配核心函数
 *
 * 算法流程：
 *   1. 验证请求大小有效性（size > 0）
 *   2. 根据当前策略查找合适的空闲块
 *   3. 如果块过大则进行分割
 *   4. 分配唯一ID并标记为已分配状态
 *
 * @param size 请求分配的内存大小（字节）
 * @return std::optional<int> 成功返回块ID，失败返回nullopt
 */
std::optional<int> MemoryPool::allocate(std::size_t size) {
    // Step 1: 验证请求大小，拒绝0大小分配
    if (size == 0) {
        return std::nullopt;
    }

    // Step 2: 根据当前策略（First/Best/Worst Fit）查找合适块
    MemoryBlock* block = findBlock(size);
    if (block == nullptr) {
        return std::nullopt;                // 无足够空间，分配失败
    }

    // Step 3: 如果找到的块大于请求大小，进行分割
    if (block->getSize() > size) {
        splitBlock(block, size);            // 分割：前半部分分配，后半部分保留为空闲
    }

    // Step 4: 标记为已分配状态
    int blockId = m_nextBlockId++;          // 分配唯一ID
    block->setId(blockId);                  // 设置块ID
    block->setFree(false);                  // 标记为已分配

    return blockId;                         // 返回块ID（分配成功）
}

/**
 * @brief 内存释放核心函数
 *
 * 算法流程：
 *   1. 根据ID查找目标内存块
 *   2. 验证块存在且为已分配状态
 *   3. 标记为空闲状态
 *   4. 自动合并相邻空闲块（减少外部碎片）
 *
 * @param blockId 要释放的块ID
 * @return bool 成功返回true，失败返回false
 */
bool MemoryPool::deallocate(int blockId) {
    // Step 1: 根据ID查找内存块
    MemoryBlock* block = findBlockById(blockId);

    // Step 2: 验证块存在且未释放
    if (block == nullptr || block->isFree()) {
        return false;                       // 块不存在或已释放，操作失败
    }

    // Step 3: 标记为空闲状态
    block->setId(MemoryBlock::INVALID_ID);  // 清除ID
    block->setFree(true);                   // 标记为空闲

    // Step 4: 自动合并相邻空闲块（核心：减少外部碎片）
    mergeAdjacentFreeBlocks(block);

    return true;                            // 释放成功
}

// =====================================================================
//                         内存分配算法实现
// =====================================================================

/**
 * @brief 策略分发函数 - 根据当前策略选择对应的查找算法
 * @param size 请求分配的内存大小
 * @return 找到的合适内存块指针，未找到返回nullptr
 */
MemoryBlock* MemoryPool::findBlock(std::size_t size) {
    switch (m_strategy) {
        case AllocationStrategy::FirstFit:
            return findFirstFit(size);      // 首次适应
        case AllocationStrategy::BestFit:
            return findBestFit(size);       // 最佳适应
        case AllocationStrategy::WorstFit:
            return findWorstFit(size);      // 最差适应
    }
    return nullptr;
}

/**
 * @brief 首次适应算法 (First Fit)
 *
 * 算法思想：从链表头部开始遍历，返回第一个大小满足请求的空闲块
 * 时间复杂度：O(n)，最好情况O(1)
 * 优点：速度快，实现简单
 * 缺点：容易在低地址区域产生碎片
 *
 * @param size 请求分配的内存大小
 * @return 第一个满足条件的空闲块，未找到返回nullptr
 */
MemoryBlock* MemoryPool::findFirstFit(std::size_t size) {
    MemoryBlock* current = m_head.get();    // current: 当前遍历指针，从链表头开始

    // 遍历整个内存块链表
    while (current != nullptr) {
        // 检查条件：块必须空闲 且 大小足够
        if (current->isFree() && current->getSize() >= size) {
            return current;                 // 找到第一个满足条件的块，立即返回
        }
        current = current->getNext();       // 移动到下一个块
    }
    return nullptr;                         // 遍历完毕未找到，返回空
}

/**
 * @brief 最佳适应算法 (Best Fit)
 *
 * 算法思想：遍历所有空闲块，选择大小最接近请求的块（最小的足够块）
 * 时间复杂度：O(n)，必须遍历完整链表
 * 优点：空间利用率高，减少浪费
 * 缺点：速度慢，容易产生微小碎片
 *
 * @param size 请求分配的内存大小
 * @return 最小的足够大空闲块，未找到返回nullptr
 */
MemoryBlock* MemoryPool::findBestFit(std::size_t size) {
    MemoryBlock* bestBlock = nullptr;       // bestBlock: 记录当前找到的最佳块
    std::size_t smallestSize = std::numeric_limits<std::size_t>::max();  // smallestSize: 最小满足条件的块大小

    MemoryBlock* current = m_head.get();    // current: 遍历指针

    // 遍历所有内存块，寻找最小的足够块
    while (current != nullptr) {
        // 检查条件：块空闲 且 大小足够
        if (current->isFree() && current->getSize() >= size) {
            // 如果当前块比已记录的最佳块更小，则更新
            if (current->getSize() < smallestSize) {
                smallestSize = current->getSize();  // 更新最小大小
                bestBlock = current;                // 更新最佳块指针
            }
        }
        current = current->getNext();
    }
    return bestBlock;                       // 返回最佳块（可能为nullptr）
}

/**
 * @brief 最差适应算法 (Worst Fit)
 *
 * 算法思想：遍历所有空闲块，选择最大的空闲块进行分配
 * 时间复杂度：O(n)，必须遍历完整链表
 * 优点：分割后剩余空间较大，便于后续分配
 * 缺点：大块快速耗尽，后续大请求可能失败
 *
 * @param size 请求分配的内存大小
 * @return 最大的空闲块，未找到返回nullptr
 */
MemoryBlock* MemoryPool::findWorstFit(std::size_t size) {
    MemoryBlock* worstBlock = nullptr;      // worstBlock: 记录当前找到的最大块
    std::size_t largestSize = 0;            // largestSize: 最大满足条件的块大小

    MemoryBlock* current = m_head.get();    // current: 遍历指针

    // 遍历所有内存块，寻找最大的空闲块
    while (current != nullptr) {
        // 检查条件：块空闲 且 大小足够
        if (current->isFree() && current->getSize() >= size) {
            // 如果当前块比已记录的最大块更大，则更新
            if (current->getSize() > largestSize) {
                largestSize = current->getSize();   // 更新最大大小
                worstBlock = current;               // 更新最大块指针
            }
        }
        current = current->getNext();
    }
    return worstBlock;                      // 返回最大块（可能为nullptr）
}

/**
 * @brief 根据块ID查找内存块
 * @param blockId 要查找的块ID
 * @return 找到的内存块指针，未找到返回nullptr
 */
MemoryBlock* MemoryPool::findBlockById(int blockId) {
    MemoryBlock* current = m_head.get();
    while (current != nullptr) {
        if (current->getId() == blockId) {
            return current;
        }
        current = current->getNext();
    }
    return nullptr;
}

// =====================================================================
//                         内存块管理算法
// =====================================================================

/**
 * @brief 内存块分割算法
 *
 * 算法思想：当找到的空闲块大于请求大小时，将其分割为两部分
 *          - 前半部分：分配给用户（大小为size）
 *          - 后半部分：作为新的空闲块保留在链表中
 * 时间复杂度：O(1)
 *
 * @param block 要分割的内存块指针
 * @param size  分配给用户的大小
 */
void MemoryPool::splitBlock(MemoryBlock* block, std::size_t size) {
    // remainingSize: 分割后剩余的空间大小
    std::size_t remainingSize = block->getSize() - size;

    // 如果剩余空间为0，无需分割，直接返回
    if (remainingSize == 0) {
        return;
    }

    // ========== 创建新的空闲块用于存储剩余空间 ==========
    auto newBlock = std::make_unique<MemoryBlock>(
        block->getStartAddress() + size,    // 新块起始地址 = 原块起始 + 分配大小
        remainingSize,                       // 新块大小 = 剩余空间
        true,                                // 新块状态 = 空闲
        MemoryBlock::INVALID_ID              // 新块ID = 无效（空闲块无ID）
    );

    // ========== 更新原块大小为分配大小 ==========
    block->setSize(size);

    // ========== 将新块插入到原块之后（维护链表结构）==========
    insertAfter(block, newBlock.release());
}

/**
 * @brief 在指定块之后插入新块（链表插入操作）
 * @param block    插入位置的前驱块
 * @param newBlock 要插入的新块
 */
void MemoryPool::insertAfter(MemoryBlock* block, MemoryBlock* newBlock) {
    // Step 1: 保存原来的后继节点
    auto oldNext = block->releaseNext();

    // Step 2: 设置新块的前后指针
    newBlock->setNext(std::move(oldNext));  // 新块的next指向原来的后继
    newBlock->setPrev(block);                // 新块的prev指向当前块

    // Step 3: 更新原后继节点的prev指针
    if (newBlock->getNext() != nullptr) {
        newBlock->getNext()->setPrev(newBlock);
    }

    // Step 4: 当前块的next指向新块
    block->setNext(std::unique_ptr<MemoryBlock>(newBlock));
}

/**
 * @brief 内存块合并算法（相邻空闲块合并）
 *
 * 算法思想：释放内存块后，检查并合并相邻的空闲块以减少外部碎片
 *          采用双向合并策略：
 *          1. 向后合并：将当前块与后继空闲块合并
 *          2. 向前合并：将当前块与前驱空闲块合并
 * 时间复杂度：O(k)，k为相邻空闲块数量
 *
 * @param block 刚释放的内存块（已标记为空闲）
 */
void MemoryPool::mergeAdjacentFreeBlocks(MemoryBlock* block) {
    // ==================== 阶段1：向后合并 ====================
    // 循环检查后继块，如果为空闲则合并
    while (block->getNext() != nullptr && block->getNext()->isFree()) {
        MemoryBlock* next = block->getNext();   // next: 待合并的后继块

        // 合并：当前块大小 += 后继块大小
        block->setSize(block->getSize() + next->getSize());

        // 从链表中移除后继块：跳过next，直接连接到next的next
        auto nextNext = next->releaseNext();
        block->setNext(std::move(nextNext));    // next会被自动释放

        // 更新新后继的prev指针
        if (block->getNext() != nullptr) {
            block->getNext()->setPrev(block);
        }
        // 继续检查新的后继是否也是空闲块
    }

    // ==================== 阶段2：向前合并 ====================
    // 循环检查前驱块，如果为空闲则合并
    while (block->getPrev() != nullptr && block->getPrev()->isFree()) {
        MemoryBlock* prev = block->getPrev();   // prev: 待合并的前驱块

        // 合并：前驱块大小 += 当前块大小
        prev->setSize(prev->getSize() + block->getSize());

        // 从链表中移除当前块：前驱直接连接到当前块的next
        auto blockNext = block->releaseNext();
        prev->setNext(std::move(blockNext));    // block会被自动释放

        // 更新新后继的prev指针
        if (prev->getNext() != nullptr) {
            prev->getNext()->setPrev(prev);
        }

        // 将block指针移动到prev，继续检查更前面的块
        block = prev;
    }
}

// =====================================================================
//                         统计与碎片率计算
// =====================================================================

/**
 * @brief 内存池统计信息与碎片率计算算法
 *
 * 算法思想：遍历内存块链表，统计各项指标并计算外部碎片率
 * 碎片率公式：FragmentationRate = 1 - (最大空闲块 / 总空闲空间)
 *   - 碎片率 = 0：无碎片（或仅有一个空闲块）
 *   - 碎片率 → 1：碎片化严重（大量小块）
 * 时间复杂度：O(n)
 *
 * @return PoolStats 包含所有统计信息的结构体
 */
PoolStats MemoryPool::getStats() const {
    PoolStats stats{};                              // stats: 统计结果结构体

    // ========== 初始化统计变量 ==========
    stats.totalSize = m_totalSize;                  // 内存池总大小
    stats.usedSize = 0;                             // 已使用空间（累加）
    stats.freeSize = 0;                             // 空闲空间（累加）
    stats.blockCount = 0;                           // 总块数
    stats.freeBlockCount = 0;                       // 空闲块数
    stats.allocatedBlockCount = 0;                  // 已分配块数
    stats.largestFreeBlock = 0;                     // 最大空闲块大小

    MemoryBlock* current = m_head.get();            // current: 遍历指针

    // ========== 遍历链表统计各项指标 ==========
    while (current != nullptr) {
        stats.blockCount++;                         // 累计总块数

        if (current->isFree()) {
            // 空闲块统计
            stats.freeBlockCount++;                 // 累计空闲块数
            stats.freeSize += current->getSize();   // 累计空闲空间
            // 更新最大空闲块大小
            stats.largestFreeBlock = std::max(stats.largestFreeBlock, current->getSize());
        } else {
            // 已分配块统计
            stats.allocatedBlockCount++;            // 累计已分配块数
            stats.usedSize += current->getSize();   // 累计已使用空间
        }
        current = current->getNext();
    }

    // ========== 计算外部碎片率 ==========
    // 公式：碎片率 = 1 - (最大空闲块 / 总空闲空间)
    // 条件：必须有空闲空间 且 空闲块数 > 1 才有碎片
    if (stats.freeSize > 0 && stats.freeBlockCount > 1) {
        stats.fragmentationRate = 1.0 - (static_cast<double>(stats.largestFreeBlock) /
                                          static_cast<double>(stats.freeSize));
    } else {
        stats.fragmentationRate = 0.0;              // 无碎片
    }

    return stats;
}

void MemoryPool::displayStatus(std::ostream& os) const {
    auto stats = getStats();

    os << "\n";
    os << "+--------+----------+----------+--------+----------+\n";
    os << "|   ID   |   起始   |   大小   |  状态  |   结束   |\n";
    os << "+--------+----------+----------+--------+----------+\n";

    MemoryBlock* current = m_head.get();
    while (current != nullptr) {
        os << *current << "\n";
        current = current->getNext();
    }

    os << "+--------+----------+----------+--------+----------+\n";
    os << "\n";
    os << "内存使用: " << stats.usedSize << "/" << stats.totalSize
       << " 字节 (" << std::fixed << std::setprecision(1)
       << (100.0 * stats.usedSize / stats.totalSize) << "%)\n";
    os << "空闲空间: " << stats.freeSize << " 字节\n";
    os << "块数量: " << stats.blockCount
       << " (已分配: " << stats.allocatedBlockCount
       << ", 空闲: " << stats.freeBlockCount << ")\n";
    os << "最大空闲块: " << stats.largestFreeBlock << " 字节\n";
    os << "碎片率: " << std::fixed << std::setprecision(1)
       << (stats.fragmentationRate * 100) << "%\n";
    os << "当前策略: " << strategyToString(m_strategy) << "\n";
}

void MemoryPool::displayVisual(std::ostream& os, std::size_t width) const {
    os << "\n内存地图:\n";
    os << "[";

    MemoryBlock* current = m_head.get();
    while (current != nullptr) {
        // Calculate visual width for this block
        std::size_t blockWidth = (current->getSize() * width) / m_totalSize;
        if (blockWidth == 0) blockWidth = 1;  // Minimum width of 1

        char fillChar = current->isFree() ? '-' : '#';
        for (std::size_t i = 0; i < blockWidth; ++i) {
            os << fillChar;
        }
        current = current->getNext();
    }

    os << "]\n";
    os << "图例: '#' = 已分配, '-' = 空闲\n";
}

} // namespace mem
