#include "parser.h"
#include <stdio.h>

void ast_free(AstNode **root) {
  if (!root || !(*root)) {
    return;
  }

  switch ((*root)->type) {
  case NODE_AND:
  case NODE_OR:
  case NODE_SEMICOLON:
  case NODE_BACKGROUND:
  case NODE_PIPE:
    if ((*root)->operator.left)
      ast_free(&(*root)->operator.left);
    if ((*root)->operator.right)
      ast_free(&(*root)->operator.right);
    break;
  case NODE_COMMAND:
    for (size_t i = 0; i < vec_size(&(*root)->command.args); ++i) {
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
    break;

  case NODE_PIPE:
    printf("PIPE\n");
    ast_print_inner(root->operator.left, depth + 1);
    ast_print_inner(root->operator.right, depth + 1);
    break;

  case NODE_AND:
    printf("AND (&&)\n");
    ast_print_inner(root->operator.left, depth + 1);
    ast_print_inner(root->operator.right, depth + 1);
    break;

  case NODE_OR:
    printf("OR (||)\n");
    ast_print_inner(root->operator.left, depth + 1);
    ast_print_inner(root->operator.right, depth + 1);
    break;

  case NODE_SEMICOLON:
    printf("SEMICOLON (;)\n");
    ast_print_inner(root->operator.left, depth + 1);
    if (root->operator.right) {
      ast_print_inner(root->operator.right, depth + 1);
    }
    break;

  case NODE_BACKGROUND:
    printf("BACKGROUND (&)\n");
    ast_print_inner(root->operator.left, depth + 1);
    if (root->operator.right) {
      ast_print_inner(root->operator.right, depth + 1);
    }
    break;

  default:
    break;
  }
}

void ast_print(AstNode *root) { ast_print_inner(root, 0); }
