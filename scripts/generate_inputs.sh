#!/bin/bash

# Possible processes are
# "darksouls" "flusp" "guaxinim" "gnu" "linus" "batata" "kernel" "servine"
# "lovelace" "jobs"
# Ordered from the one with the smallest max_burst_time to
# the one with the biggest max_burst_time

set -e

# DONT CHANGE!
COUNT_FOR_PROCESS=( 0 0 0 0 0 0 0 0 0 0 )
MAX_BURST_TIME=( ["darksouls"]=1 ["flusp"]=1 ["guaxinim"]=1 ["gnu"]=1 ["linus"]=2 ["batata"]=2 ["kernel"]=2 ["servine"]=2 ["lovelace"]=3 ["jobs"]=6 )
INPUT_DIR="input"

# CONFIGURABLE
SELECTED_PROCESSES=( "kernel" "servine" "lovelace" "jobs" )
NUM_PROCESSES=(50 250 500)
MAX_INIT_MULTIPLIER=2

NUM_SELECTED_PROCESSES=${#SELECTED_PROCESSES[@]}

if [[ ! -d "$INPUT_DIR" ]]; then
	mkdir "$INPUT_DIR"
fi

for num_processes in "${NUM_PROCESSES[@]}"; do

	max_init=$(( num_processes * MAX_INIT_MULTIPLIER ))
	output_file="${INPUT_DIR}/${num_processes}.trace"
	echo -n "" > "$output_file"

	for _ in $(seq 1 "$num_processes"); do
		init=$(( RANDOM % max_init ))
		max_burst_time=${MAX_BURST_TIME[${process}]}
		deadline=$(( 2 * (init + max_burst_time) ))

		process_index=$(( RANDOM % NUM_SELECTED_PROCESSES ))
		process=${SELECTED_PROCESSES[${process_index}]}

		process_count="${COUNT_FOR_PROCESS[${process_index}]}"
		process_name="${process}_${process_count}"

		COUNT_FOR_PROCESS[process_index]=$(( COUNT_FOR_PROCESS[process_index] + 1 ))

		echo "${process_name} ${deadline} ${init} ${max_burst_time}" >> "$output_file"
	done
done
