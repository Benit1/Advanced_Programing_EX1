#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <termios.h>
#include <sys/wait.h>
#include "myshell.h"

#define MAX_SIZE_HISTORY 20
#define BUFFER_SIZE 1024
#define NUM_VARIABLES 100

char history[MAX_SIZE_HISTORY][BUFFER_SIZE];
int history_index = 0;
int current_pos = 0;
int exit_status=0;

int is_if_block = 0;
char if_block[4096] = "";

Variable variables[NUM_VARIABLES];
int var_count = 0;

char prompt[BUFFER_SIZE] = "hello: "; // Define default prompt

// Function to print the shell prompt
void print_prompt(){
    printf("%s", prompt);
    fflush(stdout);
}

// Function to set a variable
void set_variable(char *name, char *value) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            strcpy(variables[i].value, value);
            return;
        }
    }
    strcpy(variables[var_count].name, name);
    strcpy(variables[var_count].value, value);
    var_count++;
}

// Function to get a variable's value
char *get_variable(char *name) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            return variables[i].value;
        }
    }
    return NULL;
}

// Signal handler for SIGINT (control-C)
void handle_sigint(int sig) {
    write(STDOUT_FILENO, "You typed Control-C!\n", 21);
    write(STDOUT_FILENO,prompt, strlen(prompt));
}

// Function to change the shell prompt
void change_prompt(char *argv[]) {
    strncpy(prompt, argv[2], BUFFER_SIZE - 2);
    prompt[BUFFER_SIZE - 2] = '\0';
    strncat(prompt, " ", BUFFER_SIZE - strlen(prompt) - 1);
}

// Function to handle echo command
void handle_echo(char *argv[]) {
    for (int j = 1; argv[j] != NULL; j++) {
        if (argv[j][0] == '$') {
            char *var_value = get_variable(argv[j] + 1);
            if (var_value) {
                printf("%s", var_value);
            } else {
                printf("%s", argv[j]);
            }
        } else {
            printf("%s", argv[j]);
        }
        if (argv[j + 1] != NULL) {
            printf(" ");
        }
    }
    printf("\n");
}

// Function to add a command to history
void add_command_to_history(const char *command) {
    if (history_index < MAX_SIZE_HISTORY) {
        strcpy(history[history_index++], command);
    } else {
        for (int i = 1; i < MAX_SIZE_HISTORY; i++) {
            strcpy(history[i - 1], history[i]);
        }
        strcpy(history[MAX_SIZE_HISTORY - 1], command);
    }
    current_pos = history_index;
}

// Function to read input from the user
int read_input(char *buffer) {
    enable_raw_mode();

    int index = 0;
    current_pos = history_index;

    while (1) {
        fflush(stdout);
        int c = getchar();

        if (c == '\r' || c == '\n') {
            buffer[index] = '\0';
            printf("\n");
            break;
        } else if (c == 127 || c == '\b') {  // Handle backspace
            if (index > 0) {
                buffer[--index] = '\0';
                printf("\b \b");
            }
        } else if (c == 27) {  // Escape sequence
            if (getchar() == '[') {
                c = getchar();
                handle_arrow_key(c, buffer, &index);
                continue;
            }
        } else {
            buffer[index++] = c;
            printf("%c", c);
        }
        fflush(stdout);
    }

    disable_raw_mode();
    return index;
}

// Function to enable raw mode
void enable_raw_mode() {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &raw);
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

