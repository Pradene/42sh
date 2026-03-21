#include "42sh.h"
#include "ast.h"
#include "env.h"
#include "sb.h"
#include "vec.h"

#include <ctype.h>
#include <stdio.h>

static char *expand_variable(const char *s);

static void expand_tilde(StringBuffer *sb) {
  const char *home = env_find(environ, "HOME");
  if (home) {
    sb_append(sb, home);
  } else {
    sb_append_char(sb, '~');
  }
}

static void expand_simple_variable(StringBuffer *sb, const char *s, size_t *i) {
  size_t start = *i;
  while (isalnum((unsigned char)s[*i]) || s[*i] == '_') {
    ++(*i);
  }

  char *name = strndup(s + start, *i - start);
  const char *value = env_find(environ, name);
  if (value) {
    sb_append(sb, value);
  }
  free(name);
}

static void expand_status(StringBuffer *sb, size_t *i) {
  char buf[4];
  snprintf(buf, sizeof(buf), "%d", exit_status);
  sb_append(sb, buf);
  ++(*i);
}

static char *collect_name_until_backtick(const char *s, size_t *i) {
  size_t start = *i;
  while (s[*i] && s[*i] != '`') {
    ++(*i);
  }

  char *name = strndup(s + start, *i - start);
  if (s[*i] == '`') {
    ++(*i);
  }

  return name;
}

static void expand_length(StringBuffer *sb, const char *name) {
  char buf[32];
  const char *value = env_find(environ, name);
  snprintf(buf, sizeof(buf), "%zu", value ? strlen(value) : 0);
  sb_append(sb, buf);
}

static void expand_remove_suffix(StringBuffer *sb, const char *value, const char *pattern, bool greedy) {
  char *result = strdup(value);
  size_t len = strlen(result);
  size_t plen = strlen(pattern);

  if (greedy) {
    for (size_t k = 0; k <= len; ++k) {
      if (strncmp(result + k, pattern, plen) == 0 && k + plen == len) {
        result[k] = '\0';
        break;
      }
    }
  } else {
    if (len >= plen && strcmp(result + len - plen, pattern) == 0) {
      result[len - plen] = '\0';
    }
  }

  sb_append(sb, result);
  free(result);
}

static void expand_remove_prefix(StringBuffer *sb, const char *value, const char *pattern) {
  size_t plen = strlen(pattern);
  if (strncmp(value, pattern, plen) == 0) {
    sb_append(sb, value + plen);
  } else {
    sb_append(sb, value);
  }
}

static void expand_default_op(StringBuffer *sb, const char *name, char op, bool colon, const char *word) {
  const char *value = env_find(environ, name);
  bool is_unset = (value == NULL);
  bool is_empty = (value != NULL && value[0] == '\0');
  bool condition = colon ? (is_unset || is_empty) : is_unset;

  switch (op) {
    case '-':
      if (condition) {
        char *expanded = expand_variable(word);
        sb_append(sb, expanded);
        free(expanded);
      } else {
        sb_append(sb, value);
      }
      break;
    case '=':
      if (condition) {
        char *expanded = expand_variable(word);
        Variable v = { .content = strdup(expanded), .exported = false, .readonly = false };
        ht_insert(environ, name, &v);
        sb_append(sb, expanded);
        free(expanded);
      } else {
        sb_append(sb, value);
      }
      break;
    case '?':
      if (condition) {
        char *expanded = expand_variable(word);
        fprintf(stderr, "%s: %s: %s\n", program_name, name, expanded[0] ? expanded : "parameter null or not set");
        free(expanded);
      } else {
        sb_append(sb, value);
      }
      break;
    case '+':
      if (!condition) {
        char *expanded = expand_variable(word);
        sb_append(sb, expanded);
        free(expanded);
      }
      break;
  }
}

