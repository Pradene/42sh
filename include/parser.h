#pragma once

#include "ast.h"
#include "lexer.h"
#include "status.h"

typedef struct {
  char      *input;
  size_t    position;
  Token     current_token;
  bool      token_ready;
} ParserState;

StatusCode parse(const char *input, AstNode **root);

