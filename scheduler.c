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

#define DEBUG_MODE 0

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

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

#define MAX_PROCESSES 100
#define MAX_PROCESS_NAME_SIZE 16
#define MAX_TRACE_LINE_SIZE 200

#define PRINT_WAIT_TIME_USEC 100000

#define GENERAL_DEFAULT_QUANTUM 200000
#define PRIORITY_START_QUANTUM_USEC 200000
#define PRIORITY_QUANTUM_INCREMENT_USEC 200000
#define ROUND_ROBIN_QUANTUM_USEC 100000

#define SEC_IN_USEC 1000000
#define SEC_IN_NSEC 1000000000
#define USEC_IN_NSEC 1000
#define MSEC_IN_USEC 1000

/*
 * =====================================
 * STRUCTS & TYPEDEFS & ENUMS
 * =====================================
 */

enum process_state {
	READY,
	RUNNING,
	WAITING,
	SUCCESS,
	DEADLINE,
	CANCELLED
};

enum algorithm {
	SHORTEST_FIRST = 1,
	ROUND_ROBIN = 2,
	PRIORITY = 3
};

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

	enum process_state state;
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
pthread_t print_loop_thread;
void* (*exec_function)(void*) = NULL;

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

#define new_default_function(name, num) \
	void* name(void *arg) { \
		struct process *process = (struct process *)arg; \
		print_info("Thread %s started\n", process->na##me); \
		u64 counter = 0; \
		while (counter < num) { \
			check_suspend(process); \
			counter++; \
		} \
		process->finished = 1; \
		return NULL; \
	}

new_default_function(lovelace, 200000000)
new_default_function(servine, 150000000)
new_default_function(kernel, 120000000)
new_default_function(batata, 100000000)
new_default_function(jobs, 500000000)
new_default_function(linus, 100000000)
new_default_function(gnu, 50000000)
new_default_function(guaxinim, 1000000)
new_default_function(flusp, 500000)
new_default_function(darksouls, 10000)

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

void get_function_name(char* function_name, char* process_name) {
	while(process_name[0] != '\0' && process_name[0] != '_' && process_name[0] != '\n') {
		function_name[0] = process_name[0];
		process_name++;
		function_name++;
	}
	function_name[0] = '\0';
}

int define_exec_function(char* function_name) {

	if(strcmp(function_name, "lovelace") == 0) {
		exec_function = lovelace;
	}
	else if(strcmp(function_name, "servine") == 0) {
		exec_function = lovelace;
	}
	else if(strcmp(function_name, "kernel") == 0) {
		exec_function = kernel;
	}
	else if(strcmp(function_name, "batata") == 0) {
		exec_function = batata;
	}
	else if(strcmp(function_name, "jobs") == 0) {
		exec_function = jobs;
	}
	else if(strcmp(function_name, "linus") == 0) {
		exec_function = linus;
	}
	else if(strcmp(function_name, "gnu") == 0) {
		exec_function = gnu;
	}
	else if(strcmp(function_name, "guaxinim") == 0) {
		exec_function = guaxinim;
	}
	else if(strcmp(function_name, "flusp") == 0) {
		exec_function = flusp;
	}
	else if(strcmp(function_name, "darksouls") == 0) {
		exec_function = darksouls;
	}
	else {
		err_msg = "Invalid function name";
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
	process->state = WAITING;

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
	char function_name[MAX_PROCESS_NAME_SIZE + 1];
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

		get_function_name(function_name, name);

		ret = define_exec_function(function_name);
		if (ret != 0) {
			err_msg = err_msg ? err_msg : "Error defining exec function";
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

int start_shortest_first_scheduler() {
	int ret = 0;
	struct process *process = NULL;
	void* process_ret = NULL;

	struct timeval scheduler_start_time = {};
	struct timeval time1 = {};
	struct timeval time2 = {};

	struct timespec wait_time_timespec = {};
	i64 wait_time_usec = 0;
	u64 delta_time_usec = 0;

	gettimeofday(&scheduler_start_time, NULL);

	for (u32 i = 0; i < num_processes; i++) {
		process = &processes[i];

		ret = gettimeofday(&time1, NULL);
		if (ret != 0) {
			err_msg = "Error getting time";
			return ret;
		}

		wait_time_usec = SEC_IN_USEC * (process->start_time_sec - (time1.tv_sec - scheduler_start_time.tv_sec)) - time1.tv_usec;

		if(wait_time_usec > 0) {
			print_info("Waiting for %ld usec\n", wait_time_usec);
			usleep(wait_time_usec);
		}

		ret = create_process_thread(process, exec_function);
		if(ret != 0) {
			err_msg = "Error creating process thread";
			return ret;
		}

		process->real_start_time = time1.tv_sec - scheduler_start_time.tv_sec;
		process->state = RUNNING;

		ret = gettimeofday(&time1, NULL);
		if (ret != 0) {
			err_msg = "Error getting time";
			return ret;
		}

		wait_time_timespec.tv_sec = time1.tv_sec + process->burst_time_sec;
		wait_time_timespec.tv_nsec = time1.tv_usec * USEC_IN_NSEC;

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

		gettimeofday(&time2, NULL);

		delta_time_usec = get_delta_time_usec(time1, time2);
		process->current_burst_time_usec += delta_time_usec;

		print_info("Process %s finished in time %ld secs and %ld nsecs\n",
					process->name,
					time2.tv_sec - scheduler_start_time.tv_sec,
					time2.tv_usec * USEC_IN_NSEC);

		if (ret == 0 && time2.tv_sec - scheduler_start_time.tv_sec < process->deadline_sec) {
			print_info("Process %s finished in time :)\n", process->name);
			process->state = SUCCESS;
		}
		else if(ret == 0) {
			print_info("Process %s finished but not following deadline :(\n",
						process->name);

			process->state = DEADLINE;
		}
		else {
			print_info("Process %s didn't finish in time :(\n", process->name);
			ret = pthread_cancel(process->thread);
			if (ret != 0) {
				err_msg = "Error cancelling process";
				return -1;
			}

			process->state = CANCELLED;
		}

		context_switchs += 1;
		process->real_end_time = time2.tv_sec - scheduler_start_time.tv_sec;
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
			ret = create_process_thread(process, exec_function);
			process->real_start_time = scheduler_seconds;
			if (ret) {
				err_msg = "Error creating process thread";
				return ret;
			}
		}

		process->state = RUNNING;

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
				process->state = SUCCESS;
				print_info("Process %s finished in time :)\n", process->name);
			}
			else {
				process->state = DEADLINE;
				print_info("Process %s finished but not following deadline :(\n", process->name);
			}

			process->thread = 0;
			process->real_end_time = scheduler_seconds;
			process->finished = 1;
			finished_processes += 1;
		}
		else if(process_exploded_burst_time(process)) {
			print_info("Process %s didn't finish in time :(\n", process->name);
			ret = pthread_cancel(process->thread);
			if (ret != 0) {
				err_msg = "Error cancelling process";
				return ret;
			}

			process->state = CANCELLED;
			process->thread = 0;
			process->real_end_time = scheduler_seconds;
			process->finished = 1;
			finished_processes += 1;
		}
		else {
			process->state = READY;
			suspend_process(process);
		}

		/* Even when there is only one process to finish, it will be considered
		 * a context switch, once the process is being suspended */
		context_switchs += 1;
		i = (i + 1) % num_processes;
	}

	return 0;
}

void clear_screen(u32 *num_lines) {
	/* Clear current line */
	printf("\33[2K\r");

	/* Clear previous lines */
	while(*num_lines > 0) {
		printf("\033[A\33[2K\r");
		*num_lines -= 1;
	}
}

void* print_loop(void* arg) {
	(void) arg;
	u64 time0;
	u32 num_lines = 0;
	char state;

	printf(GRN "\n====================== SCHEDULER =====================\n" RESET);

	printf(CYN "\n  SCHEDULER ALGORITHM: %s\n" \
		   "  MAX PROCESSES: %d\n" \
		   "  MAX PROCESS NAME SIZE: %d\n" \
		   "  MAX TRACE LINE SIZE: %d\n" \
		   "  TABLE PRINT WAIT TIME: %d\n" RESET,
		   scheduler_algorithm == SHORTEST_FIRST ? "Shortest First" :
		   scheduler_algorithm == ROUND_ROBIN ? "Round Robin" :
		   scheduler_algorithm == PRIORITY ? "Priority" : "?",
		   PRINT_WAIT_TIME_USEC,
		   MAX_PROCESS_NAME_SIZE,
		   MAX_PROCESS_NAME_SIZE,
		   MAX_PROCESSES);

	if(scheduler_algorithm == PRIORITY) {
		printf(CYN "  PRIORITY START QUANTUM: %d\n" \
			   "  PRIORITY QUANTUM INCREMENT: %d\n" RESET,
			   PRIORITY_START_QUANTUM_USEC,
			   PRIORITY_QUANTUM_INCREMENT_USEC);
	}
	else if(scheduler_algorithm == ROUND_ROBIN) {
		printf(CYN "  ROUND ROBIN QUANTUM: %d\n" RESET, ROUND_ROBIN_QUANTUM_USEC);
	}

	printf(MAG "\n====================== DICTIONARY ======================\n\n" \
			YEL "  Name: process name\n" \
			"  Dead: Deadline time\n" \
			"  MaxBT: Maximum burst time\n" \
			"  CurBT: Current burst time\n" \
			"  State: State of the processes. The possible ones are:\n" \
			"    - R: The process is running\n" \
			"    - r: The process is ready to run\n" \
			"    - W: The process is waiting for the init time\n" \
			"    - S: The process has succesfully finished\n" \
			"    - D: The process has finished after the Deadline\n" \
			"    - C: The process has been cancelled because burst time has exploded\n" RESET \
		  );


	printf(RED "\n======================== TABLE =========================\n\n" RESET);

	time0 = time(NULL);

	while(1) {
		u64 time1 = time(NULL);

		for(u32 p = 0; p < num_processes; p++) {
			if(processes[p].state == WAITING && processes[p].start_time_sec < time1 - time0) {
				processes[p].state = READY;
			}
		}

		printf(CYN "  TIME: %lu\n" RESET \
			   "  _______________________________________________________\n" \
			   "  | Name	| Init	| Dead	| MaxBT	| CurBT	| State	|\n",
			   time1 - time0);

		num_lines += 3;

		for(u32 p = 0; p < num_processes; p++) {
			switch (processes[p].state) {
				case READY:
					state = 'r';
					break;
				case RUNNING:
					state = 'R';
					break;
				case WAITING:
					state = 'W';
					break;
				case SUCCESS:
					state = 'S';
					break;
				case DEADLINE:
					state = 'D';
					break;
				case CANCELLED:
					state = 'C';
					break;
			}
			printf("  | %s	| %d	| %d	| %d	| %d	| %c	|\n",
					processes[p].name,
					processes[p].start_time_sec,
					processes[p].deadline_sec,
					processes[p].burst_time_sec,
					(u32)(processes[p].current_burst_time_usec / SEC_IN_USEC),
					state);
			num_lines++;
		}

		printf("  -------------------------------------------------------\n\n");
		num_lines += 2;

		usleep(PRINT_WAIT_TIME_USEC);

		fflush(stdout);

		clear_screen(&num_lines);
	}
}

int start_scheduler() {
	int ret = 0;

	print_info("Starting scheduler\n");

	if(!DEBUG_MODE) {
		pthread_create(&print_loop_thread, NULL, print_loop, NULL);
	}

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
			start_round_robin_scheduler();
			if (ret != 0) {
				err_msg = err_msg ? err_msg : "Error running round robin scheduler";
				return ret;
			}
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

	/* This is necessary to always print the last table before exiting */
	if(!DEBUG_MODE)
		usleep(PRINT_WAIT_TIME_USEC * 2);

	return 0;

error:
	print_error(err_msg);
	return ret;
}
