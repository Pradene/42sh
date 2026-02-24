#include "env.h"
#include "ast.h"
#include "vec.h"

#include <stdio.h>
#include <string.h>

void builtin_alias(AstNode *node, HashTable *env) {
  (void)env;
  
  if (node->command.args.size == 1) {
    printf("TODO: print list\n");
    return;
  }

  for (size_t i = 1; i < vec_size(&node->command.args); ++i) {
    char *equal = strchr(node->command.args.data[i], '=');
    if (!equal) {
      printf("TODO: print alias\n");
    } else {
      printf("TODO: save alias\n");
    }
  }
}