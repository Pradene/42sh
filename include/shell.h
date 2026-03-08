#pragma once

#include "ast.h"
#include "ht.h"
#include "sb.h"

typedef struct Shell {
  int32_t       input_fd;
  const char   *input_src;
} Shell;

void shell_destroy(Shell *shell);
