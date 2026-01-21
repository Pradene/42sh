#pragma once

#include "lexer.h"
#include "vec.h"
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
  char **data;
  size_t size;
  size_t capacity;
} CommandArgs;

typedef struct Command {
  CommandArgs args;
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
  };
  AstNodeType type;
} AstNode;

bool parse(Tokens *tokens, AstNode **root);
void ast_print(AstNode *root);
void ast_free(AstNode *root);
