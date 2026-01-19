#include "lexer.h"
#include "vec.h"
#include <ctype.h>

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
  case TOKEN_UNDEFINED:
    return "UNDEFINED";
  }

  __builtin_unreachable();
}

Token next_token(char *s, size_t *i) {
  while (s[*i]) {
    size_t start = *i;

    if (isspace(s[*i])) {
      ++(*i);
      continue;
    } else if (match(">>", s + *i)) {
      *i += 2;
      return (Token){
          .type = TOKEN_REDIRECT_APPEND, .s = NULL, .position = start};
    } else if (match("<<", s + *i)) {
      *i += 2;
      return (Token){.type = TOKEN_HEREDOC, .s = NULL, .position = start};
    } else if (match("&&", s + *i)) {
      *i += 2;
      return (Token){.type = TOKEN_AND, .s = NULL, .position = start};
    } else if (match("||", s + *i)) {
      *i += 2;
      return (Token){.type = TOKEN_OR, .s = NULL, .position = start};
    } else if (match(";;", s + *i)) {
      *i += 2;
      return (Token){.type = TOKEN_DBL_SEMICOLON, .s = NULL, .position = start};
    } else if (match("|", s + *i)) {
      ++(*i);
      return (Token){.type = TOKEN_PIPE, .s = NULL, .position = start};
    } else if (match("<", s + *i)) {
      ++(*i);
      return (Token){.type = TOKEN_REDIRECT_IN, .s = NULL, .position = start};
    } else if (match(">", s + *i)) {
      ++(*i);
      return (Token){.type = TOKEN_REDIRECT_OUT, .s = NULL, .position = start};
    } else if (match(";", s + *i)) {
      ++(*i);
      return (Token){.type = TOKEN_SEMICOLON, .s = NULL, .position = start};
    } else if (match("$", s + *i)) {
      ++(*i);
      return (Token){.type = TOKEN_DOLLAR, .s = NULL, .position = start};
    } else if (match("(", s + *i)) {
      ++(*i);
      return (Token){.type = TOKEN_LPAREN, .s = NULL, .position = start};
    } else if (match(")", s + *i)) {
      ++(*i);
      return (Token){.type = TOKEN_RPAREN, .s = NULL, .position = start};
    } else if (match("[", s + *i)) {
      ++(*i);
      return (Token){.type = TOKEN_LBRACKET, .s = NULL, .position = start};
    } else if (match("]", s + *i)) {
      ++(*i);
      return (Token){.type = TOKEN_RBRACKET, .s = NULL, .position = start};
    } else if (match("{", s + *i)) {
      ++(*i);
      return (Token){.type = TOKEN_LBRACE, .s = NULL, .position = start};
    } else if (match("}", s + *i)) {
      ++(*i);
      return (Token){.type = TOKEN_RBRACE, .s = NULL, .position = start};
    } else if (match("&", s + *i)) {
      ++(*i);
      return (Token){.type = TOKEN_OPERAND, .s = NULL, .position = start};
    } else {
      while (s[*i]) {
        if (isspace(s[*i]) || charset_contains("|&;[](){}<>", s[*i])) {
          break;
        }

        if (s[*i] == '\\') {
          ++(*i);
          if (s[*i]) {
            ++(*i);
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
          }
        } else {
          ++(*i);
        }
      }

      char *word = strndup(s + start, *i - start);
      return (Token){.type = TOKEN_WORD, .s = word, .position = start};
    }
  }
  return (Token){.type = TOKEN_EOF, .s = NULL, .position = *i};
}

bool tokenize(char *s, Tokens *tokens) {
  Token token;
  size_t position = 0;

  while (true) {
    token = next_token(s, &position);
    if (token.type == TOKEN_UNDEFINED)
      return false;
    if (token.type == TOKEN_EOF)
      return true;
    vec_push(tokens, token);
  }
  return true;
}
