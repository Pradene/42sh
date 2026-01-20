#include "parser.h"
#include "lexer.h"
#include "vec.h"
#include <stdio.h>
#include <stdlib.h>

static inline void ast_print_indent(int depth) { printf("%*s", depth * 2, ""); }

static void ast_print_inner(AstNode *root, int depth) {
  if (!root)
    return;

  ast_print_indent(depth);
  switch (root->type) {
  case NODE_COMMAND:
    printf("COMMAND: ");
    for (size_t i = 0; i < vec_size(&vec_begin(root).command.args); ++i) {
      printf("%s ", vec_at(&vec_begin(root).command.args, i));
    }
    printf("\n");
    return;

  case NODE_PIPE:
    printf("PIPE\n");
    ast_print_inner(root->data.pipeline.left, depth + 1);
    ast_print_inner(root->data.pipeline.right, depth + 1);
    return;

  case NODE_AND:
    printf("AND (&&)\n");
    ast_print_inner(root->data.sequence.left, depth + 1);
    ast_print_inner(root->data.sequence.right, depth + 1);
    return;

  case NODE_OR:
    printf("OR (||)\n");
    ast_print_inner(root->data.sequence.left, depth + 1);
    ast_print_inner(root->data.sequence.right, depth + 1);
    return;

  case NODE_SEMICOLON:
    printf("SEMICOLON (;)\n");
    ast_print_inner(root->data.sequence.left, depth + 1);
    ast_print_inner(root->data.sequence.right, depth + 1);
    return;

  default:
    return;
  }
}

void ast_print(AstNode *root) { ast_print_inner(root, 0); }

static AstNode *parse_command(Tokens *tokens, size_t *i) {
  AstNode *node = (AstNode *)malloc(sizeof(AstNode));
  if (!node)
    return NULL;
  node->type = NODE_COMMAND;
  node->data.command.name = NULL;
  node->data.command.args.data = NULL;
  node->data.command.args.capacity = 0;
  node->data.command.args.size = 0;
  while (*i < vec_size(tokens)) {
    Token token = vec_at(tokens, *i);
    if (token.type != TOKEN_WORD) {
      break;
    }

    vec_push(&node->data.command.args, token.s);
    if (!node->data.command.name) {
      node->data.command.name = token.s;
    }
    ++(*i);
  }
  return node;
}

static AstNode *parse_pipeline(Tokens *tokens, size_t *i) {
  AstNode *left = parse_command(tokens, i);
  if (!left) {
    return NULL;
  }
  while (*i < vec_size(tokens) && vec_at(tokens, *i).type == TOKEN_PIPE) {
    ++(*i);
    AstNode *right = parse_command(tokens, i);
    AstNode *node = (AstNode *)malloc(sizeof(AstNode));
    if (!right || !node) {
      return NULL;
    }
    node->type = NODE_PIPE;
    node->data.pipeline.right = right;
    node->data.pipeline.left = left;
    left = node;
  }
  return left;
}

static AstNode *parse_sequence(Tokens *tokens, size_t *i) {
  AstNode *left = parse_pipeline(tokens, i);

  while (*i < vec_size(tokens)) {
    Token token = vec_at(tokens, *i);
    AstNode *node = (AstNode *)malloc(sizeof(AstNode));
    if (!node) {
      return NULL;
    }

    if (token.type == TOKEN_AND) {
      node->type = NODE_AND;
    } else if (token.type == TOKEN_OR) {
      node->type = NODE_OR;
    } else if (token.type == TOKEN_SEMICOLON) {
      node->type = NODE_SEMICOLON;
    }
    ++(*i);
    AstNode *right = parse_pipeline(tokens, i);
    node->data.sequence.left = left;
    node->data.sequence.right = right;
    left = node;
  }
  return left;
}

bool parse(Tokens *tokens, AstNode **root) {
  size_t position = 0;

  *root = parse_sequence(tokens, &position);
  if (position != vec_size(tokens)) {
    return false;
  }
  return true;
}
