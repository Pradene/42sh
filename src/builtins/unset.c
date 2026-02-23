#include "builtin.h"
#include "env.h"
#include "vec.h"

void builtin_unset(AstNode *node, Variables *env) {
  for (size_t i = 1; i < vec_size(&node->command.args); ++i) {
    char *name = node->command.args.data[i];
    env_unset(env, name);
  }
}