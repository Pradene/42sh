#pragma once

#include <stdbool.h>
#include <stdlib.h>

typedef enum {
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
  TOKEN_REDIRECT_IN,
  TOKEN_HEREDOC,
  TOKEN_REDIRECT_FD_OUT,
  TOKEN_REDIRECT_FD_IN,
} TokenType;

typedef struct {
  char *s;
  size_t position;
  TokenType type;
} Token;

typedef struct {
  Token *data;
  size_t size;
  size_t capacity;
} Tokens;

const char *token_type_str(TokenType type);

typedef struct {
  bool is_ok;
  union {
    int err;
    Token ok;
  };
} TokenResult;

typedef struct {
  bool is_ok;
  union {
    int err;
    Tokens ok;
  };
} LexResult;

void tokens_free(Tokens *tokens);
void tokens_print(Tokens *tokens);

LexResult lex(char *s);
