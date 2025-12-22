#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include "MemoryBlock.h"
#include <cstddef>
#include <optional>
#include <string_view>
#include <functional>
#include <memory>

namespace mem {

/**
 * @brief Memory allocation strategy enumeration
 */
enum class AllocationStrategy {
    FirstFit,   // Allocate first block that fits
    BestFit,    // Allocate smallest block that fits
    WorstFit    // Allocate largest block that fits
};

/**
 * @brief Get string representation of allocation strategy
 */
[[nodiscard]] constexpr std::string_view strategyToString(AllocationStrategy strategy) noexcept {
    switch (strategy) {
        case AllocationStrategy::FirstFit:  return "First Fit";
        case AllocationStrategy::BestFit:   return "Best Fit";
        case AllocationStrategy::WorstFit:  return "Worst Fit";
    }
    return "Unknown";
}

/**
 * @brief Memory pool statistics
 */
struct PoolStats {
    std::size_t totalSize;
    std::size_t usedSize;
    std::size_t freeSize;
    std::size_t blockCount;
    std::size_t freeBlockCount;
    std::size_t allocatedBlockCount;
    std::size_t largestFreeBlock;
    double fragmentationRate;
};

/**
 * @brief Manages a simulated memory pool with various allocation strategies
 *
 * This class simulates operating system memory management using a doubly-linked
 * list to track memory blocks. It supports multiple allocation strategies and
 * automatic coalescing of adjacent free blocks.
 */
class MemoryPool {
public:
    // Constructor & Destructor
    explicit MemoryPool(std::size_t totalSize, AllocationStrategy strategy = AllocationStrategy::FirstFit);
    ~MemoryPool();

    // Disable copy operations
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    // Enable move operations
    MemoryPool(MemoryPool&& other) noexcept;
    MemoryPool& operator=(MemoryPool&& other) noexcept;

    // Core operations
    /**
     * @brief Allocate a block of memory
     * @param size Size of memory to allocate
     * @return Block ID if successful, std::nullopt if allocation failed
     */
    [[nodiscard]] std::optional<int> allocate(std::size_t size);

    /**
     * @brief Free a previously allocated block
     * @param blockId ID of the block to free
     * @return true if successful, false if block not found
     */
    bool deallocate(int blockId);

    // Strategy management
    void setStrategy(AllocationStrategy strategy) noexcept { m_strategy = strategy; }
    [[nodiscard]] AllocationStrategy getStrategy() const noexcept { return m_strategy; }

    // Statistics and display
    [[nodiscard]] PoolStats getStats() const;
    void displayStatus(std::ostream& os) const;
    void displayVisual(std::ostream& os, std::size_t width = 60) const;

    // Accessors
    [[nodiscard]] std::size_t getTotalSize() const noexcept { return m_totalSize; }
    [[nodiscard]] const MemoryBlock* getHead() const noexcept { return m_head.get(); }

private:
    // Memory block finding algorithms
    [[nodiscard]] MemoryBlock* findFirstFit(std::size_t size);
    [[nodiscard]] MemoryBlock* findBestFit(std::size_t size);
    [[nodiscard]] MemoryBlock* findWorstFit(std::size_t size);
    [[nodiscard]] MemoryBlock* findBlock(std::size_t size);

    // Block management
    [[nodiscard]] MemoryBlock* findBlockById(int blockId);
    void splitBlock(MemoryBlock* block, std::size_t size);
    void mergeAdjacentFreeBlocks(MemoryBlock* block);
    void insertAfter(MemoryBlock* block, MemoryBlock* newBlock);
    void removeBlock(MemoryBlock* block);
    void cleanup();

    // Member variables
    std::size_t m_totalSize;
    std::unique_ptr<MemoryBlock> m_head;
    AllocationStrategy m_strategy;
    int m_nextBlockId;
};

} // namespace mem

#endif // MEMORY_POOL_H
