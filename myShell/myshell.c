#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <signal.h>
#include "LineParser.h"

#define MAX_INPUT_SIZE 2048

#define TERMINATED -1
#define RUNNING 1
#define SUSPENDED 0
#define HISTLEN 3

typedef struct process {
    cmdLine* cmd;
    pid_t pid;
    int status;
    struct process *next;
} process;

typedef struct {
    char* command;
} HistoryEntry;
int debug = 0;
HistoryEntry* history[HISTLEN];
int newest = -1;
int oldest = 0;
int commandCount = 0;

process *process_list = NULL;

void addToHistory(const char* commandLine) {
    // Increment the newest index
    newest = (newest + 1) % HISTLEN;

    // If the queue is full, update the oldest index
    if (commandCount == HISTLEN) {
        oldest = (oldest + 1) % HISTLEN;
    }

    if(commandCount < HISTLEN) {
        commandCount++;
    }
    // Free the memory of the previous entry, if any
    if (history[newest] != NULL) {
        free(history[newest]->command);
        free(history[newest]);
    }

    // Create a new history entry
    HistoryEntry* entry = malloc(sizeof(HistoryEntry));
    entry->command = strdup(commandLine);

    // Store the entry in the history array
    history[newest] = entry;
}

void printHistory() {
    for(int i = 0; i < commandCount; i++){
        if (history[i] != NULL) {
            printf("%d %s\n", i, history[(oldest+i)%HISTLEN]->command);
        }
    }
}

void freeHistory(){
    for(int i = 0; i < commandCount; i++){
        if (history[i] != NULL) {
            free(history[i]->command);
            free(history[i]);
        }
    }
}
// Function to update the status of a process in the process list
void updateProcessStatus(process* process_list, int pid, int status) {
    process* current = process_list;

    while (current != NULL) {
        if (current->pid == pid) {
            current->status = status;
            break;
        }
        current = current->next;
    }
}

// Function to update the process list
void updateProcessList(process** process_list) {
    process* current = *process_list;

    while(current != NULL) {
        int status;
        printf("before waitpid\n");
        pid_t result = waitpid(current->pid, &status, WNOHANG | WUNTRACED | WCONTINUED);
        printf("after waitpid\n");
        if(result > 0) {
            if(WIFEXITED(status)) {
                current->status = TERMINATED;
            } else if(WIFSIGNALED(status)) {
                current->status = TERMINATED;
            } else if(WIFSTOPPED(status)) {
                current->status = SUSPENDED;
            } else if(WIFCONTINUED(status)) {
                current->status = RUNNING;
            }
        }
        current = current->next;
    }
}
void addProcess(process** process_list, cmdLine* cmd, pid_t pid) {
    process *newProcess = (process*)malloc(sizeof(process));
    newProcess->cmd = cmd;
    newProcess->pid = pid;
    newProcess->status = RUNNING;
    newProcess->next = *process_list;
    *process_list = newProcess;
}

void printProcessList(process** process_list) {
    updateProcessList(process_list);
    process *current = *process_list;
    process *prev = NULL;
    printf("PID\t\tCommand\t\tSTATUS\n");
    while(current!= NULL){   
        printf("%d\t\t%s\t\t", current->pid, current->cmd->arguments[0]);
        // printf("%d\t\t", current->pid);
        // puts(current->cmd->arguments[0]);
        if(current->status == TERMINATED) {
            printf("Terminated\n");
            // Remove the terminated process from the list
            if(prev == NULL){
                *process_list = current->next;
                freeCmdLines(current->cmd);
                free(current);
                current = *process_list;
            } else {
                prev->next = current->next;
                freeCmdLines(current->cmd);
                free(current);
                current = prev->next;
            }
            continue;
        } else if(current->status == RUNNING) {
            printf("Running\n");
        } else if(current->status == SUSPENDED) {
            printf("Suspended\n");
        }
        prev = current;
        current = current->next;
    }

}

void freeProcessList(process* process_list) {
    process *current = process_list;
    while(current != NULL) {
        process *next = current->next;
        freeCmdLines(current->cmd);
        free(current);
        current = next;
    }
}
                


