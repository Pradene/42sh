#include "ht.h"
#include "shell.h"
#include "env.h"
#include "42sh.h"

Shell shell_create() {
  return (Shell){
    .environment = (HashTable){
      .buckets = NULL,
      .size = 0,
      .capacity = 0,
      .free = env_variable_free
    },
    .aliases = (HashTable){
      .buckets = NULL,
      .size = 0,
      .capacity = 0,
      .free = free
    },
    .command = NULL,
    .status = 0,
    .interactive = false,
    .readline = NULL,
  };
}

void shell_destroy(Shell *shell) {
  ht_clear(&shell->environment);
  ht_clear(&shell->aliases);
  sb_free(&shell->input);
  ast_free(shell->command);
}
