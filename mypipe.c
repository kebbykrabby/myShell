#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main() {
    int pipeFileDes[2];
    pid_t pid;
    char buffer[1024];
    if (pipe(pipeFileDes) == -1) {
        perror("Failed to create pipe");
        exit(EXIT_FAILURE);
    }  
    pid = fork();
    if (pid == -1) {
        perror("Failed to fork");
        exit(EXIT_FAILURE);
    }
    if (pid == 0) {
        
        close(pipeFileDes[0]); 
      
        char message[] = "hello";
        if (write(pipeFileDes[1], message, strlen(message)) == -1) {
            perror("Failed to write to pipe");
            exit(EXIT_FAILURE);
        }

        printf("Child message: %s\n", message);
        exit(EXIT_SUCCESS);
    } else {
        close(pipeFileDes[1]); 
        ssize_t bytesRead = read(pipeFileDes[0], buffer, sizeof(buffer));
        if (bytesRead == -1) {
            perror("Failed to read from pipe");
            exit(EXIT_FAILURE);
        }
        printf("Parent  message: %.*s\n", (int)bytesRead, buffer);
        exit(EXIT_SUCCESS);
    }

    return 0;
}