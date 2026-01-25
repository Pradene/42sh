#include "ast.h"
#include "vec.h"

#include <stdlib.h>

static char *strip_quotes(const char *s) {
  if (!s || s[0] == '\0') {
    return strdup(s);
  }

  size_t length = strlen(s);
  char *result = (char *)malloc(length + 1);
  if (!result) {
    return NULL;
  }

  size_t i = 0;
  int in_single_quote = 0;
  int in_double_quote = 0;

  for (size_t j = 0; j < length; ++j) {
    char c = s[j];

    if (c == '\'' && !in_double_quote) {
      in_single_quote = !in_single_quote;
      continue;
    }

    if (c == '"' && !in_single_quote) {
      in_double_quote = !in_double_quote;
      continue;
    }

    if (c == '\\' && in_double_quote && j + 1 < length) {
      char next = s[j + 1];
      if (next == '"' || next == '\\') {
        result[i++] = next;
        ++j;
        continue;
      }
    }

    result[i++] = c;
  }

  result[i] = '\0';
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
    return;

  case NODE_BRACE:
  case NODE_PAREN:
    for (size_t i = 0; i < vec_size(&root->group.redirs); ++i) {
      Redir redir = vec_at(&root->command.redirs, i);
      char *original = redir.target;
      char *stripped = strip_quotes(original);

      if (stripped) {
        free(original);
        redir.target = stripped;
        vec_remove(&root->command.redirs, i);
        vec_insert(&root->command.redirs, i, redir);
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

    for (size_t i = 0; i < vec_size(&root->command.redirs); ++i) {
      Redir redir = vec_at(&root->command.redirs, i);
      char *original = redir.target;
      char *stripped = strip_quotes(original);

      if (stripped) {
        free(original);
        redir.target = stripped;
        vec_remove(&root->command.redirs, i);
        vec_insert(&root->command.redirs, i, redir);
      }
    }

    return;
  }
}
