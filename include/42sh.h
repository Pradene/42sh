#pragma once

#include <stdbool.h>

#include "ast.h"
#include "env.h"
#include "sb.h"

void sigint_handler(int code);
void sigint_heredoc_handler(int code);

void expansion(AstNode *root, const Environment *env);
void stripping(AstNode *root);

void execute_command(AstNode *root, Environment *env);
void read_heredocs(StringBuffer *sb, AstNode *root);

char *find_command_path(const char *cmd, Environment *env);

char *readline(const char *prompt);
