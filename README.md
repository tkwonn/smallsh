# small shell

A lightweight shell implementation in C that implements a subset of features of bash.  

Such features include:
- Command prompt (`:`) for running commands
- Support for blank lines and comments (lines starting with `#`)
- Variable expansion for `$$` (process ID)
- Built-in commands: `cd`, `exit`, and `status` (`$?` in bash)
- External command execution via exec family of functions (`execvp`)
- Input and output redirection (`<`, `>`)
- Foreground and background (`&`) process execution
- Custom signal handling (`SIGINT` and `SIGTSTP`)
- Maximum command length: 2048 characters
- Maximum argument count: 512

## Demo

[![asciicast](https://asciinema.org/a/5u1pndjgIQ0e1CbZdyldZPLrP.svg)](https://asciinema.org/a/5u1pndjgIQ0e1CbZdyldZPLrP)

## Example Output

```bash
❯ ./smallsh
: ls
Makefile	README.md	smallsh		smallsh.c
: ls > junk
: status
exit value 0
: cat junk
Makefile
README.md
junk
smallsh
smallsh.c
: wc < junk > junk2
: wc < junk
       5       5      42
: test -f badfile
: status
exit value 1
: wc < badfile
cannot open input file: No such file or directory
:
: # that was a blank command line, this is a comment line
: sleep 3
^C
terminated by signal 2
: status
exit value 0
background pid 87740 is done: terminated by signal 2
: cd
: pwd
/User/username
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