void input_redirect(cmdLine *pCmdLine){
     int fd = open(pCmdLine->inputRedirect, O_RDONLY);
                    if (fd == -1) {
                        fprintf(stderr, "Failed to open input file '%s'\n", pCmdLine->inputRedirect);
                        _exit(1);
                    }
                    dup2(fd, STDIN_FILENO);
                    close(fd);
}
void output_redirect(cmdLine *pCmdLine){
    int fd = open(pCmdLine->outputRedirect, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                    if (fd == -1) {
                        fprintf(stderr, "Failed to open output file '%s'\n", pCmdLine->outputRedirect);
                        _exit(1);
                    }
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
}
void execute_pipe(cmdLine *pCmdLine){
    int fd[2]; // file descriptors for the pipe
    pid_t pid1, pid2; // process IDs of the two child processes

    if (pipe(fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    pid1 = fork(); // create first child process

    if (pid1 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    else if (pid1 == 0) { // first child process (ls -l)
        if(debug) fprintf(stderr, "(child1>redirecting stdout to the write end of the pipe...)\n");
        close(STDOUT_FILENO); // close standard output
        dup(fd[1]); // duplicate the write-end of the pipe
        close(fd[0]); // close the file descriptor that was duplicated
        if(debug) fprintf(stderr, "(child1>going to execute cmd: %s )\n",pCmdLine->arguments[0]);
        if (execvp(pCmdLine->arguments[0], pCmdLine->arguments) == -1){
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    }
    else { // parent process
        if(debug) fprintf(stderr, "(parent_process>closing the write end of the pipe...)\n");
        close(fd[1]); // close the write-end of the pipe
        if(debug) fprintf(stderr, "(parent_process>forking...)\n");
        pid2 = fork(); // create second child process
        if(debug) fprintf(stderr, "(parent_process>created process with id: %d)\n", pid2);


        if (pid2 == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
       else if (pid2 == 0) { // second child process (tail -n 2)
            if(debug) fprintf(stderr, "(child2>redirecting stdin to the read end of the pipe...)\n");
            close(STDIN_FILENO); // close standard input
            dup(fd[0]); // duplicate the read-end of the pipe
            close(fd[1]); // close the file descriptor that was duplicated
            if(debug) fprintf(stderr, "(child2>going to execute cmd: %s )\n", pCmdLine->next->arguments[0]);
            if ((execvp(pCmdLine->next->arguments[0], pCmdLine->next->arguments)) == -1){
                perror("execvp");
                exit(EXIT_FAILURE);
            }
        }
        else { // parent process
            addProcess(&process_list, pCmdLine, pid1);
            if(debug) fprintf(stderr, "(parent_process>closing the read end of the pipe...)\n");
            close(fd[0]); // close the read-end of the pipe
            
            // wait for child processes to terminate
            if(debug) fprintf(stderr, "(parent_process>waiting for child processes to terminate...)\n");
            waitpid(pid1, NULL, 0);
            if(debug) fprintf(stderr, "(parent_process>waiting for child processes to terminate...)\n");
            waitpid(pid2, NULL, 0);
            updateProcessStatus(process_list, pid1, TERMINATED);
        }
    }
    if(debug) fprintf(stderr, "(parent_process>exiting...)\n");

}

void execute(cmdLine *pCmdLine) {
    if (strcmp(pCmdLine->arguments[0], "cd") == 0){
        fprintf(stderr, "Current process with PID = %d\n", getpid());
        fprintf(stderr, "Executing command: %s\n", pCmdLine->arguments[0]);
        if (chdir(pCmdLine->arguments[1]) != 0) {
        perror("chdir");
        }
        freeCmdLines(pCmdLine);
    }
    else if(strcmp(pCmdLine->arguments[0], "suspend") == 0){
        if ((kill(atoi(pCmdLine->arguments[1]), SIGTSTP)) == 0) {
            printf("Process %s has been suspended.\n", pCmdLine->arguments[1]);
        } else {
            printf("Failed to suspend process %s.\n", pCmdLine->arguments[1]);
        }
        freeCmdLines(pCmdLine);
    }
    else if(strcmp(pCmdLine->arguments[0], "wake") == 0){
        if ((kill(atoi(pCmdLine->arguments[1]), SIGCONT)) == 0) {
            printf("Process  %s has been woken up.\n", pCmdLine->arguments[1]);
        } else {
            printf("Failed to wake up process  %s.\n", pCmdLine->arguments[1]);
        }
        freeCmdLines(pCmdLine);
    }
    else if(strcmp(pCmdLine->arguments[0], "kill") == 0){
        if ((kill(atoi(pCmdLine->arguments[1]), SIGTERM)) == 0) {
            printf("Process  %s has been terminated.\n", pCmdLine->arguments[1]);
        } else {
            printf("Failed to terminate process  %s.\n", pCmdLine->arguments[1]);
        }
        freeCmdLines(pCmdLine);
    }
    else{
        if (pCmdLine->next == NULL)
        {
            pid_t pid = fork();
            if (pid == -1) {
                perror("fork");
                exit(EXIT_FAILURE);
            } else if (pid == 0) { 
                fprintf(stderr, "Child process with PID = %d\n", getpid());
                fprintf(stderr, "Executing command: %s\n", pCmdLine->arguments[0]);
                
                if (pCmdLine->inputRedirect != NULL) {
                    input_redirect(pCmdLine);
                }
                
                if (pCmdLine->outputRedirect != NULL) {
                    output_redirect(pCmdLine);
                }
                if (execvp(pCmdLine->arguments[0], pCmdLine->arguments) == -1) {
                    perror("execvp");
                    _exit(EXIT_FAILURE);
                }
            } else { 
                addProcess(&process_list, pCmdLine, pid);
                int status;
                if (pCmdLine->blocking){
                    waitpid(pid, &status, 0);
                    updateProcessStatus(process_list, pid, TERMINATED);
                }   
            }
        }
        else
        {
            if ((pCmdLine->inputRedirect != NULL && pCmdLine->idx == 0) || (pCmdLine->outputRedirect != NULL && pCmdLine->next != NULL)) {
                fprintf(stderr, "Error: Input/output redirection not compatible with pipelines\n");
                _exit(EXIT_FAILURE);
            }
            execute_pipe(pCmdLine);
        }    
        
    }
}

int main(int argc, char** argv) {
    char cwd[PATH_MAX];
    char input[MAX_INPUT_SIZE];
    for(int i = 0; i < argc; i++){
        if(strcmp(argv[i], "-d") == 0){
            debug = 1;
        }
    }
    while (1) {
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            perror("getcwd");
            exit(EXIT_FAILURE);
        }

        printf(" %s$ ", cwd);
        
        if (fgets(input, sizeof(input), stdin) == NULL) {
            perror("fgets");
            exit(EXIT_FAILURE);
        }
        
        
        input[strcspn(input, "\n")] = '\0';
         if(!strcmp(input, "!!")) {
            if(newest != -1) {
                strcpy(input, history[newest]->command);
            }
            else {
                fprintf(stderr, "no last command is available");
            }
        } else if(input[0] == '!') {
            printf("a");
            int index;
            index = atoi(&input[1]);
            if(index == 0 || index > HISTLEN) {
                fprintf(stderr, "not valid number");
                continue;
            }
            if (index > 0 && index <= commandCount) {
                printf("valid");
                strcpy(input, history[(oldest + index-1) % HISTLEN]->command);
            } else {
                printf("Invalid history index\n");
                continue;
            }
        } else {
            addToHistory(input);
        }
        if (strcmp(input, "quit") == 0) {
            printf("Exiting shell .\n");
            freeHistory();
            freeProcessList(process_list);
            exit(EXIT_SUCCESS);
        }else if(strcmp(input, "procs") == 0) {
            printProcessList(&process_list);
        } else if(strcmp(input, "history") == 0) {
            printHistory();
            continue;
        } else {
            cmdLine *parsedCmdLine = parseCmdLines(input);
            execute(parsedCmdLine);
        }
    }

    return 0;
}        

