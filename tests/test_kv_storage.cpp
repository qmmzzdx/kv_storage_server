#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include "../src/utils/kv_constant.h"
#include "../src/server/server_utils.h"

/**
 * @brief 测试字符串操作：set, get
 */
void test_string_operations()
{
    std::cout << "Testing string operations..." << std::endl;

    // 测试 set 命令
    std::vector<std::string> set_cmd = {"set", "str", "test_key", "test_value"};
    std::string out;
    do_request(set_cmd, out);

    // 验证输出不为空（应该有序列化响应）
    assert(!out.empty());
    assert(out[0] == SERIAL_NIL); // set 操作应该返回 NIL

    std::cout << "String operations test passed!" << std::endl;
}

/**
 * @brief 测试有序集合操作：zadd, zscore, zcard, zrem
 */
void test_zset_operations()
{
    std::cout << "Testing sorted set operations..." << std::endl;

    // 测试 zadd 命令
    std::vector<std::string> zadd_cmd = {"zadd", "zset", "100", "member1"};
    std::string out;
    do_request(zadd_cmd, out);

    // 验证输出不为空且包含整数响应
    assert(!out.empty());
    assert(out[0] == SERIAL_INT); // zadd 成功应该返回整数 1

    // 测试 zscore 命令
    std::vector<std::string> zscore_cmd = {"zscore", "zset", "member1"};
    out.clear();
    do_request(zscore_cmd, out);

    // 验证输出不为空
    assert(!out.empty());

    std::cout << "Sorted set operations test passed!" << std::endl;
}

/**
 * @brief 测试 keys 命令
 */
void test_keys_operation()
{
    std::cout << "Testing keys operation..." << std::endl;

    // 先添加一些测试数据
    std::vector<std::string> set_cmd1 = {"set", "str", "test_key1", "value1"};
    std::vector<std::string> set_cmd2 = {"set", "str", "test_key2", "value2"};
    std::string out;

    do_request(set_cmd1, out);
    out.clear();
    do_request(set_cmd2, out);

    // 测试 keys 命令
    std::vector<std::string> keys_cmd = {"keys"};
    out.clear();
    do_request(keys_cmd, out);

    // 验证输出不为空（应该返回数组）
    assert(!out.empty());
    assert(out[0] == SERIAL_ARR); // keys 命令应该返回数组

    std::cout << "Keys operation test passed!" << std::endl;
}

/**
 * @brief 测试错误处理
 */
void test_error_handling()
{
    std::cout << "Testing error handling..." << std::endl;

    // 测试未知命令
    std::vector<std::string> unknown_cmd = {"unknown_command"};
    std::string out;
    do_request(unknown_cmd, out);

    // 验证输出包含错误信息
    assert(!out.empty());
    assert(out[0] == SERIAL_ERR); // 未知命令应该返回错误

    // 测试无效的 zadd（非数字分数）
    std::vector<std::string> invalid_zadd = {"zadd", "zset", "not_a_number", "member"};
    out.clear();
    do_request(invalid_zadd, out);

    // 验证输出包含错误信息
    assert(!out.empty());
    assert(out[0] == SERIAL_ERR); // 无效参数应该返回错误

    std::cout << "Error handling test passed!" << std::endl;
}

/**
 * @brief 主测试函数
 */
int main()
{
    std::cout << "Starting KV Storage tests..." << std::endl;

    test_string_operations();
    test_zset_operations();
    test_keys_operation();
    test_error_handling();

    std::cout << "All KV Storage tests passed successfully!" << std::endl;

    // 关闭日志系统以确保程序正常退出
    AsyncLog::AsyncLog::Instance().Close();

    return 0;
}
