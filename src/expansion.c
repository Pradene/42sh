#include "ast.h"
#include "env.h"
#include "sb.h"
#include "vec.h"
#include "42sh.h"

#include <ctype.h>
#include <stdio.h>

static const char *get_variable(const Shell *shell, const char *name) {
  char *v;

  v = env_find(&shell->environment, name);
  if (v) {
    return v;
  }

  return "";
}

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
            const char *v_value = get_variable(shell, v_name);
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
          const char *v_value = get_variable(shell, v_name);
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
      if (redir->type == REDIRECT_HEREDOC) {
        continue;
      }
      char *expanded = expand(redir->target_path, shell);
      if (expanded) {
        free(redir->target_path);
        redir->target_path = expanded;
      }
    }

    return;

  case NODE_COMMAND:
    if (root->command.args.size != 0) {
      HashEntry *entry = ht_get(&shell->aliases, root->command.args.data[0]);
      if (entry) {
        char *alias_value = entry->value;
        Arguments args = {0};
        vec_push(&args, alias_value);
        for (size_t i = 1; i < root->command.args.size; ++i) {
          vec_push(&args, root->command.args.data[i]);
        }
        free(root->command.args.data);
        root->command.args.data = args.data;
        root->command.args.size = args.size;
        root->command.args.capacity = args.capacity;
      }
    }

    vec_foreach(char *, arg, &root->command.args) {
      char *expanded = expand(*arg, shell);
      if (expanded) {
        free(*arg);
        *arg = expanded;
      }
    }

    vec_foreach(Redir, redir, &root->command.redirs) {
      if (redir->type == REDIRECT_HEREDOC || redir->type == REDIRECT_OUT_FD ||
          redir->type == REDIRECT_IN_FD) {
        continue;
      }
      char *expanded = expand(redir->target_path, shell);
      if (expanded) {
        free(redir->target_path);
        redir->target_path = expanded;
      }
    }

    return;
  }
}
