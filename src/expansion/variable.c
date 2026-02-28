#include "shell.h"
#include "ast.h"
#include "env.h"
#include "vec.h"

#include <ctype.h>

static char *expand(const char *s, const Shell *shell) {
  StringBuffer sb = {0};

  if (!s || s[0] == '\0') {
    return strdup(s);
  }

  size_t i = 0;
  while (s[i]) {
    if (s[i] == '$') {
      size_t start = i;
      ++i;
      if (s[i] == '`') {
        ++i;
        size_t v_start = i;
        while (s[i] && s[i] != '`') {
          ++i;
        }
        if (s[i] == '`') {
          size_t v_length = i - v_start;
          char *v_name = strndup(s + v_start, v_length);
          if (v_name) {
            const char *v_value = env_find(&shell->environment, v_name);
            if (v_value) {
              sb_append(&sb, v_value);
            }
            free(v_name);
          }
          ++i;
        } else {
          sb_append(&sb, s + start);
        }
      } else if (isalpha(s[i]) || s[i] == '_') {
        size_t v_start = i;
        while (isalnum(s[i]) || s[i] == '_') {
          ++i;
        }

        size_t v_length = i - v_start;
        char *v_name = strndup(s + v_start, v_length);
        if (v_name) {
          const char *v_value = env_find(&shell->environment, v_name);
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

void expansion(AstNode *root, const Shell *shell) {
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
    vec_foreach(Redir, redir, &root->group.redirs) {
      if (redir->type == REDIRECT_HEREDOC || redir->type == REDIRECT_OUT_FD || redir->type == REDIRECT_IN_FD) {
        continue;
      }
      char *expanded = expand(redir->path, shell);
      if (expanded) {
        free(redir->path);
        redir->path = expanded;
      }
    }

    return;

  case NODE_COMMAND:
    vec_foreach(char *, arg, &root->command.args) {
      char *expanded = expand(*arg, shell);
      if (expanded) {
        free(*arg);
        *arg = expanded;
      }
    }

    vec_foreach(Redir, redir, &root->command.redirs) {
      if (redir->type == REDIRECT_HEREDOC || redir->type == REDIRECT_OUT_FD || redir->type == REDIRECT_IN_FD) {
        continue;
      }
      char *expanded = expand(redir->path, shell);
      if (expanded) {
        free(redir->path);
        redir->path = expanded;
      }
    }

    return;
  }
}