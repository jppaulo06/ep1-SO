#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <curses.h>
#include <unistd.h>
#include <string.h>

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


#define TRUE 1
#define FALSE 0

#define u32 uint32_t
#define u64 uint64_t

#define i32 int
#define i64 long long

#define MAX_USERNAME_SIZE ((size_t)32)
#define MAX_ARGS_SIZE ((size_t)32)

#define DEFAULT_COMMAND 0x2
#define CD_COMMAND 0x4
#define RM_COMMAND 0x8
#define UNAME_A_COMMAND 0x16

char username[MAX_USERNAME_SIZE] = {};
char* args[MAX_ARGS_SIZE] = {};
char* err_msg = NULL;

int read_command(char** command) {
	size_t command_buffer_size = 0;
	int ret = getline(command, &command_buffer_size, stdin);

	if(ret < 0)
		return -1;

	/* ret - 1 because of new line */
	(*command)[ret - 1] = '\0';

	print_info("Read command line: %s\n", *command);

	return 0;
}

int parse_builtin_command(char* received_command, const char* builtin_command, char** args) {

	while(*received_command != '\0' && *builtin_command != '\0') {
		if(*received_command != *builtin_command) {
			return FALSE;
		}
		received_command++;
		builtin_command++;
	}

	if(args != NULL) {
		*received_command = '\0';
		*args = received_command + 1;
	}

	return TRUE;
}

void parse_default_command(char* received_command, char** args) {
	while(*received_command != '\0' && *received_command != ' ') {
		received_command++;
	}
	if (*received_command != '\0' && args != NULL) {
		*received_command = '\0';
		*args = received_command + 1;
	}
}

void build_args(char* command, char** args) {
	char* new_arg = NULL;
	int built_args = 0;
	argsv[built_args] = command;
	while(arg = strtok(NULL, " ") != NULL && built_args < MAX_ARGS_SIZE - 1) {
		args[built_args] = new_arg;
		built_args++;
	}
	built_args[built_args] = NULL;
}

void parse_command(char* command, char** args, u32 *command_flags) {
	command = strtok(command, " ");
	if (strcmp(command, "cd") == 0) {
		*command_flags |= CD_COMMAND;
	} else if (strcmp(command, "rm") == 0) {
		*command_flags |= RM_COMMAND;
	} else if (strcmp(command, "uname -a") == 0) {
		*command_flags |= UNAME_A_COMMAND;
	} else {
		parse_default_command(command, args);
		*command_flags |= DEFAULT_COMMAND;
	}
	build_args(command, args);
}

int execute_command(u32 command_flags, char* command, char* args) {
	int ret = 0;

	switch (command_flags) {
		case CD_COMMAND :
			/* TODO: replace this with sycall */
			print_info("CD to dir %s\n", args);
			ret = chdir(args);
			if(ret) {
				err_msg = "Could not execute cd command";
				ret = -1;
			}
			break;
		case RM_COMMAND:
			/* TODO: replace this with sycall */
			print_info("Removing file %s\n", args);
			ret = remove(args);
			if(ret) {
				err_msg = "Could not execute remove command";
				ret = -1;
			}
			break;
		case UNAME_A_COMMAND:
			/* TODO: replace this with sycall */
			print_info("Executing uname -a\n", args);
			ret = execlp("/usr/bin/uname", "uname", "-a", NULL);
			if(ret) {
				err_msg = "Could not execute uname -a command";
				ret = -1;
			}
			break;
		default:
			print_info("Executing command %s with args %s\n", command, args);
			ret = execl(command, command, args, NULL);
			if(ret) {
				err_msg = "Could not execute command";
				ret =  -1;
			}
			break;
	}

	return ret;
}

int main(int argsc, char *argsv[])
{
	i32 ret = 0;

	char* command = NULL;
	char* args = NULL;

	u32 command_flags = 0;

	/* TODO: replace this with syscall */
	ret = getlogin_r(username, MAX_USERNAME_SIZE);

	if (ret) {
		err_msg = "Could not get username";
		goto error;
	}

	while(1) {
		printf("%s@new-shell:~$ ", username);

		/* TODO: use history commands */
		ret = read_command(&command);

		if(ret) {
			err_msg = "Could not read command";
			goto error;
		}

		if(fork() == 0) {
			parse_command(command, &args, &command_flags);
			ret = execute_command(command_flags, command, args);

			if(ret)
				err_msg = "Could not execute command";

			if (command)
				free(command);

			if(ret)
				print_error(err_msg);

			return ret;
		}

		wait(NULL);
	}

	return 0;

error:
	print_error(err_msg);
	return ret;
}
