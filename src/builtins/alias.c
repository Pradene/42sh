#include "env.h"
#include "ast.h"
#include "vec.h"
#include "builtin.h"

#include <stdio.h>
#include <string.h>

void builtin_unalias(AstNode *node, Shell *shell) {
  if (node->command.args.size == 1) {
    fprintf(stderr, "unalias: usage: unalias name [name ...]\n");
    return;
  }

  for (size_t i = 1; i < vec_size(&node->command.args); ++i) {
    ht_remove(&shell->aliases, node->command.args.data[i]);
  }
}

void builtin_alias(AstNode *node, Shell *shell) {
  if (node->command.args.size == 1) {
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

  for (size_t i = 1; i < vec_size(&node->command.args); ++i) {
    char *equal = strchr(node->command.args.data[i], '=');
    if (!equal) {
      HashEntry *entry = ht_get(&shell->aliases, node->command.args.data[i]);
      if (entry) {
        char *name = entry->key;
        char *value = entry->value;
        printf("%s='%s'\n", name, value);
      }
    } else {
      char *name = strndup(node->command.args.data[i], equal - node->command.args.data[i]);
      char *value = strdup(equal + 1);
      ht_insert(&shell->aliases, name, value);
      printf("alias %s='%s'\n", name, value);
      free(name);
    }
  }
}