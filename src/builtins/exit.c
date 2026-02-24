#include "ast.h"
#include "vec.h"
#include "env.h"
#include "builtin.h"

#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

static int is_numeric(const char *s) {
  size_t i = 0;

  if (!s || !s[i]) {
    return false;
  }
  if (s[i] == '+' || s[i] == '-') {
    ++i;
  }
  while (s[i]) {
    if (!isdigit(s[i])) {
      return false;
    }
    ++i;
  }
  return true;
}

void builtin_exit(AstNode *node, Shell *shell) {
  char **args = node->command.args.data;
  size_t argc = vec_size(&node->command.args);

  fprintf(stderr, "exit\n");
  
  if (argc > 2) {
    fprintf(stderr, "exit: too many arguments\n");
    return;
  }

  ht_clear(&shell->environment);
  ht_clear(&shell->aliases);
  
  if (argc == 2) {
    if (!is_numeric(args[1])) {
      fprintf(stderr, "exit: numeric argument required\n");
      ast_free(shell->command);
      exit(2);
    }

    errno = 0;
    long long value = strtoll(args[1], NULL, 10);
    if (errno == ERANGE) {
      fprintf(stderr, "exit: numeric argument required\n");
      ast_free(shell->command);
      exit(2);
    }

    ast_free(shell->command);
    exit((uint8_t)value);
  }

  ast_free(shell->command);
  exit(0);
}
