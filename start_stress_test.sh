#!/bin/bash

g++ test/stress_test.cpp -o ./bin/stress_test -std=c++17
./bin/stress_test
