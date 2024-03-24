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
#include <ctype.h>

/*
 * =====================================
 * MACROS
 * =====================================
 */

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
#define i64 int64_t

#define MAX_PROCESSES 100
#define MAX_PROCESS_NAME_SIZE 16
#define MAX_TRACE_LINE_SIZE 200

#define GENERAL_DEFAULT_QUANTUM 200000

#define PRIORITY_START_QUANTUM_USEC 200000
#define PRIORITY_QUANTUM_INCREMENT_USEC 200000
#define ROUND_ROBIN_QUANTUM_USEC 10

#define SEC_IN_USEC 1000000
#define SEC_IN_NSEC 1000000000
#define USEC_IN_NSEC 1000
#define MSEC_IN_USEC 1000

/*
 * =====================================
 * STRUCTS & TYPEDEFS & ENUMS
 * =====================================
 */

struct process {

	/* Infos read from trace file */
	char name[MAX_PROCESS_NAME_SIZE + 1];
	u32 deadline_sec;
	u32 start_time_sec;
	u32 burst_time_sec;

	u32 priority;
	u32 finished;
	u32 quantum_usec;
	u64 current_burst_time_usec;
	u64 real_start_time;
	u64 real_end_time;

	/* Infos for context switching */
	pthread_t thread;
	pthread_mutex_t* suspend_mutex;
	pthread_cond_t condition;
	u32 suspend_flag;
};

enum algorithm {
	SHORTEST_FIRST = 1,
	ROUND_ROBIN = 2,
	PRIORITY = 3
};


/*
 * =====================================
 * GLOBALS
 * =====================================
 */

struct process processes[MAX_PROCESSES] = {};
enum algorithm scheduler_algorithm;
pthread_mutex_t global_suspend_mutex;
u32 num_processes = 0;
u64 context_switchs = 0;

char* err_msg = NULL;

/*
 * =====================================
 * HELPERS FUNCTIONS
 * =====================================
 */

int min(u32 a, u32 b) {
	return a < b ? a : b;
}

int isnumber(char* str) {
	while (*str != '\0') {
		if (!isdigit(*str) && *str != '\n') {
			return 0;
		}
		str++;
	}
	return 1;
}

/*
 * =====================================
 * THREADS FUNCTIONS
 * =====================================
 */

void check_suspend(struct process *process) {
    pthread_mutex_lock(process->suspend_mutex);
    while (process->suspend_flag != 0) {
		pthread_cond_wait(&process->condition, process->suspend_mutex);
	}
    pthread_mutex_unlock(process->suspend_mutex);
}

void suspend_process(struct process *process) {
	pthread_mutex_lock(process->suspend_mutex);
	process->suspend_flag = 1;
	pthread_mutex_unlock(process->suspend_mutex);
	print_info("Thread %s suspended\n", process->name);
}

void resume_process(struct process *process) {
	print_info("------------- Resuming process %s -------------\n", process->name);
	pthread_mutex_lock(process->suspend_mutex);
	process->suspend_flag = 0;
	pthread_cond_signal(&process->condition);
	pthread_mutex_unlock(process->suspend_mutex);
}

/*
 * =====================================
 * EXECUTABLE FUNCTIONS
 * =====================================
 */

void* default_function(void *arg)
{
	struct process *process = (struct process *)arg;
	print_info("Thread %s started\n", process->name);
	u64 counter = 0;
	while (counter < 1000000000) {
		check_suspend(process);
		counter++;
	}
	process->finished = 1;
	return NULL;
}

/*
 * =====================================
 * GENERAL FUNCTIONS
 * =====================================
 */

int select_algorithm(char* alg) {
	print_info("Defining scheduler algorithm\n");

	if (!isdigit(*alg)) {
		err_msg = "Invalid Algorithm";
		return -1;
	}

	switch (atoi(alg)) {
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
			err_msg = "Invalid Algorithm";
			return -1;
	}

	return 0;
}

