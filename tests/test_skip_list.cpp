#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include "../src/utils/kv_constant.h"
#include "../src/server/server_utils.h"

/**
 * @brief 测试跳表的基本操作：插入、搜索、删除
 */
void test_skip_list_basic()
{
    std::cout << "Testing SkipList basic operations..." << std::endl;

    // 创建跳表实例
    SkipList list;

    // 测试插入操作
    assert(list.insert(100, "value100") == true);
    assert(list.insert(200, "value200") == true);
    assert(list.insert(50, "value50") == true);

    // 测试搜索操作
    assert(list.search(100) == true);
    assert(list.search(200) == true);
    assert(list.search(50) == true);
    assert(list.search(300) == false); // 不存在的键应该返回false

    // 测试删除操作
    assert(list.cancel(100) == true);
    assert(list.search(100) == false); // 删除后应该找不到
    assert(list.cancel(100) == false); // 重复删除应该返回false

    // 测试大小
    assert(list.size() == 2);

    std::cout << "SkipList basic operations test passed!" << std::endl;
}

/**
 * @brief 测试跳表的重复键处理
 */
void test_skip_list_duplicate()
{
    std::cout << "Testing SkipList duplicate keys..." << std::endl;

    SkipList list;

    // 测试重复键插入
    assert(list.insert(100, "first_value") == true);
    assert(list.insert(100, "second_value") == false); // 重复键应该插入失败

    std::cout << "SkipList duplicate keys test passed!" << std::endl;
}

/**
 * @brief 测试跳表的性能和大数据量处理
 */
void test_skip_list_performance()
{
    std::cout << "Testing SkipList performance with 1000 elements..." << std::endl;

    SkipList list;
    const int TEST_SIZE = 1000;

    // 插入测试数据
    for (int i = 0; i < TEST_SIZE; i++)
    {
        assert(list.insert(i, "value" + std::to_string(i)) == true);
    }

    // 验证所有元素都能找到
    for (int i = 0; i < TEST_SIZE; i++)
    {
        assert(list.search(i) == true);
    }

    // 删除一半元素
    for (int i = 0; i < TEST_SIZE; i += 2)
    {
        assert(list.cancel(i) == true);
    }

    // 验证删除结果
    for (int i = 0; i < TEST_SIZE; i++)
    {
        if (i % 2 == 0)
        {
            assert(list.search(i) == false); // 删除的元素应该找不到
        }
        else
        {
            assert(list.search(i) == true);  // 未删除的元素应该能找到
        }
    }

    // 验证最终大小
    assert(list.size() == TEST_SIZE / 2);
    std::cout << "SkipList performance test passed!" << std::endl;
}

/**
 * @brief 主测试函数
 */
int main()
{
    std::cout << "Starting SkipList tests..." << std::endl;

    test_skip_list_basic();
    test_skip_list_duplicate();
    test_skip_list_performance();

    std::cout << "All SkipList tests passed successfully!" << std::endl;

    // 关闭日志系统以确保程序正常退出
    AsyncLog::AsyncLog::Instance().Close();

    return 0;
}
