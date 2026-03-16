#include "env.h"
#include "ast.h"
#include "vec.h"
#include "builtin.h"

#include <stdio.h>
#include <string.h>

void builtin_alias(AstNode *node) {
  size_t argc = vec_size(&node->command.args);
  if (argc == 1) {
    for (size_t i = 0; i < aliases->capacity; ++i) {
      HtEntry *entry = aliases->buckets[i];

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
      HtEntry *entry = ht_get(aliases, args[i]);
      if (entry) {
        char *name = entry->key;
        char *value = entry->value;
        printf("%s='%s'\n", name, value);
      }
    } else {
      *equal = '\0';

      char *name = args[i];
      char *value = strdup(equal + 1);
      ht_insert(aliases, name, value);

      *equal = '=';
    }
  }
}