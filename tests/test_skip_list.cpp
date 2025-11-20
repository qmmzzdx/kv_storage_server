#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include "../src/utils/kv_constant.h"
#include "../src/server/server_utils.h"

void test_skip_list_basic()
{
    std::cout << "Testing SkipList basic operations..." << std::endl;

    SkipList list;

    // Test insert
    assert(list.insert(100, "value100") == true);
    assert(list.insert(200, "value200") == true);
    assert(list.insert(50, "value50") == true);

    // Test search
    assert(list.search(100) == true);
    assert(list.search(200) == true);
    assert(list.search(50) == true);
    assert(list.search(300) == false); // Should not exist

    // Test cancel (delete)
    assert(list.cancel(100) == true);
    assert(list.search(100) == false); // Should be deleted
    assert(list.cancel(100) == false); // Already deleted

    // Test size
    assert(list.size() == 2);

    std::cout << "SkipList basic operations test passed!" << std::endl;
}

void test_skip_list_duplicate()
{
    std::cout << "Testing SkipList duplicate keys..." << std::endl;

    SkipList list;

    // Test duplicate insertion
    assert(list.insert(100, "value1") == true);
    assert(list.insert(100, "value2") == false); // Should fail for duplicate

    std::cout << "SkipList duplicate keys test passed!" << std::endl;
}

void test_skip_list_performance()
{
    std::cout << "Testing SkipList performance with 1000 elements..." << std::endl;

    SkipList list;
    const int TEST_SIZE = 1000;

    // Insert test data
    for (int i = 0; i < TEST_SIZE; i++)
    {
        assert(list.insert(i, "value" + std::to_string(i)) == true);
    }

    // Verify all elements can be found
    for (int i = 0; i < TEST_SIZE; i++)
    {
        assert(list.search(i) == true);
    }

    // Delete half of the elements
    for (int i = 0; i < TEST_SIZE; i += 2)
    {
        assert(list.cancel(i) == true);
    }

    // Verify deletion
    for (int i = 0; i < TEST_SIZE; i++)
    {
        if (i % 2 == 0)
        {
            assert(list.search(i) == false);
        }
        else
        {
            assert(list.search(i) == true);
        }
    }

    assert(list.size() == TEST_SIZE / 2);
    std::cout << "SkipList performance test passed!" << std::endl;
}

int main()
{
    std::cout << "Starting SkipList tests..." << std::endl;

    test_skip_list_basic();
    test_skip_list_duplicate();
    test_skip_list_performance();

    std::cout << "All SkipList tests passed successfully!" << std::endl;
    return 0;
}
