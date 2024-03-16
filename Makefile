all: shell

shell:
	gcc new-shell.c -lreadline -o new-shell

clean:
	rm new-shell
