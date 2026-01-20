#pragma once

#include "lexer.h"
#include "vec.h"
#include <stdbool.h>
#include <stdlib.h>

typedef enum AstNodeType {
  NODE_COMMAND,
  NODE_REDIRECT,
  NODE_AND,
  NODE_OR,
  NODE_PIPE,
  NODE_SEMICOLON,
  NODE_PAREN,
  NODE_BRACE,
} AstNodeType;

typedef struct Redirection {
  char *target;
  int fd;
  char *type;
} Redirection;

typedef VEC(char *) CommandArgs;
typedef struct Command {
  CommandArgs args;
  Redirection *redirects;
  size_t redirect_count;
} Command;

typedef struct AstNode AstNode;
typedef struct Pipeline {
  AstNode *left;
  AstNode *right;
} Pipeline;

typedef struct Sequence {
  struct AstNode *left;
  struct AstNode *right;
  char *op; // "&&", "||", ";"
} Sequence;

typedef struct AstNode {
  union {
    Command command;
    Pipeline pipeline;
    Sequence sequence;
  } data;
  AstNodeType type;
} AstNode;

bool parse(Tokens *tokens, AstNode **root);
void ast_print(AstNode *root);
void ast_free(AstNode **root);
