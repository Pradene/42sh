#include "lexer.h"
#include "vec.h"

#include <ctype.h>
#include <string.h>

static bool charset_contains(const char *set, char c) { return strchr(set, c); }

static bool match(const char *s1, const char *s2) {
  return !strncmp(s1, s2, strlen(s1));
}

const char *token_type_str(TokenType type) {
  switch (type) {
  case TOKEN_EOF:
    return "EOF";
  case TOKEN_WORD:
    return "WORD";
  case TOKEN_AND:
    return "AND";
  case TOKEN_OR:
    return "OR";
  case TOKEN_PIPE:
    return "PIPE";
  case TOKEN_SEMICOLON:
    return "SEMICOLON";
  case TOKEN_OPERAND:
    return "OPERAND";
  case TOKEN_DOLLAR:
    return "DOLLAR";
  case TOKEN_LPAREN:
    return "LPAREN";
  case TOKEN_RPAREN:
    return "RPAREN";
  case TOKEN_LBRACKET:
    return "LBRACKET";
  case TOKEN_RBRACKET:
    return "RBRACKET";
  case TOKEN_LBRACE:
    return "LBRACE";
  case TOKEN_RBRACE:
    return "RBRACE";
  case TOKEN_DBL_SEMICOLON:
    return "DOUBLE_SEMICOLON";
  case TOKEN_REDIRECT_OUT:
    return "REDIRECT_OUT";
  case TOKEN_REDIRECT_APPEND:
    return "REDIRECT_APPEND";
  case TOKEN_HEREDOC:
    return "HEREDOC";
  case TOKEN_REDIRECT_IN:
    return "REDIRECT_IN";
  case TOKEN_REDIRECT_FD_IN:
    return "REDIRECT_FD_IN";
  case TOKEN_REDIRECT_FD_OUT:
    return "REDIRECT_FD_OUT";
  case TOKEN_UNDEFINED:
    return "UNDEFINED";
  }

  __builtin_unreachable();
}

Token next_token(char *s, size_t *i) {
  static const struct {
    const char *s;
    size_t length;
    TokenType type;
  } operators[] = {{">>", 2, TOKEN_REDIRECT_APPEND},
                   {"<<", 2, TOKEN_HEREDOC},
                   {"<&", 2, TOKEN_REDIRECT_FD_IN},
                   {">&", 2, TOKEN_REDIRECT_FD_OUT},
                   {"&&", 2, TOKEN_AND},
                   {"||", 2, TOKEN_OR},
                   {";;", 2, TOKEN_DBL_SEMICOLON},
                   {"|", 1, TOKEN_PIPE},
                   {"<", 1, TOKEN_REDIRECT_IN},
                   {">", 1, TOKEN_REDIRECT_OUT},
                   {";", 1, TOKEN_SEMICOLON},
                   {"$", 1, TOKEN_DOLLAR},
                   {"(", 1, TOKEN_LPAREN},
                   {")", 1, TOKEN_RPAREN},
                   {"[", 1, TOKEN_LBRACKET},
                   {"]", 1, TOKEN_RBRACKET},
                   {"{", 1, TOKEN_LBRACE},
                   {"}", 1, TOKEN_RBRACE},
                   {"&", 1, TOKEN_OPERAND},
                   {NULL, 0, TOKEN_UNDEFINED}};

  while (s[*i]) {
    size_t start = *i;

    if (isspace(s[*i])) {
      ++(*i);
      continue;
    }

    for (size_t j = 0; operators[j].s != NULL; j++) {
      if (match(operators[j].s, s + *i)) {
        *i += operators[j].length;
        return (Token){NULL, start, operators[j].type};
      }
    }

    while (s[*i]) {
      if (isspace(s[*i]) || charset_contains("|&;[](){}<>", s[*i])) {
        break;
      }

      if (s[*i] == '\\') {
        ++(*i);
        if (s[*i]) {
          ++(*i);
        } else {
          return (Token){NULL, start, TOKEN_UNDEFINED};
        }
      } else if (s[*i] == '\"' || s[*i] == '\'') {
        char quote = s[(*i)++];
        while (s[*i] && s[*i] != quote) {
          if (quote == '\"' && s[*i] == '\\') {
            ++(*i);
            if (s[*i]) {
              ++(*i);
            }
          } else {
            ++(*i);
          }
        }
        if (s[*i] == quote) {
          ++(*i);
        } else {
          return (Token){NULL, start, TOKEN_UNDEFINED};
        }
      } else {
        ++(*i);
      }
    }

    char *word = strndup(s + start, *i - start);
    return (Token){word, start, TOKEN_WORD};
  }

  return (Token){NULL, *i, TOKEN_EOF};
}

bool tokenize(char *s, Tokens *tokens) {
  Token token;
  size_t position = 0;

  while (true) {
    token = next_token(s, &position);
    if (token.type == TOKEN_UNDEFINED) {
      return false;
    } else if (token.type == TOKEN_EOF) {
      break;
    }
    vec_push(tokens, token);
  }
  return true;
}
