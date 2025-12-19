#include "MemoryBlock.h"
#include <iomanip>
#include <string>

namespace mem {

MemoryBlock::MemoryBlock(std::size_t startAddr, std::size_t size, bool isFree, int id)
    : m_id(id)
    , m_startAddr(startAddr)
    , m_size(size)
    , m_isFree(isFree)
    , m_prev(nullptr)
    , m_next(nullptr)
{
}

void MemoryBlock::print(std::ostream& os) const {
    os << "| " << std::setw(6) << (m_isFree ? "-" : std::to_string(m_id))
       << " | " << std::setw(8) << m_startAddr
       << " | " << std::setw(8) << m_size
       << " | " << std::setw(6) << (m_isFree ? "空闲" : "已用")
       << " | " << std::setw(8) << getEndAddress()
       << " |";
}

std::ostream& operator<<(std::ostream& os, const MemoryBlock& block) {
    block.print(os);
    return os;
}

} // namespace mem
