#pragma once

#include <stdbool.h>

#include "ast.h"
#include "env.h"
#include "sb.h"
#include "ht.h"

extern HashTable  *environ; // value -> Variable *
extern HashTable  *aliases; // value -> char *
extern uint8_t     exit_status;
extern bool        is_interactive;
extern bool        is_continuation;
extern AstNode    *last_command;
extern char       *input_string;
extern int         input_fd;

void sigint_handler(int code);

void apply_redirs(Redirs *redirs);
void apply_assignments(Assignments *assignments);
void execute_command(AstNode *root);

char *find_command_path(const char *cmd);

char *getline_from_fd();
char *getline_from_string();

char *expand_alias(const char *src);
void expansion(AstNode *root);
void stripping(AstNode *root);
void splitting(AstNode *root);

void expand_command(AstNode *root);

void cleanup();
