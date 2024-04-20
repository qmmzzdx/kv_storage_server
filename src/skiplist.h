#ifndef SKIPLIST_H
#define SKIPLIST_H

#include <iostream>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>
#include <memory>
#include <random>
#include <cstdlib>
#include <shared_mutex>
#include "asynclog.h"

template <typename K, typename V>
class Node
{
public:
    Node() {}

    Node(const K &_key, const V &_value, const int level) : key(_key), value(_value)
    {
        node_ptr_list.resize(level + 1);
    }

    ~Node() {}

    K GetKey() const { return key; }

    V GetValue() const { return value; }

    void SetValue(const V _value) { value = _value; }

    std::vector<std::shared_ptr<Node<K, V>>> node_ptr_list;

private:
    K key;
    V value;
};

template <typename K, typename V>
class SkipList
{
public:
    SkipList(int _max_level);

    ~SkipList() {}

    int GetRandomLevel();

    std::shared_ptr<Node<K, V>> CreateNode(const K &_key, const V &_value, const int _level);

    void DisplaySkipList();

    bool InsertElement(const K &_key, const V &_value);

    bool SearchElement(const K &_key);

    bool DeleteElement(const K &_key);

    void StoreFile();

    void LoadFile();

    int size() { return element_count; }

protected:
    bool GetKeyValueFromString(const std::string &s, std::string &skey, std::string &svalue);

    bool IsValidString(const std::string &s);

private:
    int max_level;
    int skiplist_level;
    int element_count;

    std::shared_mutex mtx;
    std::string delimiter = ":";
    std::string file_path = "./doc/kvdb.txt";
    std::ofstream file_writer;
    std::ifstream file_reader;
    std::shared_ptr<Node<K, V>> header;
};

template <typename K, typename V>
SkipList<K, V>::SkipList(int _max_level) : max_level(_max_level), skiplist_level(0), element_count(0)
{
    header = std::make_shared<Node<K, V>>(K(), V(), max_level);
}

template <typename K, typename V>
int SkipList<K, V>::GetRandomLevel()
{
    int cur_level = 0;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::bernoulli_distribution distrib(0.25);

    while (distrib(gen))
    {
        ++cur_level;
    }
    return std::min(cur_level, max_level);
}

template <typename K, typename V>
std::shared_ptr<Node<K, V>> SkipList<K, V>::CreateNode(const K &_key, const V &_value, const int _level)
{
    return std::make_shared<Node<K, V>>(_key, _value, _level);
}

template <typename K, typename V>
void SkipList<K, V>::DisplaySkipList()
{
    std::shared_lock<std::shared_mutex> lk(mtx);
    std::cout << "+--------SkipList--------+\n";
    for (int i = 0; i <= skiplist_level; ++i)
    {
        std::string strprint = "Level " + std::to_string(i) + ": \n";
        std::cout << strprint;
        for (auto cur = header->node_ptr_list[i]; cur != nullptr; cur = cur->node_ptr_list[i])
        {
            strprint = "Key = " + std::to_string(cur->GetKey()) + ", Value = " + cur->GetValue() + "\n";
            std::cout << strprint;
        }
    }
    std::cout << "+--------SkipList--------+\n";
}

template <typename K, typename V>
bool SkipList<K, V>::InsertElement(const K &_key, const V &_value)
{
    std::lock_guard<std::shared_mutex> lk(mtx);
    std::shared_ptr<Node<K, V>> cur = header;
    std::vector<std::shared_ptr<Node<K, V>>> update(max_level + 1);

    for (int i = skiplist_level; i >= 0; --i)
    {
        while (cur->node_ptr_list[i] != nullptr && cur->node_ptr_list[i]->GetKey() < _key)
        {
            cur = cur->node_ptr_list[i];
        }
        update[i] = cur;
    }
    cur = cur->node_ptr_list[0];
    if (cur != nullptr && cur->GetKey() == _key)
    {
        std::string strprint = "Found Key = " + std::to_string(_key) + ", exist InsertElement\n";
        AsyncLog::LOG_WARN(strprint);
        return false;
    }
    if (cur == nullptr || cur->GetKey() != _key)
    {
        int random_level = GetRandomLevel();
        if (skiplist_level < random_level)
        {
            for (int i = skiplist_level + 1; i <= random_level; ++i)
            {
                update[i] = header;
            }
            skiplist_level = random_level;
        }
        auto new_node = CreateNode(_key, _value, random_level);
        for (int i = 0; i <= random_level; ++i)
        {
            new_node->node_ptr_list[i] = update[i]->node_ptr_list[i];
            update[i]->node_ptr_list[i] = new_node;
        }
        ++element_count;
        std::string strprint = "Successfully insert Key = " + std::to_string(_key) + ", Value = " + _value + "\n";
        AsyncLog::LOG_INFO(strprint);
        return true;
    }
    return false;
}