int process_init(struct process *process, char* name, char* deadline, char* start_time, char* burst_time) {
	int ret = 0;

	if(!isnumber(deadline) || !isnumber(start_time) || !isnumber(burst_time)) {
		err_msg = "Invalid trace file";
		return -1;
	}

	if(strlen(name) > MAX_PROCESS_NAME_SIZE) {
		err_msg = "Invalid process name";
		return -1;
	}

	strcpy(process->name, name);
	process->deadline_sec = atoi(deadline);
	process->start_time_sec = atoi(start_time);
	process->burst_time_sec = atoi(burst_time);

	process->suspend_flag = 0;
	process->suspend_mutex = &global_suspend_mutex;
	process->quantum_usec = GENERAL_DEFAULT_QUANTUM;

	ret = pthread_cond_init(&process->condition, NULL);
	if (ret != 0) {
		err_msg = "Error initializing condition";
		return ret;
	}

	switch (scheduler_algorithm) {
		case SHORTEST_FIRST:
			process->priority = process->burst_time_sec;
			break;
		case PRIORITY:
			process->priority = process->deadline_sec;
			break;
		case ROUND_ROBIN:
			process->priority = process->start_time_sec;
			break;
	}

	print_info("Line %d info:\n", num_processes);
	print_info("name: %s\n", process->name);
	print_info("deadline: %d\n", process->deadline_sec);
	print_info("start_time: %d\n", process->start_time_sec);
	print_info("burst_time: %d\n", process->burst_time_sec);

	return 0;
}

int parse_trace_file(char* file_path) {
	char line[MAX_TRACE_LINE_SIZE + 1];
	FILE* file;
	char* name;
	char* deadline;
	char* start_time;
	char* burst_time;
	int ret = 0;

	print_info("Opening file %s\n", file_path);
	file = fopen(file_path, "r");
	if(file == NULL) {
		err_msg = "Error opening trace file";
		return -1;
	}
	print_info("File %s opened for read only\n", file_path);

	while(fgets(line, MAX_TRACE_LINE_SIZE, file) != NULL) {
		if (num_processes >= MAX_PROCESSES) {
			err_msg = "Too many processes";
			ret = -1;
			goto close_file;
		}

		name = strtok(line, " ");
		deadline = strtok(NULL, " ");
		start_time = strtok(NULL, " ");
		burst_time = strtok(NULL, " ");

		if(!name || !deadline || !burst_time || !start_time) {
			err_msg = "Invalid trace file";
			ret = -1;
			goto close_file;
		}

		ret = process_init(&processes[num_processes], name, deadline, start_time, burst_time);
		if (ret != 0) {
			err_msg = err_msg ? err_msg : "Error initializing process";
			goto close_file;
		}

		num_processes++;
	}

	print_info("Parsing finished\n");

close_file:
	fclose(file);
	return ret;
}

