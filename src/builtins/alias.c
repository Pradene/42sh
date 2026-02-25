#include "env.h"
#include "ast.h"
#include "vec.h"
#include "builtin.h"

#include <stdio.h>
#include <string.h>

void builtin_alias(AstNode *node, Shell *shell) {
  size_t argc = vec_size(&node->command.args);
  if (argc == 1) {
    for (size_t i = 0; i < shell->aliases.capacity; ++i) {
      HashEntry *entry = shell->aliases.buckets[i];

      while (entry) {
        char *name = entry->key;
        char *value = entry->value;
        printf("%s='%s'\n", name, value);
        entry = entry->next;
      }
    }
    return;
  }
  
  char **args = node->command.args.data;
  for (size_t i = 1; i < argc; ++i) {
    char *equal = strchr(args[i], '=');
    if (!equal) {
      HashEntry *entry = ht_get(&shell->aliases, args[i]);
      if (entry) {
        char *name = entry->key;
        char *value = entry->value;
        printf("%s='%s'\n", name, value);
      }
    } else {
      char *name = strndup(args[i], equal - args[i]);
      char *value = strdup(equal + 1);
      ht_insert(&shell->aliases, name, value);
      printf("alias %s='%s'\n", name, value);
    }
  }
}