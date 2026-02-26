#include "42sh.h"
#include "ast.h"
#include "vec.h"

#include <ctype.h>

void splitting(AstNode *root) {
  if (!root) {
    return;
  }

  switch (root->type) {
  case NODE_PIPE:
  case NODE_AND:
  case NODE_OR:
  case NODE_BACKGROUND:
  case NODE_SEMICOLON:
  case NODE_BRACE:
  case NODE_PAREN:
    return;

  case NODE_COMMAND:
    if (root->command.args.size == 0) {
      return;
    }

    Arguments args = {0};

    for (size_t j = 0; j < root->command.args.size; j++) {
      char *arg = root->command.args.data[j];
      if (!arg) {
        continue;
      }

      char *s = arg;
      size_t i = 0;
      char quote = 0;
      StringBuffer sb = {0};

      while (s[i]) {
        if (s[i] == '\"' || s[i] == '\'') {
          if (quote == 0) {
            quote = s[i];
          } else if (quote == s[i]) {
            quote = 0;
          }
          ++i;
        } else if (isspace((unsigned char)s[i]) && quote == 0) {
          if (sb.size > 0) {
            char *word = sb_as_cstr(&sb);
            vec_push(&args, word);
            sb = (StringBuffer){0};
          }
          while (isspace((unsigned char)s[i])) {
            ++i;
          }
        } else {
          sb_append_char(&sb, s[i]);
          ++i;
        }
      }

      if (sb.size > 0) {
        char *word = sb_as_cstr(&sb);
        vec_push(&args, word);
      }

      free(arg);
    }

    vec_free(&root->command.args);
    root->command.args = args;
  }
}