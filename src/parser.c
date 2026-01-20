#include "parser.h"
#include "lexer.h"
#include "vec.h"
#include <stdio.h>

void ast_free(AstNode **root) {
  switch ((*root)->type) {
  case NODE_AND:
  case NODE_OR:
    ast_free(&(*root)->sequence.left);
    ast_free(&(*root)->sequence.right);
    break;
  case NODE_SEMICOLON:
  case NODE_BACKGROUND:
    ast_free(&(*root)->sequence.left);
    if ((*root)->sequence.right) {
      ast_free(&(*root)->sequence.right);
    }
    break;
  case NODE_PIPE:
    ast_free(&(*root)->pipeline.left);
    ast_free(&(*root)->pipeline.right);
    break;
  case NODE_COMMAND:
    for (size_t i = 0; i < vec_size(&(*root)->command.args); i++) {
      free(vec_at(&(*root)->command.args, i));
    }
    vec_free(&(*root)->command.args);
    break;
  default:
    return;
  }

  free(*root);
  (*root) = NULL;
}

static void ast_print_inner(AstNode *root, int depth) {
  if (!root) {
    return;
  }

  printf("%*s", depth * 2, "");
  switch (root->type) {
  case NODE_COMMAND:
    printf("COMMAND: ");
    for (size_t i = 0; i < vec_size(&root->command.args); ++i) {
      printf("%s ", vec_at(&root->command.args, i));
    }
    printf("\n");
    return;

  case NODE_PIPE:
    printf("PIPE\n");
    ast_print_inner(root->pipeline.left, depth + 1);
    ast_print_inner(root->pipeline.right, depth + 1);
    return;

  case NODE_AND:
    printf("AND (&&)\n");
    ast_print_inner(root->sequence.left, depth + 1);
    ast_print_inner(root->sequence.right, depth + 1);
    return;

  case NODE_OR:
    printf("OR (||)\n");
    ast_print_inner(root->sequence.left, depth + 1);
    ast_print_inner(root->sequence.right, depth + 1);
    return;

  case NODE_SEMICOLON:
    printf("SEMICOLON (;)\n");
    ast_print_inner(root->sequence.left, depth + 1);
    if (root->sequence.right) {
      ast_print_inner(root->sequence.right, depth + 1);
    }
    return;

  case NODE_BACKGROUND:
    printf("BACKGROUND (&)\n");
    ast_print_inner(root->sequence.left, depth + 1);
    if (root->sequence.right) {
      ast_print_inner(root->sequence.right, depth + 1);
    }
    return;

  default:
    return;
  }
}

void ast_print(AstNode *root) { ast_print_inner(root, 0); }

static AstNode *parse_command(Tokens *tokens, size_t *i) {
  AstNode *node = (AstNode *)malloc(sizeof(AstNode));
  if (!node) {
    return NULL;
  }
  node->type = NODE_COMMAND;
  node->command.args = (CommandArgs){0};
  while (*i < vec_size(tokens)) {
    Token token = vec_at(tokens, *i);
    if (token.type != TOKEN_WORD) {
      break;
    }

    vec_push(&node->command.args, token.s);
    ++(*i);
  }
  if (vec_size(&node->command.args) == 0) {
    return NULL;
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
    node->pipeline.right = right;
    node->pipeline.left = left;
    left = node;
  }
  return left;
}

static AstNode *parse_logical(Tokens *tokens, size_t *i) {
  AstNode *left = parse_pipeline(tokens, i);
  if (!left) {
    return NULL;
  }

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
    } else {
      break;
    }

    ++(*i);

    AstNode *right = parse_pipeline(tokens, i);
    if (!right) {
      return NULL;
    }

    node->sequence.left = left;
    node->sequence.right = right;
    left = node;
  }
  return left;
}

static AstNode *parse_sequence(Tokens *tokens, size_t *i) {
  AstNode *left = parse_logical(tokens, i);
  if (!left) {
    return NULL;
  }

  while (*i < vec_size(tokens)) {
    Token token = vec_at(tokens, *i);
    AstNode *node = (AstNode *)malloc(sizeof(AstNode));
    if (!node) {
      return NULL;
    }

    if (token.type == TOKEN_OPERAND) {
      node->type = NODE_BACKGROUND;
    } else if (token.type == TOKEN_SEMICOLON) {
      node->type = NODE_SEMICOLON;
    } else {
      break;
    }

    ++(*i);

    AstNode *right;
    if (*i < vec_size(tokens)) {
      right = parse_logical(tokens, i);
    } else {
      right = NULL;
    }

    node->sequence.left = left;
    node->sequence.right = right;
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
