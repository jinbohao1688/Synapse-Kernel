#ifndef _SHELL_H_
#define _SHELL_H_

#include <stdint.h>

#define SHELL_BUFFER_SIZE 256
#define SHELL_MAX_ARGS 16

typedef struct {
    char* name;
    void (*func)(int argc, char** argv);
    char* description;
} shell_command_t;

void shell_init(void);
void shell_run(void);
void shell_prompt(void);
void shell_parse_command(char* input, int* argc, char** argv);

void shell_cmd_help(int argc, char** argv);
void shell_cmd_echo(int argc, char** argv);
void shell_cmd_clear(int argc, char** argv);
void shell_cmd_version(int argc, char** argv);
void shell_cmd_ai(int argc, char** argv);
void shell_cmd_ps(int argc, char** argv);
void shell_cmd_free(int argc, char** argv);
void shell_cmd_top(int argc, char** argv);

#endif
