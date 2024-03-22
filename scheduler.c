#define _GNU_SOURCE

#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#define DEBUG_MODE 1

#define print_info(info, ...) \
	do { \
		if(DEBUG_MODE) { \
			fprintf(stderr, "[INFO] File: %s, Line: %d: ", __FILE__, __LINE__); \
			fprintf(stderr, info, ##__VA_ARGS__); \
		} \
	} while(0)

#define print_error(error_message) \
	do { \
		if(DEBUG_MODE) \
			fprintf(stderr, "[ERROR] File: %s, Line: %d: ", __FILE__, __LINE__); \
		perror(error_message); \
	} while(0)

#define u32 uint32_t
#define u64 uint64_t

#define SHORTEST_FIRST ((size_t)1)
#define ROUND_ROBIN ((size_t)2)
#define PRIORITY ((size_t)4)

#define MAX_PROCESSES 100
#define MAX_PROCESS_NAME_SIZE 16
#define MAX_LINE_SIZE 200

#define PRIORITY_START_QUANTUM 200
#define PRIORITY_QUANTUM_INCREMENT 200
#define ROUND_ROBIN_QUANTUM 500

char* err_msg = NULL;

struct Process {
	char name[MAX_PROCESS_NAME_SIZE + 1];
	u32 burst_time;
	u64 current_burst_time;
	u32 init_time;
	u32 priority;
	u32 deadline;
	u32 quantum;
	u32 finished;
	pthread_t thread;
	pthread_cond_t condition;
	u32 suspend_flag;
	pthread_mutex_t* suspend_mutex;
};

int min(u32 a, u32 b) {
	return a < b ? a : b;
}

void check_suspend(struct Process *process) {
    pthread_mutex_lock(process->suspend_mutex);
    while (process->suspend_flag != 0) pthread_cond_wait(&process->condition, process->suspend_mutex);
    pthread_mutex_unlock(process->suspend_mutex);
}

void* four_seconds_function(void *arg)
{
	struct Process *process = (struct Process *)arg;
	print_info("Thread %s started\n", process->name);
	u64 counter = 0;
	while (counter < 1000000) {
		check_suspend(process);
		counter++;
	}
	process->finished = 1;
	return NULL;
}

void suspend_process(struct Process *process) {
	pthread_mutex_lock(process->suspend_mutex);
	process->suspend_flag = 1;
	pthread_mutex_unlock(process->suspend_mutex);
}

void resume_process(struct Process *process) {
	pthread_mutex_lock(process->suspend_mutex);
	process->suspend_flag = 0;
	pthread_cond_signal(&process->condition);
	pthread_mutex_unlock(process->suspend_mutex);
}

void sort_inc_processes(struct Process *processes, u32 num_processes) {

	struct Process *left = processes;
	struct Process *right = processes + num_processes / 2;
	struct Process* max_left = processes + num_processes / 2;
	struct Process* max_right = processes + num_processes;
	struct Process *temp = malloc(num_processes * sizeof(struct Process));

	if (num_processes < 2) {
		return;
	}

	sort_inc_processes(left, num_processes / 2);
	sort_inc_processes(right, num_processes - num_processes / 2);

	u32 i = 0;

	while (left < max_left || right < max_right) {
		if (left < max_left && (right >= max_right || left->priority < right->priority)) {
			temp[i] = *left;
			left++;
		} else {
			temp[i] = *right;
			right++;
		}
		i++;
	}

	for(i = 0; i < num_processes; i++) {
		processes[i] = temp[i];
	}

	free(temp);
}

int main(int argc, char *argv[])
{
	int ret = 0;

	struct Process processes[MAX_PROCESSES] = {};
	u32 num_processes = 0;
	u32 scheduler_algorithm = 0;
	pthread_mutex_t global_suspend_mutex;

	/*
	 * $1 - Scheduler algorithm
	 * 1. Shortest Job First
	 * 2. Round-Robin
	 * 3. Escalonamento com prioridade (usando o deadline como crit´erio para definir a quantidade de
	 * $2 - Trace file
	 * $3 - Output file
	 * quantums dada a cada processo)
	 */

	if (argc != 4) {
		err_msg = "Invalid number of arguments";
		ret = -EINVAL;
		goto error;
	}

	print_info("Defining scheduler algorithm\n");

	switch (atoi(argv[1])) {
		case 1:
			print_info("Selected Shortest first\n");
			scheduler_algorithm = SHORTEST_FIRST;
			break;
		case 2:
			print_info("Selected Round Robin\n");
			scheduler_algorithm = ROUND_ROBIN;
			break;
		case 3:
			print_info("Selected Priority\n");
			scheduler_algorithm = PRIORITY;
			break;
		default:
			err_msg = "Invalid scheduler algorithm";
			ret = -EINVAL;
			goto error;
	}

	print_info("Scheduler algorithm defined: %s\n", argv[1]);
	print_info("Opening file %s\n", argv[2]);

	FILE* file = fopen(argv[2], "r");
	if (file == NULL) {
		err_msg = "Error opening trace file";
		goto error;
	}

	print_info("Filed %s opened\n", argv[2]);
	print_info("Parsing trace file...\n");

	char line[MAX_LINE_SIZE];

	while(fgets(line, sizeof(line), file) != NULL) {
		char* name = strtok(line, " ");
		char* deadline = strtok(NULL, " ");
		char* init_time = strtok(NULL, " ");
		char* burst_time = strtok(NULL, " ");

		if(!name || !deadline || !burst_time || !init_time) {
			err_msg = "Invalid trace file";
			ret = -EINVAL;
			goto error;
		}

		if (num_processes >= MAX_PROCESSES) {
			err_msg = "Too many processes";
			ret = -EINVAL;
			goto error;
		}

		strcpy(processes[num_processes].name, name);
		processes[num_processes].deadline = atoi(deadline);
		processes[num_processes].init_time = atoi(init_time);
		processes[num_processes].burst_time = atoi(burst_time);

		print_info("Line %d info:\n", num_processes);
		print_info("name: %s\n", processes[num_processes].name);
		print_info("deadline: %d\n", processes[num_processes].deadline);
		print_info("init_time: %d\n", processes[num_processes].init_time);
		print_info("burst_time: %d\n", processes[num_processes].burst_time);

		processes[num_processes].suspend_mutex = &global_suspend_mutex;

		switch (scheduler_algorithm) {
			case SHORTEST_FIRST:
				processes[num_processes].priority = processes[num_processes].burst_time;
				break;
			case PRIORITY:
				processes[num_processes].priority = processes[num_processes].deadline;
				break;
			case ROUND_ROBIN:
				processes[num_processes].priority = processes[num_processes].init_time;
				break;
			default:
				err_msg = "Invalid scheduler algorithm";
				ret = -EINVAL;
				goto error;
		}

		processes[num_processes].suspend_flag = 0;
		pthread_cond_init(&processes[num_processes].condition, NULL);

		num_processes++;
	}

	print_info("Parsing finished\n");

	if(num_processes == 0) {
		err_msg = "No processes provided";
		goto error;
	}

	switch (scheduler_algorithm) {
		case SHORTEST_FIRST:
			print_info("Sorting by shortest first\n");
			sort_inc_processes(processes, num_processes);
			break;
		case PRIORITY:
			print_info("Sorting by deadline\n");
			sort_inc_processes(processes, num_processes);
			u32 current_quantum = PRIORITY_START_QUANTUM;
			for (int i = (int)(num_processes - 1); i >= 0; i--) {
				processes[i].quantum = current_quantum;
				current_quantum += PRIORITY_QUANTUM_INCREMENT;
			}
			break;
		case ROUND_ROBIN:
			print_info("Sorting by init time\n");
			sort_inc_processes(processes, num_processes);
			for (u32 i = 0; i < num_processes; i++)
				processes[i].quantum = ROUND_ROBIN_QUANTUM;
			break;
		default:
			err_msg = "Invalid scheduler algorithm";
			ret = -EINVAL;
			goto error;
	}

	print_info("Sorting finished\n");
	print_info("Starting scheduler\n");

	/* Start scheduler */
	switch (scheduler_algorithm) {
		case SHORTEST_FIRST:
			void* ret_val = NULL;
			struct Process *process = NULL;
			long long wait_time = 0;
			long long wait_init_time = 0;
			u32 scheduler_init_time = time(NULL);

			for (u32 i = 0; i < num_processes; i++) {
				process = &processes[i];

				print_info("============= Starting thread %s =============\n", process->name);

				wait_init_time = process->init_time - (time(NULL) - scheduler_init_time);
				if(wait_init_time > 0) {
					print_info("Waiting for %lld second(s)\n", wait_init_time);
					sleep(wait_init_time);
				}

				ret = pthread_create(&process->thread, NULL, four_seconds_function, (void*)process);
				if(ret != 0) {
					err_msg = "Error creating thread";
					goto error;
				}

				print_info("Thread %s created\n", process->name);

				wait_time = min(process->deadline - (time(NULL) - scheduler_init_time), process->burst_time);
				print_info("Max wait time for running is %lld second(s)\n", wait_time);
				struct timespec max_time = { .tv_sec = time(NULL) + wait_time, .tv_nsec = 0 };

				/*
				 * This thread will wait for the created thread to finish for
				 * the burst time of the process. If the process doesnt finished
				 * in the defined burst time, the thread will be cancelled.
				 */
				ret = pthread_timedjoin_np(process->thread, &ret_val, &max_time);

				if (ret != 0) {
					print_info("Thread %s didn't finish in time :(\n", process->name);
					ret = pthread_cancel(process->thread);
					if (ret != 0) {
						err_msg = "Error cancelling thread";
						goto error;
					}
					process->thread = 0;
				}
				else
					print_info("Thread %s finished in time :)\n", process->name);
			}
			break;

		case ROUND_ROBIN:
			u32 i = 0;
			struct timeval scheduler_init_time_timeval = {};
			struct timeval new_time = {};
			u32 finished_processes = 0;

			u64 delta_time_micro = 0;

			struct timeval scheduler_current_time = {};
			u32 scheduler_time_seconds = 0;

			gettimeofday(&scheduler_init_time_timeval, NULL);

			while(finished_processes < num_processes) {
				process = &processes[i];

				gettimeofday(&scheduler_current_time, NULL);
				scheduler_time_seconds = (scheduler_current_time.tv_sec - scheduler_init_time_timeval.tv_sec);

				if (scheduler_time_seconds < process->init_time) continue;

				if (process->thread && (scheduler_time_seconds >= process->deadline || process->current_burst_time >= process->burst_time * 1000000)) {
					print_info("Thread %s didn't finish in time :(\n", process->name);
					ret = pthread_cancel(process->thread);
					if (ret != 0) {
						err_msg = "Error cancelling thread";
						goto error;
					}
					process->thread = 0;
					finished_processes += 1;
				}
				else if (!process->thread) {
					print_info("============= Starting thread %s =============\n", process->name);
					pthread_create(&process->thread, NULL, four_seconds_function, process);
				}
				else {
					resume_process(process);
				}

				usleep(process->quantum * 1000);

				if(process->finished) {
					finished_processes += 1;
					print_info("Thread %s finished in time :)\n", process->name);
				}
				else
					suspend_process(process);

				gettimeofday(&new_time, NULL);
				delta_time_micro = (new_time.tv_sec - scheduler_current_time.tv_sec) * 1000000 + (new_time.tv_usec - scheduler_current_time.tv_usec);

				process->current_burst_time += delta_time_micro;
				i = (i + 1) % num_processes;
			}
			break;
		case PRIORITY:
			/* TODO: create priority algorithm */
			break;
		default:
			err_msg = "Invalid scheduler algorithm";
			ret = -EINVAL;
			goto error;
	}

	return 0;

error:
	print_error(err_msg);
	return ret;
}
