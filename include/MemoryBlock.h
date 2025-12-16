#ifndef MEMORY_BLOCK_H
#define MEMORY_BLOCK_H

#include <cstddef>
#include <ostream>

namespace mem {

/**
 * @brief Represents a single memory block in the memory pool
 *
 * Each block maintains its own metadata including size, allocation status,
 * and links to adjacent blocks in a doubly-linked list structure.
 */
class MemoryBlock {
public:
    // Constants
    static constexpr int INVALID_ID = -1;

    // Constructors
    MemoryBlock() = default;
    MemoryBlock(std::size_t startAddr, std::size_t size, bool isFree = true, int id = INVALID_ID);

    // Accessors
    [[nodiscard]] int getId() const noexcept { return m_id; }
    [[nodiscard]] std::size_t getStartAddress() const noexcept { return m_startAddr; }
    [[nodiscard]] std::size_t getSize() const noexcept { return m_size; }
    [[nodiscard]] std::size_t getEndAddress() const noexcept { return m_startAddr + m_size - 1; }
    [[nodiscard]] bool isFree() const noexcept { return m_isFree; }

    [[nodiscard]] MemoryBlock* getNext() const noexcept { return m_next; }
    [[nodiscard]] MemoryBlock* getPrev() const noexcept { return m_prev; }

    // Mutators
    void setId(int id) noexcept { m_id = id; }
    void setSize(std::size_t size) noexcept { m_size = size; }
    void setFree(bool free) noexcept { m_isFree = free; }
    void setNext(MemoryBlock* next) noexcept { m_next = next; }
    void setPrev(MemoryBlock* prev) noexcept { m_prev = prev; }

    // Utility
    void print(std::ostream& os) const;

    friend std::ostream& operator<<(std::ostream& os, const MemoryBlock& block);

private:
    int m_id = INVALID_ID;          // Block ID (for allocated blocks)
    std::size_t m_startAddr = 0;    // Starting address in memory pool
    std::size_t m_size = 0;         // Block size in bytes
    bool m_isFree = true;           // Allocation status

    MemoryBlock* m_prev = nullptr;  // Previous block in list
    MemoryBlock* m_next = nullptr;  // Next block in list
};

} // namespace mem

#endif // MEMORY_BLOCK_H
