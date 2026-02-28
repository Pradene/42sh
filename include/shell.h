#pragma once

#include "ast.h"
#include "ht.h"
#include "sb.h"

typedef struct Shell {
  HashTable     environment; // value -> Variable *
  HashTable     aliases;     // value -> char *
  AstNode      *command;
  uint8_t       status;
  bool          interactive;
  StringBuffer  input;
  int32_t       input_fd;
  const char   *input_src;

  char *(*readline)(struct Shell *shell);
} Shell;

Shell shell_create();
void shell_destroy(Shell *shell);
