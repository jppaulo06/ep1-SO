#!/bin/python3

import os
from collections import namedtuple
from typing import NamedTuple
from pathlib import Path

# There are two types of graphs:
# 1 - Deadline analysis
# 2 - Context switch analysis

# Graphs in bar:
# 1 for deadline alanysis for each process size (3 in total for each machine)
# 1 for context switch analysis for each process size (3 in total for each
# machine)


INPUT_DIR = 'input'
OUTPUT_DIR = 'output'
GRAPHS_DIR = 'graphs'
ALGORITHMS = ('shortest_first', 'round_robin', 'priority')

Process = namedtuple('Process', 'name deadline init burst_time')
Result = namedtuple('Result', 'name real_init real_end')


class Processes(NamedTuple):
    size: int
    parsed: dict


def parse_processes(*args):
    for process in args:
        parse_trace_file(f"{process.size}.trace", process.parsed)


def parse_trace_file(file_name: str, dict: dict):
    with open(os.path.join(INPUT_DIR, file_name), 'r') as file:
        lines = file.readlines()
        for line in lines:
            splited_line = line.split()
            new_process = Process(
                    splited_line[0],
                    int(splited_line[1]),
                    int(splited_line[2]),
                    int(splited_line[3]))
            dict[new_process.name] = new_process


def print_output(processes_short: Processes, processes_mid: Processes,
                 processes_long: Processes):

    Path(GRAPHS_DIR).mkdir(parents=True, exist_ok=True)

    for algorithm in ALGORITHMS:

        deadline_average_short, context_switch_average_short = \
            get_averages_for_algorithm(algorithm, processes_short)

        deadline_average_mid, context_switch_average_mid = \
            get_averages_for_algorithm(algorithm, processes_mid)

        deadline_average_long, context_switch_average_long = \
            get_averages_for_algorithm(algorithm, processes_long)

        with open(os.path.join(GRAPHS_DIR, f'{algorithm}_deadline.csv'),
                  'w') as file:
            file.write(f'{processes_short.size},{deadline_average_short}\n')
            file.write(f'{processes_mid.size},{deadline_average_mid}\n')
            file.write(f'{processes_long.size},{deadline_average_long}')

        with open(os.path.join(GRAPHS_DIR, f'{algorithm}_context_switch.csv'),
                  'w') as file:
            file.write(
                    f'{processes_short.size},{context_switch_average_short}\n')
            file.write(f'{processes_mid.size},{context_switch_average_mid}\n')
            file.write(f'{processes_long.size},{context_switch_average_long}')


def get_averages_for_algorithm(algorithm: str, processes: Processes):
    deadline_sum: int = 0
    context_switch_sum: int = 0

    context_switch_average: float = 0
    deadline_average: float = 0

    tests_path = os.path.join(OUTPUT_DIR, algorithm, str(processes.size))

    for test_file in os.listdir(tests_path):
        with open(os.path.join(tests_path, test_file), 'r') as file:
            lines = file.readlines()
            for line in lines[0:len(lines) - 1]:
                splited_line = line.split()

                process_name = splited_line[0]
                deadline = int(splited_line[2])

                if deadline <= processes.parsed[process_name].deadline:
                    deadline_sum += 1

            last_line = lines[len(lines) - 1]
            context_switch_sum += int(last_line)

    deadline_average = deadline_sum / processes.size
    context_switch_average = context_switch_sum / processes.size

    return deadline_average, context_switch_average


if __name__ == '__main__':
    processes_50 = Processes(50, {})
    processes_250 = Processes(250, {})
    processes_500 = Processes(500, {})

    parse_processes(processes_50, processes_250, processes_500)
    print_output(processes_50, processes_250, processes_500)
