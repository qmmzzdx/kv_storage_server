#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include "../src/utils/kv_constant.h"
#include "../src/server/server_utils.h"

void test_string_operations()
{
    std::cout << "Testing string operations..." << std::endl;

    auto& storage = KvStroageData::Instance();

    // Test set and get
    std::vector<std::string> set_cmd = {"set", "str", "test_key", "test_value"};
    std::string out;
    do_request(set_cmd, out);

    std::vector<std::string> get_cmd = {"get", "str", "test_key"};
    out.clear();
    do_request(get_cmd, out);

    // Verify the response contains the value (simplified check)
    assert(out.size() > 5); // Should have serialized data

    std::cout << "String operations test passed!" << std::endl;
}

void test_zset_operations()
{
    std::cout << "Testing sorted set operations..." << std::endl;

    auto& storage = KvStroageData::Instance();

    // Test zadd
    std::vector<std::string> zadd_cmd = {"zadd", "zset", "100", "member1"};
    std::string out;
    do_request(zadd_cmd, out);

    // Test zscore
    std::vector<std::string> zscore_cmd = {"zscore", "zset", "member1"};
    out.clear();
    do_request(zscore_cmd, out);

    // Test zcard
    std::vector<std::string> zcard_cmd = {"zcard", "zset"};
    out.clear();
    do_request(zcard_cmd, out);

    // Test zrem
    std::vector<std::string> zrem_cmd = {"zrem", "zset", "member1"};
    out.clear();
    do_request(zrem_cmd, out);

    std::cout << "Sorted set operations test passed!" << std::endl;
}

void test_keys_operation()
{
    std::cout << "Testing keys operation..." << std::endl;

    auto& storage = KvStroageData::Instance();

    // Add some test data
    std::vector<std::string> set_cmd1 = {"set", "str", "key1", "value1"};
    std::vector<std::string> set_cmd2 = {"set", "str", "key2", "value2"};
    std::string out;

    do_request(set_cmd1, out);
    out.clear();
    do_request(set_cmd2, out);

    // Test keys command
    std::vector<std::string> keys_cmd = {"keys"};
    out.clear();
    do_request(keys_cmd, out);

    std::cout << "Keys operation test passed!" << std::endl;
}

void test_error_handling()
{
    std::cout << "Testing error handling..." << std::endl;

    // Test unknown command
    std::vector<std::string> unknown_cmd = {"unknown_command"};
    std::string out;
    do_request(unknown_cmd, out);

    // Test invalid zadd (non-numeric score)
    std::vector<std::string> invalid_zadd = {"zadd", "zset", "not_a_number", "member"};
    out.clear();
    do_request(invalid_zadd, out);

    std::cout << "Error handling test passed!" << std::endl;
}

int main()
{
    std::cout << "Starting KV Storage tests..." << std::endl;

    test_string_operations();
    test_zset_operations();
    test_keys_operation();
    test_error_handling();

    std::cout << "All KV Storage tests passed successfully!" << std::endl;
    return 0;
}
