#include "ast.h"
#include "env.h"
#include "sb.h"
#include "vec.h"

#include <ctype.h>

static char *expand(const char *s, Environment *env) {
  StringBuffer sb = {0};

  if (!s || s[0] == '\0') {
    return strdup(s);
  }

  size_t i = 0;
  while (s[i]) {
    if (s[i] == '$') {
      ++i;
      if (s[i] == '{') {
        ++i;
        size_t depth = 1;
        size_t start = i;
        while (s[i]) {
          if (s[i] == '{') {
            ++depth;
          } else if (s[i] == '}') {
            --depth;
          }

          if (depth == 0) {
            break;
          } else {
            ++i;
          }
        }

        if (s[i] == '}') {
          size_t v_length = i - start;
          char *v_name = strndup(s + start, v_length);
          if (v_name) {
            const char *v_value = env_find(env, v_name);
            if (v_value) {
              sb_append(&sb, v_value);
            }
            free(v_name);
          }
          ++i;
        } else {
          sb_append_char(&sb, '$');
          sb_append_char(&sb, '{');
          sb_append(&sb, s + start);
        }
      } else if (isalnum(s[i]) || s[i] == '_') {
        size_t start = i;
        while (isalnum(s[i]) || s[i] == '_') {
          ++i;
        }

        size_t v_length = i - start;
        char *v_name = strndup(s + start, v_length);
        if (v_name) {
          const char *v_value = env_find(env, v_name);
          if (v_value) {
            sb_append(&sb, v_value);
          }
          free(v_name);
        }
      } else {
        sb_append_char(&sb, '$');
      }
    } else {
      sb_append_char(&sb, s[i]);
      ++i;
    }
  }

  return sb_as_cstr(&sb);
}

void expansion(AstNode *root, Environment *env) {
  (void)env;
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
    for (size_t i = 0; i < vec_size(&root->command.redirs); ++i) {
      Redir redir = vec_at(&root->command.redirs, i);
      char *original = redir.target;
      char *expanded = expand(original, env);

      if (expanded) {
        free(original);
        redir.target = expanded;
        vec_remove(&root->command.redirs, i);
        vec_insert(&root->command.redirs, i, redir);
      }
    }

    return;

  case NODE_COMMAND:
    for (size_t i = 0; i < vec_size(&root->command.args); ++i) {
      char *original = vec_at(&root->command.args, i);
      char *expanded = expand(original, env);

      if (expanded) {
        free(original);
        vec_remove(&root->command.args, i);
        vec_insert(&root->command.args, i, expanded);
      }
    }

    for (size_t i = 0; i < vec_size(&root->command.redirs); ++i) {
      Redir redir = vec_at(&root->command.redirs, i);
      char *original = redir.target;
      char *expanded = expand(original, env);

      if (expanded) {
        free(original);
        redir.target = expanded;
        vec_remove(&root->command.redirs, i);
        vec_insert(&root->command.redirs, i, redir);
      }
    }

    return;
  }
}
