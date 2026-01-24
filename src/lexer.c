#include "lexer.h"
#include "error.h"
#include "vec.h"

#include <ctype.h>
#include <stdio.h>

void tokens_free(Tokens *tokens) {
  for (size_t i = 0; i < vec_size(tokens); ++i) {
    Token token = vec_at(tokens, i);
    if (token.s) {
      free(token.s);
    }
  }
  vec_free(tokens);
}

const char *token_type_cstr(TokenType type) {
  static const char *names[] = {
      [TOKEN_LPAREN] = "LPAREN",
      [TOKEN_RPAREN] = "RPAREN",
      [TOKEN_LBRACE] = "LBRACE",
      [TOKEN_RBRACE] = "RBRACE",
      [TOKEN_LBRACKET] = "LBRACKET",
      [TOKEN_RBRACKET] = "RBRACKET",
      [TOKEN_PIPE] = "PIPE",
      [TOKEN_OPERAND] = "OPERAND",
      [TOKEN_SEMICOLON] = "SEMICOLON",
      [TOKEN_DBL_SEMICOLON] = "DOUBLE_SEMICOLON",
      [TOKEN_AND] = "AND",
      [TOKEN_OR] = "OR",
      [TOKEN_WORD] = "WORD",
      [TOKEN_EOF] = "EOF",
      [TOKEN_REDIRECT_OUT] = "REDIRECT_OUT",
      [TOKEN_REDIRECT_APPEND] = "REDIRECT_APPEND",
      [TOKEN_REDIRECT_IN] = "REDIRECT_IN",
      [TOKEN_HEREDOC] = "HEREDOC",
      [TOKEN_REDIRECT_FD_OUT] = "REDIRECT_FD_OUT",
      [TOKEN_REDIRECT_FD_IN] = "REDIRECT_FD_IN",
  };

  if (type < sizeof(names) / sizeof(names[0]) && names[type]) {
    return names[type];
  }

  return "UNKNOWN";
}

void tokens_print(Tokens *tokens) {
  for (size_t i = 0; i < vec_size(tokens); ++i) {
    Token token = vec_at(tokens, i);
    printf("%s: %s\n", token_type_cstr(token.type), token.s ? token.s : "");
  }
}

StatusCode next_token(char *s, size_t *i, Token *token) {
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
        *token =
            (Token){.s = NULL, .position = start, .type = operators[j].type};
        return OK;
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
          return INCOMPLETE_INPUT;
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
          return INCOMPLETE_INPUT;
        }
      } else {
        ++(*i);
      }
    }

    char *word = strndup(s + start, *i - start);
    *token = (Token){.s = word, .position = start, .type = TOKEN_WORD};
    return OK;
  }

  *token = (Token){.s = NULL, .position = *i, .type = TOKEN_EOF};
  return OK;
}

StatusCode lex(char *input, Tokens *tokens) {
  *tokens = (Tokens){0};
  size_t position = 0;

  while (true) {
    Token token;
    StatusCode err = next_token(input, &position, &token);
    if (err != OK) {
      tokens_free(tokens);
      return err;
    }

    if (token.type == TOKEN_EOF) {
      break;
    }
    vec_push(tokens, token);
  }

  return OK;
}
