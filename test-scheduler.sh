#!/bin/bash

echo "Compiling scheduler..."
make scheduler

echo "Running tests..."

#echo "Test 0"
#./scheduler 1 test.trace output

#echo "Test 1"
#./scheduler 2 test.trace output

echo "Test 2"
./scheduler 3 test.trace output

