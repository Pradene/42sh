#pragma once

#include <stdbool.h>

#include "ast.h"
#include "env.h"
#include "sb.h"
#include "ht.h"

typedef struct {
  HashTable environment; // value -> Variable *
  HashTable aliases;     // value -> char *
  AstNode   *command;
  uint8_t   status;
} Shell;

void sigint_handler(int code);
void sigint_heredoc_handler(int code);

void expansion(AstNode *root, const Shell *shell);
void stripping(AstNode *root);
void word_splitting(AstNode *root);

void execute_command(AstNode *root, Shell *shell);
void read_heredocs(StringBuffer *sb, AstNode *root);

char *find_command_path(const char *cmd, const char *path);

char *readline(const char *prompt);