template <typename K, typename V>
bool SkipList<K, V>::SearchElement(const K &_key)
{
    std::shared_lock<std::shared_mutex> lk(mtx);
    std::shared_ptr<Node<K, V>> cur = header;

    for (int i = skiplist_level; i >= 0; --i)
    {
        while (cur->node_ptr_list[i] != nullptr && cur->node_ptr_list[i]->GetKey() < _key)
        {
            cur = cur->node_ptr_list[i];
        }
    }
    cur = cur->node_ptr_list[0];
    if (cur != nullptr && cur->GetKey() == _key)
    {
        std::string strprint = "Found Key = " + std::to_string(_key) + ", Value = " + cur->GetValue() + "\n";
        AsyncLog::LOG_INFO(strprint);
        return true;
    }
    std::string strprint = "Not found Key = " + std::to_string(_key) + "\n";
    AsyncLog::LOG_WARN(strprint);
    return false;
}

template <typename K, typename V>
bool SkipList<K, V>::DeleteElement(const K &_key)
{
    std::lock_guard<std::shared_mutex> lk(mtx);
    std::shared_ptr<Node<K, V>> cur = header;
    std::vector<std::shared_ptr<Node<K, V>>> update(max_level + 1);

    for (int i = skiplist_level; i >= 0; --i)
    {
        while (cur->node_ptr_list[i] != nullptr && cur->node_ptr_list[i]->GetKey() < _key)
        {
            cur = cur->node_ptr_list[i];
        }
        update[i] = cur;
    }
    cur = cur->node_ptr_list[0];
    if (cur != nullptr && cur->GetKey() == _key)
    {
        for (int i = 0; i <= skiplist_level; ++i)
        {
            if (update[i]->node_ptr_list[i] != cur)
            {
                break;
            }
            update[i]->node_ptr_list[i] = cur->node_ptr_list[i];
        }
        while (skiplist_level > 0 && header->node_ptr_list[skiplist_level] == nullptr)
        {
            --skiplist_level;
        }
        std::string strprint = "Successfully delete Key = " + std::to_string(_key) + ", Value = " + cur->GetValue() + "\n";
        AsyncLog::LOG_INFO(strprint);
        --element_count;
        return true;
    }
    std::string strprint = "Not found Key = " + std::to_string(_key) + ", exist DeleteElement\n";
    AsyncLog::LOG_WARN(strprint);
    return false;
}

template <typename K, typename V>
void SkipList<K, V>::StoreFile()
{
    std::lock_guard<std::shared_mutex> lk(mtx);
    std::string strprint = "Store skiplist to " + file_path + "\n";
    std::cout << strprint;

    file_writer.open(file_path);
    for (auto cur = header->node_ptr_list[0]; cur != nullptr; cur = cur->node_ptr_list[0])
    {
        file_writer << cur->GetKey() << ": " << cur->GetValue() << std::endl;
        strprint = "Key = " + std::to_string(cur->GetKey()) + ", Value = " + cur->GetValue() + "\n";
        AsyncLog::LOG_INFO(strprint);
    }
    file_writer.flush();
    file_writer.close();
}

template <typename K, typename V>
void SkipList<K, V>::LoadFile()
{
    std::shared_lock<std::shared_mutex> lk(mtx);
    std::string line, cur_key, cur_value;
    std::string strprint = "Load skiplist from " + file_path + "\n";
    std::cout << strprint;

    file_reader.open(file_path);
    if (!file_reader.is_open())
    {
        strprint = "Could not open the file " + file_path + "\n";
        AsyncLog::LOG_ERROR(strprint);
        AsyncLog::LOG_ERROR("Program terminating.\n");
        exit(EXIT_FAILURE);
    }
    while (getline(file_reader, line))
    {
        if (GetKeyValueFromString(line, cur_key, cur_value) && InsertElement(stoi(cur_key), cur_value))
        {
            strprint = "Insert Key = " + cur_key + ", Value = " + cur_value + " from " + file_path + "\n";
            AsyncLog::LOG_INFO(strprint);
        }
    }
    file_reader.close();
}

template <typename K, typename V>
bool SkipList<K, V>::GetKeyValueFromString(const std::string &s, std::string &skey, std::string &svalue)
{
    if (IsValidString(s))
    {
        skey = s.substr(0, s.find(delimiter));
        svalue = s.substr(s.find(delimiter) + 2, s.size());
        return !skey.empty() && !svalue.empty();
    }
    return false;
}

template <typename K, typename V>
bool SkipList<K, V>::IsValidString(const std::string &s)
{
    return !s.empty() && s.find(delimiter) != std::string::npos;
}

#endif
