#ifndef HISTORY_MANAGER_H
#define HISTORY_MANAGER_H

#include <vector>
#include <string>
#include <chrono>
#include <ostream>
#include <cstddef>

namespace mem {

/**
 * @brief Type of memory operation
 */
enum class OperationType {
    Allocate,
    Deallocate
};

/**
 * @brief Get string representation of operation type
 */
[[nodiscard]] constexpr std::string_view operationToString(OperationType type) noexcept {
    switch (type) {
        case OperationType::Allocate:   return "ALLOC";
        case OperationType::Deallocate: return "FREE";
    }
    return "UNKNOWN";
}

/**
 * @brief A single history record for memory operations
 */
struct HistoryRecord {
    int recordId;
    OperationType type;
    int blockId;
    std::size_t size;
    std::size_t address;
    std::chrono::system_clock::time_point timestamp;
    bool success;
};

/**
 * @brief Manages history of memory allocation operations
 *
 * Tracks all allocate and deallocate operations with timestamps
 * and relevant metadata for debugging and analysis.
 */
class HistoryManager {
public:
    HistoryManager() = default;

    // Record operations
    void recordAllocation(int blockId, std::size_t size, std::size_t address, bool success);
    void recordDeallocation(int blockId, std::size_t size, std::size_t address, bool success);

    // Query operations
    [[nodiscard]] const std::vector<HistoryRecord>& getRecords() const noexcept { return m_records; }
    [[nodiscard]] std::size_t getRecordCount() const noexcept { return m_records.size(); }
    [[nodiscard]] bool isEmpty() const noexcept { return m_records.empty(); }

    // Display
    void displayHistory(std::ostream& os) const;
    void displaySummary(std::ostream& os) const;

    // Management
    void clear() noexcept;

private:
    void addRecord(OperationType type, int blockId, std::size_t size, std::size_t address, bool success);
    [[nodiscard]] static std::string formatTimestamp(const std::chrono::system_clock::time_point& tp);

    std::vector<HistoryRecord> m_records;
    int m_nextRecordId = 1;
};

} // namespace mem

#endif // HISTORY_MANAGER_H