// Function to disable raw mode
void disable_raw_mode() {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &raw);
    raw.c_lflag |= (ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

// Function to handle arrow key input
void handle_arrow_key(char direction, char *buffer, int *index) {
    if (direction == 'A') {  // Up arrow
        if (current_pos > 0) {
            current_pos--;
            strcpy(buffer, history[current_pos]);
            *index = strlen(buffer);
            // Clear the current line
            printf("\33[2K\r");
            print_prompt();
            printf("%s", buffer);
            fflush(stdout);
        }
    } else if (direction == 'B') {  // Down arrow
        if (current_pos < history_index - 1) {
            current_pos++;
            strcpy(buffer, history[current_pos]);
            *index = strlen(buffer);

            printf("\33[2K\r");
            print_prompt();
            printf("%s", buffer);
            fflush(stdout);

        } else if (current_pos == history_index - 1) {
            current_pos++;
            buffer[0] = '\0';
            *index = 0;

            printf("\33[2K\r");
            print_prompt();
            fflush(stdout);
        }
    }
}

// Function to handle read command
void handle_read(char *argv[]) {
    if (argv[1] == NULL) {
        fprintf(stderr, "read: missing variable name\n");
        return;
    }

    char input[1024];
    print_prompt();
    if (fgets(input, 1024, stdin) != NULL) {
        input[strcspn(input, "\n")] = '\0';
        set_variable(argv[1], input);
    } else {
        perror("fgets");
    }
}



// Function to handle cd command
void change_directory(char *argv[]) {
    if (argv[1] == NULL) {
        fprintf(stderr, "cd: missing argument\n");
    } else if (chdir(argv[1]) != 0) {
        perror("cd");
    }
}

// Function to execute commands with pipes

void execute_piped_commands(char **commands, int num_commands, int *status) {
    int i;
    int in_fd = 0;
    int pipe_fds[2];
    pid_t pid;

    for (i = 0; i < num_commands; i++) {
        if (i < num_commands - 1) {
            pipe(pipe_fds);
        }

        pid = fork();
        if (pid == 0) {
            // Child process
            if (i > 0) {
                dup2(in_fd, 0); // Replace standard input with in_fd
                close(in_fd);
            }
            if (i < num_commands - 1) {
                close(pipe_fds[0]);
                dup2(pipe_fds[1], 1); // Replace standard output with pipe_fds[1]
                close(pipe_fds[1]);
            }

            // Parse command line into tokens dynamically
            char *token;
            char **argv = malloc(10 * sizeof(char *)); // Initial allocation
            int argc = 0;
            token = strtok(commands[i], " ");
            while (token != NULL) {
                if (token[0] == '$') {
                    char *var_value = get_variable(token + 1);
                    if (var_value) {
                        argv[argc] = var_value;
                    } else {
                        argv[argc] = token;
                    }
                } else {
                    argv[argc] = token;
                }
                argc++;
                if (argc % 10 == 0) { // Reallocate if necessary
                    argv = realloc(argv, (argc + 10) * sizeof(char *));
                }
                token = strtok(NULL, " ");
            }
            argv[argc] = NULL;

            execvp(argv[0], argv);
            perror("execvp");
            exit(1);
        } else {
            // Parent process
            if (i > 0) {
                close(in_fd);
            }
            if (i < num_commands - 1) {
                close(pipe_fds[1]);
                in_fd = pipe_fds[0];
            }
            int wstatus;
            waitpid(pid, &wstatus, 0);

            // Update the status with the exit status of the last command in the pipeline
            if (WIFEXITED(wstatus)) {
                *status = WEXITSTATUS(wstatus);
            } else {
                *status = -1; // Indicate that the process did not terminate normally
            }
        }
    }
}


// Function to execute a single command with potential redirection

void execute_command(char *argv[], int redirect, int append, char *outfile, int *status) {
    int fd;
    pid_t pid = fork();

    if (pid == 0) {
        // Child process
        if (redirect == 1) {
            if (append) {
                fd = open(outfile, O_WRONLY | O_CREAT | O_APPEND, 0660); // Open file for appending
            } else {
                fd = creat(outfile, 0660); // Create/overwrite file
            }
            dup2(fd, STDOUT_FILENO); // Duplicate file descriptor to standard output
            close(fd);
        } else if (redirect == 2) {
            fd = creat(outfile, 0660); // Create/overwrite file
            dup2(fd, STDERR_FILENO); // Duplicate file descriptor to standard error
            close(fd);
        }

        execvp(argv[0], argv);
        perror("execvp");
        exit(1);
    } else {
        // Parent process
        int wstatus;
        waitpid(pid, &wstatus, 0);

        // Update the status with the exit status of the command
        if (WIFEXITED(wstatus)) {
            *status = WEXITSTATUS(wstatus);
        } else {
            *status = -1; // Indicate that the process did not terminate normally
        }
    }
}

// Function to execute a command and check its status
int execute_command_with_status(char *command) {

    int status;
    pid_t pid = fork();
    if (pid == 0) {
        char *argv[] = { "/bin/sh", "-c", command, NULL };
        execvp(argv[0], argv);
        perror("execvp");
        exit(1);
    } else if (pid < 0) {
        perror("fork");
        return 0;
    } else {
        waitpid(pid, &status, 0);
        return WIFEXITED(status) && WEXITSTATUS(status) == 0;
    }
}

// Function to handle if-else structures
int handle_if_else(char *command) {
    char *if_token = "if";
    char *then_token = "then";
    char *else_token = "else";
    char *fi_token = "fi";
    char *if_ptr = strstr(command, if_token);
    char *then_ptr = strstr(command, then_token);
    char *else_ptr = strstr(command, else_token);
    char *fi_ptr = strstr(command, fi_token);

    if (if_ptr && then_ptr && fi_ptr) {
        *then_ptr = '\0';
        *fi_ptr = '\0';

        char *if_part = if_ptr + strlen(if_token);
        char *then_part = then_ptr + strlen(then_token);
        char *else_part = else_ptr ? else_ptr + strlen(else_token) : NULL;

        if (else_ptr) {
            *else_ptr = '\0';
        }

        if (execute_command_with_status(if_part)) {
            return execute_command_with_status(then_part);
        } else if (else_part) {
            return execute_command_with_status(else_part);
        }

        return 1;
    }

    return 0;
}

int main() {
    // Set up signal handler for SIGINT
    struct sigaction sa;
    sa.sa_handler = &handle_sigint;
    sa.sa_flags = SA_RESTART;
    sigfillset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    char command[1024]; // Buffer for the command input
    char last_command[1024] = ""; // Buffer for the last executed command
    char *token;
    char *outfile;
    int i, amper, redirect, append;
    char **argv; // Array to store the command and its arguments

    while (1) {
        print_prompt();

       int index=read_input(command);
       if (index==0) continue;

        add_command_to_history(command);

        // Handle the !! command
        if (strcmp(command, "!!") == 0) {
            if (strlen(last_command) == 0) {
                printf("No previous command to repeat\n");
                exit_status=1;
                continue;
            } else {
                printf("%s\n", last_command); // Print the last command
                strcpy(command, last_command);
                exit_status=0;
                continue;
            }
        } else {
            strcpy(last_command, command);
        }

// Print the command if it was '!!' to avoid duplicate output
        if (strcmp(command, last_command) != 0) {
            printf("%s\n", command);
            exit_status=1;
        }
        // Check if the command starts an if block
        if (strncmp(command, "if ", 3) == 0 || is_if_block) {
            strcat(if_block, command);
            strcat(if_block, "\n");

            // Check if the if block is complete
            if (strstr(command, "fi") != NULL) {
                exit_status=handle_if_else(if_block);
                if (exit_status) {
                    is_if_block = 0;
                    if_block[0] = '\0';
                    continue;
                } else {
                    // If it's not a valid if block, reset and treat it as a regular command
                    is_if_block = 0;
                    if_block[0] = '\0';
                }
            } else {
                is_if_block = 1;
                continue;
            }
        }


        // Check for variable assignment
        if (command[0] == '$' && strstr(command, " = ") != NULL) {
            char *name = strtok(command + 1, " ");
            strtok(NULL, " "); // Skip the '='
            char *value = strtok(NULL, "");
            if (name && value) {
                while (*value == ' ') value++;
                char *end = value + strlen(value) - 1;
                while (end > value && *end == ' ') end--;
                *(end + 1) = '\0';

                set_variable(name, value);
                continue;
            }
        }
        // Substitute variables in the command
        char *temp_command = strdup(command);
        token = strtok(temp_command, " ");
        while (token != NULL) {
            if (token[0] == '$') {
                char *var_value = get_variable(token + 1);
                if (var_value) {
                    command[token - temp_command] = '\0';
                    strcat(command, var_value);
                    strcat(command, temp_command + (token - temp_command) + strlen(token));
                    temp_command = strdup(command);
                }
            }
            token = strtok(NULL, " ");
        }
        free(temp_command);

        // Parse command line to check for pipes
        char **commands = malloc(10 * sizeof(char *));
        int num_commands = 0;
        token = strtok(command, "|");
        while (token != NULL) {
            commands[num_commands++] = strdup(token);
            if (num_commands % 10 == 0) {
                commands = realloc(commands, (num_commands + 10) * sizeof(char *));
            }
            token = strtok(NULL, "|");
        }

        if (num_commands > 1) {
            execute_piped_commands(commands, num_commands,&exit_status);
            for (i = 0; i < num_commands; i++) {
                free(commands[i]);
            }
            free(commands);
            continue;
        }

        // Parse command line into tokens dynamically
        argv = malloc(10 * sizeof(char *));
        i = 0;
        token = strtok(command, " ");
        while (token != NULL) {
            if (token[0] == '$') {
                char *var_value = get_variable(token + 1);
                if (var_value) {
                    argv[i++] = var_value;
                } else {
                    argv[i++] = token;
                }
            } else {
                argv[i++] = token;
            }
            if (i % 10 == 0) { // Reallocate if necessary
                argv = realloc(argv, (i + 10) * sizeof(char *));
            }
            token = strtok(NULL, " ");
        }
        argv[i] = NULL;

        // Check if command is empty
        if (argv[0] == NULL) {
            free(argv);
            free(commands);
            continue;
        }

        // Handle quit command
        if (strcmp(argv[0], "quit") == 0) {
            free(argv);
            free(commands);
            break;
        }

        // Handle prompt change command
        if (i == 3 && strcmp(argv[0], "prompt") == 0 && strcmp(argv[1], "=") == 0) {
            change_prompt(argv);
            free(argv);
            free(commands);
            continue;
        }

        // Handle echo command
        if (strcmp(argv[0],"echo") ==0 && argv[1]!=NULL && strcmp(argv[1],"$?")==0) {
            char str[10];
            sprintf(str, "%d", exit_status);
            argv[1]=str;
           execute_command(argv,redirect,append,outfile,&exit_status);
            continue;
        }
        if (strcmp(argv[0], "echo") == 0) {
            handle_echo(argv);
            free(argv);
            free(commands);
            continue;
        }

        // Handle cd command
        if (strcmp(argv[0], "cd") == 0) {
            change_directory(argv);
            free(argv);
            free(commands);
            continue;
        }

        // Check if command ends with '&' for background execution
        if (!strcmp(argv[i - 1], "&")) {
            amper = 1;
            argv[i - 1] = NULL;
        } else {
            amper = 0;
        }

        // Handle read command
        if (strcmp(argv[0], "read") == 0) {
            handle_read(argv);
            free(argv);
            free(commands);
            continue;
        }

        // Handle output redirection
        if (i > 2 && (!strcmp(argv[i - 2], ">") || !strcmp(argv[i - 2], ">>") || !strcmp(argv[i - 2], "2>"))) {
            if (!strcmp(argv[i - 2], ">")) {
                redirect = 1;
                append = 0;
            } else if (!strcmp(argv[i - 2], ">>")) {
                redirect = 1;
                append = 1;
            } else if (!strcmp(argv[i - 2], "2>")) {
                redirect = 2;
                append = 0;
            }
            argv[i - 2] = NULL;
            outfile = argv[i - 1];
        } else {
            redirect = 0;
            append = 0;
        }

        // Execute the command with the appropriate redirection
        execute_command(argv, redirect, append, outfile,&exit_status);

        free(argv); // Free dynamically allocated argv
        free(commands); // Free dynamically allocated commands
    }

    return 0;
}
