#!/bin/bash

# Possible processes are
# "darksouls" "flusp" "guaxinim" "gnu" "linus" "batata" "kernel" "servine"
# "lovelace" "jobs"
# Ordered from the one with the smallest max_burst_time to
# the one with the biggest max_burst_time

set -e
declare -A  MAX_BURST_TIME

# DONT CHANGE!
MAX_BURST_TIME=( ["darksouls"]=1 ["flusp"]=1 ["guaxinim"]=1 ["gnu"]=1 ["linus"]=2 ["batata"]=2 ["kernel"]=2 ["servine"]=3 ["lovelace"]=4 ["jobs"]=10 )
COUNT_FOR_PROCESS=( 0 0 0 0 0 0 0 0 0 0 )
INPUT_DIR="input"

# CONFIGURABLE
NUM_PROCESSES=( 500 250 50 )

SELECTED_PROCESSES_SHORT=( "darksouls" "flusp" "guaxinim" "gnu" )
SELECTED_PROCESSES_MID=( "darksouls" "flusp" "guaxinim" "gnu" "linus" "batata" "kernel" )
SELECTED_PROCESSES_LARGE=( "kernel" "servine" "lovelace" "jobs" )

declare -A SELECTED_PROCESSES=( [0]="${SELECTED_PROCESSES_SHORT[@]}" [1]="${SELECTED_PROCESSES_MID[@]}" [2]="${SELECTED_PROCESSES_LARGE[@]}" )
NUM_SELECTED_PROCESSES=${#SELECTED_PROCESSES[@]}
CURRENT_SELECTED_PROCESSES=()

MAX_DEADLINE_MULTIPLIER=2.4
MAX_INIT_MULTIPLIER=( 0.05 0.7 2.4 )

if [[ ! -d "$INPUT_DIR" ]]; then
	mkdir "$INPUT_DIR"
fi

count=0
for num_processes in "${NUM_PROCESSES[@]}"; do
	output_file="${INPUT_DIR}/${num_processes}.trace"
	echo -n "" > "$output_file"

	current_selected_processes_index=$(( count % NUM_SELECTED_PROCESSES ))
	#max_init=$(( num_processes * MAX_INIT_MULTIPLIER[ current_selected_processes_index ] ))
	max_init=$(python -c "from math import floor; print(floor(${num_processes} * ${MAX_INIT_MULTIPLIER[${current_selected_processes_index}]}))")

	for process_index in seq $NUM_SELECTED_PROCESSES; do
		COUNT_FOR_PROCESS[process_index]=0
	done

	CURRENT_SELECTED_PROCESSES=( ${SELECTED_PROCESSES[${current_selected_processes_index}]} )

	num_current_selected_processes=${#CURRENT_SELECTED_PROCESSES[@]}

	for _ in $(seq 1 "$num_processes"); do
		process_index=$(( RANDOM % num_current_selected_processes ))
		process=${CURRENT_SELECTED_PROCESSES[${process_index}]}

		init=$(( RANDOM % max_init ))
		max_burst_time="${MAX_BURST_TIME[${process}]}"

		deadline=$(python -c "from math import floor; print(floor(${init} + ${max_burst_time} * ${MAX_DEADLINE_MULTIPLIER}))")

		process_count="${COUNT_FOR_PROCESS[${process_index}]}"
		process_name="${process}_${process_count}"

		echo "Process: ${process_name} Deadline: ${deadline} Init: ${init} Max Burst Time: ${max_burst_time}"

		COUNT_FOR_PROCESS[process_index]=$(( process_count + 1 ))

		echo "${process_name} ${deadline} ${init} ${max_burst_time}" >> "$output_file"
	done

	count=$(( count + 1 ))
done
