#include "MemoryPool.h"
#include "HistoryManager.h"
#include "TUI.h"

#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif
#include <limits>
#include <string>
#include <sstream>

using namespace mem;

// 读取正整数（支持直接按 Enter 使用默认值）
std::size_t readSizeWithDefault(const std::string& prompt, std::size_t defaultValue) {
    std::cout << prompt;
    std::string line;
    std::getline(std::cin, line);

    // 如果输入为空，返回默认值
    if (line.empty()) {
        return defaultValue;
    }

    // 尝试解析数字
    std::istringstream iss(line);
    std::size_t value;
    if (iss >> value) {
        return value;
    }

    // 解析失败，返回默认值
    return defaultValue;
}

// 读取正整数
std::size_t readSize(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    std::getline(std::cin, line);

    std::istringstream iss(line);
    std::size_t value;
    if (iss >> value && value > 0) {
        return value;
    }

    std::cout << "  输入无效，请重新输入。";
    return readSize(prompt);
}

// 读取整数
int readInt(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    std::getline(std::cin, line);

    std::istringstream iss(line);
    int value;
    if (iss >> value) {
        return value;
    }

    std::cout << "  输入无效，请重新输入。";
    return readInt(prompt);
}

int main() {
#ifdef _WIN32
    // 设置 Windows 控制台为 UTF-8 编码
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#endif

    // 启动界面
    tui::clearScreen();
    std::cout << "\n";
    std::cout << "  " << tui::color::BOLD << tui::color::CYAN;
    std::cout << "内存管理模拟器 v2.0" << tui::color::RESET << "\n\n";

    std::size_t poolSize = readSizeWithDefault("  请输入内存池大小（字节）[默认=1024]: ", 1024);
    if (poolSize == 0) {
        poolSize = 1024;
    }

    MemoryPool pool(poolSize);
    HistoryManager history;

    std::string lastMessage;
    tui::MessageType lastMsgType = tui::MessageType::Info;

    int choice = -1;

    while (choice != 0) {
        // 绘制主界面（自动显示内存状态）
        tui::drawMainScreen(pool, lastMessage, lastMsgType);
        lastMessage.clear();

        std::string line;
        std::getline(std::cin, line);

        if (line.empty()) {
            continue;
        }

        std::istringstream iss(line);
        if (!(iss >> choice)) {
            lastMessage = "输入无效，请输入数字。";
            lastMsgType = tui::MessageType::Error;
            continue;
        }

        switch (choice) {
            case 1: {
                // 分配内存
                std::cout << "\n";
                std::size_t size = readSize("  请输入要分配的大小（字节）: ");
                if (size == 0) {
                    lastMessage = "大小必须大于 0。";
                    lastMsgType = tui::MessageType::Error;
                    break;
                }

                auto result = pool.allocate(size);
                if (result.has_value()) {
                    const MemoryBlock* block = pool.getHead();
                    std::size_t address = 0;
                    while (block != nullptr) {
                        if (block->getId() == result.value()) {
                            address = block->getStartAddress();
                            break;
                        }
                        block = block->getNext();
                    }

                    history.recordAllocation(result.value(), size, address, true);
                    lastMessage = "已分配 " + std::to_string(size) + " 字节，地址 " +
                                  std::to_string(address) + "，ID: " + std::to_string(result.value());
                    lastMsgType = tui::MessageType::Success;
                } else {
                    history.recordAllocation(-1, size, 0, false);
                    lastMessage = "分配失败：没有足够的连续空间分配 " + std::to_string(size) + " 字节。";
                    lastMsgType = tui::MessageType::Error;
                }
                break;
            }

            case 2: {
                // 释放内存
                std::cout << "\n";
                int blockId = readInt("  请输入要释放的块 ID: ");

                const MemoryBlock* block = pool.getHead();
                std::size_t size = 0;
                std::size_t address = 0;
                while (block != nullptr) {
                    if (block->getId() == blockId) {
                        size = block->getSize();
                        address = block->getStartAddress();
                        break;
                    }
                    block = block->getNext();
                }

                bool success = pool.deallocate(blockId);
                history.recordDeallocation(blockId, size, address, success);

                if (success) {
                    lastMessage = "已释放块 ID " + std::to_string(blockId) + "（" + std::to_string(size) + " 字节）";
                    lastMsgType = tui::MessageType::Success;
                } else {
                    lastMessage = "块 ID " + std::to_string(blockId) + " 未找到或已释放。";
                    lastMsgType = tui::MessageType::Error;
                }
                break;
            }

            case 3: {
                // 显示块详情
                tui::drawBlockDetails(pool, std::cout);
                tui::waitForEnter();
                break;
            }

            case 4: {
                // 切换策略
                tui::drawStrategyMenu(pool.getStrategy(), std::cout);

                std::string strategyLine;
                std::getline(std::cin, strategyLine);
                std::istringstream strategyIss(strategyLine);
                int strategyChoice = 0;
                if (!(strategyIss >> strategyChoice)) {
                    lastMessage = "输入无效。";
                    lastMsgType = tui::MessageType::Error;
                    break;
                }

                switch (strategyChoice) {
                    case 1:
                        pool.setStrategy(AllocationStrategy::FirstFit);
                        lastMessage = "策略已切换为：首次适应 (First Fit)";
                        lastMsgType = tui::MessageType::Success;
                        break;
                    case 2:
                        pool.setStrategy(AllocationStrategy::BestFit);
                        lastMessage = "策略已切换为：最佳适应 (Best Fit)";
                        lastMsgType = tui::MessageType::Success;
                        break;
                    case 3:
                        pool.setStrategy(AllocationStrategy::WorstFit);
                        lastMessage = "策略已切换为：最差适应 (Worst Fit)";
                        lastMsgType = tui::MessageType::Success;
                        break;
                    case 0:
                        lastMessage = "已取消。";
                        lastMsgType = tui::MessageType::Info;
                        break;
                    default:
                        lastMessage = "无效选择。";
                        lastMsgType = tui::MessageType::Error;
                }
                break;
            }

            case 5: {
                // 显示历史记录
                tui::clearScreen();
                std::cout << "\n";
                std::cout << "  " << tui::color::BOLD << tui::color::CYAN;
                std::cout << "操作历史" << tui::color::RESET << "\n\n";
                history.displayHistory(std::cout);
                history.displaySummary(std::cout);
                tui::waitForEnter();
                break;
            }

            case 6: {
                // 碎片分析
                tui::drawFragmentAnalysis(pool, std::cout);
                tui::waitForEnter();
                break;
            }

            case 0: {
                // 退出
                tui::clearScreen();
                std::cout << "\n";
                std::cout << "  " << tui::color::BOLD << tui::color::CYAN;
                std::cout << "最终统计" << tui::color::RESET << "\n\n";
                tui::drawMemoryBar(pool.getStats(), std::cout);
                std::cout << "\n";
                history.displaySummary(std::cout);
                std::cout << "\n  再见！\n\n";
                break;
            }

            default:
                lastMessage = "无效选择，请重试。";
                lastMsgType = tui::MessageType::Warning;
        }
    }

    return 0;
}
