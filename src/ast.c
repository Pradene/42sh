#include "parser.h"
#include "vec.h"

#include <stdio.h>

void ast_free(AstNode *root) {
  if (!root) {
    return;
  }

  switch (root->type) {
  case NODE_REDIRECT:
    free(root->redirection.target);
    break;
  case NODE_AND:
  case NODE_OR:
  case NODE_SEMICOLON:
  case NODE_BACKGROUND:
  case NODE_PIPE:
    ast_free(root->operator.left);
    ast_free(root->operator.right);
    break;
  case NODE_COMMAND:
    ast_free(root->command.redirect);
    for (size_t i = 0; i < vec_size(&root->command); ++i) {
      free(vec_at(&root->command, i));
    }
    vec_free(&root->command);
    break;
  case NODE_PAREN:
  case NODE_BRACE:
    ast_free(root->group.redirect);
    ast_free(root->group.inner);
    break;
  default:
    return;
  }

  free(root);
}

static void ast_print_inner(AstNode *root, int depth) {
  if (!root) {
    return;
  }

  printf("%*s", depth * 2, "");
  switch (root->type) {
  case NODE_COMMAND:
    printf("COMMAND: ");
    for (size_t i = 0; i < vec_size(&root->command); ++i) {
      printf("%s ", vec_at(&root->command, i));
    }
    printf("\n");
    ast_print_inner(root->command.redirect, depth + 1);
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
    ast_print_inner(root->operator.right, depth + 1);
    break;

  case NODE_BACKGROUND:
    printf("BACKGROUND (&)\n");
    ast_print_inner(root->operator.left, depth + 1);
    ast_print_inner(root->operator.right, depth + 1);
    break;

  case NODE_PAREN:
    printf("PARENTHESIS ( () )\n");
    ast_print_inner(root->group.inner, depth + 1);
    ast_print_inner(root->group.redirect, depth + 1);
    break;

  case NODE_BRACE:
    printf("BRACE ( {} )\n");
    ast_print_inner(root->group.inner, depth + 1);
    ast_print_inner(root->group.redirect, depth + 1);
    break;

  case NODE_REDIRECT:
    printf("REDIRECT: %s\n", root->redirection.target);
    break;

  default:
    break;
  }
}

void ast_print(AstNode *root) { ast_print_inner(root, 0); }
