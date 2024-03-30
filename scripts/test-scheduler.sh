#!/bin/bash

set -e

# DONT CHANGE!
INPUT_DIR="input"
OUTPUT_DIR="output"
SHORTEST_FIRST_OUTPUT_DIR="${OUTPUT_DIR}/shortest_first"
ROUND_ROBIN_OUTPUT_DIR="${OUTPUT_DIR}/round_robin"
PRIORITY_OUTPUT_DIR="${OUTPUT_DIR}/priority"
ALG_DIRS=( "${SHORTEST_FIRST_OUTPUT_DIR}" "${ROUND_ROBIN_OUTPUT_DIR}" "${PRIORITY_OUTPUT_DIR}")

# CONFIGURABLE
NUM_PROCESSES=(50 250 500)
REPETITIONS=35

echo "Compiling scheduler..."
make scheduler

echo "Starting tests"

if [[ ! -d "$INPUT_DIR" ]]; then
	echo "[ERROR] Input directory not found."
	exit 1
fi

if [[ ! -d "${OUTPUT_DIR}" ]]; then
	mkdir "${OUTPUT_DIR}"
fi

for dir in "${ALG_DIRS[@]}"; do
	if [[ ! -d "${dir}" ]]; then
		mkdir "${dir}"
	fi
	for t in "${NUM_PROCESSES[@]}"; do
		if [[ ! -d "${dir}/${t}" ]]; then
			mkdir "${dir}/${t}"
		fi
	done
done

if [[ "$1" == "-s" ]]; then
	echo "Running scheduler in silent mode..."
fi

for t in "${NUM_PROCESSES[@]}"; do
	echo "============ TEST ${t} ============"

	for i in $(seq 1 $REPETITIONS); do
		echo "-------- Repetition ${i} --------"

		echo "Shortest First ${i}"
		./scheduler 1 "${INPUT_DIR}/${t}.trace" "${SHORTEST_FIRST_OUTPUT_DIR}/${t}/${i}.out" -s

		echo "Round Robin ${i}"
		./scheduler 2 "${INPUT_DIR}/${t}.trace" "${ROUND_ROBIN_OUTPUT_DIR}/${t}/${i}.out" -s

		echo "Priority ${i}"
		./scheduler 3 "${INPUT_DIR}/${t}.trace" "${PRIORITY_OUTPUT_DIR}/${t}/${i}.out" -s
	done
done
