#include <iostream>
#include <chrono>
#include <thread>
#include <limits>
#include <vector>
#include <string>
#include <functional>
#include "../src/skiplist.h"

SkipList<int, std::string> stress_test_lists(18);
const unsigned int WRITERS_COUNT = 1;
const unsigned int READERS_COUNT = std::thread::hardware_concurrency();
const unsigned int TEST_ELEMENTS = 100000;

void InsertElementToSkipList(unsigned int tid, unsigned int spliter)
{
    for (unsigned int i = spliter * tid, cnt = 0; cnt < spliter; ++i, ++cnt)
    {
        int val = rand() % TEST_ELEMENTS;
        stress_test_lists.InsertElement(val, std::to_string(val + WRITERS_COUNT));
    }
}

void GetElementFromSkipList(unsigned int tid, unsigned int spliter)
{
    for (unsigned int i = spliter * tid, cnt = 0; cnt < spliter; ++i, ++cnt)
    {
        int val = rand() % TEST_ELEMENTS;
        stress_test_lists.SearchElement(val);
    }
}

void TestSkipListPerformance(std::function<void(unsigned int, unsigned int)> &&T, std::string &&s, unsigned int cnt)
{
    std::vector<std::thread> ts(cnt);

    auto start = std::chrono::high_resolution_clock::now();
    for (unsigned int i = 0; i < cnt; ++i)
    {
        ts[i] = std::thread(T, i, TEST_ELEMENTS / cnt);
    }
    for (unsigned int i = 0; i < cnt; ++i)
    {
        if (ts[i].joinable())
        {
            ts[i].join();
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration_time = end - start;
    std::cout << "Total consuming times" << s << ": " << duration_time.count() << "s." << std::endl;
}

int main()
{
    srand(time(nullptr));
    
    std::cout << "CPU cores: " << std::thread::hardware_concurrency() << std::endl;
    std::cout << "Total elements: " << TEST_ELEMENTS << std::endl;
    TestSkipListPerformance(InsertElementToSkipList, "(InsertElementToSkipList)", WRITERS_COUNT);
    TestSkipListPerformance(GetElementFromSkipList, "(GetElementFromSkipList)", READERS_COUNT);

    return 0;
}
