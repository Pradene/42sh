#include "ast.h"
#include "env.h"
#include "sb.h"
#include "vec.h"
#include "42sh.h"

void expand_command(AstNode *node) {
  if (!node) {
    return;
  }

  switch (node->type) {
  case NODE_PIPE:
  case NODE_AND:
  case NODE_OR:
  case NODE_SEMICOLON:
  case NODE_BACKGROUND:
    expand_command(node->operator.left);
    expand_command(node->operator.right);
    return;
  case NODE_BRACE:
  case NODE_PAREN:
    expand_command(node->group.inner);

    expansion(node);
    splitting(node);
    stripping(node);
    return;

  case NODE_COMMAND:
    expansion(node);
    splitting(node);
    stripping(node);
    return;
  }
}
