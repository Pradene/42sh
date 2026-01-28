#pragma once

#include "status.h"

#include <stdbool.h>
#include <stdlib.h>

typedef struct {
  const char *input;
  size_t position;
} LexState;

typedef enum {
  TOKEN_LPAREN,
  TOKEN_RPAREN,
  TOKEN_LBRACE,
  TOKEN_RBRACE,
  TOKEN_PIPE,
  TOKEN_OPERAND,
  TOKEN_SEMICOLON,
  TOKEN_DBL_SEMICOLON,
  TOKEN_LBRACKET,
  TOKEN_RBRACKET,
  TOKEN_AND,
  TOKEN_OR,
  TOKEN_WORD,
  TOKEN_REDIRECT_OUT,
  TOKEN_REDIRECT_APPEND,
  TOKEN_REDIRECT_IN,
  TOKEN_HEREDOC,
  TOKEN_NEWLINE,
  TOKEN_EOF,
} TokenType;

typedef struct {
  char *s;
  size_t position;
  TokenType type;
} Token;

StatusCode next_token(LexState *state, Token *token);
