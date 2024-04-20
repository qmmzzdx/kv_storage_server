CC = g++
CXXFLAGS = -std=c++17

skiplist:
	$(CC) ./src/* -o ./bin/main $(CXXFLAGS)
	rm -f ./src/*.o

clean:
	rm -f ./*.o
