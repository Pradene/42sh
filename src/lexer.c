#include "lexer.h"
#include "sb.h"
#include "status.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

const char *token_type_str(const TokenType type) {
  static const char *names[] = {
    [TOKEN_ERROR] = "ERROR",
    [TOKEN_LPAREN] = "LPAREN",
    [TOKEN_RPAREN] = "RPAREN",
    [TOKEN_LBRACE] = "LBRACE",
    [TOKEN_RBRACE] = "RBRACE",
    [TOKEN_LBRACKET] = "LBRACKET",
    [TOKEN_RBRACKET] = "RBRACKET",
    [TOKEN_PIPE] = "PIPE",
    [TOKEN_OPERAND] = "OPERAND",
    [TOKEN_SEMICOLON] = "SEMICOLON",
    [TOKEN_AND] = "AND",
    [TOKEN_OR] = "OR",
    [TOKEN_SUBSTITUTION] = "SUBSTITUTION",
    [TOKEN_WORD] = "WORD",
    [TOKEN_REDIRECT_OUT] = "REDIRECT_OUT",
    [TOKEN_REDIRECT_APPEND] = "REDIRECT_APPEND",
    [TOKEN_REDIRECT_IN] = "REDIRECT_IN",
    [TOKEN_HEREDOC] = "HEREDOC",
    [TOKEN_REDIRECT_IN_FD] = "REDIRECT_IN_FD",
    [TOKEN_REDIRECT_OUT_FD] = "REDIRECT_OUT_FD",
    [TOKEN_NEWLINE] = "NEWLINE",
    [TOKEN_EOF] = "EOF",
  };

  if (type < sizeof(names) / sizeof(names[0]) && names[type]) {
    return names[type];
  }

  return "UNKNOWN";
}

static inline bool is_delimiter(char c) {
  return isspace(c) || strchr("|&;[](){}<>", c) != NULL;
}

Token next_token(const char *s, size_t *i) {
  static const struct {
    const char *s;
    size_t length;
    TokenType type;
  } operators[] = {
    {">>", 2, TOKEN_REDIRECT_APPEND},
    {"<<", 2, TOKEN_HEREDOC},
    {">&", 2, TOKEN_REDIRECT_OUT_FD},
    {"<&", 2, TOKEN_REDIRECT_IN_FD},
    {"&&", 2, TOKEN_AND},
    {"||", 2, TOKEN_OR},
    {"$(", 2, TOKEN_SUBSTITUTION},
    {"|", 1, TOKEN_PIPE},
    {"<", 1, TOKEN_REDIRECT_IN},
    {">", 1, TOKEN_REDIRECT_OUT},
    {";", 1, TOKEN_SEMICOLON},
    {"(", 1, TOKEN_LPAREN},
    {")", 1, TOKEN_RPAREN},
    {"[", 1, TOKEN_LBRACKET},
    {"]", 1, TOKEN_RBRACKET},
    {"{", 1, TOKEN_LBRACE},
    {"}", 1, TOKEN_RBRACE},
    {"&", 1, TOKEN_OPERAND},
    {"\n", 1, TOKEN_NEWLINE},
    {NULL, 0, 0}
  };

  while (s[*i]) {
    size_t start = *i;

    for (size_t j = 0; operators[j].s != NULL; j++) {
      if (!strncmp(operators[j].s, s + *i, operators[j].length)) {
        *i += operators[j].length;
        return (Token){operators[j].type, *i, NULL};
      }
    }

    if (isspace(s[*i])) {
      ++(*i);
      continue;
    }

    StringBuffer sb = {0};
    while (s[*i]) {
      if (is_delimiter(s[*i])) {
        break;
      }

      if (s[*i] == '\\') {
        ++(*i);
        if (s[*i]) {
          if (s[*i] != '\n') {
            sb_append_char(&sb, s[*i]);
          }
          ++(*i);
        } else {
          sb_free(&sb);
          return (Token){TOKEN_ERROR, *i, NULL};
        }
      } else if (s[*i] == '$') {
        sb_append_char(&sb, s[*i]);
        ++(*i);
        if (s[*i] == '`') {
          sb_append_char(&sb, s[*i]);
          ++(*i);
          while (s[*i] && s[*i] != '`') {
            sb_append_char(&sb, s[*i]);
            ++(*i);
          }
          if (s[*i] == '`') {
            sb_append_char(&sb, s[*i]);
            ++(*i);
          } else {
            sb_free(&sb);
            return (Token){TOKEN_ERROR, *i, NULL};
          }
        } else {
          while (s[*i] && !is_delimiter(s[*i])) {
            sb_append_char(&sb, s[*i]);
            ++(*i);
          }
        }
      } else if (s[*i] == '\"' || s[*i] == '\'') {
        char quote = s[*i];
        sb_append_char(&sb, s[*i]);
        ++(*i);
        while (s[*i] && s[*i] != quote) {
          if (quote == '\"' && s[*i] == '\\') {
            sb_append_char(&sb, s[*i]);
            ++(*i);
            if (s[*i]) {
              sb_append_char(&sb, s[*i]);
              ++(*i);
            }
          } else {
            sb_append_char(&sb, s[*i]);
            ++(*i);
          }
        }
        if (s[*i] == quote) {
          sb_append_char(&sb, s[*i]);
          ++(*i);
        } else {
          sb_free(&sb);
          return (Token){TOKEN_ERROR, *i, NULL};
        }
      } else {
        sb_append_char(&sb, s[*i]);
        ++(*i);
      }
    }

    char *word = sb_as_cstr(&sb);
    return (Token){TOKEN_WORD, start, word};
  }

  return (Token){TOKEN_EOF, *i, NULL};
}