void sort_inc_processes(struct process *processes, u32 num_processes) {
	struct process *left = processes;
	struct process *right = processes + num_processes / 2;
	struct process* max_left = processes + num_processes / 2;
	struct process* max_right = processes + num_processes;
	struct process *temp = malloc(num_processes * sizeof(struct process));
	u32 i = 0;

	if (num_processes < 2) {
		return;
	}

	sort_inc_processes(left, num_processes / 2);
	sort_inc_processes(right, num_processes - num_processes / 2);

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

void sort_processes() {
	sort_inc_processes(processes, num_processes);
}

void define_quantums() {
	switch (scheduler_algorithm) {
		case SHORTEST_FIRST:
			break;
		case PRIORITY:
			u32 current_quantum = PRIORITY_START_QUANTUM_USEC;
			for (int i = (int)(num_processes - 1); i >= 0; i--) {
				processes[i].quantum_usec = current_quantum;
				current_quantum += PRIORITY_QUANTUM_INCREMENT_USEC;
			}
			break;
		case ROUND_ROBIN:
			for (u32 i = 0; i < num_processes; i++)
				processes[i].quantum_usec = ROUND_ROBIN_QUANTUM_USEC;
			break;
	}
}

void apply_priorities() {
	sort_processes();
	print_info("Sorting finished\n");
	define_quantums();
	print_info("Quanta defined\n");
}

int create_process_thread(struct process *process, void* (*function)(void*)) {
	print_info("============= Starting process %s =============\n", process->name);
	return pthread_create(&process->thread, NULL, function, (void*)process);
}

int start_shortest_first_scheduler() {
	int ret = 0;
	struct process *process = NULL;
	void* process_ret = NULL;

	struct timeval scheduler_start_time = {};
	struct timeval current_time = {};

	struct timespec wait_time_timespec = {};
	i64 wait_time_usec = 0;

	gettimeofday(&scheduler_start_time, NULL);

	for (u32 i = 0; i < num_processes; i++) {
		process = &processes[i];

		ret = gettimeofday(&current_time, NULL);
		if (ret != 0) {
			err_msg = "Error getting time";
			return ret;
		}

		wait_time_usec = SEC_IN_USEC * (process->start_time_sec - (current_time.tv_sec - scheduler_start_time.tv_sec)) - current_time.tv_usec;

		if(wait_time_usec > 0) {
			print_info("Waiting for %ld usec\n", wait_time_usec);
			usleep(wait_time_usec);
		}

		ret = create_process_thread(process, default_function);
		if(ret != 0) {
			err_msg = "Error creating process thread";
			return ret;
		}

		ret = gettimeofday(&current_time, NULL);
		if (ret != 0) {
			err_msg = "Error getting time";
			return ret;
		}

		wait_time_timespec.tv_sec = current_time.tv_sec + process->burst_time_sec;
		wait_time_timespec.tv_nsec = current_time.tv_usec * USEC_IN_NSEC;

		print_info("Defined processing time of %d secs. Limit time will be " \
					"time %ld secs and %ld nsecs\n",
					process->burst_time_sec,
					wait_time_timespec.tv_sec - scheduler_start_time.tv_sec,
					wait_time_timespec.tv_nsec);

		/*
		 * This thread will wait for the created thread to finish for
		 * the burst time of the process. If the process doesnt finished
		 * in the defined burst time, the thread will be cancelled.
		 */
		ret = pthread_timedjoin_np(process->thread, &process_ret, &wait_time_timespec);

		gettimeofday(&current_time, NULL);

		print_info("Process %s finished in time %ld secs and %ld nsecs\n",
					process->name,
					current_time.tv_sec - scheduler_start_time.tv_sec,
					current_time.tv_usec * USEC_IN_NSEC);

		if (ret == 0 && current_time.tv_sec - scheduler_start_time.tv_sec < process->deadline_sec) {
			print_info("Process %s finished in time :)\n", process->name);
		}
		else if(ret == 0) {
			print_info("Process %s finished but not following deadline :(\n",
						process->name);
		}
		else {
			print_info("Process %s didn't finish in time :(\n", process->name);
			ret = pthread_cancel(process->thread);
			if (ret != 0) {
				err_msg = "Error cancelling process";
				return -1;
			}
		}

		process->thread = 0;
	}

	return ret;
}

int process_is_not_ready(struct process *process, u64 scheduler_seconds) {
	return scheduler_seconds < process->start_time_sec || process->finished;
}

int process_has_started(struct process *process) {
	return process->thread != 0;
}

int process_exploded_burst_time(struct process *process) {
	return process->current_burst_time_usec >= process->burst_time_sec * SEC_IN_USEC;
}

int process_has_finished(struct process *process) {
	return process->finished;
}

u64 get_delta_time_usec(struct timeval time1, struct timeval time2) {
	u64 delta_time_usec = 0;

	if(time2.tv_sec == time1.tv_sec) {
		delta_time_usec = time2.tv_usec - time1.tv_usec;
	}
	else {
		delta_time_usec = (time2.tv_sec - time1.tv_sec - 1) * SEC_IN_USEC + (SEC_IN_USEC - time1.tv_usec) + time2.tv_usec;
	}

	return delta_time_usec;
}

int start_round_robin_scheduler() {
	int ret = 0;

	struct process *process = NULL;

	struct timeval scheduler_start_time = {};
	struct timeval time1 = {};
	struct timeval time2 = {};

	u64 scheduler_seconds = 0;
	u64 delta_time_usec = 0;
	u32 finished_processes = 0;
	u32 i = 0;

	gettimeofday(&scheduler_start_time, NULL);

	while(finished_processes < num_processes) {
		process = &processes[i];

		ret = gettimeofday(&time1, NULL);
		if (ret != 0) {
			err_msg = "Error getting time";
			return ret;
		}

		scheduler_seconds = (time1.tv_sec - scheduler_start_time.tv_sec);

		if(process_is_not_ready(process, scheduler_seconds)) {
			i = (i + 1) % num_processes;
			continue;
		};

		if (process_has_started(process)) {
			resume_process(process);
		}
		else {
			ret = create_process_thread(process, default_function);
			process->real_start_time = scheduler_seconds;
			if (ret) {
				err_msg = "Error creating process thread";
				return ret;
			}
		}

		usleep(process->quantum_usec);

		ret = gettimeofday(&time2, NULL);
		if (ret != 0) {
			err_msg = "Error getting time";
			return ret;
		}

		scheduler_seconds = (time2.tv_sec - scheduler_start_time.tv_sec);

		delta_time_usec = get_delta_time_usec(time1, time2);
		process->current_burst_time_usec += delta_time_usec;

		print_info("Process %s used %ld usecs of processing time\n",
					process->name, delta_time_usec);

		if(process_has_finished(process)) {
			print_info("Process %s finished in time %ld secs and %ld usecs, while deadline was %d secs\n",
					process->name,
					time2.tv_sec - scheduler_start_time.tv_sec,
					time2.tv_usec,
					process->deadline_sec);

			if(scheduler_seconds < process->deadline_sec) {
				print_info("Process %s finished in time :)\n", process->name);
			}
			else {
				print_info("Process %s finished but not following deadline :(\n", process->name);
			}

			process->thread = 0;
			process->real_end_time = scheduler_seconds;
			finished_processes += 1;
		}
		else if(process_exploded_burst_time(process)) {
			print_info("Process %s didn't finish in time :(\n", process->name);
			ret = pthread_cancel(process->thread);
			if (ret != 0) {
				err_msg = "Error cancelling process";
				return ret;
			}

			process->thread = 0;
			process->real_end_time = scheduler_seconds;
			process->finished = 1;
			finished_processes += 1;
		}
		else {
			suspend_process(process);
		}

		/* Even when there is only one process to finish, it will be considered
		 * a context switch, once the process is being suspended */
		context_switchs += 1;
		i = (i + 1) % num_processes;
	}

	return 0;
}

int start_scheduler() {
	int ret = 0;

	print_info("Starting scheduler\n");

	switch (scheduler_algorithm) {
		case SHORTEST_FIRST:
			ret = start_shortest_first_scheduler();
			if (ret != 0) {
				err_msg = err_msg ? err_msg : "Error running shortest first scheduler";
				return ret;
			}
			break;
		case ROUND_ROBIN:
			start_round_robin_scheduler();
			if (ret != 0) {
				err_msg = err_msg ? err_msg : "Error running round robin scheduler";
				return ret;
			}
			break;
		case PRIORITY:
			/* TODO: create priority algorithm */
			break;
	}

	return ret;
}

int save_output(char* file_path) {
	FILE* file;
	int ret = 0;

	print_info("Opening output file for writting %s\n", file_path);
	file = fopen(file_path, "w");
	if(file == NULL) {
		err_msg = "Error opening output file";
		return -1;
	}
	print_info("File %s opened for write only\n", file_path);

	for (u32 i = 0; i < num_processes; i++) {
		fprintf(file, "%s %lu %lu\n", processes[i].name,
				processes[i].real_start_time,
				processes[i].real_end_time);
	}

	fprintf(file, "%ld", context_switchs);

	print_info("Output file saved\n");

	fclose(file);
	return ret;
}

int main(int argc, char *argv[])
{
	int ret = 0;

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

	select_algorithm(argv[1]);
	if (ret != 0) {
		err_msg = err_msg ? err_msg : "Error defining scheduler algorithm";
		goto error;
	}

	ret = parse_trace_file(argv[2]);
	if (ret != 0) {
		err_msg = err_msg ? err_msg : "Error parsing trace file";
		goto error;
	}

	if(num_processes == 0) {
		print_info("No processes provided\n");
		return 0;
	}

	apply_priorities();
	ret = start_scheduler();
	if (ret != 0) {
		err_msg = err_msg ? err_msg : "Error starting scheduler";
		goto error;
	}

	ret = save_output(argv[3]);
	if (ret != 0) {
		err_msg = err_msg ? err_msg : "Error saving output file";
		goto error;
	}

	return 0;

error:
	print_error(err_msg);
	return ret;
}