// $`name`         -> simple variable
// $`#name`        -> length
// $`name%pat`     -> remove shortest suffix
// $`name%%pat`    -> remove longest suffix
// $`name#pat`     -> remove prefix
// $`name:-word`   -> default value
// $`name:=word`   -> assign default
// $`name:?word`   -> error if unset
// $`name:+word`   -> alternate value
static void expand_backtick(StringBuffer *sb, const char *s, size_t *i) {
  ++(*i); // skip '`'

  // $`#name` — length
  if (s[*i] == '#') {
    ++(*i);
    char *name = collect_name_until_backtick(s, i);
    expand_length(sb, name);
    free(name);
    return;
  }

  // collect name
  size_t name_start = *i;
  while (s[*i] && s[*i] != '`' && s[*i] != ':' && s[*i] != '%' && s[*i] != '#') {
    ++(*i);
  }

  char *name = strndup(s + name_start, *i - name_start);

  if (s[*i] == '`') {
    // $`name` — plain
    ++(*i);
    if (!strcmp(name, "?")) {
      char buf[4];
      snprintf(buf, sizeof(buf), "%d", exit_status);
      sb_append(sb, buf);
    } else {
      const char *value = env_find(environ, name);
      if (value) {
        sb_append(sb, value);
      }
    }
    free(name);

  } else if (s[*i] == '%') {
    // $`name%pat` or $`name%%pat`
    ++(*i);
    bool greedy = (s[*i] == '%');
    if (greedy) {
      ++(*i);
    }

    char *pattern = collect_name_until_backtick(s, i);
    const char *value = env_find(environ, name);
    if (value) {
      expand_remove_suffix(sb, value, pattern, greedy);
    }
    
    free(pattern);
    free(name);
  
  } else if (s[*i] == '#') {
    // $`name#pat`
    ++(*i);
    char *pattern = collect_name_until_backtick(s, i);
    const char *value = env_find(environ, name);
    if (value) {
      expand_remove_prefix(sb, value, pattern);
    }
    free(pattern);
    free(name);
    
  } else if (s[*i] == ':') {
    // $`name:-word`, $`name:=word`, $`name:?word`, $`name:+word`
    ++(*i);
    bool colon = true;
    char op = s[(*i)++];
    char *word = collect_name_until_backtick(s, i);
    expand_default_op(sb, name, op, colon, word);
    free(word);
    free(name);

  } else {
    // fallback — just print as-is
    sb_append_char(sb, '$');
    sb_append_char(sb, '`');
    sb_append(sb, name);
    free(name);
  }
}

static void expand_dollar(StringBuffer *sb, const char *s, size_t *i) {
  ++(*i); // skip '$'

  if (s[*i] == '`') {
    expand_backtick(sb, s, i);

  } else if (isalpha((unsigned char)s[*i]) || s[*i] == '_') {
    expand_simple_variable(sb, s, i);

  } else if (s[*i] == '?') {
    expand_status(sb, i);

  } else {
    sb_append_char(sb, '$');
  }
}

static char *expand_variable(const char *s) {
  if (!s || s[0] == '\0') {
    return strdup(s ? s : "");
  }

  StringBuffer sb = {0};
  size_t i = 0;

  while (s[i]) {
    if (s[i] == '~' && i == 0)  {
      expand_tilde(&sb);
      ++i;
    } else if (s[i] == '$') {
      expand_dollar(&sb, s, &i);
    } else {
      sb_append_char(&sb, s[i++]);
    }
  }

  return sb_as_cstr(&sb);
}

void variable_expansion(AstNode *node) {
  if (!node) {
    return;
  }

  switch (node->type) {
  case NODE_PIPE:
  case NODE_AND:
  case NODE_OR:
  case NODE_BACKGROUND:
  case NODE_SEMICOLON:
    return;

  case NODE_BRACE:
  case NODE_PAREN:
    vec_foreach(Redir, redir, &node->group.redirs) {
      if (redir->type == REDIRECT_HEREDOC || redir->type == REDIRECT_OUT_FD || redir->type == REDIRECT_IN_FD) {
        continue;
      }
      char *expanded = expand_variable(redir->path);
      if (expanded) {
        free(redir->path);
        redir->path = expanded;
      }
    }

    return;

  case NODE_COMMAND:
    vec_foreach(char *, arg, &node->command.args) {
      char *expanded = expand_variable(*arg);
      if (expanded) {
        free(*arg);
        *arg = expanded;
      }
    }

    vec_foreach(Redir, redir, &node->command.redirs) {
      if (redir->type == REDIRECT_HEREDOC || redir->type == REDIRECT_OUT_FD || redir->type == REDIRECT_IN_FD) {
        continue;
      }
      char *expanded = expand_variable(redir->path);
      if (expanded) {
        free(redir->path);
        redir->path = expanded;
      }
    }

    return;
  }
}
