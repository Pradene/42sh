#pragma once

#include <stdbool.h>

#include "ast.h"
#include "env.h"
#include "sb.h"
#include "ht.h"
#include "shell.h"

extern HashTable    *environ; // value -> Variable *
extern HashTable    *aliases; // value -> char *

extern uint8_t       exit_status;

extern bool          is_interactive;
extern bool          is_continuation;

extern AstNode      *last_command;

void sigint_handler(int code);
void sigint_heredoc_handler(int code);

void apply_redirs(Redirs *redirs);
void apply_assignments(Assignments *assignments);
void execute_command(AstNode *root, Shell *shell);

char *find_command_path(const char *cmd);

char *readline_interactive(Shell *shell);
char *readline_string(Shell *shell);
char *readline_fd(Shell *shell);

char *expand_alias(const char *src);
void expansion(AstNode *root, const Shell *shell);
void stripping(AstNode *root);
void splitting(AstNode *root);

void expand_command(AstNode *root, Shell *shell);
