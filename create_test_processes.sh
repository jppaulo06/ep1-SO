#!/bin/bash

# POSSIBLE_PROCESSES=("darksouls" "flusp" "guaxinim" "gnu" "linus" "batata" "kernel" "servine" "lovelace" "jobs" )
# COUNT_FOR_PROCESS=( 0 0 0 0 0 0 0 0 0 0 )
# EXPECTED_EXEC_TIME=( 0 0 0 0 1 1 1 1 2 5 )

POSSIBLE_PROCESSES=( "lovelace" "jobs" )
COUNT_FOR_PROCESS=( 0 0 )
NUM_POSSIBLE_PROCESSES=${#POSSIBLE_PROCESSES[@]}
EXPECTED_EXEC_TIME=( 3 6 )
MAX_START_TIME=30
NUM_PROCESSES=50

for _ in $(seq 1 $NUM_PROCESSES); do
	start_time=$(( RANDOM % MAX_START_TIME ))
	process_index=$(( RANDOM % NUM_POSSIBLE_PROCESSES ))
	expected_exec_time=${EXPECTED_EXEC_TIME[${process_index}]}
	end_time=$(( start_time + expected_exec_time + 10 ))
	process=${POSSIBLE_PROCESSES[${process_index}]}
	echo "${process}_${COUNT_FOR_PROCESS[${process_index}]} ${end_time} ${start_time} ${expected_exec_time}"
	COUNT_FOR_PROCESS[process_index]=$(( COUNT_FOR_PROCESS[process_index] + 1 ))
done
