#pragma once

#include "ast.h"
#include "lexer.h"
#include "shell.h"
#include "status.h"

typedef struct {
  Redir  **data;
  size_t size;
  size_t capacity;
} PendingHeredocs;

typedef struct {
  char            *input;
  size_t          position;
  Token           current_token;
  bool            token_ready;
  PendingHeredocs heredocs;
} ParserState;

StatusCode parse(Shell *shell);

