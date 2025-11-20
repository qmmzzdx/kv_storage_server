CC = g++
CXXFLAGS = -std=c++17 -g -Wall
CXXTHREAD = -lpthread

# 主目标
kv_storage:
	$(CC) ./src/server/*.cpp -o ./bin/kv_server $(CXXFLAGS) $(CXXTHREAD)
	$(CC) ./src/client/*.cpp -o ./bin/kv_client $(CXXFLAGS) $(CXXTHREAD)

# 测试目标
test_skip_list:
	$(CC) ./tests/test_skip_list.cpp ./src/server/*.cpp -o ./bin/test_skip_list $(CXXFLAGS) $(CXXTHREAD) -Isrc

test_kv_storage:
	$(CC) ./tests/test_kv_storage.cpp ./src/server/*.cpp -o ./bin/test_kv_storage $(CXXFLAGS) $(CXXTHREAD) -Isrc

test_network:
	$(CC) ./tests/test_network.cpp ./src/client/*.cpp ./src/server/*.cpp -o ./bin/test_network $(CXXFLAGS) $(CXXTHREAD) -Isrc

test_performance:
	$(CC) ./tests/performance_test.cpp ./src/server/*.cpp -o ./bin/test_performance $(CXXFLAGS) $(CXXTHREAD) -Isrc

# 运行所有测试
test: test_skip_list test_kv_storage test_network test_performance
	@echo "Running all tests..."
	@echo "=== SkipList Tests ==="
	@./bin/test_skip_list
	@echo "=== KV Storage Tests ==="
	@./bin/test_kv_storage
	@echo "=== Network Tests ==="
	@./bin/test_network
	@echo "=== Performance Tests ==="
	@./bin/test_performance
	@echo "All tests passed!"

# 清理
clean:
	rm -f ./src/server/*.o
	rm -f ./src/client/*.o
	rm -f ./bin/kv_server
	rm -f ./bin/kv_client
	rm -f ./bin/test_*

# 创建必要的目录
setup:
	mkdir -p bin doc

.PHONY: kv_storage test clean setup test_skip_list test_kv_storage test_network test_performance