#pragma once

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
