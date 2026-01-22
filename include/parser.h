#pragma once

#include "lexer.h"
#include <stdbool.h>

typedef enum AstNodeType {
  NODE_UNDEFINED,
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
  char *target;
  int fd;
  char *type;
} Redirection;

typedef struct CommandArgs {
} CommandArgs;

typedef struct Command {
  char **data;
  size_t size;
  size_t capacity;
} Command;

typedef struct Operator {
  struct AstNode *left;
  struct AstNode *right;
} Operator;

typedef struct Group {
  struct AstNode *inner;
} Group;

typedef struct AstNode {
  union {
    Command command;
    Operator operator;
    Group group;
    Redirection redirection;
  };
  AstNodeType type;
} AstNode;

typedef struct {
  bool is_ok;
  union {
    int err;
    AstNode *ok;
  };
} ParseResult;

ParseResult parse(Tokens *tokens);
void ast_print(AstNode *root);
void ast_free(AstNode *root);
