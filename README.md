# smallsh
This is a shell I made for a school assignment.<br/>
type 'make' to compile.<br/>
to run, type ./smallsh <br/>
It does these basic things:
1. Provide a prompt for running commands
2. Handle blank lines and comments, which are lines beginning with the # character
3. Provide expansion for the variable $$
4. Execute 3 commands exit, cd, and status via code built into the shell
5. Execute other commands by creating new processes using a function from the exec family of functions
6. Support input and output redirection
7. Support running commands in foreground and background processes
8. Implement custom handlers for 2 signals, SIGINT and SIGTSTP

USAGE:
command [arg1 arg2 ...] [< input_file] [> output_file] [&]

expands "$$" in a command into the process ID of the shell itself

supports three built-in commands: exit, cd, and status. These are the only ones that the shell handles itself - all others are passed on to a member of the exec() family of functions

Runs processes in the foreground and background. 
