CFLAGS = -Wall -Wextra -lreadline

all: scheduler

shell:
	gcc $(CFLAGS) new-shell.c -lreadline -o new-shell

scheduler: scheduler.c
	gcc $(CFLAGS) scheduler.c -lreadline -lpthread -o scheduler

clean:
	@if [ -f new-shell ]; then rm new-shell; fi
	@if [ -f scheduler ]; then rm scheduler; fi
