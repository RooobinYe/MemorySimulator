#include "MemoryPool.h"
#include "HistoryManager.h"

#include <iostream>
#include <iomanip>
#include <string>
#include <limits>
#include <cstdlib>

using namespace mem;

// 策略名称（中英文）
const char* strategyToStringCN(AllocationStrategy strategy) {
    switch (strategy) {
        case AllocationStrategy::FirstFit:  return "首次适应 (First Fit)";
        case AllocationStrategy::BestFit:   return "最佳适应 (Best Fit)";
        case AllocationStrategy::WorstFit:  return "最差适应 (Worst Fit)";
    }
    return "未知";
}

// 清屏（跨平台）
void clearScreen() {
#ifdef _WIN32
    std::system("cls");
#else
    std::system("clear");
#endif
}

// 打印主菜单
void printMenu(const MemoryPool& pool) {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════╗\n";
    std::cout << "║           动态内存管理模拟器 v1.0                  ║\n";
    std::cout << "╠════════════════════════════════════════════════════╣\n";
    std::cout << "║  内存池大小: " << std::setw(6) << pool.getTotalSize() << " 字节"
              << "    当前策略: " << strategyToStringCN(pool.getStrategy()) << std::setw(8) << " ║\n";
    std::cout << "╠════════════════════════════════════════════════════╣\n";
    std::cout << "║  [1] 分配内存                                      ║\n";
    std::cout << "║  [2] 释放内存                                      ║\n";
    std::cout << "║  [3] 显示内存状态                                  ║\n";
    std::cout << "║  [4] 显示内存地图（可视化）                        ║\n";
    std::cout << "║  [5] 碎片分析                                      ║\n";
    std::cout << "║  [6] 切换分配策略                                  ║\n";
    std::cout << "║  [7] 查看操作历史                                  ║\n";
    std::cout << "║  [8] 清空历史记录                                  ║\n";
    std::cout << "║  [0] 退出程序                                      ║\n";
    std::cout << "╚════════════════════════════════════════════════════╝\n";
    std::cout << "  请选择操作: ";
}

// 打印策略选择菜单
void printStrategyMenu() {
    std::cout << "\n选择分配策略:\n";
    std::cout << "  [1] 首次适应 (First Fit) - 选择第一个足够大的空闲块\n";
    std::cout << "  [2] 最佳适应 (Best Fit)  - 选择最小的足够大的空闲块\n";
    std::cout << "  [3] 最差适应 (Worst Fit) - 选择最大的空闲块\n";
    std::cout << "  [0] 取消\n";
    std::cout << "  请选择: ";
}

// 读取正整数
std::size_t readSize(const std::string& prompt) {
    std::size_t value;
    std::cout << prompt;
    while (!(std::cin >> value)) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "输入无效，请重新输入。" << prompt;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return value;
}

// 读取整数
int readInt(const std::string& prompt) {
    int value;
    std::cout << prompt;
    while (!(std::cin >> value)) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "输入无效，请重新输入。" << prompt;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return value;
}

// 暂停等待用户输入
void pause() {
    std::cout << "\n按回车键继续...";
    std::cin.get();
}

