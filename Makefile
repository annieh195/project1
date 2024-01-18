# Executable rule
sshell: sshell.c
	gcc -Wall -Wextra -Werror -o sshell sshell.c

# Clean rule
clean:
	rm -f sshell
