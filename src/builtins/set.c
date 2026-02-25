#include "builtin.h"
#include "env.h"
#include "ast.h"
#include "vec.h"

#include <stdio.h>

void builtin_set(AstNode *node, Shell *shell) {
  size_t argc = vec_size(&node->command.args);
  if (argc != 1) {
	fprintf(stderr, "set: usage: set\n");
	return;
  }

  for (size_t i = 0; i < shell->environment.capacity; ++i) {
	HashEntry *entry = shell->environment.buckets[i];

	while (entry) {
	  Variable *v = entry->value;
	  if (v && !v->exported) {
		printf("%s=%s\n", entry->key, v->content);
	  }
	  entry = entry->next;
	}
  }
}