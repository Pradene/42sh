#include "ast.h"
#include "vec.h"

#include <stdio.h>
#include <unistd.h>

void ast_free(AstNode *node) {
  if (!node) {
    return;
  }

  switch (node->type) {
  case NODE_AND:
  case NODE_OR:
  case NODE_SEMICOLON:
  case NODE_BACKGROUND:
  case NODE_PIPE:
    ast_free(node->operator.left);
    ast_free(node->operator.right);
    break;
  case NODE_COMMAND:
    vec_foreach(Redir, redir, &node->command.redirs) {
      if (
        redir->type == REDIRECT_IN  ||
        redir->type == REDIRECT_OUT ||
        redir->type == REDIRECT_APPEND
      ) {
        free(redir->path);
      }
    }
    vec_free(&node->command.redirs);

    vec_foreach(char *, arg, &node->command.args) {
      free(*arg);
    }
    vec_free(&node->command.args);

    vec_foreach(Assignment, arg, &node->command.assigns) {
      free(arg->name);
      free(arg->value);
    }
    vec_free(&node->command.assigns);
    break;
  case NODE_PAREN:
  case NODE_BRACE:
    vec_foreach(Redir, redir, &node->group.redirs) {
      if (
        redir->type == REDIRECT_IN ||
        redir->type == REDIRECT_OUT ||
        redir->type == REDIRECT_APPEND
      ) {
        free(redir->path);
      }
    }
    vec_free(&node->group.redirs);

    ast_free(node->group.inner);
    break;
  default:
    return;
  }

  free(node);
}

static void ast_print_redirs(Redirs *redirs, int depth) {
  if (!redirs || redirs->size == 0) {
    return;
  }

  vec_foreach(Redir, redir, redirs) {
    printf("%*s", (depth + 1) * 2, "");

    const char *type_str;
    switch (redir->type) {
    case REDIRECT_IN:
      type_str = "<";
      break;
    case REDIRECT_OUT:
      type_str = ">";
      break;
    case REDIRECT_APPEND:
      type_str = ">>";
      break;
    case REDIRECT_HEREDOC:
      type_str = "<<";
      break;
    default:
      type_str = "?";
    }

    printf("REDIRECT %s %s\n", type_str, redir->path ? redir->path : "");
  }
}

static void ast_print_inner(AstNode *node, int depth) {
  if (!node) {
    return;
  }

  printf("%*s", depth * 2, "");
  switch (node->type) {
  case NODE_COMMAND:
    printf("COMMAND: ");
    for (size_t i = 0; i < node->command.assigns.size; ++i) {
      printf("%s=%s ", node->command.assigns.data[i].name, node->command.assigns.data[i].value);
    }
    for (size_t i = 0; i < node->command.args.size; ++i) {
      printf("%s ", node->command.args.data[i]);
    }
    printf("\n");
    ast_print_redirs(&node->command.redirs, depth);
    break;
  case NODE_PIPE:
    printf("PIPE\n");
    ast_print_inner(node->operator.left, depth + 1);
    ast_print_inner(node->operator.right, depth + 1);
    break;
  case NODE_AND:
    printf("AND (&&)\n");
    ast_print_inner(node->operator.left, depth + 1);
    ast_print_inner(node->operator.right, depth + 1);
    break;
  case NODE_OR:
    printf("OR (||)\n");
    ast_print_inner(node->operator.left, depth + 1);
    ast_print_inner(node->operator.right, depth + 1);
    break;
  case NODE_SEMICOLON:
    printf("SEMICOLON (;)\n");
    ast_print_inner(node->operator.left, depth + 1);
    ast_print_inner(node->operator.right, depth + 1);
    break;
  case NODE_BACKGROUND:
    printf("BACKGROUND (&)\n");
    ast_print_inner(node->operator.left, depth + 1);
    ast_print_inner(node->operator.right, depth + 1);
    break;
  case NODE_PAREN:
    printf("PARENTHESIS ( () )\n");
    ast_print_inner(node->group.inner, depth + 1);
    ast_print_redirs(&node->group.redirs, depth);
    break;
  case NODE_BRACE:
    printf("BRACE ( {} )\n");
    ast_print_inner(node->group.inner, depth + 1);
    ast_print_redirs(&node->group.redirs, depth);
    break;
  default:
    break;
  }
}

void ast_print(AstNode *node) { ast_print_inner(node, 0); }
