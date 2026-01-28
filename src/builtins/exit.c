#include "ast.h"
#include "vec.h"
#include "env.h"

#include <unistd.h>
#include <ctype.h>
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

void builtin_exit(AstNode *node, Environment *env) {
  (void)env;
  char **args = node->command.args.data;
  size_t argc = vec_size(&node->command.args);

  write(1, "exit\n", 5);
  
  if (argc > 2) {
    write(2, "exit: too many arguments\n", 26);
    return;
  }

  if (argc == 2) {
    if (!is_numeric(args[1])) {
      write(2, "exit: numeric argument required\n", 32);
      exit(2);
    }

    errno = 0;
    long long value = strtoll(args[1], NULL, 10);
    if (errno == ERANGE) {
      write(2, "exit: numeric argument required\n", 32);
      exit(2);
    }

    exit((unsigned char)value);
  }

  exit(0);
}
