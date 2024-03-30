#!/bin/bash

echo "Compiling scheduler..."
make scheduler

echo "Test 0"
./scheduler 1 "dumb.trace" "dumb.out"

echo "Test 1"
#./scheduler 2 "dumb.trace" "dumb.out" -s

echo "Test 2"
#./scheduler 3 "dumb.trace" "dumb.out" -s

