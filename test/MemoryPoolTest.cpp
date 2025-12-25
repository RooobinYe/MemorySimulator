/**
 * @file MemoryPoolTest.cpp
 * @brief 内存池管理算法测试程序
 *
 * 测试数据采用三元组形式：（输入，理想输出，实际输出）
 * 包含：合法数据、非法数据、边界数据测试
 */

#include "MemoryPool.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>

using namespace mem;

// ======================= 测试框架 =======================

int g_total = 0, g_passed = 0;

// 测试断言：记录并输出三元组结果
#define TEST(name, input, expected, actual) do { \
    g_total++; \
    bool pass = ((expected) == (actual)); \
    if (pass) g_passed++; \
    std::cout << (pass ? "[PASS] " : "[FAIL] ") << name << "\n"; \
    std::cout << "       输入: " << input << " | 理想: " << (expected) << " | 实际: " << (actual) << "\n"; \
} while(0)

void printLine(char c = '-') { std::cout << std::string(60, c) << "\n"; }

// ======================= 一、合法数据测试 =======================

void testValidData() {
    std::cout << "\n【一、合法数据测试】\n";
    printLine('=');

    // 1.1 基本分配与释放
    {
        MemoryPool pool(1000);
        auto id1 = pool.allocate(100);
        auto id2 = pool.allocate(200);
        TEST("1.1 基本分配", "allocate(100,200)", true, id1.has_value() && id2.has_value());

        auto stats = pool.getStats();
        TEST("1.2 已用空间统计", "usedSize", (size_t)300, stats.usedSize);

        bool free1 = pool.deallocate(id1.value());
        bool free2 = pool.deallocate(id2.value());
        TEST("1.3 基本释放", "deallocate(id1,id2)", true, free1 && free2);

        stats = pool.getStats();
        TEST("1.4 释放后合并", "blockCount", (size_t)1, stats.blockCount);
    }

    // 1.2 三种分配算法
    {
        MemoryPool pool1(500, AllocationStrategy::FirstFit);
        MemoryPool pool2(500, AllocationStrategy::BestFit);
        MemoryPool pool3(500, AllocationStrategy::WorstFit);

        auto r1 = pool1.allocate(100);
        auto r2 = pool2.allocate(100);
        auto r3 = pool3.allocate(100);
        TEST("1.5 FirstFit分配", "allocate(100)", true, r1.has_value());
        TEST("1.6 BestFit分配", "allocate(100)", true, r2.has_value());
        TEST("1.7 WorstFit分配", "allocate(100)", true, r3.has_value());
    }

    // 1.3 内存块合并测试
    {
        MemoryPool pool(1000);
        auto id1 = pool.allocate(200);
        auto id2 = pool.allocate(200);
        auto id3 = pool.allocate(200);

        pool.deallocate(id2.value());  // 释放中间块
        pool.deallocate(id1.value());  // 应与id2空间合并
        pool.deallocate(id3.value());  // 全部合并

        auto stats = pool.getStats();
        TEST("1.8 完全合并", "freeSize", (size_t)1000, stats.freeSize);
    }
}

// ======================= 二、非法数据测试 =======================

void testInvalidData() {
    std::cout << "\n【二、非法数据测试】\n";
    printLine('=');

    MemoryPool pool(500);

    // 2.1 分配0字节
    auto r1 = pool.allocate(0);
    TEST("2.1 分配0字节", "allocate(0)", false, r1.has_value());

    // 2.2 分配超过容量
    auto r2 = pool.allocate(1000);
    TEST("2.2 超容量分配", "allocate(1000) on 500B", false, r2.has_value());

    // 2.3 释放无效ID
    bool r3 = pool.deallocate(-1);
    TEST("2.3 释放无效ID", "deallocate(-1)", false, r3);

    bool r4 = pool.deallocate(9999);
    TEST("2.4 释放不存在ID", "deallocate(9999)", false, r4);

    // 2.5 重复释放
    auto id = pool.allocate(100);
    pool.deallocate(id.value());
    bool r5 = pool.deallocate(id.value());
    TEST("2.5 重复释放", "deallocate(已释放)", false, r5);

    // 2.6 内存耗尽后分配
    MemoryPool smallPool(100);
    smallPool.allocate(100);
    auto r6 = smallPool.allocate(1);
    TEST("2.6 耗尽后分配", "allocate(1) on 已满", false, r6.has_value());
}

// ======================= 三、边界数据测试 =======================

void testBoundaryData() {
    std::cout << "\n【三、边界数据测试】\n";
    printLine('=');

    // 3.1 最小值测试
    {
        MemoryPool pool(1);
        auto r = pool.allocate(1);
        TEST("3.1 分配1字节", "allocate(1) on 1B池", true, r.has_value());
    }

    // 3.2 精确匹配（无剩余）
    {
        MemoryPool pool(100);
        auto id1 = pool.allocate(50);
        auto id2 = pool.allocate(50);
        (void)id1; (void)id2;
        auto stats = pool.getStats();
        TEST("3.2 精确分配", "freeBlockCount", (size_t)0, stats.freeBlockCount);
    }

    // 3.3 分配全部容量
    {
        MemoryPool pool(1024);
        auto r = pool.allocate(1024);
        TEST("3.3 分配全部容量", "allocate(1024)", true, r.has_value());
    }

    // 3.4 边界值：比容量大1
    {
        MemoryPool pool(1024);
        auto r = pool.allocate(1025);
        TEST("3.4 超出1字节", "allocate(1025) on 1024B", false, r.has_value());
    }

    // 3.5 碎片率测试
    {
        MemoryPool pool(1000);
        auto stats = pool.getStats();
        TEST("3.5 初始碎片率", "fragmentationRate", 0.0, stats.fragmentationRate);

        auto id1 = pool.allocate(200);
        auto id2 = pool.allocate(200);
        auto id3 = pool.allocate(200);
        (void)id2;
        pool.deallocate(id1.value());
        pool.deallocate(id3.value());

        stats = pool.getStats();
        TEST("3.6 产生碎片", "fragmentationRate>0", true, stats.fragmentationRate > 0);
    }

    // 3.7 压力测试：连续分配释放
    {
        MemoryPool pool(5000);
        bool success = true;
        for (int i = 0; i < 50; i++) {
            auto id = pool.allocate(50);
            if (!id.has_value() || !pool.deallocate(id.value())) {
                success = false;
                break;
            }
        }
        TEST("3.7 循环分配释放", "50次alloc/dealloc", true, success);
    }
}

// ======================= 主函数 =======================

int main() {
    std::cout << "\n";
    printLine('*');
    std::cout << "        内存池管理算法测试 (三元组形式)\n";
    printLine('*');

    testValidData();
    testInvalidData();
    testBoundaryData();

    // 输出统计
    std::cout << "\n";
    printLine('=');
    std::cout << "测试统计: " << g_passed << "/" << g_total << " 通过 ("
              << std::fixed << std::setprecision(1)
              << (100.0 * g_passed / g_total) << "%)\n";
    printLine('=');

    return g_passed == g_total ? 0 : 1;
}
