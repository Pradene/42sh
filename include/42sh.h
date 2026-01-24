#pragma once

#include "parser.h"

typedef struct {
  char **data;
  size_t size;
  size_t capacity;
} Environment;

void sigint_handler(int code);
void expansion(AstNode *root);
void stripping(AstNode *root);
void env_free(Environment *env);
StatusCode env_copy(Environment *env, char **envp);
