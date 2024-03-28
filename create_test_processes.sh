#!/bin/bash

# POSSIBLE_PROCESSES=("darksouls" "flusp" "guaxinim" "gnu" "linus" "batata" "kernel" "servine" "lovelace" "jobs" )
# COUNT_FOR_PROCESS=( 0 0 0 0 0 0 0 0 0 0 )
# EXPECTED_EXEC_TIME=( 1 1 1 1 2 2 2 2 3 6 )

INPUT_DIR="input"
POSSIBLE_PROCESSES=( "kernel" "servine" "lovelace" "jobs" )
COUNT_FOR_PROCESS=( 0 0 0 0 )
MAX_EXEC_TIME=( 2 2 3 6 )
NUM_POSSIBLE_PROCESSES=${#POSSIBLE_PROCESSES[@]}
MAX_START_TIME=30
NUM_PROCESSES=50

if [[ ! -d "$INPUT_DIR" ]]; then
	mkdir "$INPUT_DIR"
fi

echo -n "" > "${INPUT_DIR}/${NUM_PROCESSES}.trace"

for _ in $(seq 1 $NUM_PROCESSES); do
	start_time=$(( RANDOM % MAX_START_TIME ))
	process_index=$(( RANDOM % NUM_POSSIBLE_PROCESSES ))
	expected_exec_time=${MAX_EXEC_TIME[${process_index}]}
	end_time=$(( 2 * (start_time + expected_exec_time) ))
	process=${POSSIBLE_PROCESSES[${process_index}]}
	echo "${process}_${COUNT_FOR_PROCESS[${process_index}]} ${end_time} ${start_time} ${expected_exec_time}" >> "${INPUT_DIR}/${NUM_PROCESSES}.trace"
	COUNT_FOR_PROCESS[process_index]=$(( COUNT_FOR_PROCESS[process_index] + 1 ))
done
