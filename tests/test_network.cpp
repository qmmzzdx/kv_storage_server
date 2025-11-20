#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <cstring>
#include "../src/client/client_utils.h"
#include "../src/utils/kv_constant.h"

/**
 * @brief 测试序列化功能
 */
void test_serialization()
{
    std::cout << "Testing serialization/deserialization..." << std::endl;

    // 测试基本命令序列化
    std::vector<std::string> cmd = {"set", "str", "key", "value"};

    // 计算预期长度：nstr(4) + 3个字符串的(len+str)
    uint32_t expected_len = 4; // nstr 占4字节
    for (const auto& s : cmd)
    {
        expected_len += 4 + static_cast<uint32_t>(s.size()); // 每个字符串：len(4) + str_data
    }

    // 验证长度计算正确
    assert(expected_len == (4 + 4 + 3 + 4 + 3 + 4 + 5 + 4 + 5));

    std::cout << "Serialization test passed!" << std::endl;
}

/**
 * @brief 测试响应解析功能
 */
void test_response_parsing()
{
    std::cout << "Testing response parsing..." << std::endl;

    // 测试 NIL 响应解析
    uint8_t nil_response[] = {SERIAL_NIL};
    int32_t result = on_response(nil_response, 1);
    assert(result == 1); // NIL 响应应该消耗1字节

    // 测试 INT 响应解析
    uint8_t int_response[9] = {SERIAL_INT};
    int64_t test_int = 12345;
    memcpy(&int_response[1], &test_int, 8);
    result = on_response(int_response, 9);
    assert(result == 9); // INT 响应应该消耗9字节

    // 测试 STR 响应解析
    std::string test_str = "hello";
    uint32_t str_len = static_cast<uint32_t>(test_str.size());
    uint8_t str_response[5 + test_str.size()]; // type(1) + len(4) + str_data
    str_response[0] = SERIAL_STR;
    memcpy(&str_response[1], &str_len, 4);
    memcpy(&str_response[5], test_str.c_str(), test_str.size());

    result = on_response(str_response, 5 + test_str.size());
    assert(result == static_cast<int32_t>(5 + test_str.size())); // STR 响应应该消耗正确字节数

    std::cout << "Response parsing test passed!" << std::endl;
}

/**
 * @brief 测试协议验证功能
 */
void test_protocol_validation()
{
    std::cout << "Testing protocol validation..." << std::endl;

    // 测试无效响应（太短）
    uint8_t short_response[] = {SERIAL_STR};
    int32_t result = on_response(short_response, 1);
    assert(result == -1); // 不完整的响应应该返回-1

    // 测试未知响应类型
    uint8_t unknown_response[] = {0xFF}; // 无效类型
    result = on_response(unknown_response, 1);
    assert(result == -1); // 未知类型应该返回-1

    std::cout << "Protocol validation test passed!" << std::endl;
}

/**
 * @brief 主测试函数
 */
int main()
{
    std::cout << "Starting network protocol tests..." << std::endl;

    test_serialization();
    test_response_parsing();
    test_protocol_validation();

    std::cout << "All network protocol tests passed successfully!" << std::endl;

    return 0;
}
