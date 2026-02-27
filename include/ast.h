#pragma once

#include <stdbool.h>
#include <stdlib.h>

typedef enum AstNodeType {
  NODE_COMMAND,
  NODE_AND,
  NODE_OR,
  NODE_PIPE,
  NODE_SEMICOLON,
  NODE_BACKGROUND,
  NODE_PAREN,
  NODE_BRACE,
} AstNodeType;

typedef enum RedirType {
  REDIRECT_IN,
  REDIRECT_OUT,
  REDIRECT_APPEND,
  REDIRECT_HEREDOC,
  REDIRECT_IN_FD,
  REDIRECT_OUT_FD,
} RedirType;

typedef struct Redir {
  RedirType type;
  union {
    int   fd;
    char  *path;
    char  *delimiter;
  };
} Redir;

typedef struct Redirs {
  Redir *data;
  size_t size;
  size_t capacity;
} Redirs;

typedef struct Arguments {
  char **data;
  size_t size;
  size_t capacity;
} Arguments;

typedef struct Command {
  Arguments args;
  Redirs redirs;
} Command;

typedef struct Operator {
  struct AstNode *left;
  struct AstNode *right;
} Operator;

typedef struct Group {
  struct AstNode *inner;
  Redirs redirs;
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
bool has_heredocs(AstNode *root);
