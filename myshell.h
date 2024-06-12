
typedef struct {
    char name[50];
    char value[1024];
} Variable;

void set_variable(char *name, char *value);
char *get_variable(char *name);
void handle_sigint(int sig);
void change_prompt(char *argv[]);
void handle_echo(char *argv[]);
void handle_read(char *argv[]);
void change_directory(char *argv[]);
void execute_piped_commands(char **commands, int num_commands,int *status);
void execute_command(char *argv[], int redirect, int append, char *outfile,int *status);
void add_command_to_history(const char *command);
void enable_raw_mode();
void disable_raw_mode();
void handle_arrow_key(char direction, char *buffer, int *index);

#ifndef MYSHELL_MYSHELL_H
#define MYSHELL_MYSHELL_H

#endif //MYSHELL_MYSHELL_H
