#pragma once

#include "ast.h"
#include "error.h"
#include "token.h"

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
StatusCode lex(char *input, Tokens *tokens);
StatusCode parse(Tokens *tokens, AstNode **root);
