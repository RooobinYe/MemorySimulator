#include "TUI.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <vector>
#include <algorithm>

namespace tui {

// ==================== 工具函数 ====================

void clearScreen() {
#ifdef _WIN32
    std::system("cls");
#else
    std::cout << "\033[2J\033[H";
#endif
}

void waitForEnter() {
    std::cout << "\n  " << color::DIM << "按回车返回..." << color::RESET;
    std::cin.get();
}

std::string getStrategyName(mem::AllocationStrategy strategy) {
    switch (strategy) {
        case mem::AllocationStrategy::FirstFit:  return "首次适应 (First Fit)";
        case mem::AllocationStrategy::BestFit:   return "最佳适应 (Best Fit)";
        case mem::AllocationStrategy::WorstFit:  return "最差适应 (Worst Fit)";
    }
    return "未知";
}

// ==================== 绘制函数 ====================

void drawHorizontalLine(int width, std::ostream& os) {
    os << "  " << color::DIM;
    for (int i = 0; i < width; ++i) {
        os << "─";
    }
    os << color::RESET << "\n";
}

void drawMessage(const std::string& msg, MessageType type, std::ostream& os) {
    os << "  ";
    switch (type) {
        case MessageType::Success:
            os << color::GREEN << "[成功] " << color::RESET;
            break;
        case MessageType::Warning:
            os << color::YELLOW << "[警告] " << color::RESET;
            break;
        case MessageType::Error:
            os << color::RED << "[错误] " << color::RESET;
            break;
        case MessageType::Info:
        default:
            os << color::CYAN << "[信息] " << color::RESET;
            break;
    }
    os << msg << "\n";
}

void drawMemoryBar(const mem::PoolStats& stats, std::ostream& os) {
    double usagePercent = (static_cast<double>(stats.usedSize) / stats.totalSize) * 100.0;
    int filledWidth = static_cast<int>((static_cast<double>(stats.usedSize) / stats.totalSize) * MEMORY_BAR_WIDTH);

    os << "  内存 [";

    // 根据使用率选择颜色
    if (usagePercent > 80) {
        os << color::RED;
    } else if (usagePercent > 50) {
        os << color::YELLOW;
    } else {
        os << color::GREEN;
    }

    for (int i = 0; i < MEMORY_BAR_WIDTH; ++i) {
        if (i < filledWidth) {
            os << "█";
        } else {
            os << color::GRAY << "░" << color::RESET;
            if (usagePercent > 80) os << color::RED;
            else if (usagePercent > 50) os << color::YELLOW;
            else os << color::GREEN;
        }
    }

    os << color::RESET << "] ";
    os << stats.usedSize << "/" << stats.totalSize << " ";
    os << std::fixed << std::setprecision(1) << usagePercent << "%\n";
}

void drawMemoryMap(const mem::MemoryPool& pool, std::ostream& os) {
    const int MAP_WIDTH = 60;
    std::size_t totalSize = pool.getTotalSize();

    // 收集所有块信息
    struct BlockInfo {
        int id;
        std::size_t start;
        std::size_t size;
        bool isFree;
        int visualWidth;
    };
    std::vector<BlockInfo> blocks;

    const mem::MemoryBlock* current = pool.getHead();
    while (current != nullptr) {
        BlockInfo info;
        info.id = current->getId();
        info.start = current->getStartAddress();
        info.size = current->getSize();
        info.isFree = current->isFree();
        // 计算可视化宽度，至少为2
        info.visualWidth = std::max(2, static_cast<int>((static_cast<double>(info.size) / totalSize) * MAP_WIDTH));
        blocks.push_back(info);
        current = current->getNext();
    }

    // 调整宽度确保总和为 MAP_WIDTH
    int totalWidth = 0;
    for (const auto& b : blocks) totalWidth += b.visualWidth;
    if (!blocks.empty() && totalWidth != MAP_WIDTH) {
        blocks.back().visualWidth += (MAP_WIDTH - totalWidth);
    }

    os << "\n";

    // 第一行：内存块可视化
    os << "  ";
    for (const auto& block : blocks) {
        if (block.isFree) {
            os << color::GRAY;
        } else {
            os << color::GREEN;
        }
        for (int i = 0; i < block.visualWidth; ++i) {
            os << (block.isFree ? "░" : "█");
        }
        os << color::RESET;
    }
    os << "\n";

    // 第二行：块标识
    os << "  ";
    for (const auto& block : blocks) {
        std::string label;
        if (block.isFree) {
            label = "空闲";
            os << color::GRAY;
        } else {
            label = "ID:" + std::to_string(block.id);
            os << color::GREEN;
        }

        // 居中显示标签
        int padding = (block.visualWidth - static_cast<int>(label.length())) / 2;
        if (padding < 0) padding = 0;

        for (int i = 0; i < padding; ++i) os << " ";

        // 截断过长的标签
        if (static_cast<int>(label.length()) > block.visualWidth) {
            os << label.substr(0, block.visualWidth);
        } else {
            os << label;
            for (int i = 0; i < block.visualWidth - padding - static_cast<int>(label.length()); ++i) os << " ";
        }
        os << color::RESET;
    }
    os << "\n";

    // 第三行：大小
    os << "  ";
    for (const auto& block : blocks) {
        std::string sizeStr = std::to_string(block.size) + "B";
        os << color::DIM;

        int padding = (block.visualWidth - static_cast<int>(sizeStr.length())) / 2;
        if (padding < 0) padding = 0;

        for (int i = 0; i < padding; ++i) os << " ";

        if (static_cast<int>(sizeStr.length()) > block.visualWidth) {
            os << sizeStr.substr(0, block.visualWidth);
        } else {
            os << sizeStr;
            for (int i = 0; i < block.visualWidth - padding - static_cast<int>(sizeStr.length()); ++i) os << " ";
        }
        os << color::RESET;
    }
    os << "\n";
}

void drawMainScreen(const mem::MemoryPool& pool, const std::string& lastMessage, MessageType msgType) {
    clearScreen();

    auto stats = pool.getStats();
    std::ostream& os = std::cout;

    // 标题行
    os << "\n";
    os << "  " << color::BOLD << color::CYAN << "内存管理模拟器 v2.0" << color::RESET;
    os << "                        ";
    os << color::DIM << "[" << getStrategyName(pool.getStrategy()) << "]" << color::RESET << "\n\n";

    // 内存使用进度条
    drawMemoryBar(stats, os);

    // 内存地图
    drawMemoryMap(pool, os);

    // 分隔线
    os << "\n";
    drawHorizontalLine(60, os);

    // 统计信息
    os << "  块数: " << stats.blockCount;
    os << " | 已用: " << color::GREEN << stats.allocatedBlockCount << color::RESET;
    os << " | 空闲: " << color::GRAY << stats.freeBlockCount << color::RESET;
    os << " | 碎片率: ";

    double fragPercent = stats.fragmentationRate * 100;
    if (fragPercent > 50) {
        os << color::RED;
    } else if (fragPercent > 25) {
        os << color::YELLOW;
    } else {
        os << color::GREEN;
    }
    os << std::fixed << std::setprecision(1) << fragPercent << "%" << color::RESET;

    os << " | 最大空闲: " << stats.largestFreeBlock << "B\n";

    drawHorizontalLine(60, os);

    // 菜单
    os << "\n";
    os << "  " << color::BOLD << "[1]" << color::RESET << " 分配   ";
    os << color::BOLD << "[2]" << color::RESET << " 释放   ";
    os << color::BOLD << "[3]" << color::RESET << " 详情   ";
    os << color::BOLD << "[4]" << color::RESET << " 策略   ";
    os << color::BOLD << "[5]" << color::RESET << " 历史   ";
    os << color::BOLD << "[6]" << color::RESET << " 碎片   ";
    os << color::BOLD << "[0]" << color::RESET << " 退出\n";

    // 消息区域
    os << "\n";
    if (!lastMessage.empty()) {
        drawMessage(lastMessage, msgType, os);
    }

    os << "\n  > ";
}

void drawBlockDetails(const mem::MemoryPool& pool, std::ostream& os) {
    clearScreen();

    os << "\n";
    os << "  " << color::BOLD << color::CYAN << "内存块详情" << color::RESET << "\n\n";

    // 表头
    os << "  " << color::DIM;
    os << std::setw(6) << "ID" << "  ";
    os << std::setw(8) << "起始" << "  ";
    os << std::setw(8) << "大小" << "  ";
    os << std::setw(8) << "状态" << "  ";
    os << std::setw(8) << "结束";
    os << color::RESET << "\n";

    drawHorizontalLine(50, os);

    const mem::MemoryBlock* current = pool.getHead();
    while (current != nullptr) {
        os << "  ";

        // ID
        if (current->isFree()) {
            os << color::GRAY << std::setw(6) << "-" << color::RESET;
        } else {
            os << color::GREEN << std::setw(6) << current->getId() << color::RESET;
        }
        os << "  ";

        // 起始
        os << std::setw(8) << current->getStartAddress() << "  ";

        // 大小
        os << std::setw(8) << current->getSize() << "  ";

        // 状态
        if (current->isFree()) {
            os << color::GRAY << std::setw(8) << "空闲" << color::RESET;
        } else {
            os << color::GREEN << std::setw(8) << "已用" << color::RESET;
        }
        os << "  ";

        // 结束
        os << std::setw(8) << (current->getStartAddress() + current->getSize() - 1);

        os << "\n";
        current = current->getNext();
    }

    drawHorizontalLine(50, os);
}

void drawFragmentAnalysis(const mem::MemoryPool& pool, std::ostream& os) {
    clearScreen();

    auto stats = pool.getStats();

    os << "\n";
    os << "  " << color::BOLD << color::CYAN << "碎片分析" << color::RESET << "\n\n";

    os << "  总内存:       " << std::setw(8) << stats.totalSize << " 字节\n";
    os << "  已用内存:     " << color::GREEN << std::setw(8) << stats.usedSize << " 字节" << color::RESET << "\n";
    os << "  空闲内存:     " << color::GRAY << std::setw(8) << stats.freeSize << " 字节" << color::RESET << "\n";
    os << "\n";
    os << "  总块数:       " << std::setw(8) << stats.blockCount << "\n";
    os << "  已分配块:     " << color::GREEN << std::setw(8) << stats.allocatedBlockCount << color::RESET << "\n";
    os << "  空闲块:       " << color::GRAY << std::setw(8) << stats.freeBlockCount << color::RESET << "\n";
    os << "  最大空闲块:   " << std::setw(8) << stats.largestFreeBlock << " 字节\n";
    os << "\n";

    drawHorizontalLine(40, os);

    double fragPercent = stats.fragmentationRate * 100;
    os << "\n  外部碎片率: ";

    // 碎片率进度条
    os << "[";
    int fragBarWidth = 20;
    int filledWidth = static_cast<int>((fragPercent / 100.0) * fragBarWidth);

    if (fragPercent > 50) {
        os << color::RED;
    } else if (fragPercent > 25) {
        os << color::YELLOW;
    } else {
        os << color::GREEN;
    }

    for (int i = 0; i < fragBarWidth; ++i) {
        os << (i < filledWidth ? "█" : "░");
    }
    os << color::RESET << "] ";

    os << std::fixed << std::setprecision(1) << fragPercent << "%";

    if (fragPercent > 50) {
        os << color::RED << "  严重" << color::RESET;
    } else if (fragPercent > 25) {
        os << color::YELLOW << "  中等" << color::RESET;
    } else {
        os << color::GREEN << "  良好" << color::RESET;
    }
    os << "\n";
}

void drawStrategyMenu(mem::AllocationStrategy current, std::ostream& os) {
    clearScreen();

    os << "\n";
    os << "  " << color::BOLD << color::CYAN << "选择分配策略" << color::RESET << "\n\n";

    auto printOption = [&](int num, mem::AllocationStrategy strat, const std::string& desc) {
        os << "  ";
        if (current == strat) {
            os << color::GREEN << "[*] ";
        } else {
            os << "[ ] ";
        }
        os << color::BOLD << "[" << num << "]" << color::RESET << " ";
        os << getStrategyName(strat);
        if (current == strat) {
            os << color::GREEN << " (当前)" << color::RESET;
        }
        os << "\n      " << color::DIM << desc << color::RESET << "\n\n";
    };

    printOption(1, mem::AllocationStrategy::FirstFit, "分配第一个足够大的空闲块");
    printOption(2, mem::AllocationStrategy::BestFit, "分配最小的足够大的空闲块");
    printOption(3, mem::AllocationStrategy::WorstFit, "分配最大的空闲块");

    os << "  " << color::BOLD << "[0]" << color::RESET << " 取消\n";
    os << "\n  > ";
}

} // namespace tui
