#include "lexer.h"
#include "error.h"
#include "vec.h"

#include <ctype.h>

void tokens_free(Tokens *tokens) {
  for (size_t i = 0; i < vec_size(tokens); ++i) {
    Token token = vec_at(tokens, i);
    if (token.s) {
      free(token.s);
    }
  }
  vec_free(tokens);
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
  }

  __builtin_unreachable();
}

TokenResult next_token(char *s, size_t *i) {
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
                   {NULL, 0, 0}};

  while (s[*i]) {
    size_t start = *i;

    if (isspace(s[*i])) {
      ++(*i);
      continue;
    }

    for (size_t j = 0; operators[j].s != NULL; j++) {
      if (!strncmp(operators[j].s, s + *i, operators[j].length)) {
        *i += operators[j].length;
        return (TokenResult){.is_ok = true,
                             .ok = {NULL, start, operators[j].type}};
      }
    }

    while (s[*i]) {
      if (isspace(s[*i]) || strchr("|&;[](){}<>", s[*i])) {
        break;
      }

      if (s[*i] == '\\') {
        ++(*i);
        if (s[*i]) {
          ++(*i);
        } else {
          return (TokenResult){.is_ok = false, .err = INCOMPLETE_INPUT};
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
          return (TokenResult){.is_ok = false, .err = INCOMPLETE_INPUT};
        }
      } else {
        ++(*i);
      }
    }

    char *word = strndup(s + start, *i - start);
    return (TokenResult){.is_ok = true, .ok = {word, start, TOKEN_WORD}};
  }

  return (TokenResult){.is_ok = true, .ok = {NULL, *i, TOKEN_EOF}};
}

LexResult lex(char *s) {
  Tokens tokens = {0};
  size_t position = 0;

  while (true) {
    TokenResult result = next_token(s, &position);
    if (!result.is_ok) {
      return (LexResult){.is_ok = false, .err = result.err};
    }

    Token token = result.ok;
    if (token.type == TOKEN_EOF) {
      break;
    }
    vec_push(&tokens, token);
  }

  return (LexResult){.is_ok = true, .ok = tokens};
}
