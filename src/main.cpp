#include <iostream>
#include <string>
#include "skiplist.h"

int main()
{
    SkipList<int, std::string> test_list(6);

    for (int i = 0; i < 10; ++i)
    {
        test_list.InsertElement(i, std::to_string(i));
    }
    std::cout << "test_list size = " << test_list.size() << std::endl;

    test_list.StoreFile();
    test_list.SearchElement(6);
    test_list.SearchElement(10);

    test_list.DisplaySkipList();

    test_list.DeleteElement(1);
    test_list.DeleteElement(4);
    std::cout << "test_list size = " << test_list.size() << std::endl;
    test_list.DisplaySkipList();

    return 0;
}
