#pragma once

#include <stdbool.h>

#include "ast.h"
#include "env.h"
#include "sb.h"
#include "ht.h"
#include "shell.h"

void sigint_handler(int code);
void sigint_heredoc_handler(int code);

void execute_command(Shell *shell);

char *find_command_path(const char *cmd, const char *path);

char *readline_interactive(Shell *shell);
char *readline_string(Shell *shell);
char *readline_fd(Shell *shell);

void expand_alias(Shell *shell);
void expansion(AstNode *root, const Shell *shell);
void stripping(AstNode *root);
void splitting(AstNode *root);

void expand_command(Shell *shell);
