#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <random>
#include "../src/utils/kv_constant.h"
#include "../src/server/server_utils.h"

void test_skip_list_performance()
{
    std::cout << "Testing SkipList performance..." << std::endl;

    SkipList list;
    const int OPERATIONS = 10000;

    auto start = std::chrono::high_resolution_clock::now();

    // Insert performance
    for (int i = 0; i < OPERATIONS; i++)
    {
        list.insert(i, "value" + std::to_string(i));
    }

    auto insert_end = std::chrono::high_resolution_clock::now();

    // Search performance
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, OPERATIONS - 1);

    for (int i = 0; i < OPERATIONS; i++)
    {
        list.search(dist(gen));
    }

    auto search_end = std::chrono::high_resolution_clock::now();

    auto insert_duration = std::chrono::duration_cast<std::chrono::milliseconds>(insert_end - start);
    auto search_duration = std::chrono::duration_cast<std::chrono::milliseconds>(search_end - insert_end);

    std::cout << "SkipList Performance Results:" << std::endl;
    std::cout << "  Insert " << OPERATIONS << " elements: " << insert_duration.count() << " ms" << std::endl;
    std::cout << "  Search " << OPERATIONS << " elements: " << search_duration.count() << " ms" << std::endl;
    std::cout << "  Insert throughput: " << (OPERATIONS * 1000.0 / insert_duration.count()) << " ops/sec" << std::endl;
    std::cout << "  Search throughput: " << (OPERATIONS * 1000.0 / search_duration.count()) << " ops/sec" << std::endl;
}

void test_kv_storage_performance()
{
    std::cout << "Testing KV Storage performance..." << std::endl;

    auto& storage = KvStroageData::Instance();
    const int OPERATIONS = 5000;

    auto start = std::chrono::high_resolution_clock::now();

    // Set performance
    for (int i = 0; i < OPERATIONS; i++)
    {
        std::vector<std::string> cmd = {"set", "str", "key" + std::to_string(i), "value" + std::to_string(i)};
        std::string out;
        do_request(cmd, out);
    }

    auto set_end = std::chrono::high_resolution_clock::now();

    // Get performance
    for (int i = 0; i < OPERATIONS; i++)
    {
        std::vector<std::string> cmd = {"get", "str", "key" + std::to_string(i)};
        std::string out;
        do_request(cmd, out);
    }

    auto get_end = std::chrono::high_resolution_clock::now();

    auto set_duration = std::chrono::duration_cast<std::chrono::milliseconds>(set_end - start);
    auto get_duration = std::chrono::duration_cast<std::chrono::milliseconds>(get_end - set_end);

    std::cout << "KV Storage Performance Results:" << std::endl;
    std::cout << "  Set " << OPERATIONS << " operations: " << set_duration.count() << " ms" << std::endl;
    std::cout << "  Get " << OPERATIONS << " operations: " << get_duration.count() << " ms" << std::endl;
    std::cout << "  Set throughput: " << (OPERATIONS * 1000.0 / set_duration.count()) << " ops/sec" << std::endl;
    std::cout << "  Get throughput: " << (OPERATIONS * 1000.0 / get_duration.count()) << " ops/sec" << std::endl;
}

int main()
{
    std::cout << "Starting performance tests..." << std::endl;

    test_skip_list_performance();
    test_kv_storage_performance();

    std::cout << "All performance tests completed!" << std::endl;
    return 0;
}
