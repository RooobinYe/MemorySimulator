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

// ================== Core Operations ==================

std::optional<int> MemoryPool::allocate(std::size_t size) {
    if (size == 0) {
        return std::nullopt;
    }

    MemoryBlock* block = findBlock(size);
    if (block == nullptr) {
        return std::nullopt;
    }

    // Split the block if necessary
    if (block->getSize() > size) {
        splitBlock(block, size);
    }

    // Mark as allocated
    int blockId = m_nextBlockId++;
    block->setId(blockId);
    block->setFree(false);

    return blockId;
}

bool MemoryPool::deallocate(int blockId) {
    MemoryBlock* block = findBlockById(blockId);
    if (block == nullptr || block->isFree()) {
        return false;
    }

    // Mark as free
    block->setId(MemoryBlock::INVALID_ID);
    block->setFree(true);

    // Merge with adjacent free blocks
    mergeAdjacentFreeBlocks(block);

    return true;
}

// ================== Finding Algorithms ==================

MemoryBlock* MemoryPool::findBlock(std::size_t size) {
    switch (m_strategy) {
        case AllocationStrategy::FirstFit:
            return findFirstFit(size);
        case AllocationStrategy::BestFit:
            return findBestFit(size);
        case AllocationStrategy::WorstFit:
            return findWorstFit(size);
    }
    return nullptr;
}

MemoryBlock* MemoryPool::findFirstFit(std::size_t size) {
    // Find the first free block that is large enough
    MemoryBlock* current = m_head.get();
    while (current != nullptr) {
        if (current->isFree() && current->getSize() >= size) {
            return current;
        }
        current = current->getNext();
    }
    return nullptr;
}

MemoryBlock* MemoryPool::findBestFit(std::size_t size) {
    // Find the smallest free block that is large enough
    MemoryBlock* bestBlock = nullptr;
    std::size_t smallestSize = std::numeric_limits<std::size_t>::max();

    MemoryBlock* current = m_head.get();
    while (current != nullptr) {
        if (current->isFree() && current->getSize() >= size) {
            if (current->getSize() < smallestSize) {
                smallestSize = current->getSize();
                bestBlock = current;
            }
        }
        current = current->getNext();
    }
    return bestBlock;
}

MemoryBlock* MemoryPool::findWorstFit(std::size_t size) {
    // Find the largest free block that is large enough
    MemoryBlock* worstBlock = nullptr;
    std::size_t largestSize = 0;

    MemoryBlock* current = m_head.get();
    while (current != nullptr) {
        if (current->isFree() && current->getSize() >= size) {
            if (current->getSize() > largestSize) {
                largestSize = current->getSize();
                worstBlock = current;
            }
        }
        current = current->getNext();
    }
    return worstBlock;
}

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

// ================== Block Management ==================

void MemoryPool::splitBlock(MemoryBlock* block, std::size_t size) {
    std::size_t remainingSize = block->getSize() - size;

    // Only split if remaining size is meaningful
    if (remainingSize == 0) {
        return;
    }

    // Create a new free block for the remaining space
    auto newBlock = std::make_unique<MemoryBlock>(
        block->getStartAddress() + size,
        remainingSize,
        true,
        MemoryBlock::INVALID_ID
    );

    // Update the original block's size
    block->setSize(size);

    // Insert the new block after the current block
    insertAfter(block, newBlock.release());
}

void MemoryPool::insertAfter(MemoryBlock* block, MemoryBlock* newBlock) {
    // 保存原来的 next
    auto oldNext = block->releaseNext();

    // 新块指向原来的 next
    newBlock->setNext(std::move(oldNext));
    newBlock->setPrev(block);

    if (newBlock->getNext() != nullptr) {
        newBlock->getNext()->setPrev(newBlock);
    }

    // block 指向新块
    block->setNext(std::unique_ptr<MemoryBlock>(newBlock));
}

void MemoryPool::mergeAdjacentFreeBlocks(MemoryBlock* block) {
    // Merge with next block if it's free
    while (block->getNext() != nullptr && block->getNext()->isFree()) {
        MemoryBlock* next = block->getNext();
        block->setSize(block->getSize() + next->getSize());

        // 获取 next 的 next，然后删除 next
        auto nextNext = next->releaseNext();
        block->setNext(std::move(nextNext));

        if (block->getNext() != nullptr) {
            block->getNext()->setPrev(block);
        }
        // next 会在 block->setNext() 时自动被删除
    }

    // Merge with previous block if it's free
    while (block->getPrev() != nullptr && block->getPrev()->isFree()) {
        MemoryBlock* prev = block->getPrev();
        prev->setSize(prev->getSize() + block->getSize());

        // 获取 block 的 next
        auto blockNext = block->releaseNext();
        prev->setNext(std::move(blockNext));

        if (prev->getNext() != nullptr) {
            prev->getNext()->setPrev(prev);
        }
        // block 会在 prev->setNext() 时自动被删除
        block = prev;
    }
}

// ================== Statistics and Display ==================

PoolStats MemoryPool::getStats() const {
    PoolStats stats{};
    stats.totalSize = m_totalSize;
    stats.usedSize = 0;
    stats.freeSize = 0;
    stats.blockCount = 0;
    stats.freeBlockCount = 0;
    stats.allocatedBlockCount = 0;
    stats.largestFreeBlock = 0;

    MemoryBlock* current = m_head.get();
    while (current != nullptr) {
        stats.blockCount++;
        if (current->isFree()) {
            stats.freeBlockCount++;
            stats.freeSize += current->getSize();
            stats.largestFreeBlock = std::max(stats.largestFreeBlock, current->getSize());
        } else {
            stats.allocatedBlockCount++;
            stats.usedSize += current->getSize();
        }
        current = current->getNext();
    }

    // Calculate fragmentation rate
    // External fragmentation = 1 - (largest free block / total free space)
    if (stats.freeSize > 0 && stats.freeBlockCount > 1) {
        stats.fragmentationRate = 1.0 - (static_cast<double>(stats.largestFreeBlock) /
                                          static_cast<double>(stats.freeSize));
    } else {
        stats.fragmentationRate = 0.0;
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
