#include "HistoryManager.h"
#include <iomanip>
#include <sstream>
#include <ctime>

namespace mem {

void HistoryManager::addRecord(OperationType type, int blockId, std::size_t size,
                                std::size_t address, bool success) {
    HistoryRecord record{};
    record.recordId = m_nextRecordId++;
    record.type = type;
    record.blockId = blockId;
    record.size = size;
    record.address = address;
    record.timestamp = std::chrono::system_clock::now();
    record.success = success;

    m_records.push_back(record);
}

void HistoryManager::recordAllocation(int blockId, std::size_t size,
                                       std::size_t address, bool success) {
    addRecord(OperationType::Allocate, blockId, size, address, success);
}

void HistoryManager::recordDeallocation(int blockId, std::size_t size,
                                         std::size_t address, bool success) {
    addRecord(OperationType::Deallocate, blockId, size, address, success);
}

void HistoryManager::clear() noexcept {
    m_records.clear();
    m_nextRecordId = 1;
}

std::string HistoryManager::formatTimestamp(const std::chrono::system_clock::time_point& tp) {
    auto time_t_val = std::chrono::system_clock::to_time_t(tp);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        tp.time_since_epoch()) % 1000;

    std::tm tm_val{};
#ifdef _WIN32
    localtime_s(&tm_val, &time_t_val);
#else
    localtime_r(&time_t_val, &tm_val);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm_val, "%H:%M:%S") << "." << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

void HistoryManager::displayHistory(std::ostream& os) const {
    if (m_records.empty()) {
        os << "\n暂无操作记录。\n";
        return;
    }

    os << "\n";
    os << "+------+----------+--------+--------+----------+----------+---------+\n";
    os << "|  #   |   时间   |  类型  | 块ID   |   大小   |   地址   |  状态   |\n";
    os << "+------+----------+--------+--------+----------+----------+---------+\n";

    for (const auto& record : m_records) {
        const char* typeStr = (record.type == OperationType::Allocate) ? "分配" : "释放";
        os << "| " << std::setw(4) << record.recordId
           << " | " << std::setw(8) << formatTimestamp(record.timestamp).substr(0, 8)
           << " | " << std::setw(6) << typeStr
           << " | " << std::setw(6) << record.blockId
           << " | " << std::setw(8) << record.size
           << " | " << std::setw(8) << record.address
           << " | " << std::setw(7) << (record.success ? "成功" : "失败")
           << " |\n";
    }

    os << "+------+----------+--------+--------+----------+----------+---------+\n";
}

void HistoryManager::displaySummary(std::ostream& os) const {
    std::size_t totalAllocs = 0;
    std::size_t successAllocs = 0;
    std::size_t totalDeallocs = 0;
    std::size_t successDeallocs = 0;
    std::size_t totalAllocated = 0;
    std::size_t totalFreed = 0;

    for (const auto& record : m_records) {
        if (record.type == OperationType::Allocate) {
            totalAllocs++;
            if (record.success) {
                successAllocs++;
                totalAllocated += record.size;
            }
        } else {
            totalDeallocs++;
            if (record.success) {
                successDeallocs++;
                totalFreed += record.size;
            }
        }
    }

    os << "\n=== 操作统计 ===\n";
    os << "分配操作:   " << successAllocs << "/" << totalAllocs << " 成功\n";
    os << "释放操作:   " << successDeallocs << "/" << totalDeallocs << " 成功\n";
    os << "累计分配:   " << totalAllocated << " 字节\n";
    os << "累计释放:   " << totalFreed << " 字节\n";
}

} // namespace mem