int main() {
    // 默认内存池大小
    constexpr std::size_t DEFAULT_POOL_SIZE = 1024;

    std::cout << "╔════════════════════════════════════════════════════╗\n";
    std::cout << "║           动态内存管理模拟器 v1.0                  ║\n";
    std::cout << "╚════════════════════════════════════════════════════╝\n\n";

    std::size_t poolSize = readSize("请输入内存池大小（字节）[默认=1024]: ");
    if (poolSize == 0) {
        poolSize = DEFAULT_POOL_SIZE;
    }

    MemoryPool pool(poolSize);
    HistoryManager history;

    int choice = -1;

    while (choice != 0) {
        printMenu(pool);

        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "输入无效，请输入数字。\n";
            pause();
            continue;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        switch (choice) {
            case 1: {
                // 分配内存
                std::size_t size = readSize("请输入要分配的大小（字节）: ");
                if (size == 0) {
                    std::cout << "错误：大小必须大于 0。\n";
                    break;
                }

                auto result = pool.allocate(size);
                if (result.has_value()) {
                    // 查找块以获取其地址
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
                    std::cout << "\n分配成功！已分配 " << size << " 字节。\n";
                    std::cout << "块 ID: " << result.value() << "，起始地址: " << address << "\n";
                } else {
                    history.recordAllocation(-1, size, 0, false);
                    std::cout << "\n错误：无法分配 " << size << " 字节。\n";
                    std::cout << "没有足够的连续空闲内存。\n";
                }
                pause();
                break;
            }

            case 2: {
                // 释放内存
                int blockId = readInt("请输入要释放的块 ID: ");

                // 释放前查找块信息
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
                    std::cout << "\n释放成功！已释放块 ID " << blockId << "。\n";
                } else {
                    std::cout << "\n错误：块 ID " << blockId << " 未找到或已释放。\n";
                }
                pause();
                break;
            }

            case 3: {
                // 显示内存状态
                pool.displayStatus(std::cout);
                pause();
                break;
            }

            case 4: {
                // 显示内存地图
                pool.displayStatus(std::cout);
                pool.displayVisual(std::cout);
                pause();
                break;
            }

            case 5: {
                // 碎片分析
                auto stats = pool.getStats();
                std::cout << "\n=== 碎片分析 ===\n";
                std::cout << "总内存:       " << stats.totalSize << " 字节\n";
                std::cout << "已用内存:     " << stats.usedSize << " 字节\n";
                std::cout << "空闲内存:     " << stats.freeSize << " 字节\n";
                std::cout << "总块数:       " << stats.blockCount << "\n";
                std::cout << "空闲块数:     " << stats.freeBlockCount << "\n";
                std::cout << "已分配块数:   " << stats.allocatedBlockCount << "\n";
                std::cout << "最大空闲块:   " << stats.largestFreeBlock << " 字节\n";
                std::cout << "-------------------------------\n";
                std::cout << "外部碎片率: " << std::fixed << std::setprecision(1)
                          << (stats.fragmentationRate * 100) << "%\n";

                if (stats.fragmentationRate > 0.5) {
                    std::cout << "警告：检测到高碎片率！\n";
                } else if (stats.fragmentationRate > 0.25) {
                    std::cout << "提示：存在中等碎片。\n";
                } else {
                    std::cout << "状态：碎片率较低。\n";
                }
                pause();
                break;
            }

            case 6: {
                // 切换分配策略
                printStrategyMenu();
                int strategyChoice = 0;
                std::cin >> strategyChoice;
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                switch (strategyChoice) {
                    case 1:
                        pool.setStrategy(AllocationStrategy::FirstFit);
                        std::cout << "策略已切换为：首次适应 (First Fit)\n";
                        break;
                    case 2:
                        pool.setStrategy(AllocationStrategy::BestFit);
                        std::cout << "策略已切换为：最佳适应 (Best Fit)\n";
                        break;
                    case 3:
                        pool.setStrategy(AllocationStrategy::WorstFit);
                        std::cout << "策略已切换为：最差适应 (Worst Fit)\n";
                        break;
                    case 0:
                        std::cout << "已取消。\n";
                        break;
                    default:
                        std::cout << "无效选择。\n";
                }
                pause();
                break;
            }

            case 7: {
                // 查看操作历史
                history.displayHistory(std::cout);
                history.displaySummary(std::cout);
                pause();
                break;
            }

            case 8: {
                // 清空历史记录
                history.clear();
                std::cout << "历史记录已清空。\n";
                pause();
                break;
            }

            case 0:
                // 退出
                std::cout << "\n=== 最终统计 ===\n";
                pool.displayStatus(std::cout);
                history.displaySummary(std::cout);
                std::cout << "\n再见！\n";
                break;

            default:
                std::cout << "无效选择，请重试。\n";
                pause();
        }
    }

    return 0;
}
