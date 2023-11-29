# myShell
myShell is a command interpreter program that gets user commands and executes them.

mypipeline.c: This file contains the implementation of a standalone program (executable) called mypipeline. This program creates a pipeline of two child processes, similar to the shell command "ls -l | tail -n 2". The code should include the necessary steps to create a pipe, fork child processes, and redirect standard input and output.

myshell.c: This file contains the implementation of a shell program with extended features. It includes the following enhancements:

- Implementation of pipes in the shell, supporting command lines with one pipe between two processes.

- Implementation of a process manager that allows printing, suspending, waking up, and killing processes. This part involves creating a linked list to store information about running/suspended processes and implementing functions to add, print, and manipulate this process list.

- Addition of a history mechanism to the shell, keeping track of previous command lines. The history supports commands like history, !!, and !n. The implementation involves using a circular queue for the history list.

makefile: This file includes instructions for compiling and linking the mypipeline and myshell programs. It specifies the dependencies and compilation options.
