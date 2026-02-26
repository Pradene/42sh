#pragma once

#include "status.h"

#include <stdbool.h>
#include <stdlib.h>

typedef enum {
  TOKEN_ERROR,
  TOKEN_LPAREN,
  TOKEN_RPAREN,
  TOKEN_LBRACE,
  TOKEN_RBRACE,
  TOKEN_PIPE,
  TOKEN_OPERAND,
  TOKEN_SEMICOLON,
  TOKEN_LBRACKET,
  TOKEN_RBRACKET,
  TOKEN_AND,
  TOKEN_OR,
  TOKEN_SUBSTITUTION,
  TOKEN_WORD,
  TOKEN_REDIRECT_OUT,
  TOKEN_REDIRECT_APPEND,
  TOKEN_REDIRECT_IN,
  TOKEN_HEREDOC,
  TOKEN_REDIRECT_IN_FD,
  TOKEN_REDIRECT_OUT_FD,
  TOKEN_NEWLINE,
  TOKEN_EOF,
} TokenType;

typedef struct {
  TokenType type;
  size_t    position;
  char      *s;
} Token;

const char *token_type_str(const TokenType type);
Token next_token(const char *input, size_t *position);
