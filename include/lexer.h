#pragma once

#include "vec.h"
#include <stdbool.h>
#include <stdlib.h>

typedef enum {
  TOKEN_UNDEFINED,
  TOKEN_LPAREN,
  TOKEN_RPAREN,
  TOKEN_LBRACE,
  TOKEN_RBRACE,
  TOKEN_DOLLAR,
  TOKEN_PIPE,
  TOKEN_OPERAND,
  TOKEN_SEMICOLON,
  TOKEN_DBL_SEMICOLON,
  TOKEN_LBRACKET,
  TOKEN_RBRACKET,
  TOKEN_AND,
  TOKEN_OR,
  TOKEN_WORD,
  TOKEN_EOF,
  TOKEN_REDIRECT_OUT,
  TOKEN_REDIRECT_APPEND,
  TOKEN_HEREDOC,
  TOKEN_REDIRECT_IN,
} TokenType;

typedef struct {
  TokenType type;
  char *s;
  size_t position;
} Token;

typedef VEC(Token) Tokens;

const char *token_type_str(TokenType type);
bool tokenize(char *s, Tokens *tokens);
