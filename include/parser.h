#pragma once

#include "lexer.h"
#include <stdbool.h>

typedef enum AstNodeType {
  NODE_COMMAND,
  NODE_REDIRECT,
  NODE_AND,
  NODE_OR,
  NODE_PIPE,
  NODE_SEMICOLON,
  NODE_BACKGROUND,
  NODE_PAREN,
  NODE_BRACE,
} AstNodeType;

typedef struct Redirection {
  int fd;
  TokenType type;
  char *target;
} Redirection;

typedef struct Redirections {
  Redirection *data;
  size_t size;
  size_t capacity;
} Redirections;

typedef struct Arguments {
  char **data;
  size_t size;
  size_t capacity;
} Arguments;

typedef struct Command {
  Arguments args;
  Redirections redirects;
} Command;

typedef struct Operator {
  struct AstNode *left;
  struct AstNode *right;
} Operator;

typedef struct Group {
  struct AstNode *inner;
  Redirections redirects;
} Group;

typedef struct AstNode {
  union {
    Command command;
    Operator operator;
    Group group;
  };
  AstNodeType type;
} AstNode;

void ast_print(AstNode *root);
void ast_free(AstNode *root);

StatusCode parse(Tokens *tokens, AstNode **root);
