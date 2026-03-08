#include "ht.h"
#include "shell.h"
#include "env.h"
#include "42sh.h"

void shell_destroy(Shell *shell) {
  ht_destroy(environ);
  ht_destroy(aliases);
  ast_free(last_command);
  (void)shell;
}
