#include "parser.h"
#include "sb.h"
#include "vec.h"

static char *expand(const char *s) {
  StringBuffer sb = {0};

  if (!s || s[0] == '\0') {
    return strdup(s);
  }

  size_t i = 0;
  while (s[i]) {
    if (s[i] == '$') {
      ++i;
    } else {
      sb_append_char(&sb, s[i]);
      ++i;
    }
  }

  return sb_as_cstr(&sb);
}

void expansion(AstNode *root) {
  if (!root) {
    return;
  }

  switch (root->type) {
  case NODE_PIPE:
  case NODE_AND:
  case NODE_OR:
  case NODE_BACKGROUND:
  case NODE_SEMICOLON:
    expansion(root->operator.left);
    expansion(root->operator.right);
    return;

  case NODE_BRACE:
  case NODE_PAREN:
    expansion(root->group.inner);

    for (size_t i = 0; i < vec_size(&root->command.redirects); ++i) {
      Redirection redirect = vec_at(&root->command.redirects, i);
      char *original = redirect.target;
      char *expanded = expand(original);

      if (expanded) {
        free(original);
        redirect.target = expanded;
        vec_remove(&root->command.redirects, i);
        vec_insert(&root->command.redirects, i, redirect);
      }
    }

    return;

  case NODE_COMMAND:
    for (size_t i = 0; i < vec_size(&root->command.args); ++i) {
      char *original = vec_at(&root->command.args, i);
      char *expanded = expand(original);

      if (expanded) {
        free(original);
        vec_remove(&root->command.args, i);
        vec_insert(&root->command.args, i, expanded);
      }
    }

    for (size_t i = 0; i < vec_size(&root->command.redirects); ++i) {
      Redirection redirect = vec_at(&root->command.redirects, i);
      char *original = redirect.target;
      char *expanded = expand(original);

      if (expanded) {
        free(original);
        redirect.target = expanded;
        vec_remove(&root->command.redirects, i);
        vec_insert(&root->command.redirects, i, redirect);
      }
    }

    return;
  }
}
