#include "parser.h"
#include "vec.h"

#include <stdio.h>

void ast_free(AstNode *root) {
  if (!root) {
    return;
  }

  switch (root->type) {
  case NODE_REDIRECT:
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
    for (size_t i = 0; i < vec_size(&root->command.redirects); ++i) {
      Redirection redirect = vec_at(&root->command.redirects, i);
      free(redirect.target);
    }
    vec_free(&root->command.redirects);
    for (size_t i = 0; i < vec_size(&root->command.args); ++i) {
      free(vec_at(&root->command.args, i));
    }
    vec_free(&root->command.args);
    break;
  case NODE_PAREN:
  case NODE_BRACE:
    for (size_t i = 0; i < vec_size(&root->group.redirects); ++i) {
      Redirection redirect = vec_at(&root->group.redirects, i);
      free(redirect.target);
    }
    vec_free(&root->group.redirects);
    ast_free(root->group.inner);
    break;
  default:
    return;
  }

  free(root);
}

static void ast_print_redirects(Redirections *redirects, int depth) {
  if (!redirects || redirects->size == 0) {
    return;
  }
  for (size_t i = 0; i < redirects->size; ++i) {
    Redirection *redir = &redirects->data[i];
    printf("%*s", (depth + 1) * 2, "");

    const char *type_str;
    switch (redir->type) {
    case TOKEN_REDIRECT_IN:
      type_str = "<";
      break;
    case TOKEN_REDIRECT_OUT:
      type_str = ">";
      break;
    case TOKEN_REDIRECT_APPEND:
      type_str = ">>";
      break;
    case TOKEN_HEREDOC:
      type_str = "<<";
      break;
    default:
      type_str = "?";
    }

    printf("REDIRECT %s %s\n", type_str, redir->target);
  }
}

static void ast_print_inner(AstNode *root, int depth) {
  if (!root) {
    return;
  }
  printf("%*s", depth * 2, "");
  switch (root->type) {
  case NODE_COMMAND:
    printf("COMMAND: ");
    for (size_t i = 0; i < root->command.args.size; ++i) {
      printf("%s ", root->command.args.data[i]);
    }
    printf("\n");
    ast_print_redirects(&root->command.redirects, depth);
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
    ast_print_redirects(&root->group.redirects, depth);
    break;
  case NODE_BRACE:
    printf("BRACE ( {} )\n");
    ast_print_inner(root->group.inner, depth + 1);
    ast_print_redirects(&root->group.redirects, depth);
    break;
  default:
    break;
  }
}

void ast_print(AstNode *root) { ast_print_inner(root, 0); }
