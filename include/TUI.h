#ifndef TUI_H
#define TUI_H

#include "MemoryPool.h"
#include <string>
#include <iostream>

namespace tui {

// ==================== ANSI 颜色定义 ====================

namespace color {
    // 重置
    constexpr const char* RESET     = "\033[0m";

    // 样式
    constexpr const char* BOLD      = "\033[1m";
    constexpr const char* DIM       = "\033[2m";

    // 前景色
    constexpr const char* BLACK     = "\033[30m";
    constexpr const char* RED       = "\033[31m";
    constexpr const char* GREEN     = "\033[32m";
    constexpr const char* YELLOW    = "\033[33m";
    constexpr const char* BLUE      = "\033[34m";
    constexpr const char* MAGENTA   = "\033[35m";
    constexpr const char* CYAN      = "\033[36m";
    constexpr const char* WHITE     = "\033[37m";
    constexpr const char* GRAY      = "\033[90m";

    // 亮色
    constexpr const char* BRIGHT_GREEN  = "\033[92m";
    constexpr const char* BRIGHT_YELLOW = "\033[93m";
    constexpr const char* BRIGHT_CYAN   = "\033[96m";
}

// ==================== 消息类型 ====================

enum class MessageType {
    Info,
    Success,
    Warning,
    Error
};

// ==================== 界面宽度常量 ====================

constexpr int SCREEN_WIDTH = 65;
constexpr int MEMORY_BAR_WIDTH = 40;

// ==================== 绘制函数 ====================

/**
 * @brief 清屏
 */
void clearScreen();

/**
 * @brief 绘制启动画面（ASCII Art Logo）
 */
void drawSplashScreen();

/**
 * @brief 绘制主界面（含内存状态）
 */
void drawMainScreen(const mem::MemoryPool& pool, const std::string& lastMessage = "", MessageType msgType = MessageType::Info);

/**
 * @brief 绘制内存使用进度条
 */
void drawMemoryBar(const mem::PoolStats& stats, std::ostream& os = std::cout);

/**
 * @brief 绘制内存地图（块分布）
 */
void drawMemoryMap(const mem::MemoryPool& pool, std::ostream& os = std::cout);

/**
 * @brief 绘制内存块详情表格
 */
void drawBlockDetails(const mem::MemoryPool& pool, std::ostream& os = std::cout);

/**
 * @brief 绘制碎片分析界面
 */
void drawFragmentAnalysis(const mem::MemoryPool& pool, std::ostream& os = std::cout);

/**
 * @brief 绘制带颜色的消息
 */
void drawMessage(const std::string& msg, MessageType type, std::ostream& os = std::cout);

/**
 * @brief 绘制策略选择菜单
 */
void drawStrategyMenu(mem::AllocationStrategy current, std::ostream& os = std::cout);

/**
 * @brief 绘制水平分隔线
 */
void drawHorizontalLine(char left, char mid, char right, std::ostream& os = std::cout);

/**
 * @brief 暂停等待用户按键
 */
void waitForEnter();

/**
 * @brief 获取策略的中文名称
 */
std::string getStrategyName(mem::AllocationStrategy strategy);

} // namespace tui

#endif // TUI_H
