# small shell

A lightweight shell implementation in C that implements a subset of features of bash.  

Such features include:
- Command prompt for running commands
- Support for blank lines and comments (lines starting with #)
- Variable expansion for $$ (process ID)
- Built-in commands: exit, cd, and status
- External command execution via exec family of functions
- Input and output redirection (<, >)
- Foreground and background process execution
- Custom signal handling (SIGINT and SIGTSTP)
- Maximum command length: 2048 characters
- Maximum argument count: 512

## Demo

https://asciinema.org/a/5u1pndjgIQ0e1CbZdyldZPLrP

## Example Output

```bash
❯ ./smallsh
: ls > output.txt
: cat output.txt
Makefile
README.md
output.txt
smallsh
smallsh.c
: status
exit value 0
: sleep 3
^C
terminated by signal 2
: status
exit value 0
background pid 87740 is done: terminated by signal 2
: ps
 PID TTY          TIME CMD
12345 pts/0    00:00:00 sleep
12300 pts/0    00:00:01 bash
12301 pts/0    00:00:03 smallsh
12346 pts/0    00:00:00 ps
: # This is a comment
: echo Current PID is $$
Current PID is 87617
: nonexistent
nonexistent: no such file or directory
: status
exit value 1
: ^Z
Entering foreground-only mode (& is now ignored)
: sleep 3 &
: ^Z
Exiting foreground-only mode
: sleep 3 &
background pid is 87891
: exit
```