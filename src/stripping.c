#include "parser.h"
#include "vec.h"

static char *strip_quotes(const char *s) {
  if (!s || s[0] == '\0') {
    return strdup(s);
  }

  size_t length = strlen(s);
  char *result = (char *)malloc(length + 1);
  if (!result) {
    return NULL;
  }

  size_t out_idx = 0;
  int in_single_quote = 0;
  int in_double_quote = 0;

  for (size_t i = 0; i < length; ++i) {
    char c = s[i];

    if (c == '\'' && !in_double_quote) {
      in_single_quote = !in_single_quote;
      continue;
    }

    if (c == '"' && !in_single_quote) {
      in_double_quote = !in_double_quote;
      continue;
    }

    if (c == '\\' && in_double_quote && i + 1 < length) {
      char next = s[i + 1];
      if (next == '"' || next == '\\') {
        result[out_idx++] = next;
        ++i;
        continue;
      }
    }

    result[out_idx++] = c;
  }

  result[out_idx] = '\0';
  return result;
}

void stripping(AstNode *root) {
  if (!root) {
    return;
  }

  switch (root->type) {
  case NODE_PIPE:
  case NODE_AND:
  case NODE_OR:
  case NODE_BACKGROUND:
  case NODE_SEMICOLON:
    stripping(root->operator.left);
    stripping(root->operator.right);
    return;

  case NODE_BRACE:
  case NODE_PAREN:
    stripping(root->group.inner);

    for (size_t i = 0; i < vec_size(&root->command.redirects); ++i) {
      Redirection redirect = vec_at(&root->command.redirects, i);
      char *original = redirect.target;
      char *stripped = strip_quotes(original);

      if (stripped) {
        free(original);
        redirect.target = stripped;
        vec_remove(&root->command.redirects, i);
        vec_insert(&root->command.redirects, i, redirect);
      }
    }

    return;

  case NODE_COMMAND:
    for (size_t i = 0; i < vec_size(&root->command.args); ++i) {
      char *original = vec_at(&root->command.args, i);
      char *stripped = strip_quotes(original);

      if (stripped) {
        free(original);
        vec_remove(&root->command.args, i);
        vec_insert(&root->command.args, i, stripped);
      }
    }

    for (size_t i = 0; i < vec_size(&root->command.redirects); ++i) {
      Redirection redirect = vec_at(&root->command.redirects, i);
      char *original = redirect.target;
      char *stripped = strip_quotes(original);

      if (stripped) {
        free(original);
        redirect.target = stripped;
        vec_remove(&root->command.redirects, i);
        vec_insert(&root->command.redirects, i, redirect);
      }
    }

    return;
  }
}
