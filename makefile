CC = g++
CXXFLAGS = -std=c++17
CXXTHREAD = -lpthread

kv_storage:
	$(CC) ./src/server/*.cpp -o ./bin/kv_server $(CXXFLAGS) $(CXXTHREAD)
	rm -f ./src/server/*.o
	$(CC) ./src/client/*.cpp -o ./bin/kv_client $(CXXFLAGS) $(CXXTHREAD)
	rm -f ./src/client/*.o

clean:
	rm -f ./src/server/*.o
	rm -f ./src/client/*.o
