all: shell

shell:
	gcc new-shell.c -o new-shell

clean:
	rm new-shell
