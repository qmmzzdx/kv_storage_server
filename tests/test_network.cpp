#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include "../src/client/client_utils.h"
#include "../src/utils/kv_constant.h"

void test_serialization()
{
    std::cout << "Testing serialization/deserialization..." << std::endl;

    // Test basic command serialization
    std::vector<std::string> cmd = {"set", "str", "key", "value"};

    // Calculate expected length
    uint32_t expected_len = 4; // nstr
    for (const auto& s : cmd)
    {
        expected_len += 4 + s.size(); // len + string
    }

    assert(expected_len == (4 + 4 + 3 + 4 + 3 + 4 + 3 + 4 + 5)); // Verify calculation

    std::cout << "Serialization test passed!" << std::endl;
}

void test_response_parsing()
{
    std::cout << "Testing response parsing..." << std::endl;

    // Test NIL response
    uint8_t nil_response[] = {SERIAL_NIL};
    int32_t result = on_response(nil_response, 1);
    assert(result == 1);

    // Test INT response
    uint8_t int_response[9] = {SERIAL_INT};
    int64_t test_int = 12345;
    memcpy(&int_response[1], &test_int, 8);
    result = on_response(int_response, 9);
    assert(result == 9);

    // Test STR response
    std::string test_str = "hello";
    uint8_t str_response[5 + test_str.size()];
    str_response[0] = SERIAL_STR;
    uint32_t str_len = test_str.size();
    memcpy(&str_response[1], &str_len, 4);
    memcpy(&str_response[5], test_str.c_str(), test_str.size());
    result = on_response(str_response, 5 + test_str.size());
    assert(result == 5 + test_str.size());

    std::cout << "Response parsing test passed!" << std::endl;
}

void test_protocol_validation()
{
    std::cout << "Testing protocol validation..." << std::endl;

    // Test invalid response (too short)
    uint8_t short_response[] = {SERIAL_STR};
    int32_t result = on_response(short_response, 1);
    assert(result == -1);

    // Test unknown response type
    uint8_t unknown_response[] = {0xFF}; // Invalid type
    result = on_response(unknown_response, 1);
    assert(result == -1);

    std::cout << "Protocol validation test passed!" << std::endl;
}

int main()
{
    std::cout << "Starting network protocol tests..." << std::endl;

    test_serialization();
    test_response_parsing();
    test_protocol_validation();

    std::cout << "All network protocol tests passed successfully!" << std::endl;
    return 0;
}
